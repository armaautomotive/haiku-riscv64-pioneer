/* Distributed under the MIT License. */
#include <cassert>
#include <cstdint>
#include <cstring>
#include <cstdio>
#include <cinttypes>
#include <initializer_list>
#include "../../../src/system/boot/platform/riscv/pioneer_smp.h"
using uint64 = uint64_t;
using uint32 = uint32_t;
using addr_t = uint64_t;
using bigtime_t = int64_t;
#define B_PRIu64 PRIu64
#define B_PRIx64 PRIx64
enum { SBI_SUCCESS = 0, SBI_HART_STATE_STOPPED = 1 };
struct sbiret { long error, value; };
static struct { unsigned num_cpus; } gKernelArgs;
static PioneerApEntry sEntries[64];
static struct { unsigned plicContext; } sCpus[64];
static unsigned mode = 4, started, stopped = 1, startFail, entryState = 2;
static bool active = true;
static bigtime_t tick;
struct Stop { const char* reason; };
[[noreturn]] static void PioneerKexecStop(bool success, const char* reason) { assert(!success); throw Stop{reason}; }
static bool PioneerKexecPreloadActive() { return active; }
static unsigned PioneerKexecMode() { return mode; }
static void pioneer_ap_entry() {}
static bigtime_t system_time() { return (tick += 1000000); }
static sbiret sbi_hart_get_status(uint64) { return {0, stopped}; }
static sbiret sbi_hart_start(uint64 hart, addr_t target, addr_t opaque)
{
	assert(target == (addr_t)&pioneer_ap_entry);
	PioneerApEntry* entry = (PioneerApEntry*)opaque;
	assert(entry->hart == hart && entry->cpu != 0 && entry->state == 0);
	started++;
	if (startFail) return {-1, 0};
	entry->state = entryState;
	return {0, 0};
}
static void mock_dprintf(const char*, ...) {}
#define dprintf mock_dprintf
static void InvokeStart(uint64 pageTable, uint64 kernel_entry)
#include "pioneer_smp_start.inc"
#undef dprintf

static void Prepare(unsigned count)
{
	gKernelArgs.num_cpus = count;
	started = startFail = 0; stopped = 1; entryState = 2; tick = 0; active = true; mode = 4;
	for (unsigned i = 0; i < 64; i++) {
		sEntries[i] = {};
		sEntries[i].satp = 123; sEntries[i].kernelEntry = 456;
		sEntries[i].cpu = i; sEntries[i].hart = i ? i - 1 : 63;
	}
}
static void ExpectStop(const char* reason)
{
	try { InvokeStart(123, 456); assert(false); }
	catch (const Stop& stop) { assert(strcmp(reason, stop.reason) == 0); }
}

int main()
{
	PioneerCpuTopology topology = {};
	PioneerCpuRecord selected[64];
	assert(!PioneerSelectCpus(topology, 1, 64, selected));
	for (unsigned i = 0; i < 64; i++) assert(PioneerAddCpu(topology, 63 - i, 100 + 63 - i));
	assert(!PioneerAddCpu(topology, 1, 101));
	assert(!PioneerAddCpu(topology, 64, 164));
	assert(!PioneerAddCpuContext(topology, 0, 9, 0));
	for (unsigned i = 0; i < 64; i++) {
		assert(PioneerAddCpuContext(topology, 100 + i, i & 1 ? 11 : UINT32_MAX, 2 * i));
		assert(PioneerAddCpuContext(topology, 100 + i, 9, 2 * i + 1));
	}
	assert(!PioneerAddCpuContext(topology, 101, 9, 3));
	for (unsigned boot = 0; boot < 64; boot++) {
		for (unsigned count = 1; count <= 64; count *= 2) {
			assert(PioneerSelectCpus(topology, boot, count, selected));
			assert(selected[0].hart == boot);
			uint64 mask = 0;
			for (unsigned i = 0; i < count; i++) {
				assert(!(mask & (UINT64_C(1) << selected[i].hart)));
				mask |= UINT64_C(1) << selected[i].hart;
				assert(selected[i].phandle == 100 + selected[i].hart);
				assert(selected[i].context == 2 * selected[i].hart + 1);
			}
			if (count == 64) assert(mask == UINT64_MAX);
		}
	}
	for (unsigned count : {0, 3, 63, 65}) assert(!PioneerSelectCpus(topology, 1, count, selected));
	assert(!PioneerSelectCpus(topology, 64, 64, selected));
	auto bad = topology; bad.contexts &= ~1ULL;
	assert(!PioneerSelectCpus(bad, 1, 64, selected));
	bad = {}; assert(!PioneerAddCpu(bad, 0, 0)); assert(!PioneerAddCpu(bad, 0, UINT32_MAX));
	assert(PioneerAddCpu(bad, 0, 1)); assert(!PioneerAddCpu(bad, 1, 1));
	assert(!PioneerAddCpuContext(bad, 1, 9, 128));
	assert(!PioneerAddCpuContext(bad, 1, 7, 0));
	for (unsigned count : {1, 2, 8, 64}) { Prepare(count); InvokeStart(123, 456); assert(started == count - 1); }
	Prepare(64); active = false; InvokeStart(123, 456); assert(started == 0);
	Prepare(64); mode = 3; ExpectStop("AP_start_requires_execution_mode"); assert(started == 0);
	Prepare(64); sEntries[1].satp++; ExpectStop("AP_descriptor_changed"); assert(started == 0);
	Prepare(64); stopped = 0; ExpectStop("AP_not_stopped"); assert(started == 0);
	Prepare(64); startFail = 1; ExpectStop("AP_HSM_start_failed"); assert(started == 1);
	Prepare(64); entryState = 3; ExpectStop("AP_entry_failed"); assert(started == 1);
	Prepare(64); entryState = 4; ExpectStop("AP_entry_failed"); assert(started == 1);
	Prepare(64); entryState = 0; ExpectStop("AP_entry_timeout"); assert(started == 1);
	puts("PASS: complete/unique CPU+PLIC topology, boot-hart ordering, bounded production HSM startup and refusal paths");
}
