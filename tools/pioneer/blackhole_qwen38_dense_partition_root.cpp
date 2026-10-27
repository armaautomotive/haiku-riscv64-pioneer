/* SPDX-License-Identifier: MIT */
// NEW QDR1: actual96-worker card controller. Local prepared inputs are
// supplied by FOUR real QDP producers in the joined qualification. Native
// source/init/clock/host/joint-memory/Tensix/NoC ownership remains OWED.
#if defined(PIONEER_Q38_MIXED_RECURRENT_PROJECTION_ROOT_IMAGE)
#include "PioneerQ38MixedRecurrentProjectionRootImageRole.h"
#elif defined(PIONEER_Q38_MIXED_LIVE_BLOCK_ROOT_IMAGE)
#include "PioneerQ38MixedLiveBlockRootImageRole.h"
#elif defined(PIONEER_Q38_MIXED_BLOCK_ROOT_IMAGE)
#include "PioneerQ38MixedBlockRootImageRole.h"
#elif defined(PIONEER_Q38_MIXED_JOINED_ROOT_IMAGE)
#include "PioneerQ38MixedJoinedRootImageRole.h"
#elif defined(PIONEER_Q38_MIXED_PROJECTION_ROOT_IMAGE)
#include "PioneerQ38MixedProjectionRootImageRole.h"
#elif defined(PIONEER_Q38_MIXED_DENSE_ROOT_IMAGE)
#include "PioneerQ38MixedDenseRootImageRole.h"
#else
#include "PioneerQ38DensePartitionImageRole.h"
#endif
#define PIONEER_ATTENTION_FREESTANDING
#include "PioneerQ38DensePositiveSum.h"
#include "PioneerQ38DensePartitionRootLayout.h"
#if defined(PIONEER_Q38_MIXED_DENSE_ROOT_IMAGE) || defined(PIONEER_Q38_MIXED_PROJECTION_ROOT_IMAGE) || defined(PIONEER_Q38_MIXED_JOINED_ROOT_IMAGE) || defined(PIONEER_Q38_MIXED_BLOCK_ROOT_IMAGE) || defined(PIONEER_Q38_MIXED_LIVE_BLOCK_ROOT_IMAGE) || defined(PIONEER_Q38_MIXED_RECURRENT_PROJECTION_ROOT_IMAGE)
#if defined(PIONEER_Q38_MIXED_RECURRENT_PROJECTION_ROOT_IMAGE)
#include "PioneerQ38MixedRecurrentProjectionRootLayout.h"
#elif defined(PIONEER_Q38_MIXED_LIVE_BLOCK_ROOT_IMAGE)
#include "PioneerQ38MixedLiveBlockRootLayout.h"
#elif defined(PIONEER_Q38_MIXED_BLOCK_ROOT_IMAGE)
#include "PioneerQ38MixedBlockRootLayout.h"
#elif defined(PIONEER_Q38_MIXED_JOINED_ROOT_IMAGE)
#include "PioneerQ38MixedJoinedRootLayout.h"
#elif defined(PIONEER_Q38_MIXED_PROJECTION_ROOT_IMAGE)
#include "PioneerQ38MixedProjectionRootLayout.h"
#else
#include "PioneerQ38MixedDenseRootLayout.h"
#endif
extern "C" void q38_mixed_root_seal();
#endif
extern "C" void scalar_fault_park() __attribute__((noreturn));
namespace DensePartitionRoot {
#if defined(PIONEER_Q38_MIXED_DENSE_ROOT_IMAGE) || defined(PIONEER_Q38_MIXED_PROJECTION_ROOT_IMAGE) || defined(PIONEER_Q38_MIXED_JOINED_ROOT_IMAGE) || defined(PIONEER_Q38_MIXED_BLOCK_ROOT_IMAGE) || defined(PIONEER_Q38_MIXED_LIVE_BLOCK_ROOT_IMAGE) || defined(PIONEER_Q38_MIXED_RECURRENT_PROJECTION_ROOT_IMAGE)
#if defined(PIONEER_Q38_MIXED_RECURRENT_PROJECTION_ROOT_IMAGE)
namespace L=PioneerQ38MixedRecurrentProjectionRootLayout;
#elif defined(PIONEER_Q38_MIXED_LIVE_BLOCK_ROOT_IMAGE)
namespace L=PioneerQ38MixedLiveBlockRootLayout;
#elif defined(PIONEER_Q38_MIXED_BLOCK_ROOT_IMAGE)
namespace L=PioneerQ38MixedBlockRootLayout;
#elif defined(PIONEER_Q38_MIXED_JOINED_ROOT_IMAGE)
namespace L=PioneerQ38MixedJoinedRootLayout;
#elif defined(PIONEER_Q38_MIXED_PROJECTION_ROOT_IMAGE)
namespace L=PioneerQ38MixedProjectionRootLayout;
#else
namespace L=PioneerQ38MixedDenseRootLayout;
#endif
namespace M=PioneerQ38MixedWorkerLayout;
#else
namespace L=PioneerQ38DensePartitionRootLayout;
#endif
namespace W=PioneerQ38DensePartitionLayout;
static volatile uint32_t* Words(unsigned a){return reinterpret_cast<volatile uint32_t*>(a);}
static float* Floats(unsigned a){return reinterpret_cast<float*>(a);}
static void Fence(){asm volatile("fence" ::: "memory");}
static unsigned Counter(unsigned i){return i==0?0x208:i==1?0x20c:(i==2 || i==6)?0x210:i==3?0x214:i==4?0x204:i==5?0x220:0x228;}
static unsigned Source(unsigned i){return !i?0x100:i<=6?0x118+(i-1)*4:i<=12?0x130+(i-7)*4:i==13?0x150:i==14?0x154:0x16c;}
static bool Quiet(){
 for(unsigned i=0;i<4;i++)if(*Words(0xffb20040+i*0x800))return false;
 for(unsigned i=0;i<32;i++)if(*Words(0xffb20240+i*4)&255)return false;
 return true;
}
static const uint32_t* Frame(unsigned w){return reinterpret_cast<const uint32_t*>(L::Shadow+w*256);}
static void Seal(const uint32_t* d){
#if defined(PIONEER_Q38_MIXED_DENSE_ROOT_IMAGE) || defined(PIONEER_Q38_MIXED_PROJECTION_ROOT_IMAGE) || defined(PIONEER_Q38_MIXED_JOINED_ROOT_IMAGE) || defined(PIONEER_Q38_MIXED_BLOCK_ROOT_IMAGE) || defined(PIONEER_Q38_MIXED_LIVE_BLOCK_ROOT_IMAGE) || defined(PIONEER_Q38_MIXED_RECURRENT_PROJECTION_ROOT_IMAGE)
 q38_mixed_root_seal();
#endif
 for(unsigned i=0;i<64;i++)if(Words(L::Header)[i]!=d[i])scalar_fault_park();
 for(unsigned i=0;i<16;i++)if(*Words(0xffb20000+Source(i))!=d[16+i])scalar_fault_park();
 if(!Quiet())scalar_fault_park();
}
static void FrameSeal(unsigned w){
#if defined(PIONEER_Q38_MIXED_JOINED_ROOT_IMAGE) || defined(PIONEER_Q38_MIXED_BLOCK_ROOT_IMAGE) || defined(PIONEER_Q38_MIXED_LIVE_BLOCK_ROOT_IMAGE)
 if(Words(L::Header)[0]!=PioneerQ38MixedProjectionRootLayout::Magic){
  if(w<96)for(unsigned i=0;i<64;i++)if(Words(L::Frames+w*256)[i]!=Frame(w)[i])scalar_fault_park();
  return;
 }
#endif
#if defined(PIONEER_Q38_MIXED_PROJECTION_ROOT_IMAGE) || defined(PIONEER_Q38_MIXED_JOINED_ROOT_IMAGE) || defined(PIONEER_Q38_MIXED_BLOCK_ROOT_IMAGE) || defined(PIONEER_Q38_MIXED_LIVE_BLOCK_ROOT_IMAGE) || defined(PIONEER_Q38_MIXED_RECURRENT_PROJECTION_ROOT_IMAGE)
 if(w>=119)scalar_fault_park();
 for(unsigned i=0;i<8;i++)if(Words(L::Parts+w*32)[i]!=Words(L::PartsShadow+w*32)[i])scalar_fault_park();
 if(Words(L::Rows)[w]!=Words(L::RowsShadow)[w])scalar_fault_park();
 for(unsigned i=0;i<16;i++)if(Words(L::MatrixHeader)[i]!=Words(L::HeaderShadow)[i])scalar_fault_park();
 for(unsigned i=0;i<8;i++)if(Words(L::Banks)[i]!=Words(L::BankShadow)[i])scalar_fault_park();
#else
 for(unsigned i=0;i<64;i++)if(Words(L::Frames+w*256)[i]!=Frame(w)[i])scalar_fault_park();
#endif
}
static void Copy(unsigned to,unsigned from,unsigned words){for(unsigned i=0;i<words;i++)Words(to)[i]=Words(from)[i];}
static void Transfer(const uint32_t* d,uint32_t* tracked,unsigned w,unsigned remote,unsigned bytes,bool write){
 unsigned local=L::Scratch+(remote&31);
 if(w>=
#if defined(PIONEER_Q38_MIXED_DENSE_ROOT_IMAGE) || defined(PIONEER_Q38_MIXED_PROJECTION_ROOT_IMAGE) || defined(PIONEER_Q38_MIXED_JOINED_ROOT_IMAGE) || defined(PIONEER_Q38_MIXED_BLOCK_ROOT_IMAGE) || defined(PIONEER_Q38_MIXED_LIVE_BLOCK_ROOT_IMAGE) || defined(PIONEER_Q38_MIXED_RECURRENT_PROJECTION_ROOT_IMAGE)
 119
#else
 96
#endif
 || !bytes || bytes>16384-(remote&31) || ((bytes|remote)&3))scalar_fault_park();
 Seal(d);
#if defined(PIONEER_Q38_MIXED_DENSE_ROOT_IMAGE) || defined(PIONEER_Q38_MIXED_PROJECTION_ROOT_IMAGE) || defined(PIONEER_Q38_MIXED_JOINED_ROOT_IMAGE) || defined(PIONEER_Q38_MIXED_BLOCK_ROOT_IMAGE) || defined(PIONEER_Q38_MIXED_LIVE_BLOCK_ROOT_IMAGE) || defined(PIONEER_Q38_MIXED_RECURRENT_PROJECTION_ROOT_IMAGE)
#if defined(PIONEER_Q38_MIXED_PROJECTION_ROOT_IMAGE) || defined(PIONEER_Q38_MIXED_JOINED_ROOT_IMAGE) || defined(PIONEER_Q38_MIXED_BLOCK_ROOT_IMAGE) || defined(PIONEER_Q38_MIXED_LIVE_BLOCK_ROOT_IMAGE) || defined(PIONEER_Q38_MIXED_RECURRENT_PROJECTION_ROOT_IMAGE)
 FrameSeal(w);
#else
 if(w<96)FrameSeal(w);
#endif
 unsigned route=M::Peer(w+1);
#else
 FrameSeal(w);unsigned route=Frame(w)[14];
#endif
 uint32_t expected[8];
 for(unsigned i=0;i<8;i++){
  if(*Words(0xffb20000+Counter(i))!=tracked[i])scalar_fault_park();
  unsigned delta=i==(write?5U:1U)?(bytes+63)>>6:i==2 || i==6 || i==(write?4U:0U) || i==(write?7U:3U)?1U:0U;
  expected[i]=tracked[i]+delta;if(i==6 && expected[6]!=expected[2])scalar_fault_park();
 }
#if defined(PIONEER_Q38_MIXED_JOINED_ROOT_IMAGE) || defined(PIONEER_Q38_MIXED_BLOCK_ROOT_IMAGE) || defined(PIONEER_Q38_MIXED_LIVE_BLOCK_ROOT_IMAGE)
 unsigned root=d[0]==PioneerQ38MixedProjectionRootLayout::Magic?d[9]:d[5];
 const uint32_t command[]={write?local:remote,0,write?root:route,write?remote:local,0,write?route:root,0,write?0x212U:0,bytes,0};
#elif defined(PIONEER_Q38_MIXED_PROJECTION_ROOT_IMAGE) || defined(PIONEER_Q38_MIXED_RECURRENT_PROJECTION_ROOT_IMAGE)
 const uint32_t command[]={write?local:remote,0,write?d[9]:route,write?remote:local,0,write?route:d[9],0,write?0x212U:0,bytes,0};
#else
 const uint32_t command[]={write?local:remote,0,write?d[5]:route,write?remote:local,0,write?route:d[5],0,write?0x212U:0,bytes,0};
#endif
 for(unsigned i=0;i<10;i++)Words(0xffb20000)[i]=command[i];
 for(unsigned i=0;i<10;i++)if(Words(0xffb20000)[i]!=command[i])scalar_fault_park();
 Fence();*Words(0xffb20040)=1;(void)*Words(0xffb20040);Fence();
 for(unsigned poll=0;poll<65536;poll++){
  bool done=!*Words(0xffb20040);for(unsigned i=0;i<8;i++)done &= *Words(0xffb20000+Counter(i))==expected[i];
  if(done){
   Seal(d);
#if defined(PIONEER_Q38_MIXED_DENSE_ROOT_IMAGE) || defined(PIONEER_Q38_MIXED_PROJECTION_ROOT_IMAGE) || defined(PIONEER_Q38_MIXED_JOINED_ROOT_IMAGE) || defined(PIONEER_Q38_MIXED_BLOCK_ROOT_IMAGE) || defined(PIONEER_Q38_MIXED_LIVE_BLOCK_ROOT_IMAGE) || defined(PIONEER_Q38_MIXED_RECURRENT_PROJECTION_ROOT_IMAGE)
#if defined(PIONEER_Q38_MIXED_PROJECTION_ROOT_IMAGE) || defined(PIONEER_Q38_MIXED_JOINED_ROOT_IMAGE) || defined(PIONEER_Q38_MIXED_BLOCK_ROOT_IMAGE) || defined(PIONEER_Q38_MIXED_LIVE_BLOCK_ROOT_IMAGE) || defined(PIONEER_Q38_MIXED_RECURRENT_PROJECTION_ROOT_IMAGE)
   FrameSeal(w);
#else
   if(w<96)FrameSeal(w);
#endif
#else
   FrameSeal(w);
#endif
   for(unsigned i=0;i<10;i++)if(Words(0xffb20000)[i]!=command[i])scalar_fault_park();
   for(unsigned i=0;i<8;i++)tracked[i]=expected[i];
   Words(L::Summary)[6]++;Words(L::Summary)[7]+=(bytes+63)>>6;return;
  }
 }
 scalar_fault_park(); // UNKNOWN: no later packet/retry/reset/cleanup grant
}
static uint32_t Register(const uint32_t* d,uint32_t* tracked,unsigned w,unsigned at){Transfer(d,tracked,w,at,4,false);return *Words(L::Scratch+(at&31));}
#if defined(PIONEER_Q38_MIXED_DENSE_ROOT_IMAGE) || defined(PIONEER_Q38_MIXED_PROJECTION_ROOT_IMAGE) || defined(PIONEER_Q38_MIXED_JOINED_ROOT_IMAGE) || defined(PIONEER_Q38_MIXED_BLOCK_ROOT_IMAGE) || defined(PIONEER_Q38_MIXED_LIVE_BLOCK_ROOT_IMAGE) || defined(PIONEER_Q38_MIXED_RECURRENT_PROJECTION_ROOT_IMAGE)
#include "blackhole_qwen38_mixed_dense_root_control.inc"
#else
static void Job(unsigned w,unsigned phase){
 const auto* f=Frame(w);const uint32_t j[]={W::Magic,phase,phase,f[40],f[15],f[3],f[4],f[5],f[48],f[49],0,0,0,0,0,0};
 for(unsigned i=0;i<16;i++)Words(L::Scratch)[i]=j[i];
}
static void Wait(const uint32_t* d,uint32_t* tracked,unsigned w,unsigned phase){
 for(unsigned poll=0;poll<65536;poll++){
  // Read reset FIRST. Never sample a held peer's PC as execution proof.
  if(Register(d,tracked,w,0xffb121b0)!=0x46800)scalar_fault_park();
  Transfer(d,tracked,w,W::Status,64,false);uint32_t status[16];for(unsigned i=0;i<16;i++)status[i]=Words(L::Scratch)[i];
  unsigned previous=phase?phase-1:0;
  if(status[3]!=W::Magic || status[2] || status[0]<previous || status[0]>phase || status[1]<previous || status[1]>phase || status[4]>2 || (phase<4 && status[4]==2))scalar_fault_park();
  for(unsigned i=6;i<16;i++)if(status[i])scalar_fault_park();
  unsigned pc=Register(d,tracked,w,0xffb13140);
  if(pc==0x40058 || pc<W::Code || pc>=W::Code+10804 || (pc&3))scalar_fault_park();
  if(status[0]!=phase || status[1]!=phase)continue;
  if(phase==4){if(status[4]!=2 || status[5]!=3)scalar_fault_park();if(pc!=0x40038)continue;}
  else {if(status[4] || status[5]!=phase)scalar_fault_park();if(pc!=0x40064 && pc!=0x40068)continue;}
  return;
 }
 scalar_fault_park();
}
static void Snapshot(const uint32_t* d,uint32_t* tracked,unsigned w,unsigned phase){
 const auto* f=Frame(w);Wait(d,tracked,w,phase);
 Transfer(d,tracked,w,W::Header,256,false);for(unsigned i=0;i<64;i++)if(Words(L::Scratch)[i]!=f[i])scalar_fault_park();
 for(unsigned i=0;i<16;i++)if(Register(d,tracked,w,0xffb20000+Source(i))!=f[16+i])scalar_fault_park();
 Transfer(d,tracked,w,W::Job,64,false);
 if(!phase){for(unsigned i=0;i<16;i++)if(Words(L::Scratch)[i])scalar_fault_park();}
 else {uint32_t actual[16];for(unsigned i=0;i<16;i++)actual[i]=Words(L::Scratch)[i];if(!W::JobShape(actual,f,phase,phase))scalar_fault_park();}
 if(Register(d,tracked,w,W::Doorbell)!=phase)scalar_fault_park();
 if(phase && phase<4){
  Transfer(d,tracked,w,W::Summary,256,false);Copy(L::Proofs+w*256,L::Scratch,64);const auto* p=reinterpret_cast<const uint32_t*>(L::Proofs+w*256);
  unsigned reads=phase==2?0:f[46]>>1,flits=phase==2?0:f[47]>>1;
  if(p[0]!=W::Magic || p[1]!=phase || p[2]!=phase || p[3]!=f[48] || p[4]!=f[49] || p[5]!=6 || p[6]!=reads || p[7]!=flits)scalar_fault_park();
  for(unsigned i=0;i<8;i++){
   unsigned delta=i==1?flits:i==0 || i==2 || i==3 || i==6?reads:0U;
   if(p[8+i]!=Words(L::Counters)[w*8+i] || uint32_t(p[16+i]-p[8+i])!=delta)scalar_fault_park();
  }
  for(unsigned i=0;i<16;i++)if(p[24+i]!=f[16+i])scalar_fault_park();
  for(unsigned i=0;i<8;i++)if(p[40+i]!=f[32+i])scalar_fault_park();
  const unsigned expected[]={f[40],f[15],f[3],f[4],f[5],f[6],f[46],f[47]};
  for(unsigned i=0;i<8;i++)if(p[48+i]!=expected[i])scalar_fault_park();
  for(unsigned i=56;i<64;i++)if(p[i])scalar_fault_park();
 }
 for(unsigned i=0;i<8;i++){
  unsigned actual=Register(d,tracked,w,0xffb20000+Counter(i));
  if(phase && actual!=(phase==4?Words(L::Counters)[w*8+i]:Words(L::Proofs+w*256)[16+i]))scalar_fault_park();
  Words(L::Counters)[w*8+i]=actual;
  if(i==6 && actual!=Words(L::Counters)[w*8+2])scalar_fault_park();
 }
 if(phase==2 || phase==3){
  Transfer(d,tracked,w,W::Parameters,64,false);
  for(unsigned i=0;i<16;i++)if(Words(L::Scratch)[i]!=Words(L::Parameters)[f[5]*16+i])scalar_fault_park();
 }
 if(phase==1 || phase==2){
  Transfer(d,tracked,w,W::Local,256,false);Copy(L::Reports+w*256,L::Scratch,64);
  const auto* r=reinterpret_cast<const uint32_t*>(L::Reports+w*256);
  if(r[0]!=phase || r[1]!=f[49] || r[2]!=f[48] || r[3]!=f[15])scalar_fault_park();
  for(unsigned h=0;h<6;h++){
   if((r[4+h]&0x7fffffff)>=0x7f800000 || (!f[49] && r[4+h]!=0xff7fffff))scalar_fault_park();
   if(phase==1)Words(L::Maxima)[w*6+h]=r[4+h];
   else if(r[4+h]!=Words(L::Maxima)[w*6+h] || r[19+h*4]!=1 || !PioneerQ38DensePositiveSum::Record(r[16+h*4],r[17+h*4],int(r[18+h*4])))scalar_fault_park();
  }
  for(unsigned i=10;i<64;i++)if((phase==1 || i<16 || i>=40) && r[i])scalar_fault_park();
 }
}
#endif
static void Capture(const uint32_t* d){
 for(unsigned group=0;group<4;group++)for(unsigned ordinal=0;ordinal<24;ordinal++){
  unsigned w=group*24+ordinal;Copy(L::Shadow+w*256,L::Frames+w*256,64);const auto* f=Frame(w);
  if(!L::Child(d,f,group,ordinal))scalar_fault_park();
  const auto* first=Frame(group*24);for(unsigned i=8;i<14;i++)if(f[i]!=first[i])scalar_fault_park();
  if(!ordinal)for(unsigned old=0;old<group;old++)for(unsigned a=0;a<2;a++)for(unsigned b=0;b<2;b++){
   const auto* previous=Frame(old*24);
   if(f[10+a]==previous[10+b] && f[8+a]<previous[8+b]+4194304 && previous[8+b]<f[8+a]+4194304)scalar_fault_park();
  }
 }
 for(unsigned i=0;i<6144;i++){
  unsigned q=Words(L::Input)[i],g=Words(L::Gate)[i];
  if((q&0x7fffffff)>0x477fe000 || (g&0x7fffffff)>0x477fe000)scalar_fault_park();
  Words(L::InputShadow)[i]=q;Words(L::GateShadow)[i]=g;
 }
 for(unsigned i=6144;i<7168;i++){
  unsigned p=Words(L::Input)[i];if((p&0x7c00)==0x7c00 || (p&0x7c000000)==0x7c000000)scalar_fault_park();
  Words(L::InputShadow)[i]=p;
 }
}
static void Inputs(const uint32_t* d,uint32_t* tracked){
 for(unsigned group=0;group<4;group++){
  for(unsigned i=0;i<1536;i++)Words(L::Scratch)[i]=PioneerAttentionBits(PioneerAttentionHalf(PioneerAttentionToHalf(PioneerAttentionFloat(Words(L::InputShadow)[group*1536+i]))));
  for(unsigned ordinal=0;ordinal<24;ordinal++)Transfer(d,tracked,group*24+ordinal,W::HalfQuery,6144,true);
  Copy(L::Scratch,L::InputShadow+24576+group*512,128);
  for(unsigned ordinal=0;ordinal<24;ordinal++)Transfer(d,tracked,group*24+ordinal,W::NewK,512,true);
  Copy(L::Scratch,L::InputShadow+26624+group*512,128);
  for(unsigned ordinal=0;ordinal<24;ordinal++)Transfer(d,tracked,group*24+ordinal,W::NewV,512,true);
 }
}
static void Launch(const uint32_t* d,uint32_t* tracked,unsigned phase){
 if(phase==2 || phase==3)for(unsigned w=0;w<96;w++){Copy(L::Scratch,L::Parameters+Frame(w)[5]*64,16);Transfer(d,tracked,w,W::Parameters,64,true);}
#if defined(PIONEER_Q38_MIXED_DENSE_ROOT_IMAGE) || defined(PIONEER_Q38_MIXED_PROJECTION_ROOT_IMAGE) || defined(PIONEER_Q38_MIXED_JOINED_ROOT_IMAGE) || defined(PIONEER_Q38_MIXED_BLOCK_ROOT_IMAGE) || defined(PIONEER_Q38_MIXED_LIVE_BLOCK_ROOT_IMAGE) || defined(PIONEER_Q38_MIXED_RECURRENT_PROJECTION_ROOT_IMAGE)
 for(unsigned w=0;w<96;w++){Job(w,phase);Transfer(d,tracked,w,M::Job,256,true);}
 for(unsigned w=0;w<96;w++){*Words(L::Scratch)=Words(L::Nonces)[w]+1;Transfer(d,tracked,w,M::Doorbell,4,true);Words(L::Nonces)[w]++;}
#else
 for(unsigned w=0;w<96;w++){Job(w,phase);Transfer(d,tracked,w,W::Job,64,true);}
 for(unsigned w=0;w<96;w++){*Words(L::Scratch)=phase;Transfer(d,tracked,w,W::Doorbell,4,true);}
#endif
 for(unsigned w=0;w<96;w++)Snapshot(d,tracked,w,phase);
}
static void Maximum(){
 for(unsigned group=0;group<4;group++){
  for(unsigned i=0;i<16;i++)Words(L::Parameters)[group*16+i]=0;
  for(unsigned h=0;h<6;h++){
   float maximum=-3.4028234663852886e38f;
   for(unsigned ordinal=0;ordinal<24;ordinal++){float local=PioneerAttentionFloat(Words(L::Maxima)[(group*24+ordinal)*6+h]);if(local>maximum)maximum=local;}
   if(PioneerAttentionBits(maximum)==0xff7fffff)scalar_fault_park();
   Words(L::Parameters)[group*16+h]=PioneerAttentionBits(maximum);
  }
 }
}
static void Inverse(){
 for(unsigned group=0;group<4;group++)for(unsigned h=0;h<6;h++){
  PioneerQ38DensePositiveSum sum;
  for(unsigned ordinal=0;ordinal<24;ordinal++){
   unsigned at=L::Reports+(group*24+ordinal)*256+(16+h*4)*4;
   if(!sum.Add(Words(at)[0],Words(at)[1],int(Words(at)[2])))scalar_fault_park();
  }
  float inverse;if(!sum.Inverse(inverse))scalar_fault_park();Words(L::Parameters)[group*16+8+h]=PioneerAttentionBits(inverse);
 }
}
static float Flush(float x){unsigned b=PioneerAttentionBits(x);return b&0x7f800000?x:PioneerAttentionFloat(b&0x80000000);}
#ifdef PIONEER_Q38_SIGMOID_PRECISION_IMAGE
#if !defined(PIONEER_Q38_MIXED_LIVE_BLOCK_ROOT_IMAGE) || PIONEER_Q38_MIXED_LIVE_BLOCK_ROOT_IMAGE != 1
#error New sigmoid is selected only by the separately built LOCAL live-root candidate
#endif
#include "PioneerQ38SigmoidPrecision.h"
#endif
static float Sigmoid(float x){
#ifdef PIONEER_Q38_SIGMOID_PRECISION_IMAGE
 float result;if(!PioneerQ38SigmoidPrecision(x,result))scalar_fault_park();return result;
#else
 float a=-PioneerAttentionFloat(PioneerAttentionBits(x)&0x7fffffff),e;
 if(a> -80)e=PioneerGraphExp(a);else if(a<=-104)e=0;else {float half=PioneerGraphExp(a*.5f);e=Flush(half*half);}
 return PioneerGraphDividePositive(x>=0?1.f:e,1.f+e);
#endif
}
static void Merge(const uint32_t* d,uint32_t* tracked){
 for(unsigned i=0;i<12288;i++)Words(L::Accumulate)[i]=0;
 for(unsigned w=0;w<96;w++){
  Transfer(d,tracked,w,W::Partial,12288,false);unsigned group=Frame(w)[5];
  for(unsigned i=0;i<1536;i++){
   float a=PioneerAttentionFloat(Words(L::Scratch)[i*2]),b=PioneerAttentionFloat(Words(L::Scratch)[i*2+1]);
   if((PioneerAttentionBits(a)&0x7f800000)==0x7f800000 || (PioneerAttentionBits(b)&0x7f800000)==0x7f800000)scalar_fault_park();
   auto* out=Floats(L::Accumulate)+(group*1536+i)*2;PioneerAttentionAdd(a,out[0],out[1]);PioneerAttentionAdd(b,out[0],out[1]);
  }
 }
 Seal(d);
 for(unsigned i=0;i<6144;i++){
  float p=Floats(L::Accumulate)[i*2]+Floats(L::Accumulate)[i*2+1];
  float value=Flush(p*Sigmoid(PioneerAttentionFloat(Words(L::GateShadow)[i])));
  if((PioneerAttentionBits(p)&0x7f800000)==0x7f800000 || (PioneerAttentionBits(value)&0x7f800000)==0x7f800000)scalar_fault_park();
  Words(L::Output)[i]=PioneerAttentionBits(value);
 }
}
}
extern "C" unsigned scalar_main(){
 using namespace DensePartitionRoot;(void)&PioneerQ38LearnedRms;
 uint32_t d[64],tracked[8],before[8];for(unsigned i=0;i<64;i++)d[i]=Words(L::Header)[i];
 if(!L::Shape(d))scalar_fault_park();
 Seal(d);Capture(d);
 for(unsigned i=0;i<64;i++)Words(L::Summary)[i]=0;
 for(unsigned i=0;i<8;i++){before[i]=tracked[i]=*Words(0xffb20000+Counter(i));if(i==6 && tracked[6]!=tracked[2])scalar_fault_park();}
#if defined(PIONEER_Q38_MIXED_DENSE_ROOT_IMAGE) || defined(PIONEER_Q38_MIXED_PROJECTION_ROOT_IMAGE) || defined(PIONEER_Q38_MIXED_JOINED_ROOT_IMAGE) || defined(PIONEER_Q38_MIXED_BLOCK_ROOT_IMAGE) || defined(PIONEER_Q38_MIXED_LIVE_BLOCK_ROOT_IMAGE) || defined(PIONEER_Q38_MIXED_RECURRENT_PROJECTION_ROOT_IMAGE)
 for(unsigned w=0;w<119;w++)Admit(d,tracked,w);
 // NO worker reset/boot/STOP. Install only the next attention DATA frame
 // after ALL retained source/job/counter/physical admission checks finish.
 for(unsigned w=0;w<96;w++){Copy(L::Scratch,L::Shadow+w*256,64);Transfer(d,tracked,w,W::Header,256,true);}
#else
 for(unsigned w=0;w<96;w++)Snapshot(d,tracked,w,0);
#endif
 Inputs(d,tracked);Launch(d,tracked,1);Maximum();Launch(d,tracked,2);Inverse();Launch(d,tracked,3);Merge(d,tracked);
#if defined(PIONEER_Q38_MIXED_DENSE_ROOT_IMAGE) || defined(PIONEER_Q38_MIXED_PROJECTION_ROOT_IMAGE) || defined(PIONEER_Q38_MIXED_JOINED_ROOT_IMAGE) || defined(PIONEER_Q38_MIXED_BLOCK_ROOT_IMAGE) || defined(PIONEER_Q38_MIXED_LIVE_BLOCK_ROOT_IMAGE) || defined(PIONEER_Q38_MIXED_RECURRENT_PROJECTION_ROOT_IMAGE)
 for(unsigned w=0;w<119;w++)Final(d,tracked,w);
#else
 Launch(d,tracked,4);
#endif
 Seal(d);for(unsigned w=0;w<96;w++)FrameSeal(w);
 for(unsigned i=0;i<7168;i++)if(Words(L::Input)[i]!=Words(L::InputShadow)[i])scalar_fault_park();
 for(unsigned i=0;i<6144;i++)if(Words(L::Gate)[i]!=Words(L::GateShadow)[i])scalar_fault_park();
 auto* s=Words(L::Summary);
 for(unsigned i=0;i<8;i++){if(*Words(0xffb20000+Counter(i))!=tracked[i])scalar_fault_park();s[8+i]=before[i];s[16+i]=tracked[i];}
 for(unsigned i=0;i<16;i++)s[24+i]=d[16+i];
 for(unsigned i=0;i<8;i++)s[40+i]=d[32+i];
 s[1]=96;s[2]=
#if defined(PIONEER_Q38_MIXED_DENSE_ROOT_IMAGE) || defined(PIONEER_Q38_MIXED_PROJECTION_ROOT_IMAGE) || defined(PIONEER_Q38_MIXED_JOINED_ROOT_IMAGE) || defined(PIONEER_Q38_MIXED_BLOCK_ROOT_IMAGE) || defined(PIONEER_Q38_MIXED_LIVE_BLOCK_ROOT_IMAGE) || defined(PIONEER_Q38_MIXED_RECURRENT_PROJECTION_ROOT_IMAGE)
 3;
#else
 4;
#endif
 s[3]=6144;s[4]=d[3];s[5]=d[4];s[48]=d[6];s[49]=3;s[50]=96;s[51]=
#if defined(PIONEER_Q38_MIXED_DENSE_ROOT_IMAGE) || defined(PIONEER_Q38_MIXED_PROJECTION_ROOT_IMAGE) || defined(PIONEER_Q38_MIXED_JOINED_ROOT_IMAGE) || defined(PIONEER_Q38_MIXED_BLOCK_ROOT_IMAGE) || defined(PIONEER_Q38_MIXED_LIVE_BLOCK_ROOT_IMAGE) || defined(PIONEER_Q38_MIXED_RECURRENT_PROJECTION_ROOT_IMAGE)
 119;s[52]=d[48];
#else
 4;
#endif
 Fence();s[0]=L::Magic;Fence();return 6144; // LOCAL result only,NOT canonical KV/token commit
}
