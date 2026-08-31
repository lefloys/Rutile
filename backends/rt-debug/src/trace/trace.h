#ifndef RTDBG_TRACE_H
#define RTDBG_TRACE_H

/*
 * Pointer-free, little-endian trace storage for rt-debug.  Handles and host
 * addresses never cross this boundary: all references are trace-local IDs.
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef uint64_t rtdbg_id;

enum rtdbg_trace_record_kind {
	RTDBG_TRACE_SESSION_BEGIN = 1,
	RTDBG_TRACE_SESSION_END,
	RTDBG_TRACE_API_CALL,
	RTDBG_TRACE_RESOURCE_CREATE,
	RTDBG_TRACE_RESOURCE_DESTROY,
	RTDBG_TRACE_RESOURCE_SNAPSHOT,
	RTDBG_TRACE_COMMAND_BUFFER_BEGIN,
	RTDBG_TRACE_COMMAND,
	RTDBG_TRACE_COMMAND_BUFFER_END,
	RTDBG_TRACE_QUEUE_SUBMIT,
	RTDBG_TRACE_QUEUE_COMPLETE,
	RTDBG_TRACE_DRAW_BEGIN,
	RTDBG_TRACE_DRAW_END,
	RTDBG_TRACE_INVOCATION,
	RTDBG_TRACE_PIXEL_WRITE,
	RTDBG_TRACE_DIAGNOSTIC,
};

struct rtdbg_trace_record {
	uint64_t sequence;
	uint32_t kind;
	uint32_t flags;
	const uint8_t* payload;
	uint64_t payload_size;
};

struct rtdbg_trace_writer {
	void* file;
	uint64_t next_sequence;
	bool failed;
};

struct rtdbg_trace_reader {
	void* file;
	uint64_t expected_sequence;
	uint8_t* payload;
	uint64_t payload_capacity;
	bool failed;
};

bool rtdbg_trace_writer_open(struct rtdbg_trace_writer* writer, const char* path);
bool rtdbg_trace_writer_record(struct rtdbg_trace_writer* writer, uint32_t kind, uint32_t flags,
	const void* payload, uint64_t payload_size);
bool rtdbg_trace_writer_close(struct rtdbg_trace_writer* writer);

bool rtdbg_trace_reader_open(struct rtdbg_trace_reader* reader, const char* path);
bool rtdbg_trace_reader_next(struct rtdbg_trace_reader* reader, struct rtdbg_trace_record* record);
bool rtdbg_trace_reader_close(struct rtdbg_trace_reader* reader);

#endif
