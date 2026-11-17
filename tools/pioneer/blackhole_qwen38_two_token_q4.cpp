/* SPDX-License-Identifier: MIT */
// Isolated 16-output/K256, two-activation Q4_K proof. No native owner,
// weight transport, speculative state, Work selector or model inference.
#if !defined(PIONEER_Q38_TWO_TOKEN_Q4_IMAGE) || PIONEER_Q38_TWO_TOKEN_Q4_IMAGE != 1 \
 || defined(_KERNEL_MODE) || defined(PIONEER_Q38_FULL_OFFLOAD_NATIVE) \
 || defined(PIONEER_Q38_WORK_INFERENCE) || defined(PIONEER_Q38_CPU_VALIDATION) \
 || defined(PIONEER_Q38_FULL_MODEL_CPU_REFERENCE) || defined(__FAST_MATH__)
#error Separate offline two-token Q4 image required
#endif
#define block_main unused_single_token_q38_block
#include "blackhole_qwen38_block.cpp"
#undef block_main

namespace {
enum {Magic=0x32543851,Width=256,Outputs=16,Tokens=2,
 Descriptor=0x74000,Input=0x80000,InputStride=Width*2,Output=0x84000,Weight=0x90000,
 Operand=0x70920,ReadDst=0xffbd8800};
static_assert(Input+Tokens*InputStride<=Output && Output+Tokens*Outputs*4<=Weight,
 "separate immutable inputs and packed tile, disjoint output");

bool ScaleValid(uint32_t bits){
 const unsigned m=bits&0x7fffffff;
 return !m || (m>=0x2f800000 && m<=0x47800000);
}
int Value(uint16_t x){return int(x&1023)*((x&0x8000)?-1:1);}
}

extern "C" unsigned block_main(){
 const auto* job=Words(Descriptor);
 if(job[0]!=Magic || job[1]!=12 || job[2]!=2304 || job[3]
  || !ScaleValid(job[4]) || !ScaleValid(job[5]))return 0;
 for(unsigned w=6;w<16;w++)if(job[w])return 0;
 const float scale[Tokens]={Float(job[4]),Float(job[5])};
 for(unsigned w=0;w<Tokens*Width/2;w++){
  uint32_t word=Words(Input)[w];
  if(!PioneerInt8OperandValid(uint16_t(word)) || !PioneerInt8OperandValid(uint16_t(word>>16)))return 0;
 }
 volatile float accum[Tokens][Outputs];
 for(unsigned t=0;t<Tokens;t++)for(unsigned n=0;n<Outputs;n++)accum[t][n]=0;
 Setup(); // initializes all eight SrcB rows, including unused rows 2..7
 for(unsigned p=0;p<Width/16;p++){
  PioneerQ38Panel* local=reinterpret_cast<PioneerQ38Panel*>(0x91000);
  if(!PioneerQ38UnpackPanel(12,reinterpret_cast<const uint8_t*>(Weight),2304,p,local))block_fault_park();
  for(unsigned w=0;w<128;w++)Words(0x70710)[w]=local->operand[w];
  int sums[Tokens]={};
  for(unsigned t=0;t<Tokens;t++)for(unsigned w=0;w<8;w++){
   const uint32_t word=Words(Input+t*InputStride)[p*8+w];
   Words(Operand)[t*8+w]=word;
   sums[t]+=Value(uint16_t(word))+Value(uint16_t(word>>16));
  }
  (void)Words(0x70710)[127];(void)Words(Operand)[15];Fence();
  // The unchanged non-broadcast MVMUL issues four fidelity phases.
  // One SrcA unpack is shared by both independent SrcB rows.
  Matrix();
  for(unsigned t=0;t<Tokens;t++)for(unsigned n=0;n<Outputs;n++){
   const int dot=int(Words(ReadDst+t*Outputs*4)[n]);
   if(dot<-262144 || dot>262144)block_fault_park();
   const float product=(Float(local->coefficient[n])*scale[t])*float(dot);
   const float correction=(Float(local->minimum[n])*scale[t])*float(sums[t]);
   const float next=accum[t][n]+(product+correction);
   if((Bits(next)&0x7f800000)==0x7f800000)block_fault_park();
   accum[t][n]=next;
  }
 }
 for(unsigned t=0;t<Tokens;t++)for(unsigned n=0;n<Outputs;n++)Words(Output)[t*Outputs+n]=Bits(accum[t][n]);
 Fence();return Width/16;
}
