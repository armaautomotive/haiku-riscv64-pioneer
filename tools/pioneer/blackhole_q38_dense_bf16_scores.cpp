/* SPDX-License-Identifier: MIT */
// Standalone returning experiment only. No driver or inference selector uses
// this function, and this file does not grant exclusive hardware ownership.
#define PIONEER_Q38_DENSE_BF16_MATRIX_EXPERIMENT 1
#define PIONEER_ATTENTION_FREESTANDING 1
#include "PioneerQ38DenseBf16Matrix.h"
#include "PioneerQ38DenseBf16Stage.h"
#if !defined(__riscv) || __riscv_xlen != 32
#error This experiment must be compiled for on-card RV32
#endif
extern "C" void scalar_fault_park() __attribute__((noreturn));
namespace {
volatile unsigned* Words(unsigned a){return reinterpret_cast<volatile unsigned*>(a);}
struct L1 {
 unsigned Read(unsigned a){return *Words(a);}
 void Write(unsigned a,unsigned v){*Words(a)=v;}
 void Fence(){asm volatile("fence":::"memory");}
};
struct Emitter {
 void Op(unsigned v){*Words(0xffe40000)=v;}
 void Config(unsigned i,unsigned v){*Words(0xffef0000+i*4)=v;if(*Words(0xffef0000+i*4)!=v)scalar_fault_park();}
 void Drain(){
  *Words(0xffe000fc)=0;(void)*Words(0xffe000fc);asm volatile("fence":::"memory");
  Op(0xa2101816);Op(0x4500017e);
  for(unsigned i=0;i<1000000;i++)if(*Words(0xffe000fc)){asm volatile("fence":::"memory");return;}
  scalar_fault_park(); // no retry, output publication, reset or cleanup
 }
};
}
// Candidate layouts mirror the existing worker's private query and K chunk.
// The caller must independently establish that scratch A5000..A61FF and
// result 9E000..9E07F are disjoint from its COMPLETE live mixed-worker map.
// This source alone does not prove that ownership or launch is safe.
extern "C" unsigned q38_dense_bf16_scores(unsigned count,unsigned head){
 if(!count || count>32 || head>=6)return 0;
 L1 io;Emitter e;
 PioneerQ38DenseBf16Stage<L1> stage(io,0xa0000,0x85000+head*1024,0xa5000,0xa6000,count);
 if(!PioneerQ38DenseBf16Matrix::Scores(e,stage,0xa5000,0xa6000))scalar_fault_park();
 unsigned values[32];
 for(unsigned row=0;row<32;row++){
  values[row]=*Words(0xffbd8000+(row<16?32:40)*64+(row&15)*4);
  if((values[row]&0x7f800000)==0x7f800000)scalar_fault_park();
 }
 for(unsigned row=0;row<32;row++)*Words(0x9e000+row*4)=values[row];
 asm volatile("fence":::"memory");return count;
}
// Single high-by-high BF16 PV product for one chunk/block; this diagnostic
// leaf does NOT include the three residual products used by the worker.
// Caller owns cross-chunk merge and preservation of probabilities.
extern "C" unsigned q38_dense_bf16_values(unsigned count,unsigned head,unsigned first,unsigned block){
 if(!count || count>32 || head>=6 || first>=384 || count>384-first || block>=8)return 0;
 L1 io;Emitter e;
 PioneerQ38DenseBf16ValuesStage<L1> stage(io,0xa0000,0x90000+(head*384+first)*4,0xa5000,0xa6000,count,block);
 if(!PioneerQ38DenseBf16Matrix::Values(e,stage,0xa5000,0xa6000))scalar_fault_park();
 unsigned values[32];
 for(unsigned row=0;row<32;row++){
  values[row]=*Words(0xffbd8000+(row<16?32:40)*64+(row&15)*4);
  if((values[row]&0x7f800000)==0x7f800000)scalar_fault_park();
 }
 for(unsigned row=0;row<32;row++)*Words(0x9e000+row*4)=values[row];
 asm volatile("fence":::"memory");return count;
}
