/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_ROOT_CYCLE_PROFILE_H
#define PIONEER_Q38_ROOT_CYCLE_PROFILE_H
// DIAGNOSTIC ONLY. Pinned official tt-isa8a34e25e,Blackhole baby-RISC
// CSRs.md:read-only CSRRS x?,mcycle/h,x0. Never write/inhibit/reset counters.
// The three64-bit deltas are DATA,not proof/nonce/source/address authority.
// No new NoC packet,DMA,MMIO register or memory extent is introduced.
typedef __UINT64_TYPE__ PioneerQ38CycleCount;
static __attribute__((noinline,section(".q38_cycle_read"))) PioneerQ38CycleCount PioneerQ38RootCycleRead()
{
	for (unsigned attempt=0;attempt<4;attempt++) {
		uint32_t high,low,confirm;
		asm volatile("csrrs %0,0xb80,x0\ncsrrs %1,0xb00,x0\ncsrrs %2,0xb80,x0"
			: "=r"(high),"=r"(low),"=r"(confirm) :: "memory");
		if (high==confirm) return (PioneerQ38CycleCount(high)<<32)|low;
	}
	block_fault_park(); // counter sampling uncertainty:inert park,NO retry IO
}
struct PioneerQ38RootCycleProfile {
	uint32_t enabled; // full-word stores ONLY,matching the existing image model
	PioneerQ38CycleCount previous;
	uint32_t words[7];
	explicit PioneerQ38RootCycleProfile(bool work):enabled(work),previous(0),words{} {
		if (enabled) { previous=PioneerQ38RootCycleRead(); words[0]=0x31435951; }
	}
	__attribute__((noinline,section(".q38_cycle_mark"))) void Mark(unsigned phase) {
		if (!enabled) return;
		if (phase>=3) block_fault_park();
		auto now=PioneerQ38RootCycleRead();
		if (now<previous) block_fault_park();
		auto delta=now-previous; previous=now;
		words[1+phase*2]=uint32_t(delta); words[2+phase*2]=uint32_t(delta>>32);
	}
	__attribute__((noinline,section(".q38_cycle_publish"))) void Publish() {
		if (!enabled) return;
		for (unsigned w=0;w<7;w++) Words(0x7f800)[57+w]=words[w];
	}
};
#endif
