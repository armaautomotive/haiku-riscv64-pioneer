/* Distributed under the MIT License. */
#include "../kexec_trace/trace.h"
#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

struct Fixture {
	char output[256];
	size_t length;
	size_t quota;
	unsigned int calls;
	unsigned int waits;
	uint64_t clock;
	int mode;
};

static uint64_t
now(void* context)
{
	struct Fixture* f = context;
	if (f->mode == 7)
		return 0;
	if (f->mode == 8)
		return f->clock++ == 0 ? 100 : 99;
	if (f->mode == 9)
		return UINT64_MAX;
	return f->clock++;
}

static ssize_t
write_bytes(void* context, const char* bytes, size_t length)
{
	struct Fixture* f = context;
	f->calls++;
	if (f->mode == 2 && f->calls == 1)
		return -EINTR;
	if ((f->mode == 2 && f->calls == 2) || f->mode == 3)
		return -EAGAIN;
	if (f->mode == 4)
		return -EIO;
	if (f->mode == 5)
		return 0;
	if (f->mode == 6)
		return (ssize_t)length + 1;
	if (f->mode == 7)
		return -EINTR;
	if (f->quota && length > f->quota)
		length = f->quota;
	assert(f->length + length < sizeof(f->output));
	memcpy(f->output + f->length, bytes, length);
	f->length += length;
	f->output[f->length] = 0;
	return (ssize_t)length;
}

static int
wait_bytes(void* context, unsigned int milliseconds)
{
	struct Fixture* f = context;
	f->waits++;
	f->clock += milliseconds;
	return 0;
}

int
main(void)
{
	const char* token = "0123456789abcdef0123456789abcdef";
	const struct PioneerTraceOps ops = { now, write_bytes, wait_bytes };
	assert(pioneer_trace_token_valid(token, 32));
	assert(!pioneer_trace_token_valid(NULL, 32));
	assert(!pioneer_trace_token_valid(token, 31));
	assert(!pioneer_trace_token_valid(token, 33));
	assert(!pioneer_trace_token_valid("G123456789abcdef0123456789abcdef", 32));
	assert(!pioneer_trace_token_valid("0123456789abcdef0123456789abcde\n", 32));
	struct Fixture f = {0};
	assert(pioneer_trace_emit(&ops, &f, token, 32, 0) == 0);
	assert(strstr(f.output, "stage=late_shutdown action=kexec") != NULL);
	assert(strstr(f.output, "kexec_loaded=1\r\n") != NULL);
	f = (struct Fixture){ .quota = 1 };
	assert(pioneer_trace_emit(&ops, &f, token, 32, 1) == 0);
	assert(strstr(f.output, "stage=self_test action=none") != NULL);
	assert(strstr(f.output, "kexec_loaded=0\r\n") != NULL);
	f = (struct Fixture){ .mode = 2, .quota = 3 };
	assert(pioneer_trace_emit(&ops, &f, token, 32, 0) == 0 && f.waits == 1);
	const int expected[] = { -ETIMEDOUT, -EIO, -EIO, -EIO,
		-ETIMEDOUT, -ETIMEDOUT, -ETIMEDOUT };
	for (int mode = 3; mode <= 9; mode++) {
		f = (struct Fixture){ .mode = mode };
		assert(pioneer_trace_emit(&ops, &f, token, 32, 0) == expected[mode - 3]);
		assert(f.calls <= 512);
	}
	f = (struct Fixture){0};
	assert(pioneer_trace_emit(NULL, &f, token, 32, 0) == -EINVAL);
	assert(pioneer_trace_emit(&ops, &f, token, 31, 0) == -EINVAL && f.calls == 0);
	puts("kexec shutdown trace core tests: PASS");
	return 0;
}
