#include "trace.h"
#include "procs.h"
#include "debugger.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

static const uint8_t rtdbg_trace_magic[8] = {'R', 'T', 'D', 'B', 'G', 'T', 'R', 'C'};
static const uint64_t rtdbg_max_payload_size = UINT64_C(1) << 34;
static struct rtdbg_trace_writer rtdbg_writer;
static uint64_t rtdbg_next_event_sequence = 1;
static bool rtdbg_trace_started;

typedef struct rtdbg_handle {
	const void* value;
	uint64_t id;
	const char* type;
	struct rtdbg_handle* next;
} rtdbg_handle;

static rtdbg_handle* rtdbg_handles;
static uint64_t rtdbg_next_handle_id = 1;

static rtdbg_handle* rtdbg_find_handle(const void* value) {
	for (rtdbg_handle* handle = rtdbg_handles; handle; handle = handle->next) {
		if (handle->value == value) return handle;
	}
	return NULL;
}

uint64_t rtdbg_trace_handle_id(const void* value) {
	rtdbg_handle* handle = rtdbg_find_handle(value);
	return handle ? handle->id : 0;
}

static void rtdbg_record_text(uint32_t kind, const char* text) {
	rtdbg_debugger_record(rtdbg_next_event_sequence++, kind, text);
	if (rtdbg_writer.file) rtdbg_trace_writer_record(&rtdbg_writer, kind, 0, text, strlen(text));
	rtdbg_debugger_point();
}

void rtdbg_trace_event(uint32_t kind, const char* format, ...) {
	char text[512];
	va_list arguments;
	va_start(arguments, format);
	vsnprintf(text, sizeof(text), format, arguments);
	va_end(arguments);
	rtdbg_trace_open();
	rtdbg_record_text(kind, text);
}

static void rtdbg_clear_handles(void) {
	while (rtdbg_handles) {
		rtdbg_handle* next = rtdbg_handles->next;
		free(rtdbg_handles);
		rtdbg_handles = next;
	}
	rtdbg_next_handle_id = 1;
}

void rtdbg_trace_open(void) {
	if (rtdbg_trace_started) return;
	const char* path = getenv("RT_DEBUG_TRACE_PATH");
	rtdbg_clear_handles();
	rtdbg_debugger_resource_reset();
	rtdbg_next_event_sequence = 1;
	rtdbg_trace_started = true;
	if (!path || !path[0]) path = "rt-debug.trace";
	rtdbg_trace_writer_open(&rtdbg_writer, path);
}

void rtdbg_trace_close(void) {
	if (rtdbg_writer.file) rtdbg_trace_writer_close(&rtdbg_writer);
	rtdbg_clear_handles();
	rtdbg_trace_started = false;
}

void rtdbg_trace_api(const char* name) {
	rtdbg_trace_open();
	rtdbg_record_text(RTDBG_TRACE_API_CALL, name);
}

void rtdbg_trace_resource_create(const char* name, const char* type, const void* value) {
	rtdbg_trace_open();
	if (!value) { rtdbg_trace_api(name); return; }
	rtdbg_handle* handle = malloc(sizeof(*handle));
	if (!handle) { rtdbg_trace_api(name); return; }
	*handle = (rtdbg_handle){value, rtdbg_next_handle_id++, type, rtdbg_handles};
	rtdbg_handles = handle;
	rtdbg_debugger_resource_create(handle->id, type);
	char text[128];
	snprintf(text, sizeof(text), "%s %s #%llu", name, type, (unsigned long long)handle->id);
	rtdbg_record_text(RTDBG_TRACE_RESOURCE_CREATE, text);
}

void rtdbg_trace_resource_destroy(const char* name, const char* type, const void* value) {
	rtdbg_trace_open();
	rtdbg_handle** link = &rtdbg_handles;
	while (*link && (*link)->value != value) link = &(*link)->next;
	char text[128];
	uint64_t id = *link ? (*link)->id : 0;
	rtdbg_debugger_resource_destroy(id);
	snprintf(text, sizeof(text), "%s %s #%llu", name, type, (unsigned long long)id);
	rtdbg_record_text(RTDBG_TRACE_RESOURCE_DESTROY, text);
	if (*link) { rtdbg_handle* handle = *link; *link = handle->next; free(handle); }
}

void rtdbg_trace_resource_detail(const void* value, const char* format, ...) {
	char text[512];
	va_list arguments;
	va_start(arguments, format);
	vsnprintf(text, sizeof(text), format, arguments);
	va_end(arguments);
	rtdbg_debugger_resource_detail(rtdbg_trace_handle_id(value), text);
}

void rtdbg_trace_command_buffer(const char* name, const void* command_buffer) {
	rtdbg_trace_open();
	char text[128];
	snprintf(text, sizeof(text), "%s command-buffer #%llu", name, (unsigned long long)rtdbg_trace_handle_id(command_buffer));
	rtdbg_record_text(!strcmp(name, "rtCommandBufferBegin") ? RTDBG_TRACE_COMMAND_BUFFER_BEGIN : RTDBG_TRACE_COMMAND_BUFFER_END, text);
}

void rtdbg_trace_queue_submit(const void* queue, const void* command_buffer, uint64_t timepoint) {
	rtdbg_trace_open();
	char text[160];
	snprintf(text, sizeof(text), "rtQueueSubmit queue #%llu command-buffer #%llu timepoint %llu", (unsigned long long)rtdbg_trace_handle_id(queue), (unsigned long long)rtdbg_trace_handle_id(command_buffer), (unsigned long long)timepoint);
	rtdbg_record_text(RTDBG_TRACE_QUEUE_SUBMIT, text);
}

void rtdbg_trace_draw(const char* name, const void* command_buffer, uint64_t primitive_count, uint64_t instance_count) {
	rtdbg_trace_open();
	char text[160];
	snprintf(text, sizeof(text), "%s command-buffer #%llu count %llu instances %llu", name, (unsigned long long)rtdbg_trace_handle_id(command_buffer), (unsigned long long)primitive_count, (unsigned long long)instance_count);
	rtdbg_record_text(RTDBG_TRACE_COMMAND, text);
}

static bool write_u16(FILE* file, uint16_t value) { uint8_t b[2] = {(uint8_t)value, (uint8_t)(value >> 8)}; return fwrite(b, sizeof(b), 1, file) == 1; }
static bool write_u32(FILE* file, uint32_t value) { uint8_t b[4]; for (unsigned s = 0; s < 32; s += 8) b[s / 8] = (uint8_t)(value >> s); return fwrite(b, sizeof(b), 1, file) == 1; }
static bool write_u64(FILE* file, uint64_t value) { uint8_t b[8]; for (unsigned s = 0; s < 64; s += 8) b[s / 8] = (uint8_t)(value >> s); return fwrite(b, sizeof(b), 1, file) == 1; }
static bool read_u16(FILE* file, uint16_t* value) { uint8_t b[2]; if (fread(b, sizeof(b), 1, file) != 1) return false; *value = (uint16_t)(b[0] | (uint16_t)b[1] << 8); return true; }
static bool read_u32(FILE* file, uint32_t* value) { uint8_t b[4]; if (fread(b, sizeof(b), 1, file) != 1) return false; *value = 0; for (unsigned s = 0; s < 32; s += 8) *value |= (uint32_t)b[s / 8] << s; return true; }
static bool read_u64(FILE* file, uint64_t* value) { uint8_t b[8]; if (fread(b, sizeof(b), 1, file) != 1) return false; *value = 0; for (unsigned s = 0; s < 64; s += 8) *value |= (uint64_t)b[s / 8] << s; return true; }

bool rtdbg_trace_writer_open(struct rtdbg_trace_writer* writer, const char* path) {
	if (!writer || !path) return false;
	memset(writer, 0, sizeof(*writer));
	FILE* file = fopen(path, "wb");
	if (!file || fwrite(rtdbg_trace_magic, sizeof(rtdbg_trace_magic), 1, file) != 1 || !write_u16(file, 1) || !write_u16(file, 0) || !write_u32(file, 0x01020304u) || !write_u64(file, 24)) { if (file) fclose(file); return false; }
	writer->file = file; writer->next_sequence = 1; return true;
}

bool rtdbg_trace_writer_record(struct rtdbg_trace_writer* writer, uint32_t kind, uint32_t flags, const void* payload, uint64_t payload_size) {
	if (!writer || !writer->file || writer->failed || !kind || payload_size > rtdbg_max_payload_size || (!payload && payload_size)) return false;
	FILE* file = writer->file;
	if (!write_u64(file, writer->next_sequence) || !write_u32(file, kind) || !write_u32(file, flags) || !write_u64(file, payload_size) || (payload_size && fwrite(payload, (size_t)payload_size, 1, file) != 1)) { writer->failed = true; return false; }
	++writer->next_sequence; return true;
}

bool rtdbg_trace_writer_close(struct rtdbg_trace_writer* writer) {
	if (!writer) return false; bool result = !writer->failed; if (writer->file && fclose((FILE*)writer->file) != 0) result = false; memset(writer, 0, sizeof(*writer)); return result;
}

bool rtdbg_trace_reader_open(struct rtdbg_trace_reader* reader, const char* path) {
	if (!reader || !path) return false;
	memset(reader, 0, sizeof(*reader)); FILE* file = fopen(path, "rb"); uint8_t magic[8]; uint16_t major, minor; uint32_t endian; uint64_t header_size;
	if (!file || fread(magic, sizeof(magic), 1, file) != 1 || memcmp(magic, rtdbg_trace_magic, sizeof(magic)) || !read_u16(file, &major) || !read_u16(file, &minor) || !read_u32(file, &endian) || !read_u64(file, &header_size) || major != 1 || minor != 0 || endian != 0x01020304u || header_size != 24) { if (file) fclose(file); return false; }
	reader->file = file; reader->expected_sequence = 1; return true;
}

bool rtdbg_trace_reader_next(struct rtdbg_trace_reader* reader, struct rtdbg_trace_record* record) {
	if (!reader || !record || !reader->file || reader->failed) return false;
	FILE* file = reader->file; uint64_t sequence, size; uint32_t kind, flags;
	if (fgetc(file) == EOF) return false;
	if (fseek(file, -1, SEEK_CUR) != 0 || !read_u64(file, &sequence) || !read_u32(file, &kind) || !read_u32(file, &flags) || !read_u64(file, &size) || sequence != reader->expected_sequence || !kind || size > rtdbg_max_payload_size || size > SIZE_MAX) { reader->failed = true; return false; }
	if (size > reader->payload_capacity) { uint8_t* payload = realloc(reader->payload, (size_t)size); if (!payload && size) { reader->failed = true; return false; } reader->payload = payload; reader->payload_capacity = size; }
	if (size && fread(reader->payload, (size_t)size, 1, file) != 1) { reader->failed = true; return false; }
	*record = (struct rtdbg_trace_record){sequence, kind, flags, reader->payload, size}; ++reader->expected_sequence; return true;
}

bool rtdbg_trace_reader_close(struct rtdbg_trace_reader* reader) {
	if (!reader) return false; bool result = !reader->failed; if (reader->file && fclose((FILE*)reader->file) != 0) result = false; free(reader->payload); memset(reader, 0, sizeof(*reader)); return result;
}
