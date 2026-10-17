/* SPDX-License-Identifier: MIT */
// Experimental whole-position Qwen3-0.6B immutable instruction candidate.
// Only the dedicated sealed model registry may own/load it. Parallel
// attention additionally requires eight independent code/data leases.
#define PIONEER_ATTENTION_FREESTANDING
// Whole-decoder-only optimization;legacy attention images remain byte exact.
#define PIONEER_ATTENTION_EXACT_HALF_PRODUCT 1
#include "PioneerGraphNorm.h"
#include "PioneerGraphRope.h"
#include "PioneerCoordinatedAttentionProof.h"
#include "../../src/add-ons/kernel/drivers/misc/PioneerDecoderLayout.h"
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
static void DecoderCopy(unsigned destination,unsigned source,unsigned words)
{
	for (unsigned i=0;i<words;i++) Words(destination)[i]=Words(source)[i];
	Fence();
	for (unsigned i=0;i<words;i++) if (Words(destination)[i]!=Words(source)[i]) coordinator_fault_park();
}
static void DecoderRead(unsigned destination,uint32_t source,unsigned bytes,uint32_t (&total)[4])
{
	if ((source&511) || (destination&511) || (bytes!=512 && bytes!=4096) || source<0x04000000 || source>=0x82400000
		|| bytes>0x82400000-source || destination!=TT_DECODER_NORM || !PacketIdle()) coordinator_fault_park();
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
static void DecoderQuantize(unsigned source,unsigned n)
{
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
}
static void DecoderMatrix(uint32_t base,unsigned k,unsigned n,uint32_t (&total)[4],bool final=false)
{
	volatile uint32_t* j=Words(0x140000); volatile auto& root=DecoderJob();
	for (unsigned i=0;i<64;i++) j[i]=0;
	j[0]=0x31445243; j[1]=final?6:1; j[2]=119; j[3]=k; j[4]=n; j[5]=base; j[6]=root.nonce;
	if (final) { j[7]=root.hostLow; j[8]=root.hostHigh; j[9]=root.hostBytes; }
	unsigned peers=n/32; if (peers>119) peers=119;
	for (unsigned i=0;i<peers;i++) j[64+i]=root.sequence[i];
	Fence();
	if (decoder_matrix_main()!=peers) coordinator_fault_park();
	for (unsigned i=0;i<4;i++) total[i]+=Words(0x143000)[i];
	for (unsigned i=0;i<peers;i++) root.sequence[i]++;
	Fence();
}
static void DecoderNorm(unsigned input,unsigned output,unsigned n,unsigned width,uint32_t weight,uint32_t (&total)[4])
{
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
}
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
	volatile auto& root=DecoderJob(); volatile uint32_t* stats=Words(0x143000);
	for (unsigned i=0;i<4;i++) stats[i]=0;
	for (unsigned group=0;group<8;group++) {
		unsigned worker=group+1; uint32_t sequence=root.sequence[group];
		if (!PeerReleased(worker,sequence-1)) coordinator_fault_park();
		// Stage/return/compare EVERY input bit before releasing this peer.
		Transfer(true,worker,0x140000+group*1024,0x90000,1024);
		Transfer(false,worker,0x143100,0x90000,1024);
		for (unsigned i=0;i<256;i++) if (Words(0x143100)[i]!=Words(0x140000)[group*256+i]) coordinator_fault_park();
		Transfer(true,worker,0x142000+group*512,0x90400,512);
		Transfer(false,worker,0x143100,0x90400,512);
		for (unsigned i=0;i<128;i++) if (Words(0x143100)[i]!=Words(0x142000)[group*128+i]) coordinator_fault_park();
		volatile uint32_t* args=Words(0x147100);
		args[0]=0x31475441; args[1]=root.tokens; args[2]=layer; args[3]=group; Fence();
		Transfer(true,worker,0x147100,0x74000,16); Transfer(false,worker,0x147120,0x74000,16);
		for (unsigned i=0;i<4;i++) if (Words(0x147120)[i]!=args[i]) coordinator_fault_park();
		for (unsigned i=0;i<8;i++) args[i]=0;
		args[3]=1; args[4]=XY(worker); args[7]=0x31475441; Fence();
		Transfer(true,worker,0x147100,0x70500,32); Transfer(false,worker,0x147120,0x70500,32);
		for (unsigned i=0;i<8;i++) if (Words(0x147120)[i]!=args[i]) coordinator_fault_park();
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
		if (!PioneerCoordinatedAttentionProof(proof,root.tokens,layer,group)) coordinator_fault_park();
		for (unsigned i=0;i<4;i++) if (snapshot[i]!=proof[i] || snapshot[4+i]!=proof[8+i]) coordinator_fault_park();
		Transfer(false,worker,0x144000+group*1024,0xb2000,1024);
		for (unsigned i=0;i<256;i++) if ((Words(0x144000)[group*256+i]&0x7f800000)==0x7f800000) coordinator_fault_park();
		root.sequence[group]++; Fence();
	}
	for (unsigned i=0;i<4;i++) total[i]+=stats[i];
}
#endif
extern "C" unsigned coordinator_main()
{
	volatile auto& root=DecoderJob();
	if (!PioneerDecoderJobValid(root,PIONEER_COORDINATOR_PARALLEL_ATTENTION?2:1) || !NocIdle()) return 0;
	for (unsigned i=0;i<1024;i++) if ((Words(TT_DECODER_INPUT)[i]&0x7fffffff)>0x477fe000) return 0;
	uint32_t before[8],total[4]={}; for (unsigned i=0;i<8;i++) before[i]=*Words(0xffb20000+CounterOffset(i));
	DecoderCopy(TT_DECODER_HIDDEN,TT_DECODER_INPUT,1024);
	const float* rope=Floats(TT_DECODER_JOB_BASE+offsetof(PioneerDecoderJob,rope));
	for (unsigned layer=0;layer<28;layer++) {
		const volatile auto& l=root.layers[layer];
		DecoderCopy(TT_DECODER_RESIDUAL,TT_DECODER_HIDDEN,1024);
		DecoderNorm(TT_DECODER_HIDDEN,0x84000,1024,1024,l.attentionNorm,total); DecoderQuantize(0x84000,1024);
		DecoderMatrix(l.q,1024,2048,total); DecoderCopy(TT_DECODER_Q,0x84000,2048);
		DecoderMatrix(l.v,1024,1024,total);
		for (unsigned g=0;g<8;g++) for (unsigned i=0;i<64;i++) Words(0x142100)[g*128+i]
			=PioneerAttentionToHalf(Floats(0x84000)[g*128+i*2])|(uint32_t(PioneerAttentionToHalf(Floats(0x84000)[g*128+i*2+1]))<<16);
		DecoderMatrix(l.k,1024,1024,total); DecoderCopy(TT_DECODER_K,0x84000,1024);
		DecoderNorm(TT_DECODER_Q,TT_DECODER_Q,2048,128,l.qNorm,total);
		DecoderNorm(TT_DECODER_K,TT_DECODER_K,1024,128,l.kNorm,total);
		if (!PioneerGraphRopeNeox(2048,Floats(TT_DECODER_Q),rope,Floats(TT_DECODER_Q))
			|| !PioneerGraphRopeNeox(1024,Floats(TT_DECODER_K),rope,Floats(TT_DECODER_K))) coordinator_fault_park();
		for (unsigned g=0;g<8;g++) for (unsigned i=0;i<64;i++) Words(0x142000)[g*128+i]
			=PioneerAttentionToHalf(Floats(TT_DECODER_K)[g*128+i*2])|(uint32_t(PioneerAttentionToHalf(Floats(TT_DECODER_K)[g*128+i*2+1]))<<16);
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
		DecoderMatrix(l.gate,1024,3072,total); DecoderCopy(0x87000,0x84000,3072); DecoderMatrix(l.up,1024,3072,total);
		for (unsigned i=0;i<3072;i++) Floats(0x84000)[i]=PioneerGraphSiluFast(Floats(0x87000)[i])*Floats(0x84000)[i];
		Fence(); DecoderQuantize(0x84000,3072); DecoderMatrix(l.down,3072,1024,total); DecoderAddResidual();
	}
	DecoderNorm(TT_DECODER_HIDDEN,0x84000,1024,1024,root.finalNorm,total); DecoderQuantize(0x84000,1024);
	DecoderMatrix(root.vocabulary,1024,151936,total,true);
	volatile uint32_t* stats=Words(0x143000); for (unsigned i=0;i<24;i++) stats[i]=0;
	for (unsigned i=0;i<4;i++) stats[i]=total[i];
	stats[5]=151936; stats[6]=root.nonce; stats[7]=8;
	for (unsigned i=0;i<8;i++) stats[8+i]=before[i];
	Fence(); return Complete(119);
}
