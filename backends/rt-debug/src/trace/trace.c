#include "trace.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const uint8_t rtdbg_trace_magic[8] = {'R', 'T', 'D', 'B', 'G', 'T', 'R', 'C'};
static const uint16_t rtdbg_trace_version_major = 1;
static const uint16_t rtdbg_trace_version_minor = 0;
static const uint64_t rtdbg_max_payload_size = UINT64_C(1) << 34;

static bool rtdbg_write_u16(FILE* file, uint16_t value) {
	uint8_t bytes[2] = {(uint8_t)value, (uint8_t)(value >> 8)};
	return fwrite(bytes, sizeof(bytes), 1, file) == 1;
}
static bool rtdbg_write_u32(FILE* file, uint32_t value) {
	uint8_t bytes[4];
	for (unsigned shift = 0; shift < 32; shift += 8) bytes[shift / 8] = (uint8_t)(value >> shift);
	return fwrite(bytes, sizeof(bytes), 1, file) == 1;
}
static bool rtdbg_write_u64(FILE* file, uint64_t value) {
	uint8_t bytes[8];
	for (unsigned shift = 0; shift < 64; shift += 8) bytes[shift / 8] = (uint8_t)(value >> shift);
	return fwrite(bytes, sizeof(bytes), 1, file) == 1;
}
static bool rtdbg_read_u16(FILE* file, uint16_t* value) {
	uint8_t bytes[2]; if (fread(bytes, sizeof(bytes), 1, file) != 1) return false;
	*value = (uint16_t)(bytes[0] | (uint16_t)bytes[1] << 8); return true;
}
static bool rtdbg_read_u32(FILE* file, uint32_t* value) {
	uint8_t bytes[4]; if (fread(bytes, sizeof(bytes), 1, file) != 1) return false;
	*value = 0; for (unsigned shift = 0; shift < 32; shift += 8) *value |= (uint32_t)bytes[shift / 8] << shift; return true;
}
static bool rtdbg_read_u64(FILE* file, uint64_t* value) {
	uint8_t bytes[8]; if (fread(bytes, sizeof(bytes), 1, file) != 1) return false;
	*value = 0; for (unsigned shift = 0; shift < 64; shift += 8) *value |= (uint64_t)bytes[shift / 8] << shift; return true;
}

bool rtdbg_trace_writer_open(struct rtdbg_trace_writer* writer, const char* path) {
	if (!writer || !path) return false;
	memset(writer, 0, sizeof(*writer));
	FILE* file = fopen(path, "wb");
	if (!file) return false;
	if (fwrite(rtdbg_trace_magic, sizeof(rtdbg_trace_magic), 1, file) != 1 ||
		!rtdbg_write_u16(file, rtdbg_trace_version_major) ||
		!rtdbg_write_u16(file, rtdbg_trace_version_minor) ||
		!rtdbg_write_u32(file, 0x01020304u) || !rtdbg_write_u64(file, 24)) {
		fclose(file); return false;
	}
	writer->file = file;
	writer->next_sequence = 1;
	return true;
}

bool rtdbg_trace_writer_record(struct rtdbg_trace_writer* writer, uint32_t kind, uint32_t flags,
	const void* payload, uint64_t payload_size) {
	if (!writer || !writer->file || writer->failed || kind == 0 || payload_size > rtdbg_max_payload_size || (!payload && payload_size)) return false;
	FILE* file = writer->file;
	if (!rtdbg_write_u64(file, writer->next_sequence) || !rtdbg_write_u32(file, kind) ||
		!rtdbg_write_u32(file, flags) || !rtdbg_write_u64(file, payload_size) ||
		(payload_size && fwrite(payload, (size_t)payload_size, 1, file) != 1)) {
		writer->failed = true; return false;
	}
	++writer->next_sequence;
	return true;
}

bool rtdbg_trace_writer_close(struct rtdbg_trace_writer* writer) {
	if (!writer) return false;
	bool result = !writer->failed;
	if (writer->file && fclose((FILE*)writer->file) != 0) result = false;
	memset(writer, 0, sizeof(*writer));
	return result;
}

bool rtdbg_trace_reader_open(struct rtdbg_trace_reader* reader, const char* path) {
	if (!reader || !path) return false;
	memset(reader, 0, sizeof(*reader));
	FILE* file = fopen(path, "rb");
	uint8_t magic[8]; uint16_t major, minor; uint32_t endian; uint64_t header_size;
	if (!file || fread(magic, sizeof(magic), 1, file) != 1 || memcmp(magic, rtdbg_trace_magic, sizeof(magic)) != 0 ||
		!rtdbg_read_u16(file, &major) || !rtdbg_read_u16(file, &minor) || !rtdbg_read_u32(file, &endian) || !rtdbg_read_u64(file, &header_size) ||
		major != rtdbg_trace_version_major || minor > rtdbg_trace_version_minor || endian != 0x01020304u || header_size != 24) {
		if (file) fclose(file); return false;
	}
	reader->file = file; reader->expected_sequence = 1;
	return true;
}

bool rtdbg_trace_reader_next(struct rtdbg_trace_reader* reader, struct rtdbg_trace_record* record) {
	if (!reader || !record || !reader->file || reader->failed) return false;
	FILE* file = reader->file;
	uint64_t sequence, size; uint32_t kind, flags;
	if (fgetc(file) == EOF) return false;
	if (fseek(file, -1, SEEK_CUR) != 0 || !rtdbg_read_u64(file, &sequence) || !rtdbg_read_u32(file, &kind) || !rtdbg_read_u32(file, &flags) || !rtdbg_read_u64(file, &size) ||
		sequence != reader->expected_sequence || kind == 0 || size > rtdbg_max_payload_size || size > SIZE_MAX) { reader->failed = true; return false; }
	if (size > reader->payload_capacity) {
		uint8_t* payload = realloc(reader->payload, (size_t)size);
		if (!payload && size) { reader->failed = true; return false; }
		reader->payload = payload; reader->payload_capacity = size;
	}
	if (size && fread(reader->payload, (size_t)size, 1, file) != 1) { reader->failed = true; return false; }
	*record = (struct rtdbg_trace_record){sequence, kind, flags, reader->payload, size};
	++reader->expected_sequence;
	return true;
}

bool rtdbg_trace_reader_close(struct rtdbg_trace_reader* reader) {
	if (!reader) return false;
	bool result = !reader->failed;
	if (reader->file && fclose((FILE*)reader->file) != 0) result = false;
	free(reader->payload); memset(reader, 0, sizeof(*reader)); return result;
}
