#include "debugger.h"
#include "texture_preview.h"
#include "trace.h"

#if defined(_WIN32)
#include <winsock2.h>
#include <windows.h>

#include <assert.h>
#include <stdio.h>
#include <string.h>

static int request(unsigned short port, const char* path, char* response, size_t capacity) {
	SOCKET client = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (client == INVALID_SOCKET) return 0;
	struct sockaddr_in address = {0};
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	address.sin_port = htons(port);
	if (connect(client, (const struct sockaddr*)&address, sizeof(address)) == SOCKET_ERROR) { closesocket(client); return 0; }
	char header[256];
	snprintf(header, sizeof(header), "GET %s HTTP/1.1\r\nHost: localhost\r\nConnection: close\r\n\r\n", path);
	send(client, header, (int)strlen(header), 0);
	size_t used = 0;
	for (;;) {
		int received = recv(client, response + used, (int)(capacity - used - 1), 0);
		if (received <= 0) break;
		used += (size_t)received;
	}
	response[used] = 0;
	closesocket(client);
	return used != 0;
}

static volatile LONG reached_pause_point;
static volatile LONG released_from_pause;

static DWORD WINAPI pause_worker(void* unused) {
	(void)unused;
	InterlockedExchange(&reached_pause_point, 1);
	rtdbg_debugger_point();
	InterlockedExchange(&released_from_pause, 1);
	return 0;
}

int main(void) {
	const unsigned short port = 48673;
	assert(_putenv_s("RT_DEBUG_PORT", "48673") == 0);
	assert(_putenv_s("RT_DEBUG_DISABLE_BROWSER", "1") == 0);
	assert(_putenv_s("RT_DEBUG_TRACE_PATH", "rt-debug-web-test.bin") == 0);
	rtdbg_debugger_start();
	rtdbg_debugger_record(7, 1, "rtInit");
	rtdbg_debugger_resource_create(11, "buffer");
	rtdbg_debugger_resource_detail(11, "memory type 2; size 60 bytes");
	rtdbg_debugger_resource_create(12, "texture");
	rtdbg_debugger_resource_destroy(12);
	const u08 pixels[] = {255, 0, 0, 255};
	rtdbg_debugger_texture_preview(9, 1, 1, pixels, sizeof(pixels));
	char response[16384] = {0};
	for (unsigned attempt = 0; attempt != 50 && !request(port, "/events", response, sizeof(response)); ++attempt) Sleep(10);
	assert(strstr(response, "rtInit"));
	assert(strstr(response, "\"resources\""));
	assert(strstr(response, "\"type\":\"buffer\""));
	assert(strstr(response, "memory type 2; size 60 bytes"));
	assert(strstr(response, "\"id\":12,\"live\":false"));
	assert(request(port, "/", response, sizeof(response)));
	assert(strstr(response, "Current software frame"));
	assert(strstr(response, "software_frame"));
	assert(strstr(response, "18446744073709551615"));
	assert(strstr(response, "window.close()"));
	assert(strstr(response, "connection ended"));
	assert(request(port, "/texture?id=9", response, sizeof(response)));
	char* texture = strstr(response, "\r\n\r\n");
	assert(texture && texture[4] == 'B' && texture[5] == 'M');
	int texture_storage;
	rtdbg_trace_open();
	rtdbg_trace_resource_create("rtTextureCreate", "texture", &texture_storage);
	rtdbg_texture_preview_resize((rt_texture)&texture_storage, RT_TEXTURE_2D, RT_RGBA8_UNORM, (rt_extent_3d){1, 1, 1}, 1);
	rtdbg_texture_preview_data((rt_texture)&texture_storage, (rt_texture_range){RT_TEXTURE_ASPECT_COLOR, 0, 1, 0, 1, {1, 1, 1}, {0, 0, 0}}, pixels);
	assert(request(port, "/texture?id=1", response, sizeof(response)));
	texture = strstr(response, "\r\n\r\n");
	assert(texture && texture[4] == 'B' && texture[5] == 'M' && (unsigned char)texture[58] == 0 && (unsigned char)texture[59] == 0 && (unsigned char)texture[60] == 255 && (unsigned char)texture[61] == 255);
	rtdbg_trace_close();
	remove("rt-debug-web-test.bin");
	assert(request(port, "/control?pause", response, sizeof(response)));
	assert(request(port, "/events", response, sizeof(response)));
	assert(strstr(response, "\"paused\":true"));
	HANDLE thread = CreateThread(NULL, 0, pause_worker, NULL, 0, NULL);
	assert(thread);
	Sleep(50);
	assert(InterlockedCompareExchange(&reached_pause_point, 0, 0));
	assert(!InterlockedCompareExchange(&released_from_pause, 0, 0));
	assert(request(port, "/control?step", response, sizeof(response)));
	assert(WaitForSingleObject(thread, 1000) == WAIT_OBJECT_0);
	assert(InterlockedCompareExchange(&released_from_pause, 0, 0));
	CloseHandle(thread);
	InterlockedExchange(&reached_pause_point, 0);
	InterlockedExchange(&released_from_pause, 0);
	thread = CreateThread(NULL, 0, pause_worker, NULL, 0, NULL);
	assert(thread);
	Sleep(50);
	assert(InterlockedCompareExchange(&reached_pause_point, 0, 0));
	assert(!InterlockedCompareExchange(&released_from_pause, 0, 0));
	assert(request(port, "/control?continue", response, sizeof(response)));
	assert(WaitForSingleObject(thread, 1000) == WAIT_OBJECT_0);
	assert(InterlockedCompareExchange(&released_from_pause, 0, 0));
	CloseHandle(thread);
	return 0;
}
#else
int main(void) { return 0; }
#endif
