/* SPDX-License-Identifier: MIT */
#if defined(_KERNEL_MODE) || defined(PIONEER_DRIVER_BUILD_TAG) || defined(PIONEER_Q38_FULL_OFFLOAD_NATIVE) \
 || defined(PIONEER_Q38_WORK_INFERENCE) || defined(TT_PIONEER_Q38_WORK_MODE) || defined(PIONEER_Q38_CPU_VALIDATION) \
 || defined(PIONEER_Q38_FULL_MODEL_CPU_REFERENCE) || defined(PIONEER_Q38_FULL_OFFLOAD_CLIENT)
#error Queued head CSR check is an isolated inspection component
#endif
#if defined(PIONEER_Q38_HEAD_CSR_BATCH_ROOT_IMAGE) || defined(PIONEER_Q38_HEAD_CSR_SERIAL_CONTROL_IMAGE)
#if (defined(PIONEER_Q38_HEAD_CSR_BATCH_ROOT_IMAGE) && PIONEER_Q38_HEAD_CSR_BATCH_ROOT_IMAGE != 1) \
 || (defined(PIONEER_Q38_HEAD_CSR_SERIAL_CONTROL_IMAGE) && PIONEER_Q38_HEAD_CSR_SERIAL_CONTROL_IMAGE != 1) \
 || (defined(PIONEER_Q38_HEAD_CSR_BATCH_ROOT_IMAGE) && defined(PIONEER_Q38_HEAD_CSR_SERIAL_CONTROL_IMAGE)) \
 || defined(PIONEER_Q38_HEAD_CSR_BATCH_INSPECTION) \
 || !defined(__riscv) || __riscv_xlen != 32 \
 || !defined(PIONEER_Q38_GLOBAL_TOKEN_ROOT_IMAGE) || PIONEER_Q38_GLOBAL_TOKEN_ROOT_IMAGE != 1 \
 || !defined(PIONEER_Q38_FULL_OFFLOAD_ROOT_IMAGE) || PIONEER_Q38_FULL_OFFLOAD_ROOT_IMAGE != 1 \
 || !defined(PIONEER_Q38_TOKEN_CANCEL_BOUNDARY) || PIONEER_Q38_TOKEN_CANCEL_BOUNDARY != 1 \
 || !defined(PIONEER_Q38_GLOBAL_CHAIN_PUSHED_COMPLETION) || PIONEER_Q38_GLOBAL_CHAIN_PUSHED_COMPLETION != 1 \
 || defined(PIONEER_Q38_HEAD_MATRIX_ROOT_INSPECTION) || defined(PIONEER_Q38_GLOBAL_CHAIN_BF16_FPU)
#error Queued head CSR check requires the distinct RAW pushed ROOT inspection image
#endif
#elif !defined(PIONEER_Q38_HEAD_CSR_BATCH_INSPECTION) || PIONEER_Q38_HEAD_CSR_BATCH_INSPECTION != 1 \
 || defined(PIONEER_Q38_GLOBAL_TOKEN_ROOT_IMAGE)
#error Queued head CSR check is an isolated inspection component
#endif
#ifndef PIONEER_Q38_HEAD_CSR_BATCH_H
#define PIONEER_Q38_HEAD_CSR_BATCH_H
#ifndef PIONEER_Q38_HEAD_CSR_SERIAL_CONTROL_IMAGE
#ifdef PIONEER_Q38_HEAD_CSR_BATCH_ROOT_IMAGE
typedef __UINT32_TYPE__ uint32_t;
#else
#include <stdint.h>
#endif

// Only the exact registers already checked by Closed() in the output head.
// Blackhole MMIO transfers are 4 bytes. Batching changes when the independent
// commands are waited on, never the width or register list. Caller must park
// on false; no further commands or retry after uncertain/readback failure.
namespace PioneerQ38HeadCsrBatch {
enum : uint32_t {Niu=0xffb20000U,Scratch=0x78000U,Control=0x40U,Available=0x64U,Overflow=0x68U};
enum : unsigned {Groups=3,Words=8,PollLimit=65536};

static inline uint32_t Counter(unsigned i)
{
	const uint32_t offsets[Words]={0x208,0x20c,0x210,0x214,0x204,0x220,0x210,0x228};
	return offsets[i];
}

static inline uint32_t Offset(unsigned group,unsigned i)
{
	if(group==2)return Counter(i);
	const unsigned source=group*Words+i;
	return !source?0x100:source<=6?0x118+(source-1)*4:source<=12?0x130+(source-7)*4:
		source==13?0x150:source==14?0x154:0x16c;
}

template<class IO> bool InitiatorsIdle(IO& io)
{
	for(unsigned i=0;i<4;i++)if(io.Read(Niu+Control+i*0x800))return false;
	return true;
}

template<class IO> bool TransactionsIdle(IO& io)
{
	for(unsigned i=0;i<32;i++)if(io.Read(Niu+0x240+i*4)&255)return false;
	return true;
}

// IO is the future ROOT's closed-worker source/route binding. Seal() proves
// the whole non-traffic boundary. Guard() proves immutable source, head and
// descriptor state while only these eight initially absent reads can be in
// flight. No calls leave an independent result or ready bit for a worker.
template<class IO>
#ifdef PIONEER_Q38_HEAD_CSR_BATCH_ROOT_IMAGE
__attribute__((optimize("Os")))
#endif
bool Verify(IO& io,unsigned worker,unsigned group,const uint32_t (&wanted)[Words],
	uint32_t* tracked)
{
	if(worker>=119 || group>=Groups || !tracked || wanted==tracked || !io.Seal() || !(io.Read(Niu+0x100)&0x10000U)
		|| !InitiatorsIdle(io) || io.Read(Niu+Available)!=0x10101010U || io.Read(Niu+Overflow)
		|| !TransactionsIdle(io))return false;
	uint32_t expected[Words];
	for(unsigned i=0;i<Words;i++){
		if(io.Read(Niu+Counter(i))!=tracked[i])return false;
		// Eight 4-byte read packets/replies and data flits, no writes.
		expected[i]=tracked[i]+(((0x4fU>>i)&1U)*Words);
	}
	if(expected[2]!=expected[6])return false;
	uint32_t command[10]={};command[2]=io.PeerRoute(worker+1);command[5]=129;command[8]=4;
	for(unsigned i=0;i<Words;i++){
		if(!io.Guard())return false;
		const uint32_t slots=io.Read(Niu+Available);
		if((slots&0xffffff00U)!=0x10101000U || !(slots&255) || (slots&255)>16 || io.Read(Niu+Overflow))return false;
		command[0]=Niu+Offset(group,i);
		// One disjoint L1 line per response, preserving low-five-bit alignment.
		command[3]=Scratch+i*32+(command[0]&31);
		for(unsigned word=0;word<10;word++)io.Write(Niu+word*4,command[word]);
		for(unsigned word=0;word<10;word++)if(io.Read(Niu+word*4)!=command[word])return false;
		io.Fence();io.Write(Niu+Control,1);(void)io.Read(Niu+Control);io.Fence();
	}
	for(unsigned poll=0;poll<PollLimit;poll++){
		const bool initiators=InitiatorsIdle(io);
		const uint32_t slots=io.Read(Niu+Available);
		if(io.Read(Niu+Overflow))return false;
		// FIFO-empty must be observed before any completion/counter reads.
		if(!initiators || slots!=0x10101010U)continue;
		if(!TransactionsIdle(io))continue;
		bool counters=true;
		for(unsigned i=0;i<Words;i++)if(io.Read(Niu+Counter(i))!=expected[i])counters=false;
		if(!counters)continue;
		if(!io.Seal())return false;
		for(unsigned word=0;word<10;word++)if(io.Read(Niu+word*4)!=command[word])return false;
		for(unsigned i=0;i<Words;i++)if(io.Read(Scratch+i*32+(Offset(group,i)&31))!=wanted[i])return false;
		volatile uint32_t* retained=tracked;
		for(unsigned i=0;i<Words;i++)retained[i]=expected[i];
		return true;
	}
	return false;
}
}
#endif
#endif
