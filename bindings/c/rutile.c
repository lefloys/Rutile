#include "rutile.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define RT__MAX_LAYERS 16
#define RT__MAX_DLLS (RT__MAX_LAYERS + 1)
#define RT__MAX_SETTINGS 64
#define RT__MAX_SETTING_NAME 64
#define RT__MAX_SETTING_VALUE 256

#if defined(_WIN32)
#define RT__THREAD_LOCAL __declspec(thread)
#elif defined(__cplusplus)
#define RT__THREAD_LOCAL thread_local
#else
#define RT__THREAD_LOCAL _Thread_local
#endif

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
typedef HMODULE rt__dll_handle;
#else
#include <dlfcn.h>
#include <limits.h>
#include <unistd.h>
typedef void* rt__dll_handle;
#endif

static rt__dll_handle rt__dll_open(const char* path);
static void* rt__dll_symbol(rt__dll_handle dll, const char* symbol);
static void rt__dll_close(rt__dll_handle dll);

#if defined(_WIN32)
static rt__dll_handle rt__dll_open(const char* path) {
	return LoadLibraryA(path);
}

static bool rt__exe_dir_path(char* out, usize out_size) {
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

static void rt__dll_last_error_message(char* out, usize out_size) {
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

static void* rt__dll_symbol(rt__dll_handle dll, const char* symbol) {
	return (void*)GetProcAddress(dll, symbol);
}

static void rt__dll_close(rt__dll_handle dll) {
	if (dll) {
		FreeLibrary(dll);
	}
}
#else
static rt__dll_handle rt__dll_open(const char* path) {
	return dlopen(path, RTLD_NOW | RTLD_LOCAL);
}

static bool rt__exe_dir_path(char* out, usize out_size) {
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

static void rt__dll_last_error_message(char* out, usize out_size) {
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

static void* rt__dll_symbol(rt__dll_handle dll, const char* symbol) {
	return dlsym(dll, symbol);
}

static void rt__dll_close(rt__dll_handle dll) {
	if (dll) {
		dlclose(dll);
	}
}
#endif

typedef struct rt_layer_link {
	rt_proc_chain chain;
	rt__dll_handle dll;
	rt_proc_chain next;
} rt_layer_link;


static rt_proc_t rt__backend_proc(const rt_proc_chain* chain, const char* name) {

	if (!rt__backend_dll || !name) {
		return NULL;
	}

	rt_proc_t proc = (rt_proc_t)rt__dll_symbol(rt__backend_dll, name);
	if (proc) {
		return proc;
	}

	return NULL;
}

static rt_proc_t rt__layer_proc_at(u32 index, const char* name) {
	if (index >= RT__MAX_LAYERS || !name) {
		return NULL;
	}

	rt_layer_link* link = &rt__layer_links[index];
	if (!link->next.get_proc) {
		return NULL;
	}

	rt_proc_t next_proc = link->next.get_proc(&link->next, name);
	if (!next_proc) {
		return NULL;
	}

	rt_proc_t wrapper = (rt_proc_t)rt__dll_symbol(link->dll, name);
	if (wrapper) {
		return wrapper;
	}

	return next_proc;
}

#define RT__LAYER_PROC_LIST(X) \
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

#define RT__LAYER_PROC(index)                                                                \
	static rt_proc_t rt__layer##index##_proc(const rt_proc_chain* chain, const char* name) { \
		return rt__layer_proc_at(index, name);                                               \
	}

RT__LAYER_PROC_LIST(RT__LAYER_PROC)

#undef RT__LAYER_PROC

#define RT__LAYER_PROC_ENTRY(index) rt__layer##index##_proc,

static rt_proc_t (*rt__layer_procs[RT__MAX_LAYERS])(const rt_proc_chain* chain, const char* name) = {
	RT__LAYER_PROC_LIST(RT__LAYER_PROC_ENTRY)
};

#undef RT__LAYER_PROC_ENTRY
#undef RT__LAYER_PROC_LIST

static enum rt_error rt__remember_dll(rt__dll_handle dll) {
	if (rt__dll_count >= RT__MAX_DLLS) {
		rt__dll_close(dll);
		return RT_IMPROPER_USAGE;
	}
	rt__dlls[rt__dll_count++] = dll;
	return RT_SUCCESS;
}

static void rt__close_dlls(void) {
	for (u32 i = rt__dll_count; i > 0; i--) {
		rt__dll_close(rt__dlls[i - 1]);
		rt__dlls[i - 1] = (rt__dll_handle)0;
	}
	rt__dll_count = 0;
}

static void rt__backend_dll_name(const char* name, char* out, usize out_size) {
#if defined(_WIN32)
	snprintf(out, out_size, "%s.dll", name);
#elif defined(__APPLE__)
	snprintf(out, out_size, "lib%s.dylib", name);
#else
	snprintf(out, out_size, "lib%s.so", name);
#endif
}

static void rt__layer_dll_name(const char* name, char* out, usize out_size) {
	const char* stem = name;
	if (strncmp(stem, "RT_", 3) == 0) {
		stem += 3;
	}

#if defined(_WIN32)
	snprintf(out, out_size, "rt-%s.dll", stem);
#elif defined(__APPLE__)
	snprintf(out, out_size, "librt-%s.dylib", stem);
#else
	snprintf(out, out_size, "librt-%s.so", stem);
#endif
	for (char* p = out; *p; p++) {
		if (*p == '_') {
			*p = '-';
		}
		if (*p >= 'A' && *p <= 'Z') {
			*p = (char)(*p - 'A' + 'a');
		}
	}
}

static enum rt_error rt__load_backend_dll(const char* path, const char* requested_name, rt__dll_handle* out_dll, char* message, usize message_size) {
	if (message && message_size) {
		message[0] = '\0';
	}
	if (!path || !requested_name || !out_dll) {
		return RT_IMPROPER_USAGE;
	}
	*out_dll = (rt__dll_handle)0;

	rt__dll_handle dll = rt__dll_open(path);
	if (!dll) {
		char dll_error[512];
		rt__dll_last_error_message(dll_error, sizeof(dll_error));
		if (message && message_size && dll_error[0]) {
			snprintf(message, message_size, "failed to load implementation DLL %s: %s", path, dll_error);
			message[message_size - 1] = '\0';
		} else if (message && message_size) {
			snprintf(message, message_size, "failed to load implementation DLL %s", path);
			message[message_size - 1] = '\0';
		}
		return RT_NO_BACKEND;
	}

	PFN_rtGetName get_name = (PFN_rtGetName)rt__dll_symbol(dll, "rtGetName");
	if (!get_name) {
		if (message && message_size) {
			snprintf(message, message_size, "implementation DLL %s does not export rtGetName", path);
			message[message_size - 1] = '\0';
		}
		rt__dll_close(dll);
		return RT_NO_BACKEND;
	}
	if (!get_name() || strcmp(get_name(), requested_name) != 0) {
		if (message && message_size) {
			snprintf(message, message_size, "implementation DLL %s reported name %s, expected %s", path, get_name() ? get_name() : "<null>", requested_name);
			message[message_size - 1] = '\0';
		}
		rt__dll_close(dll);
		return RT_NO_BACKEND;
	}

	enum rt_error err = rt__remember_dll(dll);
	if (err != RT_SUCCESS) {
		if (message && message_size) {
			snprintf(message, message_size, "too many loaded DLLs while loading %s", path);
			message[message_size - 1] = '\0';
		}
		return err;
	}

	*out_dll = dll;
	return RT_SUCCESS;
}

static enum rt_error rt__load_layer_dll(const char* path, const char* requested_name, rt_layer_link* out_link, char* message, usize message_size) {
	if (message && message_size) {
		message[0] = '\0';
	}
	if (!path || !requested_name || !out_link) {
		return RT_IMPROPER_USAGE;
	}
	out_link->dll = (rt__dll_handle)0;

	rt__dll_handle dll = rt__dll_open(path);
	if (!dll) {
		char dll_error[512];
		rt__dll_last_error_message(dll_error, sizeof(dll_error));
		if (message && message_size && dll_error[0]) {
			snprintf(message, message_size, "failed to load layer DLL %s: %s", path, dll_error);
			message[message_size - 1] = '\0';
		} else if (message && message_size) {
			snprintf(message, message_size, "failed to load layer DLL %s", path);
			message[message_size - 1] = '\0';
		}
		return RT_IMPROPER_USAGE;
	}

	PFN_rtLayerGetName get_name = (PFN_rtLayerGetName)rt__dll_symbol(dll, "rtLayerGetName");
	if (!get_name) {
		if (message && message_size) {
			snprintf(message, message_size, "layer DLL %s does not export rtLayerGetName", path);
			message[message_size - 1] = '\0';
		}
		rt__dll_close(dll);
		return RT_IMPROPER_USAGE;
	}
	if (!get_name() || strcmp(get_name(), requested_name) != 0) {
		if (message && message_size) {
			snprintf(message, message_size, "layer DLL %s reported name %s, expected %s", path, get_name() ? get_name() : "<null>", requested_name);
			message[message_size - 1] = '\0';
		}
		rt__dll_close(dll);
		return RT_IMPROPER_USAGE;
	}

	enum rt_error err = rt__remember_dll(dll);
	if (err != RT_SUCCESS) {
		if (message && message_size) {
			snprintf(message, message_size, "too many loaded DLLs while loading %s", path);
			message[message_size - 1] = '\0';
		}
		return err;
	}

	out_link->dll = dll;
	return RT_SUCCESS;
}

static enum rt_error rt__load_backend_named(const char* name, rt__dll_handle* out_dll, char* message, usize message_size) {
	char dll[128];
	rt__backend_dll_name(name, dll, sizeof(dll));
	enum rt_error err = rt__load_backend_dll(dll, name, out_dll, message, message_size);
	if (err == RT_SUCCESS) {
		return err;
	}

	{
		char exe_dir[1024];
		if (rt__exe_dir_path(exe_dir, sizeof(exe_dir))) {
			char path[1152];
			snprintf(path, sizeof(path), "%s%s", exe_dir, dll);
			path[sizeof(path) - 1] = '\0';
			err = rt__load_backend_dll(path, name, out_dll, message, message_size);
		}
	}
	return err;
}

static enum rt_error rt__load_layer_named(const char* name, rt_layer_link* out_link, char* message, usize message_size) {
	char dll[128];
	rt__layer_dll_name(name, dll, sizeof(dll));
	enum rt_error err = rt__load_layer_dll(dll, name, out_link, message, message_size);
	if (err == RT_SUCCESS) {
		return err;
	}

	{
		char exe_dir[1024];
		if (rt__exe_dir_path(exe_dir, sizeof(exe_dir))) {
			char path[1152];
			snprintf(path, sizeof(path), "%s%s", exe_dir, dll);
			path[sizeof(path) - 1] = '\0';
			err = rt__load_layer_dll(path, name, out_link, message, message_size);
		}
	}
	return err;
}

static void rt__layer_set_next(u32 index, rt_proc_chain next) {
	if (index >= RT__MAX_LAYERS) {
		return;
	}

	rt_layer_link* link = &rt__layer_links[index];
	link->next = next;

	PFN_rtLayerSetNext set_next = (PFN_rtLayerSetNext)rt__dll_symbol(link->dll, "rtLayerSetNext");
	if (set_next) {
		set_next(next);
	}

	link->chain.get_proc = rt__layer_procs[index];
}

#define RT_DEFINE_CORE_PROCEDURE(return_type, name, parameters) PFN_##name rt_##name = NULL;
RT_CORE_PROCEDURES(RT_DEFINE_CORE_PROCEDURE)
#undef RT_DEFINE_CORE_PROCEDURE

static RT__THREAD_LOCAL enum rt_error rt__loader_error = RT_SUCCESS;
static RT__THREAD_LOCAL char rt__loader_error_message[1024] = "";

static void rt__loader_set_error(enum rt_error error, const char* message) {
	rt__loader_error = error;
	if (!message) {
		rt__loader_error_message[0] = '\0';
		return;
	}
	snprintf(rt__loader_error_message, sizeof(rt__loader_error_message), "%s", message);
	rt__loader_error_message[sizeof(rt__loader_error_message) - 1] = '\0';
}

static void rt__loader_set_errorf(enum rt_error error, const char* format, ...) {
	va_list args;

	rt__loader_error = error;
	if (!format) {
		rt__loader_error_message[0] = '\0';
		return;
	}

	va_start(args, format);
	vsnprintf(rt__loader_error_message, sizeof(rt__loader_error_message), format, args);
	va_end(args);
	rt__loader_error_message[sizeof(rt__loader_error_message) - 1] = '\0';
}

static void rt__loader_print_error(void) {
	if (rt__loader_error != RT_SUCCESS && rt__loader_error_message[0]) {
		fprintf(stderr, "%s\n", rt__loader_error_message);
	}
}

static void rt__loader_clear_error_state(void) {
	rt__loader_error = RT_SUCCESS;
	rt__loader_error_message[0] = '\0';
}

static enum rt_error rt__loader_error_code(void) {
	return rt__loader_error;
}

static const char* rt__loader_error_message_text(void) {
	return rt__loader_error_message;
}

static void rt__loader_clear_error(void) {
	rt__loader_clear_error_state();
}

#undef RT__THREAD_LOCAL

static enum rt_error rt__resolve_required_proc(const char* name, rt_proc_t* out_proc, char* message, usize message_size) {
	if (message && message_size) {
		message[0] = '\0';
	}
	if (!name || !out_proc) {
		return RT_IMPROPER_USAGE;
	}

	rt_proc_t proc = rtGetProc(name);
	if (!proc) {
		*out_proc = NULL;
		if (message && message_size) {
			snprintf(message, message_size, "loaded implementation did not provide required proc: %s", name);
			message[message_size - 1] = '\0';
		}
		return RT_EXTENSION_NOT_PRESENT;
	}

	*out_proc = proc;
	return RT_SUCCESS;
}

#define RT__CORE_RESOLVE(name)                                                             \
	do {                                                                                   \
		rt_proc_t _p = NULL;                                                               \
		enum rt_error _err = rt__resolve_required_proc(#name, &_p, message, message_size); \
		if (_err != RT_SUCCESS) {                                                          \
			return _err;                                                                   \
		}                                                                                  \
		rt_##name = (PFN_##name)_p;                                                        \
	} while (0)

static enum rt_error rt__load_core(char* message, usize message_size) {
	if (message && message_size) {
		message[0] = '\0';
	}
	#define RT_RESOLVE_CORE_PROCEDURE(return_type, name, parameters) RT__CORE_RESOLVE(name);
	RT_CORE_PROCEDURES(RT_RESOLVE_CORE_PROCEDURE)
#undef RT_RESOLVE_CORE_PROCEDURE

	return RT_SUCCESS;
}

#define RT__CORE_TRY_RESOLVE(name)       \
	do {                                 \
		rt_proc_t _p = rtGetProc(#name); \
		if (_p) {                        \
			rt_##name = (PFN_##name)_p;  \
		}                                \
	} while (0)

static void rt__load_core_development(void) {
	#define RT_TRY_RESOLVE_CORE_PROCEDURE(return_type, name, parameters) RT__CORE_TRY_RESOLVE(name);
	RT_CORE_PROCEDURES(RT_TRY_RESOLVE_CORE_PROCEDURE)
#undef RT_TRY_RESOLVE_CORE_PROCEDURE
}
#undef RT__CORE_RESOLVE

enum rt_error rtLoad(const char* backend_name, const char* const* layer_names, usize layer_count) {
	rt__loader_clear_error_state();
	if (!backend_name) {
		rt__loader_set_errorf(RT_NO_BACKEND, "rtLoad backend_name is NULL");
		rt__loader_print_error();
		return RT_NO_BACKEND;
	}
	if (layer_count > RT__MAX_LAYERS) {
		rt__loader_set_errorf(RT_IMPROPER_USAGE, "rtLoad requested too many layers: %u", layer_count);
		rt__loader_print_error();
		return RT_IMPROPER_USAGE;
	}
	if (layer_count && !layer_names) {
		rt__loader_set_errorf(RT_IMPROPER_USAGE, "rtLoad layer_count is %u but layer_names is NULL", layer_count);
		rt__loader_print_error();
		return RT_IMPROPER_USAGE;
	}
	for (u32 i = 0; i < layer_count; i++) {
		if (!layer_names[i] || !layer_names[i][0]) {
			rt__loader_set_errorf(RT_IMPROPER_USAGE, "rtLoad layer %u has no name", i);
			rt__loader_print_error();
			return RT_IMPROPER_USAGE;
		}
	}

	rtUnload();

	char message[1024];
	enum rt_error err = rt__load_backend_named(backend_name, &rt__backend_dll, message, sizeof(message));
	if (err != RT_SUCCESS) {
		rt__close_dlls();
		if (rt__loader_error == RT_SUCCESS) {
			rt__loader_set_errorf(err, "%s", message[0] ? message : "failed to load implementation");
		}
		rt__loader_print_error();
		return err;
	}
		for (u32 i = 0; i < layer_count; i++) {
		err = rt__load_layer_named(layer_names[i], &rt__layer_links[i], message, sizeof(message));
		if (err != RT_SUCCESS) {
			rt__close_dlls();
			rt__loader_set_errorf(err, "%s", message[0] ? message : "failed to load layer");
			rt__loader_print_error();
			return err;
		}
	}

	rt_proc_chain chain;
	chain.get_proc = rt__backend_proc;
	for (u32 i = 0; i < layer_count; i++) {
		rt__layer_set_next(i, chain);
		chain = rt__layer_links[i].chain;
	}
	rt__chain = chain;

	err = rt__load_core(message, sizeof(message));
	if (err != RT_SUCCESS) {
		if (rt__loader_error == RT_SUCCESS) {
			rt__loader_set_errorf(err, "%s", message[0] ? message : "failed to resolve required core procedures");
		}
		rtUnload();
		rt__loader_print_error();
		return err;
	}
	rt_loaded = true;
	return RT_SUCCESS;
}

enum rt_error rtLoadDevelopment(const char* backend_name, const char* const* layer_names, usize layer_count) {
	rt__loader_clear_error_state();
	if (layer_count > RT__MAX_LAYERS) {
		rt__loader_set_errorf(RT_IMPROPER_USAGE, "rtLoadDevelopment requested too many layers: %u", layer_count);
		rt__loader_print_error();
		return RT_IMPROPER_USAGE;
	}
	if (layer_count && !layer_names) {
		rt__loader_set_errorf(RT_IMPROPER_USAGE, "rtLoadDevelopment layer_count is %u but layer_names is NULL", layer_count);
		rt__loader_print_error();
		return RT_IMPROPER_USAGE;
	}
	for (u32 i = 0; i < layer_count; i++) {
		if (!layer_names[i] || !layer_names[i][0]) {
			rt__loader_set_errorf(RT_IMPROPER_USAGE, "rtLoadDevelopment layer %u has no name", i);
			rt__loader_print_error();
			return RT_IMPROPER_USAGE;
		}
	}

	rtUnload();

	char message[1024];
	enum rt_error err = RT_SUCCESS;
	if (backend_name) {
		err = rt__load_backend_named(backend_name, &rt__backend_dll, message, sizeof(message));
		if (err != RT_SUCCESS) {
			rt__loader_set_errorf(RT_SUCCESS, "development loader skipped backend %s", backend_name);
			rt__loader_print_error();
		}
	}

	if (!rt__backend_dll) {
		rt_loaded = false;
		return RT_SUCCESS;
	}
	
	u32 loaded_layers = 0;
	for (u32 i = 0; i < layer_count; i++) {
		err = rt__load_layer_named(layer_names[i], &rt__layer_links[loaded_layers], message, sizeof(message));
		if (err != RT_SUCCESS) {
			/* Layers are part of the requested dispatch chain. Skipping one
			 * would silently run a different program, so development loading is
			 * still strict about layer availability. */
			rt__close_dlls();
			rt__loader_set_errorf(err, "%s", message[0] ? message : "failed to load layer");
			rt__loader_print_error();
			return err;
		}
		loaded_layers++;
	}

	rt_proc_chain chain;
	chain.get_proc = rt__backend_proc;
	for (u32 i = 0; i < loaded_layers; i++) {
		rt__layer_set_next(i, chain);
		chain = rt__layer_links[i].chain;
	}
	rt__chain = chain;

	rt__load_core_development();
	rt_loaded = true;
	return RT_SUCCESS;
}

void rtUnload(void) {
	rt_loaded = false;
	rt__chain.get_proc = NULL;
	rt__backend_dll = (rt__dll_handle)0;
	#define RT_CLEAR_CORE_PROCEDURE(return_type, name, parameters) rt_##name = NULL;
	RT_CORE_PROCEDURES(RT_CLEAR_CORE_PROCEDURE)
#undef RT_CLEAR_CORE_PROCEDURE
	rt__close_dlls();
}

rt_proc_t rtGetProc(const char* name) {
	if (!name || !rt__chain.get_proc) {
		return NULL;
	}
	return rt__chain.get_proc(&rt__chain, name);
}

bool rtLoaded(void) {
	return rt_loaded;
}

