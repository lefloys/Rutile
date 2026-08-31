#include "procs.h"
#include "trace.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

void rtLayerSetNext(rt_proc_chain next);
const char* rtLayerGetName(void);
void rtInit(const char* const* features, usize feature_count);
void rtExit(void);
rt_command_buffer rtCommandBufferCreate(void);
void rtCommandBufferDestroy(rt_command_buffer command_buffer);
void rtCommandBufferBegin(rt_command_buffer command_buffer);
void rtCommandBufferEnd(rt_command_buffer command_buffer);
rt_queue rtQueueCreate(enum rt_queue_capability capability);
void rtQueueDestroy(rt_queue queue);
rt_timepoint rtQueueSubmit(rt_queue queue, rt_command_buffer command_buffer);
void rtCmdDraw(rt_command_buffer command_buffer, usize vertex_count, usize first_vertex);
void rtCmdSetViewport(rt_command_buffer command_buffer, usize x, usize y, usize width, usize height, f32 min_depth, f32 max_depth);

static int init_calls;
static int exit_calls;
static int draw_calls;
static int submit_calls;
static int command_buffer_storage;
static int queue_storage;

static void fake_init(const char* const* features, usize feature_count) { (void)features; (void)feature_count; ++init_calls; }
static void fake_exit(void) { ++exit_calls; }
static rt_command_buffer fake_command_buffer_create(void) { return (rt_command_buffer)&command_buffer_storage; }
static void fake_command_buffer_destroy(rt_command_buffer value) { assert(value == (rt_command_buffer)&command_buffer_storage); }
static void fake_command_buffer_begin(rt_command_buffer value) { assert(value == (rt_command_buffer)&command_buffer_storage); }
static void fake_command_buffer_end(rt_command_buffer value) { assert(value == (rt_command_buffer)&command_buffer_storage); }
static rt_queue fake_queue_create(enum rt_queue_capability capability) { assert(capability == RT_QUEUE_GRAPHICS); return (rt_queue)&queue_storage; }
static void fake_queue_destroy(rt_queue value) { assert(value == (rt_queue)&queue_storage); }
static rt_timepoint fake_queue_submit(rt_queue value, rt_command_buffer command) { assert(value == (rt_queue)&queue_storage && command == (rt_command_buffer)&command_buffer_storage); ++submit_calls; return (rt_timepoint){42}; }
static void fake_draw(rt_command_buffer value, usize vertices, usize first_vertex) { assert(value == (rt_command_buffer)&command_buffer_storage && vertices == 3 && first_vertex == 1); ++draw_calls; }
static void fake_set_viewport(rt_command_buffer value, usize x, usize y, usize width, usize height, f32 min_depth, f32 max_depth) { assert(value == (rt_command_buffer)&command_buffer_storage && x == 2 && y == 3 && width == 640 && height == 480 && min_depth == 0.0f && max_depth == 1.0f); }
static void fake_unused(void) {}

static rt_proc_t fake_get_proc(const struct rt_proc_chain* chain, const char* name) {
	(void)chain;
	if (!strcmp(name, "rtInit")) return (rt_proc_t)fake_init;
	if (!strcmp(name, "rtExit")) return (rt_proc_t)fake_exit;
	if (!strcmp(name, "rtCommandBufferCreate")) return (rt_proc_t)fake_command_buffer_create;
	if (!strcmp(name, "rtCommandBufferDestroy")) return (rt_proc_t)fake_command_buffer_destroy;
	if (!strcmp(name, "rtCommandBufferBegin")) return (rt_proc_t)fake_command_buffer_begin;
	if (!strcmp(name, "rtCommandBufferEnd")) return (rt_proc_t)fake_command_buffer_end;
	if (!strcmp(name, "rtQueueCreate")) return (rt_proc_t)fake_queue_create;
	if (!strcmp(name, "rtQueueDestroy")) return (rt_proc_t)fake_queue_destroy;
	if (!strcmp(name, "rtQueueSubmit")) return (rt_proc_t)fake_queue_submit;
	if (!strcmp(name, "rtCmdDraw")) return (rt_proc_t)fake_draw;
	if (!strcmp(name, "rtCmdSetViewport")) return (rt_proc_t)fake_set_viewport;
	return (rt_proc_t)fake_unused;
}

static void expect_record(struct rtdbg_trace_reader* reader, unsigned kind, const char* text) {
	struct rtdbg_trace_record record;
	assert(rtdbg_trace_reader_next(reader, &record));
	assert(record.kind == kind);
	assert(record.payload_size == strlen(text));
	assert(!memcmp(record.payload, text, record.payload_size));
}

int main(void) {
	const char* path = "rt-debug-test.bin";
	assert(!strcmp(rtLayerGetName(), "rt-debug"));
#if defined(_WIN32)
	assert(_putenv_s("RT_DEBUG_DISABLE_APP", "1") == 0);
	assert(_putenv_s("RT_DEBUG_TRACE_PATH", path) == 0);
#else
	assert(setenv("RT_DEBUG_TRACE_PATH", path, 1) == 0);
#endif
	rt_proc_chain chain = {fake_get_proc};
	rtLayerSetNext(chain);
	rtInit(NULL, 0);
	rt_command_buffer command = rtCommandBufferCreate();
	rt_queue submitted_queue = rtQueueCreate(RT_QUEUE_GRAPHICS);
	rtCommandBufferBegin(command);
	rtCmdDraw(command, 3, 1);
	rtCmdSetViewport(command, 2, 3, 640, 480, 0.0f, 1.0f);
	rtCommandBufferEnd(command);
	assert(rtQueueSubmit(submitted_queue, command).value == 42);
	rtCommandBufferDestroy(command);
	rtQueueDestroy(submitted_queue);
	rtExit();
	assert(init_calls == 1 && exit_calls == 1 && draw_calls == 1 && submit_calls == 1);

	struct rtdbg_trace_reader reader;
	assert(rtdbg_trace_reader_open(&reader, path));
	struct rtdbg_trace_record record;
	expect_record(&reader, RTDBG_TRACE_API_CALL, "rtInit");
	expect_record(&reader, RTDBG_TRACE_RESOURCE_CREATE, "rtCommandBufferCreate command-buffer #1");
	expect_record(&reader, RTDBG_TRACE_RESOURCE_CREATE, "rtQueueCreate queue #2");
	expect_record(&reader, RTDBG_TRACE_COMMAND_BUFFER_BEGIN, "rtCommandBufferBegin command-buffer #1");
	expect_record(&reader, RTDBG_TRACE_COMMAND, "rtCmdDraw command-buffer #1 count 3 instances 1");
	expect_record(&reader, RTDBG_TRACE_COMMAND, "rtCmdSetViewport command-buffer #1 x 2 y 3 width 640 height 480 depth 0 1");
	expect_record(&reader, RTDBG_TRACE_COMMAND_BUFFER_END, "rtCommandBufferEnd command-buffer #1");
	expect_record(&reader, RTDBG_TRACE_QUEUE_SUBMIT, "rtQueueSubmit queue #2 command-buffer #1 timepoint 42");
	expect_record(&reader, RTDBG_TRACE_RESOURCE_DESTROY, "rtCommandBufferDestroy command-buffer #1");
	expect_record(&reader, RTDBG_TRACE_RESOURCE_DESTROY, "rtQueueDestroy queue #2");
	expect_record(&reader, RTDBG_TRACE_API_CALL, "rtExit");
	assert(!rtdbg_trace_reader_next(&reader, &record));
	assert(rtdbg_trace_reader_close(&reader));
	remove(path);
	return 0;
}
