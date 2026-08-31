#include "context.hpp"
#include "error.hpp"
#include "resource/queue.hpp"

#include <chrono>
#include <memory>
#include <new>
#include <vector>

rtd3d12_context* current_context = nullptr;

rtd3d12_context* rtd3d12_get_current_context() {
	return current_context;
}

void rtd3d12_context::log_startup_time(std::chrono::steady_clock::time_point start) {
	const auto elapsed = std::chrono::steady_clock::now() - start;
	const auto milliseconds = std::chrono::duration<double, std::milli>(elapsed).count();
	rtd3d12_print("rt-d3d12: initialized in {:.3f} ms\n", milliseconds);
}

bool rtd3d12_context::create_factory() {
	UINT flags = 0;
#if defined(RTD3D12_ENABLE_D3D12_VALIDATION)
	ID3D12Debug* debug = nullptr;
	if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debug)))) {
		debug->EnableDebugLayer();
		debug->Release();
		flags |= DXGI_CREATE_FACTORY_DEBUG;
	}
#endif

	HRESULT result = CreateDXGIFactory2(flags, IID_PPV_ARGS(&dxgi_factory));
	if (FAILED(result)) {
		rtd3d12_fail(rtd3d12_error_from_hresult(result), "CreateDXGIFactory2 failed: 0x{:08x}", static_cast<u32>(result));
		return false;
	}
	return true;
}

bool rtd3d12_context::pick_adapter() {
	for (UINT i = 0;; i++) {
		IDXGIAdapter1* adapter = nullptr;
		DXGI_ADAPTER_DESC1 desc;
		HRESULT result = dxgi_factory->EnumAdapterByGpuPreference(
			i,
			DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE,
			IID_PPV_ARGS(&adapter)
		);
		if (result == DXGI_ERROR_NOT_FOUND) {
			break;
		}
		if (FAILED(result)) {
			rtd3d12_fail(rtd3d12_error_from_hresult(result), "EnumAdapterByGpuPreference failed: 0x{:08x}", static_cast<u32>(result));
			return false;
		}

		adapter->GetDesc1(&desc);
		if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) {
			adapter->Release();
			continue;
		}

		if (SUCCEEDED(D3D12CreateDevice(adapter, D3D_FEATURE_LEVEL_11_0, __uuidof(ID3D12Device), nullptr))) {
			dxgi_adapter = adapter;
			return true;
		}

		adapter->Release();
	}

	rtd3d12_fail(rt::error::incompatible_driver, "no compatible D3D12 adapter found");
	return false;
}

bool rtd3d12_context::create_device() {
	HRESULT result = D3D12CreateDevice(dxgi_adapter, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&d3d_device));
	if (FAILED(result)) {
		rtd3d12_fail(rtd3d12_error_from_hresult(result), "D3D12CreateDevice failed: 0x{:08x}", static_cast<u32>(result));
		return false;
	}

	if (!rt_queue_t::create(this, rt::queue_capability::graphics)) {
		return false;
	}
	return true;
}

void rtd3d12_context::destroy_queues() {
	for (rt_queue_t*& queue : timepoint_queues) {
		if (queue) {
			queue->destroy();
			queue = nullptr;
		}
	}
}

rtd3d12_context* rtd3d12_context::create(rtd3d12_context_flags flags) {
	auto* result = rtd3d12::allocate<rtd3d12_context>(flags);
	if (!result) {
		return nullptr;
	}

	if (rtError() != rt::error::success) {
		delete result;
		return nullptr;
	}

	return result;
}

rtd3d12_context::rtd3d12_context(rtd3d12_context_flags context_flags) : flags(context_flags) {
	initialize();
}

void rtd3d12_context::initialize() {
	const auto start = std::chrono::steady_clock::now();
	if (!create_factory()) {
		return;
	}
	if (!pick_adapter()) {
		return;
	}
	if (!create_device()) {
		return;
	}

	log_startup_time(start);
}

rtd3d12_context::~rtd3d12_context() {
	destroy_queues();
	if (dxgi_factory) {
		dxgi_factory->Release();
		dxgi_factory = nullptr;
	}
	if (dxgi_adapter) {
		dxgi_adapter->Release();
		dxgi_adapter = nullptr;
	}
	if (d3d_device) {
		d3d_device->Release();
		d3d_device = nullptr;
	}
}

void rtd3d12_context::report_validation() {
#if defined(RTD3D12_ENABLE_D3D12_VALIDATION)
	ID3D12InfoQueue* messages = nullptr;
	if (!d3d_device || FAILED(d3d_device->QueryInterface(IID_PPV_ARGS(&messages)))) {
		return;
	}
	const UINT64 count = messages->GetNumStoredMessagesAllowedByRetrievalFilter();
	for (UINT64 index = 0; index < count; ++index) {
		SIZE_T size = 0;
		messages->GetMessage(index, nullptr, &size);
		/* GetMessage writes a D3D12_MESSAGE header followed by its text.  The
		 * backing storage must meet the header's alignment, not byte alignment. */
		auto storage = std::make_unique_for_overwrite<std::byte[]>(size);
		auto* message = reinterpret_cast<D3D12_MESSAGE*>(storage.get());
		if (SUCCEEDED(messages->GetMessage(index, message, &size))) {
			rtd3d12_print("D3D12 validation: {}\n", message->pDescription);
		}
	}
	messages->ClearStoredMessages();
	messages->Release();
#endif
}
