#include "context.hpp"
#include "error.hpp"
#include "resource/queue.hpp"

#include <chrono>
#include <new>
#include <vector>

rtdx_context* current_context = nullptr;

rtdx_context* rtdx_get_current_context() {
	return current_context;
}

static u64 rtdx_now_ns() {
	using clock = std::chrono::steady_clock;
	return static_cast<u64>(std::chrono::duration_cast<std::chrono::nanoseconds>(clock::now().time_since_epoch()).count());
}

static void rtdx_log_startup_time(u64 start_ns) {
	u64 elapsed_ns = rtdx_now_ns() - start_ns;
	rtdx_printf("rt-d3d12: initialized in %.3f ms\n", static_cast<double>(elapsed_ns) / 1000000.0);
}

static bool rtdx_context_create_factory(rtdx_context* ctx) {
	UINT flags = 0;
#if defined(RTDX_ENABLE_D3D12_VALIDATION)
	ID3D12Debug* debug = nullptr;
	if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debug)))) {
		debug->EnableDebugLayer();
		debug->Release();
		flags |= DXGI_CREATE_FACTORY_DEBUG;
	}
#endif

	HRESULT result = CreateDXGIFactory2(flags, IID_PPV_ARGS(&ctx->dxgi_factory));
	if (FAILED(result)) {
		rtdx_throwf(rtdx_error_from_hresult(result), "CreateDXGIFactory2 failed: 0x%08x", static_cast<u32>(result));
		return false;
	}
	return true;
}

static void rtdx_context_query_present_features(rtdx_context* ctx) {
	IDXGIFactory5* factory5 = nullptr;
	if (FAILED(ctx->dxgi_factory->QueryInterface(IID_PPV_ARGS(&factory5)))) {
		return;
	}

	ctx->allow_tearing = false;
	BOOL allow_tearing = FALSE;
	if (SUCCEEDED(factory5->CheckFeatureSupport(
			DXGI_FEATURE_PRESENT_ALLOW_TEARING,
			&allow_tearing,
			static_cast<UINT>(sizeof(allow_tearing))
		))) {
		ctx->allow_tearing = allow_tearing == TRUE;
	}
	factory5->Release();
}

static bool rtdx_context_pick_adapter(rtdx_context* ctx) {
	for (UINT i = 0;; i++) {
		IDXGIAdapter1* adapter = nullptr;
		DXGI_ADAPTER_DESC1 desc;
		HRESULT result = ctx->dxgi_factory->EnumAdapterByGpuPreference(
			i,
			DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE,
			IID_PPV_ARGS(&adapter)
		);
		if (result == DXGI_ERROR_NOT_FOUND) {
			break;
		}
		if (FAILED(result)) {
			rtdx_throwf(rtdx_error_from_hresult(result), "EnumAdapterByGpuPreference failed: 0x%08x", static_cast<u32>(result));
			return false;
		}

		adapter->GetDesc1(&desc);
		if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) {
			adapter->Release();
			continue;
		}

		if (SUCCEEDED(D3D12CreateDevice(adapter, D3D_FEATURE_LEVEL_11_0, __uuidof(ID3D12Device), nullptr))) {
			ctx->dxgi_adapter = adapter;
			return true;
		}

		adapter->Release();
	}

	rtdx_throwf(RT_INCOMPATIBLE_DRIVER, "no compatible D3D12 adapter found");
	return false;
}

static bool rtdx_context_create_device(rtdx_context* ctx) {
	HRESULT result = D3D12CreateDevice(ctx->dxgi_adapter, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&ctx->d3d_device));
	if (FAILED(result)) {
		rtdx_throwf(rtdx_error_from_hresult(result), "D3D12CreateDevice failed: 0x%08x", static_cast<u32>(result));
		return false;
	}

	if (!rtdx_queue_create(ctx, RT_QUEUE_GRAPHICS)) {
		return false;
	}
	return true;
}

static void rtdx_context_destroy_queues(rtdx_context* ctx) {
	while (ctx->queue_count) {
		rtdx_queue_destroy(ctx, ctx->queues[ctx->queue_count - 1]);
	}
	delete[] ctx->queues;
	ctx->queues = nullptr;
	ctx->queue_count = 0;
	for (usize index = 0; index < sizeof(ctx->timepoint_queues) / sizeof(*ctx->timepoint_queues); index++) {
		if (ctx->timepoint_queues[index]) {
			rtdx_resource_release(RTDX_RESOURCE_BASE(ctx->timepoint_queues[index]));
			ctx->timepoint_queues[index] = NULL;
		}
	}
}

rtdx_context* rtdx_create_context(rtdx_context_flags flags) {
	auto* result = new (std::nothrow) rtdx_context{};
	if (!result) {
		rtdx_throwf(RT_OUT_OF_HOST_MEMORY, "failed to allocate DirectX 12 context");
		return nullptr;
	}

	result->flags = flags;
	rtdx_context_init(result);
	if (rtError() != RT_SUCCESS) {
		rtdx_context_destroy(result);
		return nullptr;
	}

	return result;
}

void rtdx_context_init(rtdx_context* ctx) {
	u64 start_ns = rtdx_now_ns();
	ctx->queue_lock = rt_mutex_create();
	if (!ctx->queue_lock) {
		rtdx_throwf(RT_PLATFORM_FAILURE, "failed to create DirectX queue synchronization");
		return;
	}
	if (!rtdx_context_create_factory(ctx)) {
		return;
	}
	rtdx_context_query_present_features(ctx);
	if (!rtdx_context_pick_adapter(ctx)) {
		return;
	}
	if (!rtdx_context_create_device(ctx)) {
		return;
	}

	rtdx_log_startup_time(start_ns);
}

void rtdx_context_finish(rtdx_context* ctx) {
	if (!ctx) {
		return;
	}

	ctx->shutting_down = true;
	rtdx_context_destroy_queues(ctx);
	rt_mutex_destroy(ctx->queue_lock);
	ctx->queue_lock = nullptr;
	if (ctx->graphics_fence_event) {
		rt_event_destroy(ctx->graphics_fence_event);
		ctx->graphics_fence_event = nullptr;
	}
	rtdx_release(&ctx->d3d_graphics_fence);
	rtdx_release(&ctx->d3d_graphics_queue);
	rtdx_release(&ctx->d3d_device);
	rtdx_release(&ctx->dxgi_adapter);
	rtdx_release(&ctx->dxgi_factory);
}

void rtdx_context_destroy(rtdx_context* ctx) {
	if (!ctx) {
		return;
	}

	rtdx_context_finish(ctx);
	delete ctx;
}

void rtdx_context_report_validation(rtdx_context* ctx) {
#if defined(RTDX_ENABLE_D3D12_VALIDATION)
	ID3D12InfoQueue* messages = nullptr;
	if (!ctx || !ctx->d3d_device || FAILED(ctx->d3d_device->QueryInterface(IID_PPV_ARGS(&messages)))) {
		return;
	}
	const UINT64 count = messages->GetNumStoredMessagesAllowedByRetrievalFilter();
	for (UINT64 index = 0; index < count; ++index) {
		SIZE_T size = 0;
		messages->GetMessage(index, nullptr, &size);
		std::vector<unsigned char> storage(size);
		auto* message = reinterpret_cast<D3D12_MESSAGE*>(storage.data());
		if (SUCCEEDED(messages->GetMessage(index, message, &size))) {
			rtdx_printf("D3D12 validation: %s\n", message->pDescription);
		}
	}
	messages->ClearStoredMessages();
	messages->Release();
#else
	(void)ctx;
#endif
}
