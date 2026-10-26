/* SPDX-License-Identifier: MIT */
#include "PioneerQ38DenseAttentionImageRole.h"
#define PIONEER_ATTENTION_FREESTANDING
#include "PioneerQ38DensePrepare.h"
#include "PioneerQ38DensePrepareLayout.h"
#include "PioneerQ38DenseAttentionLayout.h"
extern "C" void scalar_fault_park() __attribute__((noreturn));
namespace DenseAttention {
namespace L=PioneerQ38DenseAttentionLayout;
static volatile uint32_t* Words(unsigned a){return reinterpret_cast<volatile uint32_t*>(a);}
static float* Floats(unsigned a){return reinterpret_cast<float*>(a);}
static void Fence(){asm volatile("fence" ::: "memory");}
static void Emit(uint32_t op){*Words(0xffe40000)=op;}
static unsigned Lane(unsigned lane){return (lane>>3)*16+(lane&7)*2;}
static unsigned Counter(unsigned i){return i==0?0x208:i==1?0x20c:(i==2 || i==6)?0x210:i==3?0x214:i==4?0x204:i==5?0x220:0x228;}
static unsigned Source(unsigned i){return !i?0x100:i<=6?0x118+(i-1)*4:i<=12?0x130+(i-7)*4:i==13?0x150:i==14?0x154:0x16c;}
static bool Quiet(){
 for(unsigned i=0;i<4;i++)if(*Words(0xffb20040+i*0x800))return false;
 for(unsigned i=0;i<32;i++)if(*Words(0xffb20240+i*4)&255)return false;
 return true;
}
static void Seal(const uint32_t* d){
 for(unsigned i=0;i<64;i++)if(Words(L::Header)[i]!=d[i])scalar_fault_park();
 for(unsigned i=0;i<16;i++)if(*Words(0xffb20000+Source(i))!=d[16+i])scalar_fault_park();
 if(!Quiet())scalar_fault_park();
}
static void Drain(){
 *Words(0xffe000fc)=0;(void)*Words(0xffe000fc);Fence();Emit(0xa2101816);Emit(0x4500017e);
 for(unsigned i=0;i<1000000;i++)if(*Words(0xffe000fc)){Fence();return;}
 scalar_fault_park(); // UNKNOWN,no reset/retry/publication
}
static void Setup(){
 Emit(0xb2000000);Drain();for(unsigned i=1;i<68;i++)Emit(0xb2000000|(i<<16));
 Emit(0x3700000f);Emit(0x8a000002);Drain();
 const unsigned index[]={0,1,2,3,6,7,220},value[]={0x4231,0x20220000,0,0,0,0,0};
 for(unsigned i=0;i<7;i++){*Words(0xffef0000+index[i]*4)=value[i];if(*Words(0xffef0000+index[i]*4)!=value[i])scalar_fault_park();}
 Fence();
}
static void AddProduct(){
 Emit(0x84001940);Emit(0x850a2450);Emit(0x850a5262);Emit(0x850a5672);
 Emit(0x850a2772);Emit(0x850a4662);Emit(0x850a7670);Emit(0x850a3730);Emit(0x7c000520);Drain();
} // exact half product plus compensated high2/low3,not partially-fused MAD accumulation
static void Constant(float x){unsigned p=PioneerAttentionBits(x);Emit(0x71100000|(p>>16));Emit(0x711a0000|(p&65535));}
static void Read(unsigned which,unsigned first,unsigned previous,const uint32_t* d,uint32_t* tracked){
 if(which>1 || !previous || previous>32 || first>=d[4] || previous>d[4]-first)scalar_fault_park();
 Seal(d);unsigned bytes=previous*512;uint32_t expected[8];
 for(unsigned i=0;i<8;i++){
  if(*Words(0xffb20000+Counter(i))!=tracked[i])scalar_fault_park();
  unsigned delta=i==1?bytes/64:i==0 || i==2 || i==3 || i==6?1U:0U;
  expected[i]=tracked[i]+delta;if(i==6 && expected[6]!=expected[2])scalar_fault_park();
 }
 const uint32_t command[]={d[8+which]+first*512,0,d[12+which],L::Chunk,0,d[14],0,0,bytes,0};
 for(unsigned i=0;i<10;i++)Words(0xffb20000)[i]=command[i];
 for(unsigned i=0;i<10;i++)if(Words(0xffb20000)[i]!=command[i])scalar_fault_park();
 Fence();*Words(0xffb20040)=1;(void)*Words(0xffb20040);Fence();
 for(unsigned poll=0;poll<65536;poll++){
  bool done=!*Words(0xffb20040);for(unsigned i=0;i<8;i++)done &= *Words(0xffb20000+Counter(i))==expected[i];
  if(done){
   Seal(d);for(unsigned i=0;i<10;i++)if(Words(0xffb20000)[i]!=command[i])scalar_fault_park();
   for(unsigned i=0;i<8;i++)tracked[i]=expected[i];
   Words(L::Summary)[6]++;Words(L::Summary)[7]+=bytes/64;return;
  }
 }
 scalar_fault_park(); // UNKNOWN:never another prefix packet,KV write or reset
}
static unsigned Chunk(unsigned which,unsigned first,const uint32_t* d,uint32_t* tracked){
 unsigned count=d[7]-first;if(count>32)count=32;
 unsigned prior=first<d[4]?d[4]-first:0;if(prior>count)prior=count;
 if(prior)Read(which,first,prior,d,tracked);
 if(prior<count){
  if(prior+1!=count || first+prior!=d[4])scalar_fault_park();
  // Current token K/V are ACTUAL QDP results,kept PROVISIONAL in L1.
  // Never read stale current-position cache or commit canonical KV early.
  for(unsigned i=0;i<128;i++)Words(L::Chunk+prior*512)[i]=Words(which?L::NewV:L::NewK)[i];
 }
 for(unsigned i=0;i<count*128;i++){
  unsigned p=Words(L::Chunk)[i];if((p&0x7c00)==0x7c00 || (p&0x7c000000)==0x7c000000)scalar_fault_park();
 }
 return count;
}
static void Scores(unsigned first,unsigned count){
 auto* dst=Words(0xffbd8000);const auto* k=reinterpret_cast<const uint16_t*>(L::Chunk);
 Emit(0x702b0000);Emit(0x703b0000);Drain();
 for(unsigned lane=count;lane<32;lane++)dst[Lane(lane)]=0;
 Fence();
 for(unsigned dimension=0;dimension<256;dimension++){
  for(unsigned lane=0;lane<count;lane++)dst[Lane(lane)]=PioneerAttentionBits(PioneerAttentionHalf(k[lane*256+dimension]));
  (void)dst[Lane(count-1)];Fence();Emit(0x70030000);Constant(Floats(L::HalfQuery)[dimension]);AddProduct();
 }
 Emit(0x850a2300);Emit(0x72030008);Drain();
 for(unsigned lane=0;lane<count;lane++){
  float score=PioneerAttentionFloat(Words(0xffbd8200)[Lane(lane)])*.0625f;
  if((PioneerAttentionBits(score)&0x7f800000)==0x7f800000)scalar_fault_park();
  Floats(L::Scores)[first+lane]=score;
 }
}
static void Values(unsigned first,unsigned count){
 const auto* v=reinterpret_cast<const uint16_t*>(L::Chunk);auto* dst=Words(0xffbd8000);
 for(unsigned block=0;block<8;block++){
  // Keep all256 high/low lanes in NEW local scratch between chunks.
  // Prefix V is read ONCE per chunk,not once per32-dimensional block.
  for(unsigned lane=0;lane<32;lane++){
   dst[Lane(lane)]=Words(L::Accumulate)[block*64+lane];
   Words(0xffbd8100)[Lane(lane)]=Words(L::Accumulate)[block*64+32+lane];
  }
  (void)Words(0xffbd8100)[Lane(31)];Fence();Emit(0x70230000);Emit(0x70330004);Drain();
  for(unsigned t=0;t<count;t++){
   for(unsigned lane=0;lane<32;lane++)dst[Lane(lane)]=PioneerAttentionBits(PioneerAttentionHalf(v[t*256+block*32+lane]));
   (void)dst[Lane(31)];Fence();Emit(0x70030000);Constant(Floats(L::Scores)[first+t]);AddProduct();
  }
  for(unsigned part=0;part<2;part++){
   Emit(part?0x72330008:0x72230008);Drain();
   for(unsigned lane=0;lane<32;lane++)Words(L::Accumulate)[block*64+part*32+lane]=Words(0xffbd8200)[Lane(lane)];
  }
 }
}
static float Flush(float x){unsigned b=PioneerAttentionBits(x);return b&0x7f800000?x:PioneerAttentionFloat(b&0x80000000);}
static float Sigmoid(float x){
 float a=-PioneerAttentionFloat(PioneerAttentionBits(x)&0x7fffffff),e;
 if(a> -80)e=PioneerGraphExp(a);
 else if(a<=-104)e=0;
 else {float half=PioneerGraphExp(a*.5f);e=Flush(half*half);}
 return PioneerGraphDividePositive(x>=0?1.f:e,1.f+e);
} // preserve NORMAL exp(-80..-87);only genuine subnormals FTZ
}
extern "C" unsigned scalar_main(){
 using namespace DenseAttention;namespace P=PioneerQ38DensePrepareLayout;
 uint32_t d[64],tracked[8],before[8];for(unsigned i=0;i<64;i++)d[i]=Words(L::Header)[i];
 if(!L::Shape(d))return 0;
 Seal(d);for(unsigned i=0;i<64;i++)Words(L::Summary)[i]=0;
 for(unsigned i=0;i<8;i++){before[i]=tracked[i]=*Words(0xffb20000+Counter(i));if(i==6 && tracked[6]!=tracked[2])scalar_fault_park();}
 if(!PioneerQ38DensePrepare::Run(Floats(P::Qg),Floats(P::Key),Floats(P::Value),Floats(P::QWeight),Floats(P::KWeight),
  Floats(P::Rope),Floats(P::Query),Floats(P::Gate),reinterpret_cast<uint32_t*>(P::PackedK),reinterpret_cast<uint32_t*>(P::PackedV)))scalar_fault_park();
 unsigned head=d[5]-L::Group(d[5])*6;
 for(unsigned i=0;i<256;i++){
  unsigned q=Words(P::Query)[head*256+i];Words(L::Query)[i]=q;Words(L::Gate)[i]=Words(P::Gate)[head*256+i];
  Floats(L::HalfQuery)[i]=PioneerAttentionHalf(PioneerAttentionToHalf(PioneerAttentionFloat(q)));
 }
 for(unsigned i=0;i<128;i++){Words(L::NewK)[i]=Words(P::PackedK)[i];Words(L::NewV)[i]=Words(P::PackedV)[i];}
 Seal(d);Setup();
 for(unsigned first=0;first<d[7];first+=32)Scores(first,Chunk(0,first,d,tracked));
 float maximum=-3.4028234663852886e38f;
 for(unsigned t=0;t<d[7];t++)if(Floats(L::Scores)[t]>maximum)maximum=Floats(L::Scores)[t];
 float denominator=0,low=0;
 for(unsigned t=0;t<d[7];t++){
  float e=PioneerAttentionExp(Floats(L::Scores)[t]-maximum);Floats(L::Scores)[t]=e;PioneerAttentionAdd(e,denominator,low);
 }
 float inverse=PioneerAttentionReciprocal(denominator,low);
 if(!(inverse>0) || (PioneerAttentionBits(inverse)&0x7f800000)==0x7f800000)scalar_fault_park();
 for(unsigned t=0;t<d[7];t++)Floats(L::Scores)[t]=PioneerAttentionHalf(PioneerAttentionToHalf(Floats(L::Scores)[t]*inverse));
 for(unsigned i=0;i<512;i++)Words(L::Accumulate)[i]=0;
 for(unsigned first=0;first<d[7];first+=32)Values(first,Chunk(1,first,d,tracked));
 Drain();Seal(d);
 for(unsigned i=0;i<256;i++){
  unsigned block=i>>5,lane=i&31;float p=Floats(L::Accumulate)[block*64+lane]+Floats(L::Accumulate)[block*64+32+lane];
  float g=Flush(p*Sigmoid(Floats(L::Gate)[i]));
  if((PioneerAttentionBits(p)&0x7f800000)==0x7f800000 || (PioneerAttentionBits(g)&0x7f800000)==0x7f800000)scalar_fault_park();
  Floats(L::PreGate)[i]=p;Floats(L::Output)[i]=g;
 }
 Seal(d);auto* s=Words(L::Summary);
 if(s[6]!=d[46] || s[7]!=d[47])scalar_fault_park();
 for(unsigned i=0;i<8;i++){
  unsigned delta=i==1?d[47]:i==0 || i==2 || i==3 || i==6?d[46]:0U;
  if(tracked[i]!=*Words(0xffb20000+Counter(i)) || uint32_t(tracked[i]-before[i])!=delta)scalar_fault_park();
  s[8+i]=before[i];s[16+i]=tracked[i];
 }
 s[1]=256;s[2]=d[7];s[3]=d[5];s[4]=d[3];s[5]=d[4];
 for(unsigned i=0;i<16;i++)s[24+i]=d[16+i];
 for(unsigned i=0;i<8;i++)s[40+i]=d[32+i];
 s[48]=P::Magic;s[49]=1536;s[50]=2;s[51]=0;s[52]=1;s[53]=PioneerAttentionBits(maximum);
 s[54]=PioneerAttentionBits(denominator);s[55]=PioneerAttentionBits(inverse);s[56]=d[40];Fence();s[0]=L::Magic;Fence();
 return d[7]; // complete LOCAL selected head,NOT KV/token commit/native grant
}
