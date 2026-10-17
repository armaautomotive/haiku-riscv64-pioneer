/* SPDX-License-Identifier: MIT */
// OFFLINE linked-image prerequisite ONLY. No driver/operation loads this.
#define PIONEER_ATTENTION_FREESTANDING
#include "PioneerGraphNorm.h"
#include "PioneerGraphRope.h"
static volatile uint32_t* Words(uint32_t a) { return reinterpret_cast<volatile uint32_t*>(a); }
extern "C" void scalar_fault_park() __attribute__((noreturn));
extern "C" unsigned scalar_main()
{
	volatile uint32_t* job=Words(0x74000); unsigned n=job[1],mode=job[2];
	if (job[0]!=0x31504c47 || !n || n>3072 || (n&31) || mode>6
		|| (mode==5?((job[3]!=128 && job[3]!=1024) || (n&(job[3]-1))):job[3]!=0)) return 0;
	if (mode==5) {
		unsigned width=job[3];
		for (unsigned i=0;i<n;i+=width) if (!PioneerGraphRmsNorm(width,reinterpret_cast<const float*>(0x84000+i*4),1e-6f,
			reinterpret_cast<float*>(0x90000+i*4))) scalar_fault_park();
		asm volatile("fence" ::: "memory"); return n;
	}
	if (mode==6) {
		if (!PioneerGraphRopeNeox(n,reinterpret_cast<const float*>(0x84000),reinterpret_cast<const float*>(0x87000),
			reinterpret_cast<float*>(0x90000))) scalar_fault_park();
		asm volatile("fence" ::: "memory"); return n;
	}
	for (unsigned i=0;i<n;i++) {
		float a=PioneerAttentionFloat(Words(0x84000)[i]);
		float b=PioneerAttentionFloat(Words(0x87000)[i]);
		if (mode==2) { Words(0x90000)[i]=PioneerAttentionBits(PioneerGraphDividePositive(a,b)); continue; }
		if (mode==4) { Words(0x90000)[i]=PioneerAttentionBits(PioneerGraphSqrtPositive(a)); continue; }
		if ((PioneerAttentionBits(a)&0x7f800000)==0x7f800000
			|| (mode && (PioneerAttentionBits(b)&0x7f800000)==0x7f800000)) scalar_fault_park();
		float value=mode==3?PioneerGraphSiluFast(a)*b:mode?PioneerGraphSilu(a)*b:a;
		if ((PioneerAttentionBits(value)&0x7f800000)==0x7f800000) scalar_fault_park();
		Words(0x90000)[i]=PioneerAttentionBits(value);
	}
	if (mode!=2 && mode!=4) for (unsigned block=0;block<n/32;block++) {
#if PIONEER_GRAPH_PACKED_Q8
		uint32_t operand[16],scale=0;
		if (!PioneerGraphQuantize32(reinterpret_cast<const float*>(0x90000+block*128),operand,scale)) scalar_fault_park();
#else
		float values[32]; uint32_t operand[16],scale=0;
		for (unsigned i=0;i<32;i++) values[i]=PioneerAttentionFloat(Words(0x90000)[block*32+i]);
		if (!PioneerGraphQuantize32(values,operand,scale)) scalar_fault_park();
#endif
		for (unsigned i=0;i<16;i++) Words(0x80000)[block*16+i]=operand[i];
		Words(0x83000)[block]=scale;
	}
	asm volatile("fence" ::: "memory"); return n;
}
