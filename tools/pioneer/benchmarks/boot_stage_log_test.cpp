// Host-side contract tests; no Haiku runtime or running board required.
#include <assert.h>
#include <stdio.h>
#include <util/BootStageLog.h>

int main()
{
	BootStageLog<2> log;
	log.Reset(100);
	assert(log.Count() == 0 && log.Dropped() == 0);
	assert(log.Start() == 100 && log.Last() == 100);
	assert(log.Append("A", "first", 110));
	assert(log.Append("B", "second", 130));
	assert(log.At(0).time == 110 && log.At(0).delta == 10);
	assert(log.At(1).time == 130 && log.At(1).delta == 20);
	assert(!log.Append("C", "overflow", 150));
	assert(!log.Append("D", "overflow", 180));
	assert(log.Count() == 2 && log.Dropped() == 2);
	assert(log.Last() == 180 && log.At(1).time == 130);
	log.Reset(200);
	assert(log.Count() == 0 && log.Dropped() == 0);
	assert(log.Append("E", "same timestamp", 200));
	assert(log.At(0).delta == 0);

	BootStageLog<64> boot;
	boot.Reset(0);
	for (int i = 1; i <= 64; i++)
		assert(boot.Append("stage", "static description", i * 10));
	assert(boot.Count() == 64 && boot.Dropped() == 0);
	assert(boot.At(63).delta == 10);
	assert(!boot.Append("overflow", "static description", 650));
	assert(boot.Dropped() == 1 && boot.Last() == 650);
	puts("PASS: boot stage timestamps, capacity, overflow, reset");
}
