/* SPDX-License-Identifier: MIT */
// OFFLINE seven-bank preload research ONLY. No driver selects this image.
// Future kernel MUST own/seal ALL bank/host extents and prove every other
// core/initiator held. Never substitute it into an active matrix cohort.
#define PIONEER_ATTENTION_FREESTANDING
#include "../../src/add-ons/kernel/drivers/misc/PioneerQ38SpanShape.h"
extern "C" void block_fault_park() __attribute__((noreturn));
static volatile uint32_t* Words(uint32_t a) { return reinterpret_cast<volatile uint32_t*>(a); }
static void Fence() { asm volatile("fence" ::: "memory"); }
static unsigned Counter(unsigned i)
{
	const unsigned offsets[]={0x208,0x20c,0x210,0x214,0x204,0x220,0x210,0x228}; return offsets[i];
}
static unsigned Source(unsigned i)
{
	return !i?0x100:i<=6?0x118+(i-1)*4:i<=12?0x130+(i-7)*4:i==13?0x150:i==14?0x154:0x16c;
}
static bool Sources(const uint32_t* d)
{
	bool same=true; for (unsigned i=0;i<16;i++) same &= *Words(0xffb20000+Source(i))==d[16+i]; return same;
}
static bool Idle()
{
	bool idle=true;
	for (unsigned i=0;i<4;i++) idle &= !*Words(0xffb20040+i*0x800);
	for (unsigned i=0;i<32;i++) idle &= !(*Words(0xffb20240+i*4)&255);
	return idle;
}
static void Packet(bool write,bool host,unsigned at,unsigned words,const uint32_t* d)
{
	// Descriptor is a private frozen stack copy,NOT a mutable user pointer.
	// Exactly four packet shapes:host<->fixed L1,bank<->fixed L1.
	if (!words || words>4096 || (words&1023) || at>d[4] || words*4>d[4]-at
		|| (at&4095) || !Sources(d) || !Idle()) block_fault_park();
	unsigned remote=(host?d[7]:d[5])+at,high=host?d[8]:0,route=host?1555U:d[11];
	unsigned local=host?(write?0x84000U:0x144000U):(write?0x144000U:0x84000U);
	const unsigned offsets[]={write?0x204U:0x208U,write?0x220U:0x20cU,0x210,write?0x228U:0x214U};
	uint32_t expected[4]; for (unsigned i=0;i<4;i++) expected[i]=*Words(0xffb20000+offsets[i])+(i==1?words/16:1);
	const uint32_t command[]={write?local:remote,write?0:high,write?129U:route,
		write?remote:local,write?high:0,write?route:129U,0,write?0x212U:0,words*4,0};
	for (unsigned i=0;i<10;i++) Words(0xffb20000)[i]=command[i];
	for (unsigned i=0;i<10;i++) if (Words(0xffb20000)[i]!=command[i]) block_fault_park();
	Fence(); *Words(0xffb20040)=1; (void)*Words(0xffb20040); Fence();
	for (unsigned attempt=0;attempt<65536;attempt++) {
		bool complete=!*Words(0xffb20040);
		for (unsigned i=0;i<4;i++) complete &= *Words(0xffb20000+offsets[i])==expected[i];
		if (complete) {
			if (!Idle() || !Sources(d)) block_fault_park();
			Words(0x143000)[write?2:0]++; Words(0x143000)[write?3:1]+=words/16; Fence(); return;
		}
	}
	block_fault_park(); // UNKNOWN:no later packet/reset/restore/retry
}
static void Compare(unsigned words)
{
	uint32_t mismatch=0; for (unsigned i=0;i<words;i++) mismatch|=Words(0x84000)[i]^Words(0x144000)[i];
	if (mismatch) block_fault_park();
}
extern "C" unsigned block_main()
{
	uint32_t d[TT_Q38_SPAN_JOB_WORDS];
	for (unsigned i=0;i<TT_Q38_SPAN_JOB_WORDS;i++) d[i]=Words(0x140000)[i];
	if (!PioneerQ38SpanShape(d)) return 0; // no packet/source read for malformed shape
	if (!Sources(d) || !Idle()) block_fault_park();
	volatile uint32_t* s=Words(0x143000);
	for (unsigned i=0;i<TT_Q38_SPAN_STATS_WORDS;i++) s[i]=0;
	s[5]=d[4]; s[6]=d[6]; s[7]=TT_Q38_SPAN_MARKER;
	s[24]=d[10]; s[25]=d[11]; s[26]=d[5]; s[27]=d[7]; s[28]=d[8]; s[29]=d[9]; s[30]=1;
	for (unsigned i=0;i<8;i++) s[8+i]=*Words(0xffb20000+Counter(i));
	for (unsigned at=0;at<d[4];at+=16384) {
		unsigned words=(d[4]-at)/4; if (words>4096) words=4096;
		Packet(false,true,at,words,d);
		for (unsigned i=0;i<words;i++) Words(0x80000)[i]=Words(0x144000)[i];
		Fence();
		for (unsigned i=0;i<words;i++) if (Words(0x80000)[i]!=Words(0x144000)[i]) block_fault_park();
		Packet(true,false,at,words,d); Packet(false,false,at,words,d); Compare(words);
		// Independently ordered complement/original echoes prove freshness.
		// A future kernel MUST ALSO compare final host echo to its frozen
		// ordinary CPU input before initializing ANY page. GPU repeatability
		// cannot detect a corrupt initial host read by itself.
		for (unsigned i=0;i<words;i++) Words(0x84000)[i]=~Words(0x80000)[i];
		Fence();
		for (unsigned i=0;i<words;i++) if (Words(0x84000)[i]!=~Words(0x80000)[i]) block_fault_park();
		Packet(true,true,at,words,d); Packet(false,true,at,words,d); Compare(words);
		for (unsigned i=0;i<words;i++) Words(0x84000)[i]=Words(0x80000)[i];
		Fence();
		for (unsigned i=0;i<words;i++) if (Words(0x84000)[i]!=Words(0x80000)[i]) block_fault_park();
		Packet(true,true,at,words,d); Packet(false,true,at,words,d); Compare(words);
	}
	if (!Sources(d) || !Idle()) block_fault_park();
	for (unsigned i=0;i<TT_Q38_SPAN_JOB_WORDS;i++) if (d[i]!=Words(0x140000)[i]) block_fault_park();
	s[4]=d[4]/4096;
	for (unsigned i=0;i<8;i++) s[16+i]=*Words(0xffb20000+Counter(i));
	Fence(); return d[4]/4096;
}
