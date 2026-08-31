#include "trace.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

int main(void) {
	const char* path = "rt-debug-trace-test.bin";
	struct rtdbg_trace_writer writer;
	assert(rtdbg_trace_writer_open(&writer, path));
	assert(rtdbg_trace_writer_record(&writer, RTDBG_TRACE_API_CALL, 0, "rtInit", 6));
	assert(rtdbg_trace_writer_record(&writer, RTDBG_TRACE_COMMAND, 7, "rtCmdDraw", 9));
	assert(rtdbg_trace_writer_record(&writer, RTDBG_TRACE_API_CALL, 0, "rtExit", 6));
	assert(rtdbg_trace_writer_close(&writer));

	struct rtdbg_trace_reader reader;
	struct rtdbg_trace_record record;
	assert(rtdbg_trace_reader_open(&reader, path));
	assert(rtdbg_trace_reader_next(&reader, &record));
	assert(record.sequence == 1 && record.kind == RTDBG_TRACE_API_CALL && record.payload_size == 6);
	assert(!memcmp(record.payload, "rtInit", 6));
	assert(rtdbg_trace_reader_next(&reader, &record));
	assert(record.sequence == 2 && record.kind == RTDBG_TRACE_COMMAND && record.flags == 7 && record.payload_size == 9);
	assert(!memcmp(record.payload, "rtCmdDraw", 9));
	assert(rtdbg_trace_reader_next(&reader, &record));
	assert(record.sequence == 3 && record.kind == RTDBG_TRACE_API_CALL && record.payload_size == 6);
	assert(!memcmp(record.payload, "rtExit", 6));
	assert(!rtdbg_trace_reader_next(&reader, &record));
	assert(rtdbg_trace_reader_close(&reader));
	remove(path);
	return 0;
}
