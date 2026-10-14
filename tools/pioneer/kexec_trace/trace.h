/* Distributed under the MIT License. */
#ifndef PIONEER_KEXEC_TRACE_H
#define PIONEER_KEXEC_TRACE_H
#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>

struct PioneerTraceOps {
	uint64_t (*now)(void* context);
	ssize_t (*write)(void* context, const char* bytes, size_t length);
	int (*wait)(void* context, unsigned int milliseconds);
};

int pioneer_trace_token_valid(const char* token, size_t length);
int pioneer_trace_emit(const struct PioneerTraceOps* ops, void* context,
	const char* token, size_t length, int selfTest);
#endif
