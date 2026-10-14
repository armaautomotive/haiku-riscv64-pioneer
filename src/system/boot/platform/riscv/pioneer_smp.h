/* Distributed under the MIT License. */
#ifndef HAIKU_PIONEER_SMP_H
#define HAIKU_PIONEER_SMP_H

#include <stdint.h>
#include <stddef.h>

struct PioneerCpuRecord {
	uint32_t hart, phandle, context;
};

struct PioneerCpuTopology {
	PioneerCpuRecord cpus[64];
	uint64_t present, contexts;
};

static inline bool
PioneerCpuCountValid(uint32_t count)
{
	return count != 0 && count <= 64 && (count & (count - 1)) == 0;
}

static inline bool
PioneerAddCpu(PioneerCpuTopology& topology, uint32_t hart, uint32_t phandle)
{
	if (hart >= 64 || phandle == 0 || phandle == UINT32_MAX
		|| (topology.present & (UINT64_C(1) << hart)) != 0)
		return false;
	for (unsigned i = 0; i < 64; i++) {
		if ((topology.present & (UINT64_C(1) << i)) && topology.cpus[i].phandle == phandle)
			return false;
	}
	topology.cpus[hart] = {hart, phandle, UINT32_MAX};
	topology.present |= UINT64_C(1) << hart;
	return true;
}

static inline bool
PioneerAddCpuContext(PioneerCpuTopology& topology, uint32_t phandle,
	uint32_t interrupt, uint32_t context)
{
	if (context >= 128 || (interrupt != 9 && interrupt != 11 && interrupt != UINT32_MAX)) return false;
	for (unsigned i = 0; i < 64; i++) {
		if (!(topology.present & (UINT64_C(1) << i)) || topology.cpus[i].phandle != phandle)
			continue;
		// Pioneer marks unavailable machine-mode slots with ffffffff. Keep
		// their positions in the PLIC context numbering, never assign them.
		if (interrupt != 9) return true;
		if (topology.contexts & (UINT64_C(1) << i)) return false;
		for (unsigned j = 0; j < 64; j++) {
			if ((topology.contexts & (UINT64_C(1) << j)) && topology.cpus[j].context == context)
				return false;
		}
		topology.cpus[i].context = context;
		topology.contexts |= UINT64_C(1) << i;
		return true;
	}
	return false;
}

static inline bool
PioneerSelectCpus(const PioneerCpuTopology& topology, uint32_t bootHart,
	uint32_t count, PioneerCpuRecord selected[64])
{
	if (!PioneerCpuCountValid(count) || bootHart >= 64
		|| topology.present != UINT64_MAX || topology.contexts != UINT64_MAX)
		return false;
	selected[0] = topology.cpus[bootHart];
	unsigned next = 1;
	for (unsigned hart = 0; hart < 64 && next < count; hart++) {
		if (hart != bootHart) selected[next++] = topology.cpus[hart];
	}
	return next == count;
}

// Assembly ABI. Static loader storage, identity-mapped and reserved together
// with the loader; never allocate descriptors after final page-table setup.
struct PioneerApEntry {
	uint64_t satp, stackBase, stackSize, kernelEntry, kernelArgs, cpu, hart;
	volatile uint64_t state; // 0 not entered, 1 physical entry, 2 final mappings.
	uint64_t mae;
	volatile uint64_t cause, epc, tval;
};
static_assert(sizeof(PioneerApEntry) == 96, "AP descriptor ABI");
static_assert(offsetof(PioneerApEntry, state) == 56, "AP state ABI");
static_assert(offsetof(PioneerApEntry, mae) == 64, "AP MAEE ABI");
static_assert(offsetof(PioneerApEntry, cause) == 72, "AP trap ABI");

static inline void PioneerSmpMemoryBarrier()
{
#ifdef __riscv
	asm volatile("fence rw, rw" ::: "memory");
#else
	__atomic_thread_fence(__ATOMIC_SEQ_CST);
#endif
}

#endif
