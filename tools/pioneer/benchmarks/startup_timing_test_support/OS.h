// Host-only shim: select with the test's explicit -I path, never in an OS build.
#ifndef STARTUP_TIMING_TEST_OS_H
#define STARTUP_TIMING_TEST_OS_H
#include <stdint.h>
#include <inttypes.h>
typedef int64_t bigtime_t;
typedef uint32_t uint32;
#define B_PRIdBIGTIME PRId64
#define B_PRIu32 PRIu32
bigtime_t system_time();
#endif
