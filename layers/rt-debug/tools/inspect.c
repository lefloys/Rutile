#include "trace.h"

#include <stdio.h>

static const char* kind_name(unsigned kind) {
	switch (kind) {
	case RTDBG_TRACE_API_CALL: return "api";
	case RTDBG_TRACE_RESOURCE_CREATE: return "resource-create";
	case RTDBG_TRACE_RESOURCE_DESTROY: return "resource-destroy";
	case RTDBG_TRACE_COMMAND_BUFFER_BEGIN: return "command-buffer-begin";
	case RTDBG_TRACE_COMMAND_BUFFER_END: return "command-buffer-end";
	case RTDBG_TRACE_COMMAND: return "command";
	case RTDBG_TRACE_QUEUE_SUBMIT: return "queue-submit";
	default: return "unknown";
	}
}

int main(int argc, char** argv) {
	if (argc != 2) { fprintf(stderr, "usage: rt-debug-inspect <trace>\n"); return 2; }
	struct rtdbg_trace_reader reader;
	if (!rtdbg_trace_reader_open(&reader, argv[1])) { fprintf(stderr, "cannot read %s\n", argv[1]); return 1; }
	struct rtdbg_trace_record record;
	while (rtdbg_trace_reader_next(&reader, &record)) {
		printf("%llu %-21s", (unsigned long long)record.sequence, kind_name(record.kind));
		if (record.payload_size) printf(" %.*s", (int)record.payload_size, (const char*)record.payload);
		putchar('\n');
	}
	return rtdbg_trace_reader_close(&reader) ? 0 : 1;
}
