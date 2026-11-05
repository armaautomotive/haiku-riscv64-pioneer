/* SPDX-License-Identifier: MIT */
// NEW OFFBOARD image. No native owner or installed Work path accepts it.
#if !defined(__riscv) || __riscv_xlen!=32 || !defined(PIONEER_Q4_FPU_COMPACT_STREAM_K)
#error Fixed RV32I Q4 streaming width required
#endif
#if !defined(PIONEER_Q38_COMPACT_Q4_FPU_STREAM_IMAGE) || PIONEER_Q38_COMPACT_Q4_FPU_STREAM_IMAGE != 1 \
 || defined(_KERNEL_MODE) || defined(PIONEER_Q38_FULL_OFFLOAD_NATIVE) || defined(PIONEER_Q38_FULL_OFFLOAD_CLIENT) \
 || defined(PIONEER_Q38_WORK_INFERENCE) || defined(TT_PIONEER_Q38_WORK_MODE) \
 || defined(PIONEER_Q38_CPU_VALIDATION) || defined(PIONEER_Q38_FULL_MODEL_CPU_REFERENCE) \
 || defined(PIONEER_DRIVER_BUILD_TAG) || defined(TT_PIONEER_DRIVER_RELOAD) \
 || defined(PIONEER_Q4_FPU_STREAM_K) || defined(PIONEER_Q38_COMPACT_Q4_FPU_IMAGE) || defined(__FAST_MATH__)
#error Distinct compact full-K OFFBOARD image only
#endif
#define PIONEER_DRAM_SWEEP_FREESTANDING 1
#define PIONEER_ATTENTION_FREESTANDING
#include "PioneerQ38Q4FpuCompactStreamLoader.h"
#include "PioneerQ38Q4FpuCompactStreamCompute.h"
static_assert(PioneerQ38Q4FpuCompactStreamLayout::Width(PIONEER_Q4_FPU_COMPACT_STREAM_K),"exact Q4 model widths only");
extern "C" void q4_fpu_compact_stream_fault() __attribute__((noreturn));
// Compiler aggregate initialization ONLY. Word stores are restricted to the
// owned ABI stack;this cannot become a hidden scalar operand-copy path.
extern "C" __attribute__((noinline)) void* memset(void* destination,int value,unsigned bytes) {
	unsigned a=reinterpret_cast<unsigned>(destination);
	if(value || (a&3) || (bytes&3) || a<0x6e000 || a>0x70000 || bytes>0x70000-a)q4_fpu_compact_stream_fault();
	volatile unsigned* p=static_cast<volatile unsigned*>(destination);
	for(unsigned i=0;i<bytes/4;i++)p[i]=0;
	return destination;
}
namespace {
using L=PioneerQ38Q4FpuCompactStreamLayout;using C=PioneerQ38Q4FpuCompactStreamCompute;
static inline volatile unsigned* Words(unsigned a){return reinterpret_cast<volatile unsigned*>(a);}
struct IO {
	unsigned Read(unsigned a){return *Words(a);}
	void Write(unsigned a,unsigned v){*Words(a)=v;}
	void Fence(){asm volatile("fence":::"memory");}
};
struct Emitter {
	__attribute__((always_inline)) inline void Op(unsigned word) {
		if(__builtin_constant_p(word))asm volatile(".word %0"::"i"((word<<2)|(word>>30)):"memory");
		else *Words(0xffe40000)=word;
	}
	void Drain() {
		*Words(0xffe000fc)=0;(void)*Words(0xffe000fc);asm volatile("fence":::"memory");
		Op(0xa2101816);Op(0x4500017e);
		for(unsigned p=0;p<1000000;p++)if(*Words(0xffe000fc)){asm volatile("fence":::"memory");return;}
		q4_fpu_compact_stream_fault(); // park,no next group/packet/read/reset/cleanup
	}
	void Config(unsigned i,unsigned v){volatile unsigned* p=Words(0xffef0000+i*4);*p=v;if(*p!=v)q4_fpu_compact_stream_fault();}
};
}
extern "C" unsigned q4_fpu_compact_stream_main() {
	unsigned frame[8];for(unsigned i=0;i<8;i++)frame[i]=Words(L::Frame)[i];
	if(frame[0]!=0x31534351 || frame[1]!=1 || frame[2]!=PIONEER_Q4_FPU_COMPACT_STREAM_K
		|| !L::Route(frame[3]) || !frame[4] || frame[5] || frame[6] || frame[7])q4_fpu_compact_stream_fault();
	for(unsigned pass=0;pass<2;pass++) {
		IO io;Emitter emitter;PioneerQ38Q4FpuCompactStreamLoader loader;
		if(!loader.Begin(io,PIONEER_Q4_FPU_COMPACT_STREAM_K,frame[3]) || !loader.Open(io,0,0))q4_fpu_compact_stream_fault();
		for(unsigned slice=0;slice<2;slice++)if(!loader.Slice(io))q4_fpu_compact_stream_fault();
		if(!loader.Wait(io))q4_fpu_compact_stream_fault();
		C::Begin(emitter); // SOLE Dst clear per FULL-K job,NOT per K256
		for(unsigned block=0;block<PIONEER_Q4_FPU_COMPACT_STREAM_K/256;block++) {
			unsigned buffer=block&1,next=block+1<PIONEER_Q4_FPU_COMPACT_STREAM_K/256;
			if(!loader.Acquire(block,buffer) || (next && !loader.Open(io,block+1,buffer^1)))q4_fpu_compact_stream_fault();
			if(next && !loader.Slice(io))q4_fpu_compact_stream_fault();
			if(!C::LoadMetadata(emitter,buffer))q4_fpu_compact_stream_fault();
			for(unsigned group=0;group<8;group++) {
				if(next && group==0 && !loader.Slice(io))q4_fpu_compact_stream_fault();
				if(!C::Group(emitter,buffer,group))q4_fpu_compact_stream_fault();
			}
			if(!loader.Release(block,buffer) || (next && !loader.Wait(io)))q4_fpu_compact_stream_fault();
		}
		if(!loader.Complete(io))q4_fpu_compact_stream_fault();
		for(unsigned row=0;row<2;row++)for(unsigned col=0;col<16;col++)
			Words(L::Output+pass*128)[row*16+col]=Words(0xffbd8000+(row?40:32)*64)[col];
		asm volatile("fence":::"memory");
	}
	for(unsigned i=0;i<8;i++)if(Words(L::Frame)[i]!=frame[i])q4_fpu_compact_stream_fault();
	return 0x04000000|PIONEER_Q4_FPU_COMPACT_STREAM_K;
}

