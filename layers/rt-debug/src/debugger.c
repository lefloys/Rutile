#include "debugger.h"
#include "web.h"

#if defined(_WIN32)

#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <windows.h>
#include <shellapi.h>

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

enum { RTDBG_EVENT_CAPACITY = 2048, RTDBG_EVENT_TEXT_CAPACITY = 192, RTDBG_TEXTURE_PREVIEW_CAPACITY = 16 };

struct rtdbg_event {
	uint64_t sequence;
	uint32_t kind;
	char text[RTDBG_EVENT_TEXT_CAPACITY];
};

struct rtdbg_texture_preview {
	uint64_t texture_id;
	usize width;
	usize height;
	u08* rgba;
	usize byte_count;
};

struct rtdbg_resource {
	uint64_t id;
	bool live;
	char type[32];
	struct rtdbg_resource_detail* details;
	struct rtdbg_resource_detail* details_tail;
	unsigned detail_count;
	struct rtdbg_resource* next;
};

struct rtdbg_resource_detail {
	char* text;
	struct rtdbg_resource_detail* next;
};

static INIT_ONCE rtdbg_once = INIT_ONCE_STATIC_INIT;
static CRITICAL_SECTION rtdbg_lock;
static CONDITION_VARIABLE rtdbg_control_changed = CONDITION_VARIABLE_INIT;
static struct rtdbg_event rtdbg_events[RTDBG_EVENT_CAPACITY];
static struct rtdbg_texture_preview rtdbg_texture_previews[RTDBG_TEXTURE_PREVIEW_CAPACITY];
static struct rtdbg_resource* rtdbg_resources;
static size_t rtdbg_event_first;
static size_t rtdbg_event_count;
static bool rtdbg_paused;
static unsigned rtdbg_step_grants;
static uint64_t rtdbg_software_frame;
static bool rtdbg_started;

static BOOL CALLBACK rtdbg_initialize_once(PINIT_ONCE once, PVOID parameter, PVOID* context) {
	(void)once;
	(void)parameter;
	(void)context;
	InitializeCriticalSection(&rtdbg_lock);
	return TRUE;
}

static void rtdbg_initialize(void) {
	InitOnceExecuteOnce(&rtdbg_once, rtdbg_initialize_once, NULL, NULL);
}

static void rtdbg_send(SOCKET socket, const char* text) {
	while (*text) {
		int sent = send(socket, text, (int)strlen(text), 0);
		if (sent <= 0) return;
		text += sent;
	}
}

static void rtdbg_send_bytes(SOCKET socket, const void* bytes, size_t size) {
	const char* text = bytes;
	while (size) {
		int sent = send(socket, text, (int)(size > INT_MAX ? INT_MAX : size), 0);
		if (sent <= 0) return;
		text += sent;
		size -= (size_t)sent;
	}
}

static void rtdbg_send_header(SOCKET socket, const char* type) {
	char header[256];
	snprintf(header, sizeof(header), "HTTP/1.1 200 OK\r\nContent-Type: %s\r\nCache-Control: no-store\r\nConnection: close\r\n\r\n", type);
	rtdbg_send(socket, header);
}

static void rtdbg_send_json_string(SOCKET socket, const char* text) {
	rtdbg_send(socket, "\"");
	for (; *text; ++text) {
		if (*text == '\\' || *text == '\"') { char escaped[3] = {'\\', *text, 0}; rtdbg_send(socket, escaped); }
		else if (*text == '\n') rtdbg_send(socket, "\\n");
		else if ((unsigned char)*text >= 0x20) { char character[2] = {*text, 0}; rtdbg_send(socket, character); }
	}
	rtdbg_send(socket, "\"");
}

static void rtdbg_send_events(SOCKET socket) {
	rtdbg_send_header(socket, "application/json; charset=utf-8");
	EnterCriticalSection(&rtdbg_lock);
	char status[128];
	snprintf(status, sizeof(status), "{\"paused\":%s,\"software_frame\":%llu,\"resources\":[", rtdbg_paused ? "true" : "false", (unsigned long long)rtdbg_software_frame);
	rtdbg_send(socket, status);
	for (const struct rtdbg_resource* resource = rtdbg_resources; resource; resource = resource->next) {
		char prefix[128];
		snprintf(prefix, sizeof(prefix), "%s{\"id\":%llu,\"live\":%s,\"type\":", resource == rtdbg_resources ? "" : ",", (unsigned long long)resource->id, resource->live ? "true" : "false");
		rtdbg_send(socket, prefix);
		rtdbg_send_json_string(socket, resource->type);
		rtdbg_send(socket, ",\"details\":[");
		for (const struct rtdbg_resource_detail* detail = resource->details; detail; detail = detail->next) {
			rtdbg_send_json_string(socket, detail->text);
			if (detail->next) rtdbg_send(socket, ",");
		}
		rtdbg_send(socket, "]}");
	}
	rtdbg_send(socket, "],\"events\":[");
	for (size_t index = 0; index < rtdbg_event_count; ++index) {
		const struct rtdbg_event* event = &rtdbg_events[(rtdbg_event_first + index) % RTDBG_EVENT_CAPACITY];
		char prefix[96];
		snprintf(prefix, sizeof(prefix), "%s{\"sequence\":%llu,\"kind\":%u,\"text\":", index ? "," : "", (unsigned long long)event->sequence, event->kind);
		rtdbg_send(socket, prefix);
		rtdbg_send_json_string(socket, event->text);
		rtdbg_send(socket, "}");
	}
	rtdbg_send(socket, "]}");
	LeaveCriticalSection(&rtdbg_lock);
}

static void rtdbg_send_u32_le(u08* bytes, uint32_t value) {
	bytes[0] = (u08)value;
	bytes[1] = (u08)(value >> 8);
	bytes[2] = (u08)(value >> 16);
	bytes[3] = (u08)(value >> 24);
}

static void rtdbg_send_texture(SOCKET socket, uint64_t texture_id) {
	rtdbg_send_header(socket, "image/bmp");
	EnterCriticalSection(&rtdbg_lock);
	struct rtdbg_texture_preview* preview = NULL;
	for (unsigned index = 0; index < RTDBG_TEXTURE_PREVIEW_CAPACITY; ++index) if (rtdbg_texture_previews[index].texture_id == texture_id) { preview = &rtdbg_texture_previews[index]; break; }
	if (!preview || preview->width > INT32_MAX || preview->height > INT32_MAX || preview->width > SIZE_MAX / preview->height || preview->width * preview->height > SIZE_MAX / 4 || preview->byte_count != preview->width * preview->height * 4) { LeaveCriticalSection(&rtdbg_lock); return; }
	usize image_size = preview->byte_count;
	if (image_size > UINT32_MAX - 54) { LeaveCriticalSection(&rtdbg_lock); return; }
	u08 header[54] = {'B', 'M'};
	rtdbg_send_u32_le(header + 2, (uint32_t)(54 + image_size));
	rtdbg_send_u32_le(header + 10, 54);
	rtdbg_send_u32_le(header + 14, 40);
	rtdbg_send_u32_le(header + 18, (uint32_t)preview->width);
	rtdbg_send_u32_le(header + 22, (uint32_t)preview->height);
	header[26] = 1;
	header[28] = 32;
	rtdbg_send_u32_le(header + 34, (uint32_t)image_size);
	rtdbg_send_bytes(socket, header, sizeof(header));
	u08* row = malloc(preview->width * 4);
	if (row) {
		for (usize y = preview->height; y-- > 0;) {
			for (usize x = 0; x < preview->width; ++x) {
				const u08* rgba = preview->rgba + (y * preview->width + x) * 4;
				u08* bgra = row + x * 4;
				bgra[0] = rgba[2]; bgra[1] = rgba[1]; bgra[2] = rgba[0]; bgra[3] = rgba[3];
			}
			rtdbg_send_bytes(socket, row, preview->width * 4);
		}
		free(row);
	}
	LeaveCriticalSection(&rtdbg_lock);
}

static void rtdbg_apply_control(const char* request) {
	EnterCriticalSection(&rtdbg_lock);
	if (strstr(request, "GET /control?pause ")) {
		rtdbg_paused = true;
	} else if (strstr(request, "GET /control?continue ")) {
		rtdbg_paused = false;
		rtdbg_step_grants = 0;
		WakeAllConditionVariable(&rtdbg_control_changed);
	} else if (strstr(request, "GET /control?step ")) {
		rtdbg_paused = true;
		rtdbg_step_grants = 1;
		WakeAllConditionVariable(&rtdbg_control_changed);
	}
	LeaveCriticalSection(&rtdbg_lock);
}

static DWORD WINAPI rtdbg_server_thread(void* unused) {
	(void)unused;
	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) return 0;
	SOCKET listener = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (listener == INVALID_SOCKET) return 0;
	struct sockaddr_in address = {0};
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	const char* configured_port = getenv("RT_DEBUG_PORT");
	address.sin_port = configured_port ? htons((u_short)strtoul(configured_port, NULL, 10)) : 0;
	if (bind(listener, (const struct sockaddr*)&address, sizeof(address)) == SOCKET_ERROR || listen(listener, SOMAXCONN) == SOCKET_ERROR) { closesocket(listener); return 0; }
	int size = sizeof(address);
	getsockname(listener, (struct sockaddr*)&address, &size);
	char url[64];
	snprintf(url, sizeof(url), "http://127.0.0.1:%u/", ntohs(address.sin_port));
	if (!getenv("RT_DEBUG_DISABLE_BROWSER")) ShellExecuteA(NULL, "open", url, NULL, NULL, SW_SHOWNORMAL);
	for (;;) {
		SOCKET client = accept(listener, NULL, NULL);
		if (client == INVALID_SOCKET) break;
		char request[1024] = {0};
		recv(client, request, sizeof(request) - 1, 0);
		if (strstr(request, "GET /events ")) rtdbg_send_events(client);
		else if (strstr(request, "GET /texture?id=")) rtdbg_send_texture(client, strtoull(strstr(request, "GET /texture?id=") + strlen("GET /texture?id="), NULL, 10));
		else if (strstr(request, "GET /control?")) { rtdbg_apply_control(request); rtdbg_send_header(client, "text/plain"); rtdbg_send(client, "ok"); }
		else { rtdbg_send_header(client, "text/html; charset=utf-8"); rtdbg_send(client, rtdbg_web_page()); }
		closesocket(client);
	}
	closesocket(listener);
	WSACleanup();
	return 0;
}

void rtdbg_debugger_start(void) {
	if (getenv("RT_DEBUG_DISABLE_APP")) return;
	rtdbg_initialize();
	EnterCriticalSection(&rtdbg_lock);
	if (!rtdbg_started) { rtdbg_started = true; HANDLE thread = CreateThread(NULL, 0, rtdbg_server_thread, NULL, 0, NULL); if (thread) CloseHandle(thread); }
	LeaveCriticalSection(&rtdbg_lock);
}

void rtdbg_debugger_record(uint64_t sequence, uint32_t kind, const char* text) {
	rtdbg_initialize();
	EnterCriticalSection(&rtdbg_lock);
	struct rtdbg_event* event = &rtdbg_events[(rtdbg_event_first + rtdbg_event_count) % RTDBG_EVENT_CAPACITY];
	if (rtdbg_event_count == RTDBG_EVENT_CAPACITY) { rtdbg_event_first = (rtdbg_event_first + 1) % RTDBG_EVENT_CAPACITY; event = &rtdbg_events[(rtdbg_event_first + rtdbg_event_count - 1) % RTDBG_EVENT_CAPACITY]; }
	else ++rtdbg_event_count;
	event->sequence = sequence;
	event->kind = kind;
	snprintf(event->text, sizeof(event->text), "%s", text ? text : "");
	LeaveCriticalSection(&rtdbg_lock);
}

void rtdbg_debugger_resource_create(uint64_t resource_id, const char* type) {
	if (!resource_id) return;
	rtdbg_initialize();
	EnterCriticalSection(&rtdbg_lock);
	struct rtdbg_resource* resource = calloc(1, sizeof(*resource));
	if (resource) {
		resource->id = resource_id;
		resource->live = true;
		snprintf(resource->type, sizeof(resource->type), "%s", type ? type : "resource");
		if (!rtdbg_resources) rtdbg_resources = resource;
		else { struct rtdbg_resource* tail = rtdbg_resources; while (tail->next) tail = tail->next; tail->next = resource; }
	}
	LeaveCriticalSection(&rtdbg_lock);
}

void rtdbg_debugger_resource_destroy(uint64_t resource_id) {
	rtdbg_initialize();
	EnterCriticalSection(&rtdbg_lock);
	for (struct rtdbg_resource* resource = rtdbg_resources; resource; resource = resource->next) if (resource->id == resource_id) { resource->live = false; break; }
	LeaveCriticalSection(&rtdbg_lock);
}

void rtdbg_debugger_resource_detail(uint64_t resource_id, const char* text) {
	if (!resource_id || !text || !text[0]) return;
	rtdbg_initialize();
	EnterCriticalSection(&rtdbg_lock);
	for (struct rtdbg_resource* resource = rtdbg_resources; resource; resource = resource->next) {
		if (resource->id != resource_id) continue;
		if (resource->detail_count == 64) { LeaveCriticalSection(&rtdbg_lock); return; }
		struct rtdbg_resource_detail* detail = calloc(1, sizeof(*detail));
		if (!detail) break;
		detail->text = malloc(strlen(text) + 1);
		if (!detail->text) { free(detail); break; }
		memcpy(detail->text, text, strlen(text) + 1);
		if (resource->details_tail) resource->details_tail->next = detail;
		else resource->details = detail;
		resource->details_tail = detail;
		++resource->detail_count;
		break;
	}
	LeaveCriticalSection(&rtdbg_lock);
}

void rtdbg_debugger_resource_reset(void) {
	rtdbg_initialize();
	EnterCriticalSection(&rtdbg_lock);
	while (rtdbg_resources) { struct rtdbg_resource* resource = rtdbg_resources; rtdbg_resources = resource->next; while (resource->details) { struct rtdbg_resource_detail* detail = resource->details; resource->details = detail->next; free(detail->text); free(detail); } free(resource); }
	LeaveCriticalSection(&rtdbg_lock);
}

void rtdbg_debugger_point(void) {
	rtdbg_initialize();
	EnterCriticalSection(&rtdbg_lock);
	while (rtdbg_paused && !rtdbg_step_grants) SleepConditionVariableCS(&rtdbg_control_changed, &rtdbg_lock, INFINITE);
	if (rtdbg_step_grants) --rtdbg_step_grants;
	LeaveCriticalSection(&rtdbg_lock);
}

void rtdbg_debugger_texture_preview(uint64_t texture_id, usize width, usize height, const u08* rgba, usize byte_count) {
	if (!texture_id || !rgba || !width || !height || width > SIZE_MAX / height || width * height > SIZE_MAX / 4 || byte_count != width * height * 4) return;
	u08* copy = malloc(byte_count);
	if (!copy) return;
	memcpy(copy, rgba, byte_count);
	rtdbg_initialize();
	EnterCriticalSection(&rtdbg_lock);
	struct rtdbg_texture_preview* preview = NULL;
	for (unsigned index = 0; index < RTDBG_TEXTURE_PREVIEW_CAPACITY; ++index) if (rtdbg_texture_previews[index].texture_id == texture_id) { preview = &rtdbg_texture_previews[index]; break; }
	if (!preview) for (unsigned index = 0; index < RTDBG_TEXTURE_PREVIEW_CAPACITY; ++index) if (!rtdbg_texture_previews[index].texture_id) { preview = &rtdbg_texture_previews[index]; break; }
	if (!preview) preview = &rtdbg_texture_previews[texture_id % RTDBG_TEXTURE_PREVIEW_CAPACITY];
	free(preview->rgba);
	*preview = (struct rtdbg_texture_preview){texture_id, width, height, copy, byte_count};
	if (texture_id == UINT64_MAX) ++rtdbg_software_frame;
	LeaveCriticalSection(&rtdbg_lock);
}

void rtdbg_debugger_texture_preview_remove(uint64_t texture_id) {
	rtdbg_initialize();
	EnterCriticalSection(&rtdbg_lock);
	for (unsigned index = 0; index < RTDBG_TEXTURE_PREVIEW_CAPACITY; ++index) if (rtdbg_texture_previews[index].texture_id == texture_id) { free(rtdbg_texture_previews[index].rgba); rtdbg_texture_previews[index] = (struct rtdbg_texture_preview){0}; break; }
	LeaveCriticalSection(&rtdbg_lock);
}

void rtdbg_debugger_texture_preview_reset(void) {
	rtdbg_initialize();
	EnterCriticalSection(&rtdbg_lock);
	for (unsigned index = 0; index < RTDBG_TEXTURE_PREVIEW_CAPACITY; ++index) {
		free(rtdbg_texture_previews[index].rgba);
		rtdbg_texture_previews[index] = (struct rtdbg_texture_preview){0};
	}
	LeaveCriticalSection(&rtdbg_lock);
}

#else

void rtdbg_debugger_start(void) {}
void rtdbg_debugger_record(uint64_t sequence, uint32_t kind, const char* text) { (void)sequence; (void)kind; (void)text; }
void rtdbg_debugger_resource_create(uint64_t resource_id, const char* type) { (void)resource_id; (void)type; }
void rtdbg_debugger_resource_destroy(uint64_t resource_id) { (void)resource_id; }
void rtdbg_debugger_resource_detail(uint64_t resource_id, const char* text) { (void)resource_id; (void)text; }
void rtdbg_debugger_resource_reset(void) {}
void rtdbg_debugger_point(void) {}
void rtdbg_debugger_texture_preview(uint64_t texture_id, usize width, usize height, const u08* rgba, usize byte_count) { (void)texture_id; (void)width; (void)height; (void)rgba; (void)byte_count; }
void rtdbg_debugger_texture_preview_remove(uint64_t texture_id) { (void)texture_id; }
void rtdbg_debugger_texture_preview_reset(void) {}

#endif
