/* SPDX-License-Identifier: MIT */
// OFFLINE fixed primitive probe ONLY. No registry/API selects this image.
#define attention_main unused_attention_tile_main
#include "blackhole_attention.cpp"
#undef attention_main
#include "PioneerSfpuDivide.h"
struct DivideProbeEmitter { void Op(uint32_t instruction) { Emit(instruction); } };
extern "C" unsigned attention_main()
{
	const volatile uint32_t* job=Words(0x74000);
	if (job[0]!=0x31564453 || job[1]!=32 || job[2] || job[3]) return 0;
	for (unsigned lane=0;lane<32;lane++) if (!PioneerSfpuDivideDomain(Words(0x90000)[lane],Words(0x90800)[lane])) return 0;
	Setup();
	for (unsigned lane=0;lane<32;lane++) {
		Words(0xffbd8000)[LaneOffset(lane)]=Words(0x90000)[lane];
		Words(0xffbd8100)[LaneOffset(lane)]=Words(0x90800)[lane];
	}
	(void)Words(0xffbd8100)[LaneOffset(31)]; Fence();
	for (unsigned lane=0;lane<32;lane++) if (Words(0xffbd8000)[LaneOffset(lane)]!=Words(0x90000)[lane]
		|| Words(0xffbd8100)[LaneOffset(lane)]!=Words(0x90800)[lane]) attention_fault_park();
	Emit(0x70030000); Emit(0x70130004);
	DivideProbeEmitter emitter; PioneerSfpuDivideNormal(emitter);
	Emit(0x72030008); Drain();
	for (unsigned lane=0;lane<32;lane++) {
		uint32_t value=Words(0xffbd8200)[LaneOffset(lane)];
		if (value<0x00800000 || value>=0x7f800000) attention_fault_park();
		Words(0x91000)[lane]=value;
	}
	Fence(); return 32;
}
