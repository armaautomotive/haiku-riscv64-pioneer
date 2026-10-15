/* SPDX-License-Identifier: MIT */
#include <cassert>
#include <cstdint>
using uint32 = uint32_t;
using bigtime_t = int64_t;
using status_t = int32_t;
static constexpr status_t B_OK = 0, B_INTERRUPTED = -10, B_BAD_VALUE = -5;
static constexpr int B_SYSTEM_TIMEBASE = 0;
static bigtime_t now, expectedDeadline, advanceOnInterruption;
static unsigned calls, interruptCalls;
static status_t finalStatus;
static bigtime_t system_time() { return now; }
static status_t snooze_until(bigtime_t deadline, int timeBase)
{
	assert(deadline == expectedDeadline && timeBase == B_SYSTEM_TIMEBASE);
	calls++;
	if (calls <= interruptCalls) {
		now += advanceOnInterruption;
		return B_INTERRUPTED;
	}
	return finalStatus;
}
#include "pioneer_sleep.inc"
static void reset(unsigned interrupts, bigtime_t advance, status_t status = B_OK)
{
	now = 5000; expectedDeadline = 6000; calls = 0;
	interruptCalls = interrupts; advanceOnInterruption = advance;
	finalStatus = status;
}
int main()
{
	uint32 interruptions = 0;
	reset(0, 0);
	assert(wait_for_wakeup(&interruptions) == B_OK && calls == 1 && interruptions == 0);
	reset(2, 100);
	assert(wait_for_wakeup(&interruptions) == B_OK && calls == 3 && interruptions == 2);
	reset(0, 0, B_BAD_VALUE);
	assert(wait_for_wakeup(&interruptions) == B_BAD_VALUE && calls == 1 && interruptions == 2);
	reset(100, 1000);
	assert(wait_for_wakeup(&interruptions) == B_OK && calls == 1 && interruptions == 3);
	reset(100, 0);
	assert(wait_for_wakeup(&interruptions) == B_INTERRUPTED && calls == 32 && interruptions == 35);
}
