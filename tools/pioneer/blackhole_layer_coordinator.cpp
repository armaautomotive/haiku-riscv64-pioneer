/* SPDX-License-Identifier: MIT */
// Fixed Qwen0.6B private FFN/GLU image,not a complete decoder/speed claim.
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
#if PIONEER_COORDINATOR_SPAN_UPLOAD
// Contract11 fixed NEW-weight transport;caller owns and freezes both ends.
static void SpanStorage(bool write,uint32_t local,uint32_t address,unsigned words)
{
	if (!words || words>4096 || (words&1023) || (address&4095) || local!=(write?0x144000U:0x84000U) || !PacketIdle()) coordinator_fault_park();
	volatile uint32_t* noc=Words(0xffb20000);
	const unsigned offsets[4]={write?0x204U:0x208U,write?0x220U:0x20cU,0x210,write?0x228U:0x214U};
	uint32_t expected[4]; volatile uint32_t command[10];
	for (unsigned i=0;i<10;i++) command[i]=0;
	for (unsigned i=0;i<4;i++) expected[i]=noc[offsets[i]/4]+(i==1?words/16:1);
	command[0]=write?local:address; command[2]=write?129:786; command[3]=write?address:local;
	command[5]=write?786:129; command[7]=write?0x212:0; command[8]=words*4;
	for (unsigned i=0;i<10;i++) noc[i]=command[i];
	for (unsigned i=0;i<10;i++) if (noc[i]!=command[i]) coordinator_fault_park();
	Fence(); noc[0x40/4]=1; (void)noc[0x40/4]; Fence();
	for (unsigned attempt=0;attempt<65536;attempt++) {
		bool complete=!noc[0x40/4];
		for (unsigned i=0;i<4;i++) complete &= noc[offsets[i]/4]==expected[i];
		if (complete && PacketIdle()) { Words(0x143000)[write?2:0]++; Words(0x143000)[write?3:1]+=words/16; Fence(); return; }
	}
	coordinator_fault_park();
}
static void SpanCompare(unsigned words)
{
	uint32_t mismatch=0;
	for (unsigned i=0;i<words;i++) mismatch|=Words(0x84000)[i]^Words(0x144000)[i];
	if (mismatch) coordinator_fault_park();
}
static unsigned SpanUpload()
{
	volatile uint32_t* job=Words(0x140000);
	unsigned bytes=job[4]; uint32_t base=job[5],low=job[7],high=job[8];
	if (job[0]!=0x31445243 || job[1]!=7 || job[2]!=1 || job[3] || !bytes || bytes>0x100000 || (bytes&4095)
		|| (base&4095) || base<0x04000000 || base>=0x82400000 || bytes>0x82400000-base
		|| !job[6] || job[6]>=0x7fffffff || !high || high>0xffff || (low&0xfffff)
		|| (high==0xffff && low>0xfff00000) || job[9]!=0x100000 || job[10] || job[11] || job[12] || !NocIdle()) return 0;
	volatile uint32_t* stats=Words(0x143000);
	for (unsigned i=0;i<24;i++) stats[i]=0;
	stats[5]=bytes; stats[6]=job[6]; stats[7]=7;
	for (unsigned i=0;i<8;i++) stats[8+i]=*Words(0xffb20000+CounterOffset(i));
	for (unsigned at=0;at<bytes;at+=16384) {
		unsigned words=(bytes-at)/4; if (words>4096) words=4096;
		HostTransfer(false,low+at,high,words); // frozen source -> owned144000
		for (unsigned i=0;i<words;i++) Words(0x80000)[i]=Words(0x144000)[i];
		Fence();
		for (unsigned i=0;i<words;i++) if (Words(0x80000)[i]!=Words(0x144000)[i]) coordinator_fault_park();
		SpanStorage(true,0x144000,base+at,words);
		SpanStorage(false,0x84000,base+at,words); SpanCompare(words); // EVERY GDDR bit
		// Prove echo freshness,not an untouched original host buffer. Both
		// complement and original are independently ordered/read/compared.
		// Kernel must ALSO compare the final echo with its frozen CPU input
		// before accepting ANY initialized page; no checksum or learned sum.
		for (unsigned i=0;i<words;i++) Words(0x84000)[i]=~Words(0x80000)[i];
		Fence();
		for (unsigned i=0;i<words;i++) if (Words(0x84000)[i]!=~Words(0x80000)[i]) coordinator_fault_park();
		HostTransfer(true,low+at,high,words); HostTransfer(false,low+at,high,words); SpanCompare(words);
		for (unsigned i=0;i<words;i++) Words(0x84000)[i]=Words(0x80000)[i];
		Fence();
		for (unsigned i=0;i<words;i++) if (Words(0x84000)[i]!=Words(0x80000)[i]) coordinator_fault_park();
		HostTransfer(true,low+at,high,words); HostTransfer(false,low+at,high,words); SpanCompare(words);
	}
	return Complete(bytes/4096);
}
#endif
extern "C" unsigned coordinator_main()
{
	// Explicit contract12 narrow matrix uses the qualified fixed worker's
	// full published idle/source/math proof,like FFN and wide output. It
	// retains one complete owned16KiB host write/read/all-bit comparison.
	if (Words(0x140000)[1]==3) return layer_matrix_main();
#if PIONEER_COORDINATOR_SPAN_UPLOAD
	if (Words(0x140000)[1]==7) return SpanUpload();
#endif
#if PIONEER_COORDINATOR_WIDE_OUTPUT
	// Separate explicit bounded wide-result operation; ordinary FFN/GLU
	// retain their own fixed shape/host-owner checks below.
	if (Words(0x140000)[1]==6) return layer_matrix_main();
#endif
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
