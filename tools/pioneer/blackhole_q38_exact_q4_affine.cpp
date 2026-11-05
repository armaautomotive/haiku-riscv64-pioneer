/* SPDX-License-Identifier: MIT */
// Distinct full-K OFFBOARD image. No installed owner can launch this.
#define PIONEER_ATTENTION_FREESTANDING
typedef __UINT32_TYPE__ uint32_t;
#include "PioneerQ38ExactQ4Affine.h"
#ifdef PIONEER_EXACT_Q4_TWO_PRODUCT
#if PIONEER_EXACT_Q4_TWO_PRODUCT != 1 || defined(PIONEER_EXACT_Q4_AFFINE_FIXED)
#error Separate two-product packet image only
#endif
#ifdef PIONEER_EXACT_Q4_TWO_PRODUCT_REPLAY
#if PIONEER_EXACT_Q4_TWO_PRODUCT_REPLAY != 1
#error Separate replay two-product packet image only
#endif
#include "PioneerQ38ExactQ4TwoProductReplay.h"
using SelectedQ4Affine=PioneerQ38ExactQ4TwoProductReplay;
#else
#include "PioneerQ38ExactQ4TwoProduct.h"
using SelectedQ4Affine=PioneerQ38ExactQ4TwoProductAffine;
#endif
#else
#ifdef PIONEER_EXACT_Q4_TWO_PRODUCT_REPLAY
#error Replay packet image requires two-product parent
#endif
using SelectedQ4Affine=PioneerQ38ExactQ4Affine;
#endif
#ifdef PIONEER_EXACT_Q4_AFFINE_FIXED
#if PIONEER_EXACT_Q4_AFFINE_FIXED != 1 || PIONEER_EXACT_Q4_AFFINE_K != 17408
#error Distinct fixed17408 synthetic qualification only
#endif
#include "PioneerQ38ExactQ4FixedData.h"
#endif
#if !defined(__riscv) || __riscv_xlen != 32 || !defined(PIONEER_EXACT_Q4_AFFINE_K)
#error Select a distinct fixed RV32I original-Q4 full-K prototype
#endif
static_assert(PioneerQ38ExactQ4Affine::Width(PIONEER_EXACT_Q4_AFFINE_K),"original Qwen width only");
extern "C" void exact_q4_affine_fault() __attribute__((noreturn));
namespace {
static inline volatile unsigned* Words(unsigned a){return reinterpret_cast<volatile unsigned*>(a);}
struct Emitter {
#ifdef PIONEER_EXACT_Q4_AFFINE_FIXED
 unsigned seed=37;
#endif
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
#ifdef PIONEER_EXACT_Q4_AFFINE_FIXED
  using F=PioneerQ38ExactQ4FixedData;
  for(unsigned word=0;word<32;word++){
   unsigned value=0;
   for(unsigned half=0;half<8;half++){
    const unsigned i=word*8+half,q=F::Quant(seed,p,i/16,i%16);const int z=int(q==15?14:q)-7;
    value|=(z<0?unsigned(-z)|8:unsigned(z))<<(half*4);
   }
   Words(0x90000)[word]=value;
  }
  for(unsigned word=0;word<8;word++){
   unsigned mask=0;for(unsigned bit=0;bit<32;bit++){const unsigned i=word*32+bit;mask|=unsigned(F::Quant(seed,p,i/16,i%16)==15)<<bit;}
   Words(0x90300)[word]=mask;
  }
  sum=0;for(unsigned k=0;k<16;k++)sum+=F::Input(seed,p,k);
  for(unsigned r=0;r<8;r++)for(unsigned i=0;i<8;i++)Words(0x91000)[r*8+i]=F::InputBits(seed,p,i*2)|(F::InputBits(seed,p,i*2+1)<<16);
  for(unsigned n=0;n<16;n++){Words(PioneerQ38ExactQ4Affine::Coefficient)[n]=F::Coefficient(seed,p,n);Words(PioneerQ38ExactQ4Affine::Minimum)[n]=F::Minimum(seed,p,n);}
  scale=F::Scale(seed,p);return true;
#else
  // Scalar copies/bit permutation ONLY,all on card. No learned dot/FP
  // affine calculation on the RISC controller and no host round trip.
  const volatile unsigned* src=Words(PioneerQ38ExactQ4Affine::Source+p*PioneerQ38ExactQ4Affine::PacketBytes);
  for(unsigned i=0;i<32;i++)Words(0x90000)[i]=src[i];
  for(unsigned i=0;i<8;i++)Words(0x90300)[i]=src[32+i];
  for(unsigned r=0;r<8;r++)for(unsigned i=0;i<8;i++)Words(0x91000)[r*8+i]=src[40+i];
  for(unsigned i=0;i<16;i++){Words(PioneerQ38ExactQ4Affine::Coefficient)[i]=src[48+i];Words(PioneerQ38ExactQ4Affine::Minimum)[i]=src[64+i];}
  scale=src[80];sum=int(src[81]);return true;
#endif
 }
};
}
extern "C" unsigned exact_q4_affine_main(){
 Emitter e;
 for(unsigned pass=0;pass<2;pass++) {
#ifdef PIONEER_EXACT_Q4_AFFINE_FIXED
  e.seed=pass?176:37;
#endif
  if(!SelectedQ4Affine::Run(e,PIONEER_EXACT_Q4_AFFINE_K))exact_q4_affine_fault();
  for(unsigned n=0;n<16;n++)Words(PioneerQ38ExactQ4Affine::Output+pass*64)[n]=Words(0xffbd8000+108*64)[n];
  asm volatile("fence":::"memory");
 }
 return 2;
}
