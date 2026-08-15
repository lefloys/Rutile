#include "rutile.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define RTL_MAX_LAYERS 16
#define RTL_MAX_DLLS (RTL_MAX_LAYERS + 1)
#define RTL_MAX_SETTINGS 64
#define RTL_MAX_SETTING_NAME 64
#define RTL_MAX_SETTING_VALUE 256

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
typedef HMODULE rtl_dll_handle;
#else
#include <dlfcn.h>
#include <limits.h>
#include <unistd.h>
typedef void* rtl_dll_handle;
#endif

static rtl_dll_handle rtl_dll_open(const char* path);
static void* rtl_dll_symbol(rtl_dll_handle dll, const char* symbol);
static void rtl_dll_close(rtl_dll_handle dll);

#if defined(_WIN32)
static rtl_dll_handle rtl_dll_open(const char* path) {
	return LoadLibraryA(path);
}
static bool rtl_dll_path_is_missing(const char* path) {
	DWORD attributes = GetFileAttributesA(path);
	if (attributes != INVALID_FILE_ATTRIBUTES) {
		return false;
	}

	DWORD error = GetLastError();
	return error == ERROR_FILE_NOT_FOUND || error == ERROR_PATH_NOT_FOUND;
}

static bool rtl_exe_dir_path(char* out, usize out_size) {
	DWORD length;
	char* slash;
	char* backslash;

	if (!out || out_size == 0) {
		return false;
	}
	out[0] = '\0';

	length = GetModuleFileNameA(NULL, out, (DWORD)out_size);
	if (length == 0 || length >= out_size) {
		out[0] = '\0';
		return false;
	}

	slash = strrchr(out, '/');
	backslash = strrchr(out, '\\');
	if (backslash && (!slash || backslash > slash)) {
		slash = backslash;
	}
	if (!slash) {
		out[0] = '\0';
		return false;
	}

	slash[1] = '\0';
	return true;
}

static void rtl_dll_last_error_message(char* out, usize out_size) {
	DWORD err;
	DWORD written;

	if (!out || out_size == 0) {
		return;
	}
	out[0] = '\0';

	err = GetLastError();
	if (!err) {
		return;
	}

	written = FormatMessageA(
		FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		err,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		out,
		(DWORD)out_size,
		NULL
	);
	if (written == 0) {
		snprintf(out, out_size, "Windows error %llu", (u64)err);
	}
	out[out_size - 1] = '\0';
}

static void* rtl_dll_symbol(rtl_dll_handle dll, const char* symbol) {
	return (void*)GetProcAddress(dll, symbol);
}

static void rtl_dll_close(rtl_dll_handle dll) {
	if (dll) {
		FreeLibrary(dll);
	}
}
#else
static rtl_dll_handle rtl_dll_open(const char* path) {
	return dlopen(path, RTLD_NOW | RTLD_LOCAL);
}

static bool rtl_exe_dir_path(char* out, usize out_size) {
	if (!out || out_size == 0) {
		return false;
	}
	out[0] = '\0';

#if defined(__linux__)
	char path[PATH_MAX];
	ssize_t length = readlink("/proc/self/exe", path, sizeof(path) - 1);
	if (length <= 0) {
		return false;
	}
	path[length] = '\0';

	char* slash = strrchr(path, '/');
	if (!slash) {
		return false;
	}
	*(slash + 1) = '\0';

	snprintf(out, out_size, "%s", path);
	out[out_size - 1] = '\0';
	return true;
#else
	return false;
#endif
}

static void rtl_dll_last_error_message(char* out, usize out_size) {
	const char* err;

	if (!out || out_size == 0) {
		return;
	}
	out[0] = '\0';

	err = dlerror();
	if (err) {
		snprintf(out, out_size, "%s", err);
		out[out_size - 1] = '\0';
	}
}

static void* rtl_dll_symbol(rtl_dll_handle dll, const char* symbol) {
	return dlsym(dll, symbol);
}

static void rtl_dll_close(rtl_dll_handle dll) {
	if (dll) {
		dlclose(dll);
	}
}
#endif

typedef struct rtl_layer_link {
	rt_proc_chain chain;
	rtl_dll_handle dll;
	rt_proc_chain next;
} rtl_layer_link;

static rtl_dll_handle rtl_backend_dll;
static rtl_dll_handle rtl_dlls[RTL_MAX_DLLS];
static rtl_layer_link rtl_layer_links[RTL_MAX_LAYERS];
static rt_proc_chain rtl_chain;
static usize rtl_dll_count;
static bool rtl_loaded;

rt_proc_t rtl_backend_proc(const char* name) {
	if (!rtl_backend_dll || !name) {
		return NULL;
	}

	return (rt_proc_t)rtl_dll_symbol(rtl_backend_dll, name);
}

static rt_proc_t rtl_chain_backend_proc(const rt_proc_chain* chain, const char* name) {
	return rtl_backend_proc(name);
}

static rt_proc_t rtl_layer_proc_at(usize index, const char* name) {
	if (index >= RTL_MAX_LAYERS || !name) {
		return NULL;
	}

	rtl_layer_link* link = &rtl_layer_links[index];
	if (!link->next.get_proc) {
		return NULL;
	}

	rt_proc_t next_proc = link->next.get_proc(&link->next, name);
	if (!next_proc) {
		return NULL;
	}

	rt_proc_t wrapper = (rt_proc_t)rtl_dll_symbol(link->dll, name);
	if (wrapper) {
		return wrapper;
	}

	return next_proc;
}

#define RTL_LAYER_PROC_LIST(X) \
	X(0)                       \
	X(1)                       \
	X(2)                       \
	X(3)                       \
	X(4)                       \
	X(5)                       \
	X(6)                       \
	X(7)                       \
	X(8)                       \
	X(9)                       \
	X(10)                      \
	X(11)                      \
	X(12)                      \
	X(13)                      \
	X(14)                      \
	X(15)

#define RTL_LAYER_PROC(index)                                                                \
	static rt_proc_t rtl_layer##index##_proc(const rt_proc_chain* chain, const char* name) { \
		return rtl_layer_proc_at(index, name);                                               \
	}

RTL_LAYER_PROC_LIST(RTL_LAYER_PROC)

#undef RTL_LAYER_PROC

#define RTL_LAYER_PROC_ENTRY(index) rtl_layer##index##_proc,

static rt_proc_t (*rtl_layer_procs[RTL_MAX_LAYERS])(const rt_proc_chain* chain, const char* name) = {
	RTL_LAYER_PROC_LIST(RTL_LAYER_PROC_ENTRY)
};

#undef RTL_LAYER_PROC_ENTRY
#undef RTL_LAYER_PROC_LIST

static enum rt_error rtl_remember_dll(rtl_dll_handle dll) {
	if (rtl_dll_count >= RTL_MAX_DLLS) {
		rtl_dll_close(dll);
		return RT_IMPROPER_USAGE;
	}
	rtl_dlls[rtl_dll_count++] = dll;
	return RT_SUCCESS;
}

static void rtl_close_dlls(void) {
	for (usize i = rtl_dll_count; i > 0; i--) {
		rtl_dll_close(rtl_dlls[i - 1]);
		rtl_dlls[i - 1] = (rtl_dll_handle)0;
	}
	rtl_dll_count = 0;
}

static bool rtl_dll_name(const char* stem, char* out, usize out_size) {
	int length;

	if (!stem || !out || !out_size) {
		return false;
	}

#if defined(_WIN32)
	length = snprintf(out, out_size, "%s.dll", stem);
#elif defined(__APPLE__)
	length = snprintf(out, out_size, "%s.dylib", stem);
#else
	length = snprintf(out, out_size, "%s.so", stem);
#endif
	if (length < 0 || (usize)length >= out_size) {
		out[out_size - 1] = '\0';
		return false;
	}
	return true;
}

static enum rt_error rtl_load_backend_dll(const char* path, const char* requested_name, rtl_dll_handle* out_dll, char* message, usize message_size) {
	if (message && message_size) {
		message[0] = '\0';
	}
	if (!path || !requested_name || !out_dll) {
		return RT_IMPROPER_USAGE;
	}
	*out_dll = (rtl_dll_handle)0;

	rtl_dll_handle dll = rtl_dll_open(path);
	if (!dll) {
		char dll_error[512];
	#if defined(_WIN32)
		DWORD dll_error_code = GetLastError();
		bool missing = rtl_dll_path_is_missing(path);
		SetLastError(dll_error_code);
	#endif
		rtl_dll_last_error_message(dll_error, sizeof(dll_error));
	#if defined(_WIN32)
		if (message && message_size && missing && dll_error[0]) {
			snprintf(message, message_size, "backend %s DLL %s was not found: %s", requested_name, path, dll_error);
			message[message_size - 1] = '\0';
		} else if (message && message_size && missing) {
			snprintf(message, message_size, "backend %s DLL %s was not found", requested_name, path);
			message[message_size - 1] = '\0';
		} else if (message && message_size && dll_error[0]) {
			snprintf(message, message_size, "backend %s DLL %s exists but could not be loaded; a required dependency may be missing: %s", requested_name, path, dll_error);
			message[message_size - 1] = '\0';
		} else if (message && message_size) {
			snprintf(message, message_size, "backend %s DLL %s exists but could not be loaded", requested_name, path);
			message[message_size - 1] = '\0';
		}
	#else
		if (message && message_size && dll_error[0]) {
			snprintf(message, message_size, "failed to load backend %s DLL %s: %s", requested_name, path, dll_error);
			message[message_size - 1] = '\0';
		} else if (message && message_size) {
			snprintf(message, message_size, "failed to load backend %s DLL %s", requested_name, path);
			message[message_size - 1] = '\0';
		}
	#endif
		return RT_NO_BACKEND;
	}

	PFN_rtGetName get_name = (PFN_rtGetName)rtl_dll_symbol(dll, "rtGetName");
	if (!get_name) {
		if (message && message_size) {
			snprintf(message, message_size, "backend %s DLL %s does not export rtGetName", requested_name, path);
			message[message_size - 1] = '\0';
		}
		rtl_dll_close(dll);
		return RT_NO_BACKEND;
	}
	const char* actual_name = get_name();
	if (!actual_name || strcmp(actual_name, requested_name) != 0) {
		if (message && message_size) {
			snprintf(message, message_size, "backend %s from DLL %s does not match requested backend %s", actual_name ? actual_name : "<null>", path, requested_name);
			message[message_size - 1] = '\0';
		}
		rtl_dll_close(dll);
		return RT_NO_BACKEND;
	}

	enum rt_error err = rtl_remember_dll(dll);
	if (err != RT_SUCCESS) {
		if (message && message_size) {
			snprintf(message, message_size, "too many loaded DLLs while loading backend %s from %s", requested_name, path);
			message[message_size - 1] = '\0';
		}
		return err;
	}

	*out_dll = dll;
	return RT_SUCCESS;
}

static enum rt_error rtl_load_layer_dll(const char* path, const char* requested_name, rtl_layer_link* out_link, char* message, usize message_size) {
	if (message && message_size) {
		message[0] = '\0';
	}
	if (!path || !requested_name || !out_link) {
		return RT_IMPROPER_USAGE;
	}
	out_link->dll = (rtl_dll_handle)0;

	rtl_dll_handle dll = rtl_dll_open(path);
	if (!dll) {
		char dll_error[512];
		rtl_dll_last_error_message(dll_error, sizeof(dll_error));
		if (message && message_size && dll_error[0]) {
			snprintf(message, message_size, "failed to load layer %s DLL %s: %s", requested_name, path, dll_error);
			message[message_size - 1] = '\0';
		} else if (message && message_size) {
			snprintf(message, message_size, "failed to load layer %s DLL %s", requested_name, path);
			message[message_size - 1] = '\0';
		}
		return RT_IMPROPER_USAGE;
	}

	PFN_rtLayerGetName get_name = (PFN_rtLayerGetName)rtl_dll_symbol(dll, "rtLayerGetName");
	if (!get_name) {
		if (message && message_size) {
			snprintf(message, message_size, "layer %s DLL %s does not export rtLayerGetName", requested_name, path);
			message[message_size - 1] = '\0';
		}
		rtl_dll_close(dll);
		return RT_IMPROPER_USAGE;
	}
	if (!get_name() || strcmp(get_name(), requested_name) != 0) {
		if (message && message_size) {
			snprintf(message, message_size, "layer %s from DLL %s does not match requested layer %s", get_name() ? get_name() : "<null>", path, requested_name);
			message[message_size - 1] = '\0';
		}
		rtl_dll_close(dll);
		return RT_IMPROPER_USAGE;
	}

	enum rt_error err = rtl_remember_dll(dll);
	if (err != RT_SUCCESS) {
		if (message && message_size) {
			snprintf(message, message_size, "too many loaded DLLs while loading layer %s from %s", requested_name, path);
			message[message_size - 1] = '\0';
		}
		return err;
	}

	out_link->dll = dll;
	return RT_SUCCESS;
}

static enum rt_error rtl_load_backend_named(const char* name, rtl_dll_handle* out_dll, char* message, usize message_size) {
	char dll[128];
	if (!rtl_dll_name(name, dll, sizeof(dll))) {
		if (message && message_size) {
			snprintf(message, message_size, "backend name is too long: %s", name ? name : "<null>");
			message[message_size - 1] = '\0';
		}
		return RT_IMPROPER_USAGE;
	}
	enum rt_error err = rtl_load_backend_dll(dll, name, out_dll, message, message_size);
	if (err == RT_SUCCESS) {
		return err;
	}

	{
		char exe_dir[1024];
		if (rtl_exe_dir_path(exe_dir, sizeof(exe_dir))) {
			char path[1152];
			snprintf(path, sizeof(path), "%s%s", exe_dir, dll);
			path[sizeof(path) - 1] = '\0';
			err = rtl_load_backend_dll(path, name, out_dll, message, message_size);
		}
	}
	return err;
}

static enum rt_error rtl_load_layer_named(const char* name, rtl_layer_link* out_link, char* message, usize message_size) {
	char dll[128];
	if (!rtl_dll_name(name, dll, sizeof(dll))) {
		if (message && message_size) {
			snprintf(message, message_size, "layer name is too long: %s", name ? name : "<null>");
			message[message_size - 1] = '\0';
		}
		return RT_IMPROPER_USAGE;
	}
	enum rt_error err = rtl_load_layer_dll(dll, name, out_link, message, message_size);
	if (err == RT_SUCCESS) {
		return err;
	}

	{
		char exe_dir[1024];
		if (rtl_exe_dir_path(exe_dir, sizeof(exe_dir))) {
			char path[1152];
			snprintf(path, sizeof(path), "%s%s", exe_dir, dll);
			path[sizeof(path) - 1] = '\0';
			err = rtl_load_layer_dll(path, name, out_link, message, message_size);
		}
	}
	return err;
}

static void rtl_layer_set_next(usize index, rt_proc_chain next) {
	if (index >= RTL_MAX_LAYERS) {
		return;
	}

	rtl_layer_link* link = &rtl_layer_links[index];
	link->next = next;

	PFN_rtLayerSetNext set_next = (PFN_rtLayerSetNext)rtl_dll_symbol(link->dll, "rtLayerSetNext");
	if (set_next) {
		set_next(next);
	}

	link->chain.get_proc = rtl_layer_procs[index];
}

#define RT_DEFINE_CORE_PROCEDURE(return_type, name, parameters, arguments) PFN_##name rt_##name = NULL;
RT_CORE_PROCEDURES(RT_DEFINE_CORE_PROCEDURE)
#undef RT_DEFINE_CORE_PROCEDURE

static void rtl_print_load_error(const char* message) {
	if (message && message[0]) {
		fprintf(stderr, "%s\n", message);
	}
}

static const char* rtl_backend_name(void) {
	PFN_rtGetName get_name = (PFN_rtGetName)rtl_dll_symbol(rtl_backend_dll, "rtGetName");
	if (!get_name) {
		return NULL;
	}
	return get_name();
}

static enum rt_error rtl_resolve_required_proc(const char* name, rt_proc_t* out_proc, char* message, usize message_size) {
	if (message && message_size) {
		message[0] = '\0';
	}
	if (!name || !out_proc) {
		return RT_IMPROPER_USAGE;
	}

	rt_proc_t proc = rtGetProc(name);
	if (!proc) {
		const char* backend_name = rtl_backend_name();
		*out_proc = NULL;
		if (message && message_size) {
			snprintf(message, message_size, "backend %s does not export required procedure: %s", backend_name ? backend_name : "<unknown>", name);
			message[message_size - 1] = '\0';
		}
		return RT_EXTENSION_NOT_PRESENT;
	}

	*out_proc = proc;
	return RT_SUCCESS;
}

static enum rt_error rtl_load_core(char* message, usize message_size) {
	if (message && message_size) {
		message[0] = '\0';
	}
	#define RT_RESOLVE_CORE_PROCEDURE(return_type, name, parameters, arguments)            \
		do {                                                                                \
			rt_proc_t _p = NULL;                                                            \
			enum rt_error _err = rtl_resolve_required_proc(#name, &_p, message, message_size); \
			if (_err != RT_SUCCESS) {                                                       \
				return _err;                                                                \
			}                                                                               \
			rt_##name = (PFN_##name)_p;                                                     \
		} while (0);
	RT_CORE_PROCEDURES(RT_RESOLVE_CORE_PROCEDURE)
#undef RT_RESOLVE_CORE_PROCEDURE

	return RT_SUCCESS;
}

static void rtl_load_core_development(void) {
	#define RT_TRY_RESOLVE_CORE_PROCEDURE(return_type, name, parameters, arguments) \
		do {                                                             \
			rt_proc_t _p = rtGetProc(#name);                              \
			if (_p) {                                                     \
				rt_##name = (PFN_##name)_p;                               \
			}                                                             \
		} while (0);
	RT_CORE_PROCEDURES(RT_TRY_RESOLVE_CORE_PROCEDURE)
#undef RT_TRY_RESOLVE_CORE_PROCEDURE
}

enum rt_error rtLoad(const char* backend_name, const char* const* layer_names, usize layer_count) {
	char message[1024];

	if (rtl_loaded) {
		snprintf(message, sizeof(message), "rtLoad called while a backend is already loaded");
		rtl_print_load_error(message);
		return RT_IMPROPER_USAGE;
	}
	if (!backend_name) {
		snprintf(message, sizeof(message), "rtLoad backend_name is NULL");
		rtl_print_load_error(message);
		return RT_NO_BACKEND;
	}
	if (layer_count > RTL_MAX_LAYERS) {
		snprintf(message, sizeof(message), "rtLoad requested too many layers: %zu", layer_count);
		rtl_print_load_error(message);
		return RT_IMPROPER_USAGE;
	}
	if (layer_count && !layer_names) {
		snprintf(message, sizeof(message), "rtLoad layer_count is %zu but layer_names is NULL", layer_count);
		rtl_print_load_error(message);
		return RT_IMPROPER_USAGE;
	}
	for (usize i = 0; i < layer_count; i++) {
		if (!layer_names[i] || !layer_names[i][0]) {
			snprintf(message, sizeof(message), "rtLoad layer %zu has no name", i);
			rtl_print_load_error(message);
			return RT_IMPROPER_USAGE;
		}
	}

	enum rt_error err = rtl_load_backend_named(backend_name, &rtl_backend_dll, message, sizeof(message));
	if (err != RT_SUCCESS) {
		rtl_print_load_error(message[0] ? message : "failed to load implementation");
		rtUnload();
		return err;
	}
	for (usize i = 0; i < layer_count; i++) {
		err = rtl_load_layer_named(layer_names[i], &rtl_layer_links[i], message, sizeof(message));
		if (err != RT_SUCCESS) {
			rtl_print_load_error(message[0] ? message : "failed to load layer");
			rtUnload();
			return err;
		}
	}

	rt_proc_chain chain;
	chain.get_proc = rtl_chain_backend_proc;
	for (usize i = 0; i < layer_count; i++) {
		rtl_layer_set_next(i, chain);
		chain = rtl_layer_links[i].chain;
	}
	rtl_chain = chain;

	err = rtl_load_core(message, sizeof(message));
	if (err != RT_SUCCESS) {
		rtl_print_load_error(message[0] ? message : "failed to resolve required core procedures");
		rtUnload();
		return err;
	}
	rtl_loaded = true;
	return RT_SUCCESS;
}

enum rt_error rtLoadDevelopment(const char* backend_name, const char* const* layer_names, usize layer_count) {
	char message[1024];

	if (layer_count > RTL_MAX_LAYERS) {
		snprintf(message, sizeof(message), "rtLoadDevelopment requested too many layers: %zu", layer_count);
		rtl_print_load_error(message);
		return RT_IMPROPER_USAGE;
	}
	if (layer_count && !layer_names) {
		snprintf(message, sizeof(message), "rtLoadDevelopment layer_count is %zu but layer_names is NULL", layer_count);
		rtl_print_load_error(message);
		return RT_IMPROPER_USAGE;
	}
	for (usize i = 0; i < layer_count; i++) {
		if (!layer_names[i] || !layer_names[i][0]) {
			snprintf(message, sizeof(message), "rtLoadDevelopment layer %zu has no name", i);
			rtl_print_load_error(message);
			return RT_IMPROPER_USAGE;
		}
	}

	rtUnload();

	enum rt_error err = RT_SUCCESS;
	if (backend_name) {
		err = rtl_load_backend_named(backend_name, &rtl_backend_dll, message, sizeof(message));
		if (err != RT_SUCCESS) {
			rtl_print_load_error(message[0] ? message : "failed to load implementation");
		}
	}

	if (!rtl_backend_dll) {
		rtl_loaded = false;
		return RT_SUCCESS;
	}

	usize loaded_layers = 0;
	for (usize i = 0; i < layer_count; i++) {
		err = rtl_load_layer_named(layer_names[i], &rtl_layer_links[loaded_layers], message, sizeof(message));
		if (err != RT_SUCCESS) {
			/* Layers are part of the requested dispatch chain. Skipping one
			 * would silently run a different program, so development loading is
			 * still strict about layer availability. */
			rtl_print_load_error(message[0] ? message : "failed to load layer");
			rtUnload();
			return err;
		}
		loaded_layers++;
	}

	rt_proc_chain chain;
	chain.get_proc = rtl_chain_backend_proc;
	for (usize i = 0; i < loaded_layers; i++) {
		rtl_layer_set_next(i, chain);
		chain = rtl_layer_links[i].chain;
	}
	rtl_chain = chain;

	rtl_load_core_development();
	rtl_loaded = true;
	return RT_SUCCESS;
}

void rtUnload(void) {
	rtl_loaded = false;
	rtl_chain.get_proc = NULL;
	rtl_backend_dll = (rtl_dll_handle)0;
	#define RT_CLEAR_CORE_PROCEDURE(return_type, name, parameters, arguments) rt_##name = NULL;
	RT_CORE_PROCEDURES(RT_CLEAR_CORE_PROCEDURE)
#undef RT_CLEAR_CORE_PROCEDURE
	rtl_close_dlls();
}

rt_proc_t rtGetProc(const char* name) {
	if (!name || !rtl_chain.get_proc) {
		return NULL;
	}
	return rtl_chain.get_proc(&rtl_chain, name);
}

bool rtLoaded(void) {
	return rtl_loaded;
}
