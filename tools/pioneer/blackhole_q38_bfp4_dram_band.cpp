/* SPDX-License-Identifier: MIT */
// New OFFBOARD-only C64 GDDR -> BFP4 full-K synthetic band image.
// No installed owner, production driver, model Work or CPU reference selects it.
#define PIONEER_DRAM_SWEEP_FREESTANDING 1
#include "PioneerNativeBfp4DramBand.h"
#include "PioneerNativeBfp4BandTiming.h"
#if !defined(__riscv) || __riscv_xlen!=32 || !defined(PIONEER_BFP4_DRAM_BAND_K)
#error Select a fixed RV32I/Zicsr BFP4 GDDR diagnostic image
#endif
static_assert(PioneerNativeBfp4DramBandLayout::Width(PIONEER_BFP4_DRAM_BAND_K),"exact Qwen width only");
extern "C" void native_dram_band_fault() __attribute__((noreturn));
namespace {
static volatile unsigned* Words(unsigned address){return reinterpret_cast<volatile unsigned*>(address);}
struct IO {
 unsigned Read(unsigned a){return *Words(a);}
 void Write(unsigned a,unsigned v){*Words(a)=v;}
 void Fence(){asm volatile("fence":::"memory");}
};
struct Emitter {
 void Op(unsigned word){*Words(0xffe40000)=word;}
 void Drain(){
  *Words(0xffe000fc)=0;(void)*Words(0xffe000fc);asm volatile("fence":::"memory");
  Op(0xa2101816);Op(0x4500017e);
  for(unsigned attempt=0;attempt<1000000;attempt++)if(*Words(0xffe000fc)){asm volatile("fence":::"memory");return;}
  native_dram_band_fault();
 }
 void Config(unsigned index,unsigned value){volatile unsigned* at=Words(0xffef0000+index*4);*at=value;if(*at!=value)native_dram_band_fault();}
};
__attribute__((noinline)) bool Clock(unsigned* words){
 for(unsigned attempt=0;attempt<4;attempt++){
  unsigned high,low,confirm;
  asm volatile("csrrs %0,0xb80,x0\ncsrrs %1,0xb00,x0\ncsrrs %2,0xb80,x0"
   :"=r"(high),"=r"(low),"=r"(confirm)::"memory");
  if(high==confirm){words[0]=low;words[1]=high;return true;}
 }
 return false;
}
}
extern "C" unsigned native_dram_band_main(){
 unsigned frame[8];for(unsigned i=0;i<8;i++)frame[i]=Words(0x74000)[i];
 if(frame[0]!=0x34534442 || frame[1]!=1 || frame[2]!=PIONEER_BFP4_DRAM_BAND_K
  || !PioneerNativeBfp4DramBand::Loader::Route(frame[3]) || !frame[4] || frame[5] || frame[6] || frame[7])native_dram_band_fault();
 for(unsigned pass=0;pass<2;pass++){
  unsigned start[2]={},end[2]={};bool startValid=Clock(start);
  IO io;PioneerNativeBfp4DramBand::Loader loader;
  if(!loader.Run(io,PIONEER_BFP4_DRAM_BAND_K,frame[3]))native_dram_band_fault();
  Emitter emitter;if(!PioneerNativeBfp4DramBand::Compute(emitter,PIONEER_BFP4_DRAM_BAND_K))native_dram_band_fault();
  bool endValid=Clock(end);unsigned record[8];
  PioneerNativeBfp4BandTiming::Record(PIONEER_BFP4_DRAM_BAND_K,pass,start,startValid,end,endValid,record);
  for(unsigned i=0;i<8;i++)Words(PioneerNativeBfp4BandTiming::Address+pass*32)[i]=record[i];
  for(unsigned row=0;row<2;row++)for(unsigned column=0;column<16;column++)
   Words(PioneerNativeBfp4DramBandLayout::Output+pass*128)[row*16+column]=Words(0xffbd8000+(row?40:32)*64)[column];
  asm volatile("fence":::"memory");
  for(unsigned i=0;i<8;i++)if(Words(0x74000)[i]!=frame[i])native_dram_band_fault();
 }
 return 0x02000000|PIONEER_BFP4_DRAM_BAND_K;
}
