/* Distributed under the MIT License. */
#include <cassert>
#include <cstdint>
#include <cstdio>

static unsigned count, cpu, phase, atomicSets, atomicClears, sets, clears, fpuCalls;
struct Mask {
	uint64_t bits;
	void SetBit(unsigned bit) { assert(phase == 0); bits |= UINT64_C(1) << bit; sets++; }
	void SetBitAtomic(unsigned bit) { SetBit(bit); atomicSets++; }
	void ClearBit(unsigned bit) { assert(phase == 2); bits &= ~(UINT64_C(1) << bit); clears++; }
	void ClearBitAtomic(unsigned bit) { ClearBit(bit); atomicClears++; }
};
struct RISCV64VMTranslationMap {
	Mask active;
	uintptr_t satp;
	Mask& ActiveOnCpus() { return active; }
	uintptr_t Satp() { return satp; }
};
struct AddressSpace {
	RISCV64VMTranslationMap* map;
	void* TranslationMap() { return map; }
};
struct Team { AddressSpace* address_space; };
struct Cpu { int cpu_num; };
struct Thread { Team* team; Cpu* cpu; struct { int context, fpuContext; } arch_info; };
static RISCV64VMTranslationMap* oldMap;
static RISCV64VMTranslationMap* newMap;
static unsigned smp_get_num_cpus() { return count; }
static void save_fpu(int*) { assert(phase == 0); fpuCalls++; }
static void restore_fpu(int*) { assert(phase == 0); fpuCalls++; }
static void SetSatp(uintptr_t satp)
{
	assert(phase++ == 0 && satp == newMap->satp);
	assert(oldMap->active.bits & (UINT64_C(1) << cpu));
	assert(newMap->active.bits & (UINT64_C(1) << cpu));
}
static void FlushTlbAllAsid(unsigned asid) { assert(phase++ == 1 && asid == 0); }
static void arch_context_switch(int*, int*)
{
	assert(phase++ == 2);
	assert(newMap->active.bits & (UINT64_C(1) << cpu));
	if (oldMap != newMap) assert(!(oldMap->active.bits & (UINT64_C(1) << cpu)));
}
static void InvokeContextSwitch(Thread* from, Thread* to)
#include "pioneer_context_switch.inc"

int main()
{
	for (count = 1; count <= 64; count *= 2) {
		for (cpu = 0; cpu < count; cpu++) {
			for (unsigned same = 0; same < 2; same++) {
				phase = atomicSets = atomicClears = sets = clears = fpuCalls = 0;
				uint64_t other = UINT64_C(1) << ((cpu + 1) % 64);
				RISCV64VMTranslationMap maps[] = {{{other | (UINT64_C(1) << cpu)}, 1}, {{other}, 2}};
				oldMap = &maps[0]; newMap = &maps[same ? 0 : 1];
				AddressSpace spaces[] = {{oldMap}, {newMap}};
				Team teams[] = {{&spaces[0]}, {&spaces[1]}};
				Cpu current = {(int)cpu};
				Thread from = {&teams[0], &current, {}}, to = {&teams[1], &current, {}};
				InvokeContextSwitch(&from, &to);
				assert(phase == 3 && fpuCalls == 2 && sets == 1 && clears == !same);
				assert(atomicSets == (count > 1) && atomicClears == (count > 1 && !same));
				assert(newMap->active.bits == (other | (UINT64_C(1) << cpu)));
				assert(oldMap->active.bits == (same ? newMap->active.bits : other));
			}
		}
	}
	puts("PASS: production context switch preserves same-map CPU membership and clears old map only after SATP/TLB switch");
}
