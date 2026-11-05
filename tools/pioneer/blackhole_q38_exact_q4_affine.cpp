/* SPDX-License-Identifier: MIT */
// Distinct full-K OFFBOARD image. No installed owner can launch this.
#define PIONEER_ATTENTION_FREESTANDING
typedef __UINT32_TYPE__ uint32_t;
#include "PioneerQ38ExactQ4Affine.h"
#if !defined(__riscv) || __riscv_xlen != 32 || !defined(PIONEER_EXACT_Q4_AFFINE_K)
#error Select a distinct fixed RV32I original-Q4 full-K prototype
#endif
static_assert(PioneerQ38ExactQ4Affine::Width(PIONEER_EXACT_Q4_AFFINE_K),"original Qwen width only");
extern "C" void exact_q4_affine_fault() __attribute__((noreturn));
namespace {
static inline volatile unsigned* Words(unsigned a){return reinterpret_cast<volatile unsigned*>(a);}
struct Emitter {
 void Op(unsigned word){*Words(0xffe40000)=word;}
 void Drain(){
  *Words(0xffe000fc)=0;(void)*Words(0xffe000fc);asm volatile("fence":::"memory");
  Op(0xa2101816);Op(0x4500017e);
  for(unsigned n=0;n<1000000;n++)if(*Words(0xffe000fc)){asm volatile("fence":::"memory");return;}
  exact_q4_affine_fault(); // unknown:park,NO subsequent source/output/cleanup
 }
 void Config(unsigned i,unsigned v){*Words(0xffef0000+i*4)=v;if(*Words(0xffef0000+i*4)!=v)exact_q4_affine_fault();}
 void SealData(){asm volatile("fence":::"memory");}
 unsigned Mask(unsigned i){return Words(0x90300)[i];}
 void Seven(unsigned i,unsigned v){Words(0x90100)[i]=v;}
 void Escape(unsigned i,unsigned v){Words(0x90200)[i]=v;}
 void ZeroData(){
  for(unsigned i=0;i<260;i++)Words(PioneerQ38ExactQ4Affine::Zero-16)[i]=0;
  for(unsigned i=0;i<4;i++){Words(PioneerQ38ExactQ4Affine::Coefficient-16)[i]=0;Words(PioneerQ38ExactQ4Affine::Minimum-16)[i]=0;}
 }
 bool StagePanel(unsigned p,unsigned& scale,int& sum){
  if(p>=PIONEER_EXACT_Q4_AFFINE_K/16)return false;
  // Scalar copies/bit permutation ONLY,all on card. No learned dot/FP
  // affine calculation on the RISC controller and no host round trip.
  const volatile unsigned* src=Words(PioneerQ38ExactQ4Affine::Source+p*PioneerQ38ExactQ4Affine::PacketBytes);
  for(unsigned i=0;i<32;i++)Words(0x90000)[i]=src[i];
  for(unsigned i=0;i<8;i++)Words(0x90300)[i]=src[32+i];
  for(unsigned r=0;r<8;r++)for(unsigned i=0;i<8;i++)Words(0x91000)[r*8+i]=src[40+i];
  for(unsigned i=0;i<16;i++){Words(PioneerQ38ExactQ4Affine::Coefficient)[i]=src[48+i];Words(PioneerQ38ExactQ4Affine::Minimum)[i]=src[64+i];}
  scale=src[80];sum=int(src[81]);return true;
 }
};
}
extern "C" unsigned exact_q4_affine_main(){
 Emitter e;
 for(unsigned pass=0;pass<2;pass++) {
  if(!PioneerQ38ExactQ4Affine::Run(e,PIONEER_EXACT_Q4_AFFINE_K))exact_q4_affine_fault();
  for(unsigned n=0;n<16;n++)Words(PioneerQ38ExactQ4Affine::Output+pass*64)[n]=Words(0xffbd8000+108*64)[n];
  asm volatile("fence":::"memory");
 }
 return 2;
}
