/* SPDX-License-Identifier: MIT */
// OFFLINE fixed Qwen0.6B layer candidate ONLY. No driver selects this image.
#define PIONEER_ATTENTION_FREESTANDING
#include "PioneerGraphMath.h"
#define coordinator_main layer_matrix_main
#include "blackhole_coordinator.cpp"
#undef coordinator_main
static bool LayerHost(uint32_t low,uint32_t high)
{
	return high && high<=0xffff && !(low&4095) && !(high==0xffff && low>0xffffc000);
}
static bool LayerWeight(uint32_t base)
{
	const unsigned bytes=1024U*3072U*9U/8U;
	return !(base&4095) && base>=0x04000000 && base<0x82400000 && bytes<=0x82400000-base;
}
static void LayerAdd(uint32_t (&total)[4])
{
	for (unsigned i=0;i<4;i++) total[i]+=Words(0x143000)[i];
}
static void LayerMatrix(volatile uint32_t* job,uint32_t base,unsigned k,unsigned n,unsigned expected,uint32_t (&total)[4])
{
	job[1]=1; job[3]=k; job[4]=n; job[5]=base; job[7]=job[8]=job[9]=0; Fence();
	if (layer_matrix_main()!=expected) coordinator_fault_park();
	LayerAdd(total);
}
static void LayerGlu(unsigned n,bool debug)
{
	for (unsigned i=0;i<n;i++) {
		float gate=PioneerAttentionFloat(Words(0x87000)[i]),up=PioneerAttentionFloat(Words(0x84000)[i]);
		if ((PioneerAttentionBits(gate)&0x7f800000)==0x7f800000 || (PioneerAttentionBits(up)&0x7f800000)==0x7f800000) coordinator_fault_park();
		float value=(debug?PioneerGraphSilu(gate):PioneerGraphSiluFast(gate))*up;
		if ((PioneerAttentionBits(value)&0x7f800000)==0x7f800000) coordinator_fault_park();
		Words(0x84000)[i]=PioneerAttentionBits(value);
	}
	Fence();
}
extern "C" unsigned coordinator_main()
{
	volatile uint32_t* job=Words(0x140000); unsigned operation=job[1],limit=job[2],n=job[4];
	uint32_t low=job[7],high=job[8],nonce=job[6];
	if (job[0]!=0x31445243 || (operation!=4 && operation!=5) || !limit || limit>119 || !nonce || nonce>=0x7fffffff
		|| !LayerHost(low,high) || job[9]!=16384 || job[12]>1 || !NocIdle()) return 0;
	bool debug=job[12]!=0;
	if (operation==4 && (job[3]!=1024 || n!=1024 || !LayerWeight(job[5]) || !LayerWeight(job[10]) || !LayerWeight(job[11]))) return 0;
	if (operation==5 && (!n || n>3072 || (n&31) || job[3] || job[5] || job[10] || job[11])) return 0;
	uint32_t before[8],total[4]={};
	for (unsigned i=0;i<8;i++) before[i]=*Words(0xffb20000+CounterOffset(i));
	unsigned workers=1;
	if (operation==4) {
		uint32_t gateBase=job[5],upBase=job[10],downBase=job[11];
		workers=limit<96?limit:96; unsigned downWorkers=limit<32?limit:32;
		// Every initial sequence and BOTH later increments must be bounded
		// before the first descriptor/doorbell. No wrap, partial launch.
		for (unsigned w=1;w<=workers;w++) if (!job[64+w-1] || job[64+w-1]>=0x7ffffffc) return 0;
		LayerMatrix(job,gateBase,1024,3072,workers,total);
		for (unsigned i=0;i<3072;i++) Words(0x87000)[i]=Words(0x84000)[i];
		Fence();
		for (unsigned i=0;i<3072;i++) if (Words(0x87000)[i]!=Words(0x84000)[i]) coordinator_fault_park();
		for (unsigned w=1;w<=workers;w++) job[64+w-1]++;
		LayerMatrix(job,upBase,1024,3072,workers,total);
		LayerGlu(3072,debug);
		for (unsigned block=0;block<96;block++) {
			float values[32]; uint32_t operand[16],scale=0;
			for (unsigned i=0;i<32;i++) values[i]=PioneerAttentionFloat(Words(0x84000)[block*32+i]);
			if (!PioneerGraphQuantize32(values,operand,scale)) coordinator_fault_park();
			for (unsigned i=0;i<16;i++) Words(0x80000)[block*16+i]=operand[i];
			Words(0x81800)[block]=scale; Fence();
			for (unsigned i=0;i<16;i++) if (Words(0x80000)[block*16+i]!=operand[i]) coordinator_fault_park();
			if (Words(0x81800)[block]!=scale) coordinator_fault_park();
		}
		for (unsigned w=1;w<=downWorkers;w++) job[64+w-1]++;
		LayerMatrix(job,downBase,3072,1024,downWorkers,total); n=1024;
	} else LayerGlu(n,debug);
	volatile uint32_t* stats=Words(0x143000);
	for (unsigned i=0;i<24;i++) stats[i]=0;
	for (unsigned i=0;i<4;i++) stats[i]=total[i];
	stats[5]=n; stats[6]=nonce; stats[7]=operation;
	for (unsigned i=0;i<8;i++) stats[8+i]=before[i];
	HostTransfer(true,low,high,n); HostTransfer(false,low,high,n);
	uint32_t mismatch=0;
	for (unsigned i=0;i<n;i++) mismatch|=Words(0x84000)[i]^Words(0x144000)[i];
	if (mismatch) coordinator_fault_park();
	Fence(); return Complete(workers);
}
