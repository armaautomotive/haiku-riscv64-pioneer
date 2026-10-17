/* SPDX-License-Identifier: MIT */
// Experimental whole-position Qwen3-0.6B immutable instruction candidate.
// Only the dedicated sealed model registry may own/load it. Parallel
// attention additionally requires eight independent code/data leases.
#define PIONEER_ATTENTION_FREESTANDING
// Whole-decoder-only optimization;legacy attention images remain byte exact.
#define PIONEER_ATTENTION_EXACT_HALF_PRODUCT 1
#include "PioneerGraphNorm.h"
#include "PioneerGraphRope.h"
#if PIONEER_COORDINATOR_BATCH
#include "PioneerDecoderEmbedding.h"
#include "../../src/add-ons/kernel/drivers/misc/PioneerDecoderBatch.h"
#endif
#include "PioneerCoordinatedAttentionProof.h"
#include "PioneerCoordinatedGluProof.h"
#include "../../src/add-ons/kernel/drivers/misc/PioneerDecoderStageProfile.h"
#include "../../src/add-ons/kernel/drivers/misc/PioneerDecoderLayout.h"
#include "../../src/add-ons/kernel/drivers/misc/PioneerDecoderGreedy.h"
#include "../../src/add-ons/kernel/drivers/misc/PioneerPairedFfnPlan.h"
#include <stddef.h>
#define coordinator_main decoder_matrix_main
#include "blackhole_coordinator.cpp"
#undef coordinator_main
// Reuse the EXACT qualified attention/SFPU code with names separated from
// matrix transport. Only its fixed constant sequence is linked separately.
#define Words DecoderAttentionWords
#define Fence DecoderAttentionFence
#define NocIdle DecoderAttentionNocIdle
#define Transfer DecoderAttentionTransfer
#define PIONEER_RESIDENT_ATTENTION_NAME decoder_attention_main
#include "blackhole_resident_attention.cpp"
#undef Words
#undef Fence
#undef NocIdle
#undef Transfer
extern "C" void attention_fault_park() { coordinator_fault_park(); }
static float* Floats(unsigned address) { return reinterpret_cast<float*>(address); }
static volatile PioneerDecoderJob& DecoderJob() { return *reinterpret_cast<volatile PioneerDecoderJob*>(TT_DECODER_JOB_BASE); }
#if PIONEER_COORDINATOR_STAGE_PROFILE
// Tenstorrent's get_timestamp_32b() reads this SAME documented timer.
// https://github.com/tenstorrent/tt-metal/blob/main/tt_metal/hw/inc/internal/tt-1xx/risc_common.h
// READ ONLY:never arm/freeze/write performance-control registers.
static unsigned DecoderStageClock() { Fence(); unsigned t=*Words(0xffb121f0); Fence(); return t; }
static void DecoderStageEnd(unsigned slot,unsigned start) {
	unsigned elapsed=DecoderStageClock()-start;
	Words(TT_DECODER_STAGE_L1)[4+slot]+=elapsed; Words(TT_DECODER_STAGE_L1)[12+slot]++; Fence();
}
#define DECODER_STAGE_BEGIN() unsigned stageStart=DecoderStageClock()
#define DECODER_STAGE_END(slot) DecoderStageEnd(slot,stageStart)
#else
#define DECODER_STAGE_BEGIN()
#define DECODER_STAGE_END(slot)
#endif
static void DecoderCopy(unsigned destination,unsigned source,unsigned words)
{
	for (unsigned i=0;i<words;i++) Words(destination)[i]=Words(source)[i];
	Fence();
	for (unsigned i=0;i<words;i++) if (Words(destination)[i]!=Words(source)[i]) coordinator_fault_park();
}
static void DecoderRead(unsigned destination,uint32_t source,unsigned bytes,uint32_t (&total)[4])
{
#if PIONEER_COORDINATOR_BATCH
	const volatile auto& root=DecoderJob();
	bool fixedInput=destination==0x80000 && !(source&31)
		&& ((bytes==512 && source==root.reserved[1]+(root.tokens-1)*512)
			|| (bytes==576 && source>=root.vocabulary && source-root.vocabulary<=175030272-576));
	if (!fixedInput && ((source&511) || (destination&511) || (bytes!=512 && bytes!=4096) || destination!=TT_DECODER_NORM)) coordinator_fault_park();
	if (source<0x04000000 || source>=0x82400000 || bytes>0x82400000-source || !PacketIdle()) coordinator_fault_park();
#else
	if ((source&511) || (destination&511) || (bytes!=512 && bytes!=4096) || source<0x04000000 || source>=0x82400000
		|| bytes>0x82400000-source || destination!=TT_DECODER_NORM || !PacketIdle()) coordinator_fault_park();
#endif
	volatile uint32_t* noc=Words(0xffb20000); volatile uint32_t command[10]; uint32_t expected[4];
	for (unsigned i=0;i<10;i++) command[i]=0;
	for (unsigned i=0;i<4;i++) expected[i]=noc[(0x208+i*4)/4]+(i==1?bytes/64:1);
	command[0]=source; command[2]=786; command[3]=destination; command[5]=129; command[8]=bytes;
	for (unsigned i=0;i<10;i++) noc[i]=command[i];
	for (unsigned i=0;i<10;i++) if (noc[i]!=command[i]) coordinator_fault_park();
	Fence(); noc[0x40/4]=1; (void)noc[0x40/4]; Fence();
	for (unsigned attempt=0;attempt<65536;attempt++) {
		bool done=!noc[0x40/4]; for (unsigned i=0;i<4;i++) done &= noc[(0x208+i*4)/4]==expected[i];
		if (done && PacketIdle()) { Fence(); total[0]++; total[1]+=bytes/64; return; }
	}
	coordinator_fault_park();
}
#if PIONEER_COORDINATOR_BATCH
struct DecoderEmbeddingIO {
	uint32_t (&total)[4]; unsigned tile,reads;
	bool ReadNativeEmbeddingBlock(uint32_t source,uint32_t (&output)[144]) {
		if (reads>=64 || source!=tile+(reads>>1)*576) return false;
		reads++; DecoderRead(0x80000,source,576,total);
		for (unsigned i=0;i<144;i++) output[i]=Words(0x80000)[i];
		Fence(); return true;
	}
};
static void DecoderBatchRope(uint32_t (&total)[4]) {
	const volatile auto& root=DecoderJob(); unsigned source=root.reserved[1]+(root.tokens-1)*512;
	DecoderRead(0x80000,source,512,total); DecoderCopy(0x147e00,0x80000,128);
	DecoderRead(0x80000,source,512,total);
	for (unsigned i=0;i<128;i++) {
		unsigned value=Words(0x147e00)[i],absolute=value&0x7fffffff;
		if (Words(0x80000)[i]!=value || absolute>0x3f800000 || (absolute && absolute<0x26800000)) coordinator_fault_park();
	}
	Fence();
}
#endif
static void DecoderQuantize(unsigned source,unsigned n)
{
	DECODER_STAGE_BEGIN();
	if (n!=1024 && n!=2048 && n!=3072) coordinator_fault_park();
	for (unsigned b=0;b<n/32;b++) {
		float values[32]; uint32_t operand[16],scale=0;
		for (unsigned i=0;i<32;i++) values[i]=Floats(source)[b*32+i];
		if (!PioneerGraphQuantize32(values,operand,scale)) coordinator_fault_park();
		for (unsigned i=0;i<16;i++) Words(0x80000)[b*16+i]=operand[i];
		Words(0x80000+n*2)[b]=scale; Fence();
		for (unsigned i=0;i<16;i++) if (Words(0x80000)[b*16+i]!=operand[i]) coordinator_fault_park();
		if (Words(0x80000+n*2)[b]!=scale) coordinator_fault_park();
	}
	DECODER_STAGE_END(2);
}
static void DecoderMatrix(uint32_t base,unsigned k,unsigned n,uint32_t (&total)[4],bool final=false)
{
	DECODER_STAGE_BEGIN();
	volatile uint32_t* j=Words(0x140000); volatile auto& root=DecoderJob();
	for (unsigned i=0;i<64;i++) j[i]=0;
	j[0]=0x31445243; j[1]=final?6:1; j[2]=119; j[3]=k; j[4]=n; j[5]=base; j[6]=root.nonce;
	if (final && (!PIONEER_COORDINATOR_BATCH || root.operation!=10)) { j[7]=root.hostLow; j[8]=root.hostHigh; j[9]=root.hostBytes; }
#if PIONEER_COORDINATOR_GREEDY
	if (final && root.operation==9) { j[1]=9; j[10]=root.tokens; }
#if PIONEER_COORDINATOR_BATCH
	if (final && root.operation==10) { j[1]=11; j[10]=root.tokens; }
#endif
#endif
	// A variable power-of-two divisor otherwise becomes unqualified DIVU
	// at -Os. The fixed tile size is represented by a shift explicitly.
	unsigned peers=n>>(PIONEER_COORDINATOR_SMALL_ROWS && n<=8192?4:5); if (peers>119) peers=119;
	for (unsigned i=0;i<peers;i++) j[64+i]=root.sequence[i];
	Fence();
	if (decoder_matrix_main()!=peers) coordinator_fault_park();
	for (unsigned i=0;i<4;i++) total[i]+=Words(0x143000)[i];
	for (unsigned i=0;i<peers;i++) root.sequence[i]++;
	Fence();
	DECODER_STAGE_END(final?6:0);
}
static void DecoderNorm(unsigned input,unsigned output,unsigned n,unsigned width,uint32_t weight,uint32_t (&total)[4])
{
	DECODER_STAGE_BEGIN();
	DecoderRead(TT_DECODER_NORM,weight,width*4,total);
	for (unsigned head=0;head<n;head+=width) {
		if (!PioneerGraphRmsNorm(width,Floats(input)+head,1e-6f,Floats(output)+head)) coordinator_fault_park();
		for (unsigned i=0;i<width;i++) {
			float value=Floats(output)[head+i]*Floats(TT_DECODER_NORM)[i];
			if ((PioneerAttentionBits(value)&0x7fffffff)>0x477fe000) coordinator_fault_park();
			Floats(output)[head+i]=value;
		}
	}
	Fence();
	DECODER_STAGE_END(1);
}
#if PIONEER_COORDINATOR_PAIRED_FFN
static void DecoderPairedFfn(uint32_t gate,uint32_t up,uint32_t (&total)[4])
{
	DECODER_STAGE_BEGIN(); volatile uint32_t* j=Words(0x140000); volatile auto& root=DecoderJob();
	for (unsigned i=0;i<64;i++) j[i]=0;
	j[0]=0x31445243; j[1]=10; j[2]=119; j[3]=1024; j[4]=6144; j[5]=gate; j[6]=root.nonce; j[10]=up;
	for (unsigned i=0;i<119;i++) j[64+i]=root.sequence[i];
	Fence(); if (decoder_matrix_main()!=119) coordinator_fault_park();
	for (unsigned i=0;i<4;i++) total[i]+=Words(0x143000)[i];
	for (unsigned i=0;i<119;i++) root.sequence[i]++;
	Fence(); DECODER_STAGE_END(0);
}
#endif
static void DecoderAddResidual()
{
	for (unsigned i=0;i<1024;i++) {
		float value=Floats(0x84000)[i]+Floats(TT_DECODER_RESIDUAL)[i];
		if ((PioneerAttentionBits(value)&0x7fffffff)>0x477fe000) coordinator_fault_park();
		Floats(TT_DECODER_HIDDEN)[i]=value;
	}
	Fence();
}
#if PIONEER_COORDINATOR_PARALLEL_ATTENTION
static void DecoderParallelAttention(unsigned layer,uint32_t (&total)[4])
{
	DECODER_STAGE_BEGIN();
	volatile auto& root=DecoderJob(); volatile uint32_t* stats=Words(0x143000);
	for (unsigned i=0;i<4;i++) stats[i]=0;
#if PIONEER_COORDINATOR_CUMULATIVE_COUNTERS
	if (!NocIdle()) coordinator_fault_park();
	for (unsigned i=0;i<8;i++) stats[8+i]=*Words(0xffb20000+CounterOffset(i));
	if (stats[10]!=stats[14]) coordinator_fault_park();
#endif
	for (unsigned group=0;group<8;group++) {
		unsigned worker=group+1; uint32_t sequence=root.sequence[group];
		if (!PeerReleased(worker,sequence-1)) coordinator_fault_park();
#if !PIONEER_COORDINATOR_PULL_INPUT
		// Stage/return/compare EVERY input bit before releasing this peer.
		Transfer(true,worker,0x140000+group*1024,0x90000,1024);
		Transfer(false,worker,0x143100,0x90000,1024);
		for (unsigned i=0;i<256;i++) if (Words(0x143100)[i]!=Words(0x140000)[group*256+i]) coordinator_fault_park();
		Transfer(true,worker,0x142000+group*512,0x90400,512);
		Transfer(false,worker,0x143100,0x90400,512);
		for (unsigned i=0;i<128;i++) if (Words(0x143100)[i]!=Words(0x142000)[group*128+i]) coordinator_fault_park();
#endif
		volatile uint32_t* args=Words(0x147100);
		args[0]=PIONEER_COORDINATOR_PARALLEL_ROPE?0x32575441:0x31475441; args[1]=root.tokens; args[2]=layer; args[3]=group; Fence();
		Transfer(true,worker,0x147100,0x74000,16); Transfer(false,worker,0x147120,0x74000,16);
		for (unsigned i=0;i<4;i++) if (Words(0x147120)[i]!=args[i]) coordinator_fault_park();
#if !PIONEER_COORDINATOR_DESCRIPTOR_TABLE
		for (unsigned i=0;i<8;i++) args[i]=0;
		args[3]=1; args[4]=XY(worker); args[7]=0x31475441; Fence();
		Transfer(true,worker,0x147100,0x70500,32); Transfer(false,worker,0x147120,0x70500,32);
		for (unsigned i=0;i<8;i++) if (Words(0x147120)[i]!=args[i]) coordinator_fault_park();
#endif
		*Words(0x147140)=sequence; Fence(); Transfer(true,worker,0x147140,0x71300,4);
	}
	// All eight jobs have been issued BEFORE waiting for any completion.
	for (unsigned group=0;group<8;group++) {
		unsigned worker=group+1; uint32_t sequence=root.sequence[group]; bool idle=false;
		for (unsigned attempt=0;attempt<65536;attempt++) {
			if (PeerReleased(worker,sequence)) { idle=true; break; }
			Transfer(false,worker,0x147088,0x17ffc8,4);
			uint32_t marker=*Words(0x147088);
			if (marker==0x4e4f4346 || marker==0x464e5441) coordinator_fault_park();
		}
		if (!idle) coordinator_fault_park();
		Transfer(false,worker,0x147180,0x71340,64);
		volatile uint32_t* snapshot=Words(0x147180);
		if (snapshot[8]!=0x7200c || snapshot[9]!=root.tokens || snapshot[10]!=0x314e5441 || snapshot[11]!=sequence) coordinator_fault_park();
		Transfer(false,worker,0x143100,0x90600,96);
		volatile uint32_t* proof=Words(0x143100);
		if (!PioneerCoordinatedAttentionProof(proof,root.tokens,layer,group,PIONEER_COORDINATOR_PULL_INPUT,PIONEER_COORDINATOR_PARALLEL_ROPE)) coordinator_fault_park();
		for (unsigned i=0;i<4;i++) if (snapshot[i]!=proof[i] || snapshot[4+i]!=proof[8+i]) coordinator_fault_park();
		Transfer(false,worker,0x144000+group*1024,0xb2000,1024);
		for (unsigned i=0;i<256;i++) if ((Words(0x144000)[group*256+i]&0x7f800000)==0x7f800000) coordinator_fault_park();
		root.sequence[group]++; Fence();
#if PIONEER_COORDINATOR_PULL_DIAGNOSTIC
		if (!group) {
			volatile uint32_t* d=Words(0x147900); d[0]=0x4640e400; // 12345.0f,DIAGNOSTIC ONLY
			Transfer(false,worker,0x147940,0x90000,32); d[1]=Words(0x140000)[0]; d[2]=Words(0x147940)[0];
			unsigned mismatch=0; for (unsigned i=0;i<8;i++) mismatch+=Words(0x140000)[i]!=Words(0x147940)[i];
			Transfer(false,worker,0x147940,0x90400,32); d[3]=PioneerAttentionBits(PioneerAttentionHalf(uint16_t(Words(0x142000)[0])));
			d[4]=PioneerAttentionBits(PioneerAttentionHalf(uint16_t(Words(0x147940)[0])));
			for (unsigned i=0;i<8;i++) mismatch+=Words(0x142000)[i]!=Words(0x147940)[i];
			d[9]=PioneerAttentionBits(float(layer)); d[10]=PioneerAttentionBits(float(mismatch)); Fence();
		}
#endif
	}
	for (unsigned i=0;i<4;i++) total[i]+=stats[i];
	DECODER_STAGE_END(5);
}
#endif
#if PIONEER_COORDINATOR_PARALLEL_GLU
static void DecoderParallelGlu(unsigned layer,uint32_t (&total)[4])
{
	DECODER_STAGE_BEGIN(); volatile auto& root=DecoderJob(); volatile uint32_t* stats=Words(0x143000);
	for (unsigned i=0;i<4;i++) stats[i]=0;
#if PIONEER_COORDINATOR_CUMULATIVE_COUNTERS
	if (!NocIdle()) coordinator_fault_park();
	for (unsigned i=0;i<8;i++) stats[8+i]=*Words(0xffb20000+CounterOffset(i));
	if (stats[10]!=stats[14]) coordinator_fault_park();
#endif
	for (unsigned group=0;group<6;group++) {
		unsigned worker=group+1; uint32_t sequence=root.sequence[group];
		if (!PeerReleased(worker,sequence-1)) coordinator_fault_park();
#if !PIONEER_COORDINATOR_PULL_INPUT
		Transfer(true,worker,0x87000+group*2048,0x90000,2048); Transfer(false,worker,0x143100,0x90000,2048);
		for (unsigned i=0;i<512;i++) if (Words(0x143100)[i]!=Words(0x87000)[group*512+i]) coordinator_fault_park();
		Transfer(true,worker,0x84000+group*2048,0x90800,2048); Transfer(false,worker,0x143100,0x90800,2048);
		for (unsigned i=0;i<512;i++) if (Words(0x143100)[i]!=Words(0x84000)[group*512+i]) coordinator_fault_park();
#endif
		volatile uint32_t* args=Words(0x147100); args[0]=0x314c4741; args[1]=512; args[2]=layer; args[3]=group; Fence();
		Transfer(true,worker,0x147100,0x74000,16); Transfer(false,worker,0x147120,0x74000,16);
		for (unsigned i=0;i<4;i++) if (Words(0x147120)[i]!=args[i]) coordinator_fault_park();
#if !PIONEER_COORDINATOR_DESCRIPTOR_TABLE
		for (unsigned i=0;i<8;i++) args[i]=0;
		args[3]=1; args[4]=XY(worker); args[7]=0x314c4741; Fence();
		Transfer(true,worker,0x147100,0x70500,32); Transfer(false,worker,0x147120,0x70500,32);
		for (unsigned i=0;i<8;i++) if (Words(0x147120)[i]!=args[i]) coordinator_fault_park();
#endif
		*Words(0x147140)=sequence; Fence(); Transfer(true,worker,0x147140,0x71300,4);
	}
	for (unsigned group=0;group<6;group++) {
		unsigned worker=group+1; uint32_t sequence=root.sequence[group]; bool idle=false;
		for (unsigned attempt=0;attempt<65536;attempt++) {
			if (PeerReleased(worker,sequence)) { idle=true; break; }
			Transfer(false,worker,0x147088,0x17ffc8,4); unsigned marker=*Words(0x147088);
			if (marker==0x4e4f4346 || marker==0x464e5441) coordinator_fault_park();
		}
		if (!idle) coordinator_fault_park();
		Transfer(false,worker,0x147180,0x71340,64); volatile uint32_t* snapshot=Words(0x147180);
		if (snapshot[8]!=0x7200c || snapshot[9]!=512 || snapshot[10]!=0x314e5441 || snapshot[11]!=sequence) coordinator_fault_park();
		Transfer(false,worker,0x143100,0x91500,128); volatile uint32_t* proof=Words(0x143100);
		if (!PioneerCoordinatedGluProof(proof,layer,group,PIONEER_COORDINATOR_PULL_INPUT)) coordinator_fault_park();
		for (unsigned i=0;i<4;i++) if (snapshot[i]!=proof[i] || snapshot[4+i]!=proof[8+i]) coordinator_fault_park();
		Transfer(false,worker,0x80000+group*1024,0x91000,1024);
		Transfer(false,worker,0x81800+group*64,0x91400,64);
		for (unsigned i=0;i<512;i++) {
			uint16_t operand=uint16_t(Words(0x80000)[group*256+i/2]>>((i&1)*16));
			if ((operand&0x7c00)!=0x4000 || (operand&1023)>(operand&0x8000?128U:127U)) coordinator_fault_park();
		}
		for (unsigned i=0;i<16;i++) {
			unsigned bits=Words(0x81800)[group*16+i];
			if ((bits&0x80000000) || bits>0x477fe000 || PioneerAttentionBits(PioneerAttentionHalf(PioneerAttentionToHalf(PioneerAttentionFloat(bits))))!=bits) coordinator_fault_park();
		}
		root.sequence[group]++; Fence();
#if PIONEER_COORDINATOR_PULL_DIAGNOSTIC
		if (!group) {
			volatile uint32_t* d=Words(0x147900);
			Transfer(false,worker,0x147940,0x90000,32); d[5]=Words(0x87000)[0]; d[6]=Words(0x147940)[0];
			unsigned mismatch=0; for (unsigned i=0;i<8;i++) mismatch+=Words(0x87000)[i]!=Words(0x147940)[i];
			Transfer(false,worker,0x147940,0x90800,32); d[7]=Words(0x84000)[0]; d[8]=Words(0x147940)[0];
			for (unsigned i=0;i<8;i++) mismatch+=Words(0x84000)[i]!=Words(0x147940)[i];
			d[11]=PioneerAttentionBits(float(mismatch)); Fence();
		}
#endif
	}
	for (unsigned i=0;i<4;i++) total[i]+=stats[i];
	DECODER_STAGE_END(4);
}
#endif
extern "C" unsigned coordinator_main()
{
	volatile auto& root=DecoderJob();
	if (!PioneerDecoderJobValid(root,PIONEER_COORDINATOR_BATCH?10:PIONEER_COORDINATOR_PARALLEL_ROPE?9:PIONEER_COORDINATOR_SMALL_ROWS?8:PIONEER_COORDINATOR_DESCRIPTOR_TABLE?7:PIONEER_COORDINATOR_PULL_INPUT?6:PIONEER_COORDINATOR_PAIRED_FFN?5:PIONEER_COORDINATOR_GREEDY?4:PIONEER_COORDINATOR_PARALLEL_GLU?3:PIONEER_COORDINATOR_PARALLEL_ATTENTION?2:1) || !NocIdle()) return 0;
#if PIONEER_COORDINATOR_PULL_DIAGNOSTIC
	if (root.operation!=8) return 0; // Never a sampling/inference target.
	for (unsigned i=0;i<16;i++) Words(0x147900)[i]=0;
#endif
	DECODER_STAGE_BEGIN();
#if PIONEER_COORDINATOR_STAGE_PROFILE
	for (unsigned i=0;i<32;i++) Words(TT_DECODER_STAGE_L1)[i]=0;
	Words(TT_DECODER_STAGE_L1)[1]=root.nonce; Words(TT_DECODER_STAGE_L1)[2]=root.tokens; Words(TT_DECODER_STAGE_L1)[3]=PIONEER_COORDINATOR_BATCH?5:PIONEER_COORDINATOR_PARALLEL_ROPE?4:PIONEER_COORDINATOR_PAIRED_FFN?3:PIONEER_COORDINATOR_PARALLEL_GLU?2:1; Fence();
#endif
	for (unsigned i=0;i<1024;i++) if ((Words(TT_DECODER_INPUT)[i]&0x7fffffff)>0x477fe000) return 0;
	uint32_t before[8],total[4]={}; for (unsigned i=0;i<8;i++) before[i]=*Words(0xffb20000+CounterOffset(i));
#if PIONEER_COORDINATOR_BATCH
	unsigned firstTokens=root.tokens,completed=0;
	bool autonomous=root.operation==10;
	for (unsigned i=0;i<64;i++) Words(0x146c00)[i]=0;
	for (unsigned step=0;step<root.reserved[0];step++) {
		DecoderBatchRope(total);
#endif
	DecoderCopy(TT_DECODER_HIDDEN,TT_DECODER_INPUT,1024);
	const float* rope=Floats(TT_DECODER_JOB_BASE+offsetof(PioneerDecoderJob,rope));
#if PIONEER_COORDINATOR_PARALLEL_ROPE && !PIONEER_COORDINATOR_BATCH
	// Frozen through every attention completion for this position. These
	// three512B spans are in the existing140000..148000 bulk backup,not
	// in matrix gathers144000 or NoC scratch147000..147940.
	DecoderCopy(0x147e00,TT_DECODER_JOB_BASE+offsetof(PioneerDecoderJob,rope),128);
	(void)rope;
#endif
	(void)rope;
	for (unsigned layer=0;layer<28;layer++) {
		const volatile auto& l=root.layers[layer];
		DecoderCopy(TT_DECODER_RESIDUAL,TT_DECODER_HIDDEN,1024);
		DecoderNorm(TT_DECODER_HIDDEN,0x84000,1024,1024,l.attentionNorm,total); DecoderQuantize(0x84000,1024);
		DecoderMatrix(l.q,1024,2048,total); DecoderCopy(TT_DECODER_Q,0x84000,2048);
		DecoderMatrix(l.v,1024,1024,total);
		for (unsigned g=0;g<8;g++) for (unsigned i=0;i<64;i++) Words(0x142100)[g*128+i]
			=PioneerAttentionToHalf(Floats(0x84000)[g*128+i*2])|(uint32_t(PioneerAttentionToHalf(Floats(0x84000)[g*128+i*2+1]))<<16);
		DecoderMatrix(l.k,1024,1024,total); DecoderCopy(TT_DECODER_K,0x84000,1024);
#if PIONEER_COORDINATOR_PARALLEL_ROPE
		DecoderRead(TT_DECODER_NORM,l.qNorm,512,total); DecoderCopy(0x147a00,TT_DECODER_NORM,128);
		DecoderRead(TT_DECODER_NORM,l.kNorm,512,total); DecoderCopy(0x147c00,TT_DECODER_NORM,128);
#else
		DecoderNorm(TT_DECODER_Q,TT_DECODER_Q,2048,128,l.qNorm,total);
		DecoderNorm(TT_DECODER_K,TT_DECODER_K,1024,128,l.kNorm,total);
		{ DECODER_STAGE_BEGIN();
			if (!PioneerGraphRopeNeox(2048,Floats(TT_DECODER_Q),rope,Floats(TT_DECODER_Q))
				|| !PioneerGraphRopeNeox(1024,Floats(TT_DECODER_K),rope,Floats(TT_DECODER_K))) coordinator_fault_park();
			DECODER_STAGE_END(3);
		}
		for (unsigned g=0;g<8;g++) for (unsigned i=0;i<64;i++) Words(0x142000)[g*128+i]
			=PioneerAttentionToHalf(Floats(TT_DECODER_K)[g*128+i*2])|(uint32_t(PioneerAttentionToHalf(Floats(TT_DECODER_K)[g*128+i*2+1]))<<16);
#endif
		DecoderCopy(0x140000,TT_DECODER_Q,2048);
#if PIONEER_COORDINATOR_PARALLEL_ATTENTION
		DecoderParallelAttention(layer,total);
#else
		Words(0x74000)[0]=0x3152564b; Words(0x74000)[1]=root.tokens; Words(0x74000)[2]=layer; Words(0x74000)[3]=root.kvBase; Fence();
		if (decoder_attention_main()!=root.tokens) coordinator_fault_park();
		total[0]+=16*((root.tokens+63)>>6); total[1]+=64*root.tokens; total[2]+=16; total[3]+=64;
#endif
		DecoderQuantize(0x144000,2048); DecoderMatrix(l.o,2048,1024,total); DecoderAddResidual();
		DecoderCopy(TT_DECODER_RESIDUAL,TT_DECODER_HIDDEN,1024);
		DecoderNorm(TT_DECODER_HIDDEN,0x84000,1024,1024,l.ffnNorm,total); DecoderQuantize(0x84000,1024);
#if PIONEER_COORDINATOR_PAIRED_FFN
		DecoderPairedFfn(l.gate,l.up,total);
#else
		DecoderMatrix(l.gate,1024,3072,total); DecoderCopy(0x87000,0x84000,3072); DecoderMatrix(l.up,1024,3072,total);
#endif
#if PIONEER_COORDINATOR_PARALLEL_GLU
		DecoderParallelGlu(layer,total);
#else
		{ DECODER_STAGE_BEGIN();
			for (unsigned i=0;i<3072;i++) Floats(0x84000)[i]=PioneerGraphSiluFast(Floats(0x87000)[i])*Floats(0x84000)[i];
			DECODER_STAGE_END(4);
		}
		Fence(); DecoderQuantize(0x84000,3072);
#endif
		DecoderMatrix(l.down,3072,1024,total); DecoderAddResidual();
	}
	DecoderNorm(TT_DECODER_HIDDEN,0x84000,1024,1024,root.finalNorm,total); DecoderQuantize(0x84000,1024);
	DecoderMatrix(root.vocabulary,1024,151936,total,true);
#if PIONEER_COORDINATOR_BATCH
	if (!autonomous) { completed=1; break; }
	uint32_t tokenPacket[32]; for (unsigned i=0;i<32;i++) tokenPacket[i]=Words(0x84000)[i];
	if (!PioneerDecoderGreedyValid(tokenPacket,root.nonce,root.tokens)) coordinator_fault_park();
	unsigned selected=tokenPacket[3];
	Words(0x146c00)[8+step]=selected; Words(0x146c00)[24+step]=tokenPacket[4]; completed++;
	if (PioneerDecoderIsEog(selected,root.reserved[2],root.reserved[3]) || completed==root.reserved[0]) break;
	DecoderEmbeddingIO io={total,root.vocabulary+(selected>>4)*18432,0};
	if (!PioneerDecoderEmbedding(io,selected,root.vocabulary,Floats(TT_DECODER_INPUT)) || io.reads!=64) coordinator_fault_park();
	root.tokens++; Fence();
	}
	if (autonomous) {
	Words(0x146c00)[0]=TT_DECODER_BATCH_MAGIC; Words(0x146c00)[1]=root.nonce; Words(0x146c00)[2]=151936;
	Words(0x146c00)[3]=completed; Words(0x146c00)[4]=firstTokens; Words(0x146c00)[5]=root.tokens;
	Words(0x146c00)[6]=119; Words(0x146c00)[7]=TT_DECODER_BATCH_OPERATION;
	unsigned verified=0;
	if (!PioneerDecoderBatchPacket(Words(0x146c00),root.nonce,firstTokens,root.reserved[0],root.reserved[2],root.reserved[3],verified)
		|| verified!=completed) coordinator_fault_park();
	DecoderCopy(0x84000,0x146c00,64);
	uint32_t prior[4]; for (unsigned i=0;i<4;i++) prior[i]=Words(0x143000)[i];
	HostTransfer(true,root.hostLow,root.hostHigh,64); HostTransfer(false,root.hostLow,root.hostHigh,64);
	for (unsigned i=0;i<64;i++) if (Words(0x84000)[i]!=Words(0x144000)[i]) coordinator_fault_park();
	for (unsigned i=0;i<4;i++) total[i]+=Words(0x143000)[i]-prior[i];
	}
#if PIONEER_COORDINATOR_STAGE_PROFILE
	Words(TT_DECODER_STAGE_L1)[2]=root.tokens; Words(TT_DECODER_STAGE_L1)[20]=completed;
#endif
#endif
	volatile uint32_t* stats=Words(0x143000); for (unsigned i=0;i<24;i++) stats[i]=0;
	for (unsigned i=0;i<4;i++) stats[i]=total[i];
	stats[5]=151936; stats[6]=root.nonce; stats[7]=PIONEER_COORDINATOR_GREEDY?root.operation:8;
	for (unsigned i=0;i<8;i++) stats[8+i]=before[i];
	DECODER_STAGE_END(7);
#if PIONEER_COORDINATOR_STAGE_PROFILE
	Words(TT_DECODER_STAGE_L1)[0]=0x31525044; Fence();
#endif
	Fence(); return Complete(119);
}
