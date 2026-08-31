#ifndef RTDBG_TRACE_H
#define RTDBG_TRACE_H

#include <stdbool.h>
#include <stdint.h>

enum rtdbg_trace_record_kind {
	RTDBG_TRACE_API_CALL = 1,
	RTDBG_TRACE_RESOURCE_CREATE,
	RTDBG_TRACE_RESOURCE_DESTROY,
	RTDBG_TRACE_COMMAND_BUFFER_BEGIN,
	RTDBG_TRACE_COMMAND,
	RTDBG_TRACE_COMMAND_BUFFER_END,
	RTDBG_TRACE_QUEUE_SUBMIT,
	RTDBG_TRACE_DIAGNOSTIC,
};

struct rtdbg_trace_record {
	uint64_t sequence;
	uint32_t kind;
	uint32_t flags;
	const uint8_t* payload;
	uint64_t payload_size;
};

struct rtdbg_trace_writer { void* file; uint64_t next_sequence; bool failed; };
struct rtdbg_trace_reader { void* file; uint64_t expected_sequence; uint8_t* payload; uint64_t payload_capacity; bool failed; };

bool rtdbg_trace_writer_open(struct rtdbg_trace_writer* writer, const char* path);
bool rtdbg_trace_writer_record(struct rtdbg_trace_writer* writer, uint32_t kind, uint32_t flags, const void* payload, uint64_t payload_size);
bool rtdbg_trace_writer_close(struct rtdbg_trace_writer* writer);
bool rtdbg_trace_reader_open(struct rtdbg_trace_reader* reader, const char* path);
bool rtdbg_trace_reader_next(struct rtdbg_trace_reader* reader, struct rtdbg_trace_record* record);
bool rtdbg_trace_reader_close(struct rtdbg_trace_reader* reader);

void rtdbg_trace_open(void);
void rtdbg_trace_close(void);
void rtdbg_trace_api(const char* name);
void rtdbg_trace_resource_create(const char* name, const char* type, const void* handle);
void rtdbg_trace_resource_destroy(const char* name, const char* type, const void* handle);
void rtdbg_trace_resource_detail(const void* handle, const char* format, ...);
void rtdbg_trace_command_buffer(const char* name, const void* command_buffer);
void rtdbg_trace_queue_submit(const void* queue, const void* command_buffer, uint64_t timepoint);
void rtdbg_trace_draw(const char* name, const void* command_buffer, uint64_t primitive_count, uint64_t instance_count);
void rtdbg_trace_event(uint32_t kind, const char* format, ...);
uint64_t rtdbg_trace_handle_id(const void* handle);

#endif
