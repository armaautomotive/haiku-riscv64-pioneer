/* Distributed under the MIT License. */
#include "trace.h"
#include <errno.h>
#include <stdio.h>

int
pioneer_trace_token_valid(const char* token, size_t length)
{
	if (token == NULL || length != 32)
		return 0;
	for (size_t index = 0; index < length; index++) {
		if (!((token[index] >= '0' && token[index] <= '9')
			|| (token[index] >= 'a' && token[index] <= 'f')))
			return 0;
	}
	return 1;
}

int
pioneer_trace_emit(const struct PioneerTraceOps* ops, void* context,
	const char* token, size_t length, int selfTest)
{
	if (!pioneer_trace_token_valid(token, length) || ops == NULL
		|| ops->now == NULL || ops->write == NULL || ops->wait == NULL)
		return -EINVAL;
	char message[192];
	int count = snprintf(message, sizeof(message),
		"\r\nPIONEER_KEXEC_TRACE v1 stage=%s action=%s token=%.*s kexec_loaded=%d\r\n",
		selfTest ? "self_test" : "late_shutdown", selfTest ? "none" : "kexec",
		(int)length, token, selfTest ? 0 : 1);
	if (count <= 0 || (size_t)count >= sizeof(message))
		return -EINVAL;
	uint64_t started = ops->now(context);
	if (started == UINT64_MAX)
		return -ETIMEDOUT;
	size_t sent = 0;
	for (unsigned int attempt = 0; attempt < 512; attempt++) {
		uint64_t now = ops->now(context);
		if (now == UINT64_MAX || now < started || now - started >= 1000)
			return -ETIMEDOUT;
		ssize_t written = ops->write(context, message + sent, (size_t)count - sent);
		if (written > 0) {
			if ((size_t)written > (size_t)count - sent)
				return -EIO;
			sent += (size_t)written;
			if (sent == (size_t)count)
				return 0;
		} else if (written == -EAGAIN) {
			int result = ops->wait(context, 20);
			if (result < 0 && result != -EINTR)
				return result;
		} else if (written != -EINTR)
			return written < 0 ? (int)written : -EIO;
	}
	return -ETIMEDOUT;
}
