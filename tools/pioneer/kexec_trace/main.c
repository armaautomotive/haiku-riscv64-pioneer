/* Distributed under the MIT License. Native Linux userspace only. */
#define _DEFAULT_SOURCE
#define _POSIX_C_SOURCE 200809L
#include "trace.h"
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <poll.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/utsname.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>

static uint64_t
now_ms(void* context)
{
	(void)context;
	struct timespec value;
	if (clock_gettime(CLOCK_MONOTONIC, &value) != 0)
		return UINT64_MAX;
	return (uint64_t)value.tv_sec * 1000 + (uint64_t)value.tv_nsec / 1000000;
}

static ssize_t
write_tty(void* context, const char* bytes, size_t length)
{
	ssize_t result = write(*(int*)context, bytes, length);
	return result < 0 ? -errno : result;
}

static int
wait_tty(void* context, unsigned int milliseconds)
{
	struct pollfd descriptor = { .fd = *(int*)context, .events = POLLOUT };
	int result = poll(&descriptor, 1, (int)milliseconds);
	if (result < 0)
		return -errno;
	return descriptor.revents & (POLLERR | POLLHUP | POLLNVAL) ? -EIO : 0;
}

static void
deadline(int signalNumber)
{
	(void)signalNumber;
	_exit(124);
}

int
main(int argc, char** argv)
{
	if (argc != 2)
		return 0;
	int selfTest = strcmp(argv[1], "--self-test") == 0;
	if (!selfTest && strcmp(argv[1], "kexec") != 0)
		return 0;
	if (geteuid() != 0)
		return 1;
	signal(SIGALRM, deadline);
	alarm(3);
	struct utsname identity;
	if (uname(&identity) != 0 || strcmp(identity.machine, "riscv64") != 0)
		return 1;
	char uartPath[PATH_MAX];
	if (realpath("/sys/class/tty/ttyS0/device", uartPath) == NULL
		|| strcmp(uartPath, "/sys/devices/platform/soc/7040000000.serial") != 0)
		return 1;
	int armed = open("/run/pioneer-kexec-trace/armed",
		O_RDONLY | O_CLOEXEC | O_NOFOLLOW | O_NONBLOCK);
	if (armed < 0)
		return selfTest ? 1 : 0;
	struct stat state;
	char token[33];
	int valid = fstat(armed, &state) == 0 && S_ISREG(state.st_mode)
		&& state.st_uid == 0 && state.st_gid == 0 && state.st_nlink == 1
		&& (state.st_mode & 07777) == 0600 && state.st_size == (off_t)sizeof(token)
		&& read(armed, token, sizeof(token)) == (ssize_t)sizeof(token)
		&& token[32] == '\n' && pioneer_trace_token_valid(token, 32);
	close(armed);
	if (!valid)
		return 1;
	int loaded = open("/sys/kernel/kexec_loaded", O_RDONLY | O_CLOEXEC);
	char loadedState[2];
	valid = loaded >= 0 && read(loaded, loadedState, sizeof(loadedState)) == 2
		&& loadedState[0] == (selfTest ? '0' : '1') && loadedState[1] == '\n';
	if (loaded >= 0)
		close(loaded);
	if (!valid)
		return 1;
	int tty = open("/dev/ttyS0", O_WRONLY | O_NOCTTY | O_NONBLOCK | O_CLOEXEC | O_NOFOLLOW);
	if (tty < 0)
		return 1;
	struct termios settings;
	if (fstat(tty, &state) != 0 || !S_ISCHR(state.st_mode)
		|| tcgetattr(tty, &settings) != 0)
		return 1;
	cfmakeraw(&settings);
	settings.c_cflag |= CLOCAL | CREAD;
	settings.c_cflag &= ~(CRTSCTS | HUPCL);
	if (cfsetispeed(&settings, B115200) != 0 || cfsetospeed(&settings, B115200) != 0
		|| tcsetattr(tty, TCSANOW, &settings) != 0)
		return 1;
	const struct PioneerTraceOps ops = { now_ms, write_tty, wait_tty };
	if (pioneer_trace_emit(&ops, &tty, token, 32, selfTest) != 0)
		return 1;
	uint64_t started = now_ms(NULL);
	for (unsigned int attempt = 0; attempt < 100; attempt++) {
		int pending = 0;
		if (ioctl(tty, TIOCOUTQ, &pending) != 0)
			return 1;
		if (pending == 0) {
			struct timespec finish = { 0, 20000000 };
			nanosleep(&finish, NULL);
			close(tty);
			return 0;
		}
		if (now_ms(NULL) - started >= 1000)
			return 1;
		struct timespec interval = { 0, 10000000 };
		nanosleep(&interval, NULL);
	}
	return 1;
}
