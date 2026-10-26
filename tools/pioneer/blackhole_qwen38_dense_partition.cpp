/* SPDX-License-Identifier: MIT */
// NEW96-worker/4KV-group partition kernel. SIX queries reuse each KV
// chunk; global reductions are SEPARATE card-controller dependencies.
// No CPU numeric reference, host output endpoint or canonical KV write.
#include "PioneerQ38DensePartitionImageRole.h"
#define PIONEER_ATTENTION_FREESTANDING
#include "PioneerQ38LearnedRms.h"
#include "PioneerQ38DensePartitionLayout.h"
extern "C" void scalar_fault_park() __attribute__((noreturn));
extern "C" unsigned partition_wait(unsigned);
namespace DensePartition {
namespace L=PioneerQ38DensePartitionLayout;
static void JobSeal(const uint32_t*,unsigned);
static void ParametersSeal(const uint32_t*);
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
static void Read(unsigned which,unsigned first,unsigned previous,const uint32_t* d,uint32_t* tracked,const uint32_t* job,const uint32_t* params){
 if(which>1 || !previous || previous>32 || first>=d[4] || previous>d[4]-first)scalar_fault_park();
 JobSeal(job,job[2]);if(params)ParametersSeal(params);
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
   Seal(d);JobSeal(job,job[2]);if(params)ParametersSeal(params);
   for(unsigned i=0;i<10;i++)if(Words(0xffb20000)[i]!=command[i])scalar_fault_park();
   for(unsigned i=0;i<8;i++)tracked[i]=expected[i];
   Words(L::Summary)[6]++;Words(L::Summary)[7]+=bytes/64;return;
  }
 }
 scalar_fault_park(); // UNKNOWN:never another prefix packet,KV write or reset
}
static unsigned Chunk(unsigned which,unsigned first,const uint32_t* d,uint32_t* tracked,const uint32_t* job,const uint32_t* params){
 unsigned count=d[49]-first;if(count>32)count=32;
 unsigned global=d[48]+first,prior=global<d[4]?d[4]-global:0;if(prior>count)prior=count;
 if(prior)Read(which,global,prior,d,tracked,job,params);
 if(prior<count){
  if(prior+1!=count || global+prior!=d[4])scalar_fault_park();
  for(unsigned i=0;i<128;i++)Words(L::Chunk+prior*512)[i]=Words(which?L::PrivateV:L::PrivateK)[i];
 }
 for(unsigned i=0;i<count*128;i++){
  unsigned p=Words(L::Chunk)[i];if((p&0x7c00)==0x7c00 || (p&0x7c000000)==0x7c000000)scalar_fault_park();
 }
 return count;
}
static void Scores(unsigned first,unsigned count,unsigned head){
 auto* dst=Words(0xffbd8000);const auto* k=reinterpret_cast<const uint16_t*>(L::Chunk);
 Emit(0x702b0000);Emit(0x703b0000);Drain();
 for(unsigned lane=count;lane<32;lane++)dst[Lane(lane)]=0;
 Fence();
 for(unsigned dimension=0;dimension<256;dimension++){
  for(unsigned lane=0;lane<count;lane++)dst[Lane(lane)]=PioneerAttentionBits(PioneerAttentionHalf(k[lane*256+dimension]));
  (void)dst[Lane(count-1)];Fence();Emit(0x70030000);Constant(Floats(L::PrivateQuery)[head*256+dimension]);AddProduct();
 }
 Emit(0x850a2300);Emit(0x72030008);Drain();
 for(unsigned lane=0;lane<count;lane++){
  float score=PioneerAttentionFloat(Words(0xffbd8200)[Lane(lane)])*.0625f;
  if((PioneerAttentionBits(score)&0x7f800000)==0x7f800000)scalar_fault_park();
  Floats(L::Scores)[head*L::Stride+first+lane]=score;
 }
}
static void Values(unsigned first,unsigned count,unsigned head){
 const auto* v=reinterpret_cast<const uint16_t*>(L::Chunk);auto* dst=Words(0xffbd8000);
 for(unsigned block=0;block<8;block++){
  // Keep all256 high/low lanes in NEW local scratch between chunks.
  // Prefix V is read ONCE per chunk,not once per32-dimensional block.
  for(unsigned lane=0;lane<32;lane++){
   dst[Lane(lane)]=Words(L::Accumulate)[head*512+block*64+lane];
   Words(0xffbd8100)[Lane(lane)]=Words(L::Accumulate)[head*512+block*64+32+lane];
  }
  (void)Words(0xffbd8100)[Lane(31)];Fence();Emit(0x70230000);Emit(0x70330004);Drain();
  for(unsigned t=0;t<count;t++){
   for(unsigned lane=0;lane<32;lane++)dst[Lane(lane)]=PioneerAttentionBits(PioneerAttentionHalf(v[t*256+block*32+lane]));
   (void)dst[Lane(31)];Fence();Emit(0x70030000);Constant(Floats(L::Scores)[head*L::Stride+first+t]);AddProduct();
  }
  for(unsigned part=0;part<2;part++){
   Emit(part?0x72330008:0x72230008);Drain();
   for(unsigned lane=0;lane<32;lane++)Words(L::Accumulate)[head*512+block*64+part*32+lane]=Words(0xffbd8200)[Lane(lane)];
  }
 }
}

static void JobSeal(const uint32_t* j,unsigned sequence){
 for(unsigned i=0;i<16;i++)if(Words(L::Job)[i]!=j[i])scalar_fault_park();
 if(*Words(L::Doorbell)!=sequence)scalar_fault_park();
}
static void ParametersSeal(const uint32_t* p){
 for(unsigned i=0;i<16;i++)if(Words(L::Parameters)[i]!=p[i])scalar_fault_park();
}
static void Capture(){
 for(unsigned i=0;i<1536;i++){
  uint32_t q=Words(L::HalfQuery)[i];
  if((q&0x7fffffff)>0x477fe000 || PioneerAttentionBits(PioneerAttentionHalf(PioneerAttentionToHalf(PioneerAttentionFloat(q))))!=q)scalar_fault_park();
  Words(L::PrivateQuery)[i]=q;
 }
 for(unsigned which=0;which<2;which++)for(unsigned i=0;i<128;i++){
  uint32_t x=Words(which?L::NewV:L::NewK)[i];
  if((x&0x7c00)==0x7c00 || (x&0x7c000000)==0x7c000000)scalar_fault_park();
  Words(which?L::PrivateV:L::PrivateK)[i]=x;
 }
}
static void LocalScores(const uint32_t* d,uint32_t* tracked,float* maximum,const uint32_t* job){
 Capture();
 for(unsigned h=0;h<6;h++)maximum[h]=-3.4028234663852886e38f;
 for(unsigned i=0;i<64;i++)Words(L::Local)[i]=0;
 if(d[49]){
  Setup();
  for(unsigned first=0;first<d[49];first+=32){
   unsigned count=Chunk(0,first,d,tracked,job,nullptr);
   for(unsigned h=0;h<6;h++)Scores(first,count,h);
  }
 }
 for(unsigned h=0;h<6;h++){
  for(unsigned t=0;t<d[49];t++)if(Floats(L::Scores)[h*L::Stride+t]>maximum[h])maximum[h]=Floats(L::Scores)[h*L::Stride+t];
  Words(L::Local)[4+h]=PioneerAttentionBits(maximum[h]);
 }
 Words(L::Local)[1]=d[49];Words(L::Local)[2]=d[48];Words(L::Local)[3]=d[15];
 Fence();Words(L::Local)[0]=1;Fence();
}
static void LocalExp(const uint32_t* d,const uint32_t* p,const float* local,float* global){
 for(unsigned h=0;h<6;h++){
  float maximum=PioneerAttentionFloat(p[h]);
  if((p[h]&0x7fffffff)>=0x7f800000 || maximum<local[h] || p[8+h])scalar_fault_park();
  global[h]=maximum;
 }
 if(p[6] || p[7] || p[14] || p[15])scalar_fault_park();
 for(unsigned h=0;h<6;h++){
  PioneerQ38RmsSum sum;
  for(unsigned t=0;t<d[49];t++){
   float e=PioneerAttentionExp(Floats(L::Scores)[h*L::Stride+t]-global[h]);
   if(!sum.Add(e))scalar_fault_park();
   Floats(L::Scores)[h*L::Stride+t]=e;
  }
  Words(L::Local)[16+h*4]=sum.high;Words(L::Local)[17+h*4]=sum.low;
  Words(L::Local)[18+h*4]=uint32_t(sum.exponent);Words(L::Local)[19+h*4]=1;
 }
 ParametersSeal(p);Fence();Words(L::Local)[0]=2;Fence();
}
static void LocalValues(const uint32_t* d,const uint32_t* p,const float* global,uint32_t* tracked,const uint32_t* job){
 if(p[6] || p[7] || p[14] || p[15])scalar_fault_park();
 for(unsigned h=0;h<6;h++){
  float inverse=PioneerAttentionFloat(p[8+h]);
  if(p[h]!=PioneerAttentionBits(global[h]) || !(inverse>0) || inverse>1 || (p[8+h]&0x7fffffff)>=0x7f800000)scalar_fault_park();
  for(unsigned t=0;t<d[49];t++){
   float e=Floats(L::Scores)[h*L::Stride+t];
   Floats(L::Scores)[h*L::Stride+t]=PioneerAttentionHalf(PioneerAttentionToHalf(e*inverse));
  }
 }
 for(unsigned i=0;i<3072;i++)Words(L::Accumulate)[i]=0;
 if(d[49]){
  Setup();
  for(unsigned first=0;first<d[49];first+=32){
   unsigned count=Chunk(1,first,d,tracked,job,p);
   for(unsigned h=0;h<6;h++)Values(first,count,h);
  }
  Drain();
 }
 Seal(d);ParametersSeal(p);
 for(unsigned h=0;h<6;h++)for(unsigned dimension=0;dimension<256;dimension++){
  unsigned block=dimension>>5,lane=dimension&31;
  uint32_t high=Words(L::Accumulate)[h*512+block*64+lane],low=Words(L::Accumulate)[h*512+block*64+32+lane];
  if((high&0x7f800000)==0x7f800000 || (low&0x7f800000)==0x7f800000)scalar_fault_park();
  Words(L::Partial)[h*512+dimension*2]=high;Words(L::Partial)[h*512+dimension*2+1]=low;
 }
 Fence();Words(L::Local)[0]=3;Fence();
}
}
extern "C" unsigned scalar_main(){
 using namespace DensePartition;
 (void)&PioneerQ38LearnedRms; // only the exact software-binary64 SUM is reused
 uint32_t d[64],tracked[8],before[8],job[16],parameters[16];float local[6],global[6];
 for(unsigned i=0;i<64;i++)d[i]=Words(L::Header)[i];
 if(!L::Shape(d) || *Words(L::Doorbell))scalar_fault_park();
 Seal(d);
 for(unsigned i=0;i<16;i++)Words(L::Status)[i]=0;
 Words(L::Status)[3]=L::Magic;Fence();
 unsigned last=0,phase=0;
 for(;;){
  unsigned request=partition_wait(last);
  Fence();for(unsigned i=0;i<16;i++)job[i]=Words(L::Job)[i];
  unsigned operation=job[1];
  if(request!=last+1 || !L::JobShape(job,d,request,operation) || operation!=phase+1 || operation>4)scalar_fault_park();
  JobSeal(job,request);Seal(d);
  Words(L::Status)[0]=request;Words(L::Status)[4]=1;Fence();
  if(operation==4){
   // Only after ALL three completed phases; partial/UNKNOWN paths do not
   // invent a STOP/cleanup grant or publish provisional KV/token state.
   JobSeal(job,request);Seal(d);Words(L::Status)[1]=request;Words(L::Status)[4]=2;Fence();return 3;
  }
  for(unsigned i=0;i<64;i++)Words(L::Summary)[i]=0;
  for(unsigned i=0;i<8;i++){
   before[i]=tracked[i]=*Words(0xffb20000+Counter(i));
   if(i==6 && tracked[6]!=tracked[2])scalar_fault_park();
  }
  if(operation>1)for(unsigned i=0;i<16;i++)parameters[i]=Words(L::Parameters)[i];
  if(operation==1)LocalScores(d,tracked,local,job);
  else if(operation==2)LocalExp(d,parameters,local,global);
  else LocalValues(d,parameters,global,tracked,job);
  JobSeal(job,request);Seal(d);
  unsigned reads=operation==2?0:d[46]>>1,flits=operation==2?0:d[47]>>1;
  auto* s=Words(L::Summary);
  if(s[6]!=reads || s[7]!=flits)scalar_fault_park();
  for(unsigned i=0;i<8;i++){
   unsigned delta=i==1?flits:i==0 || i==2 || i==3 || i==6?reads:0U;
   if(tracked[i]!=*Words(0xffb20000+Counter(i)) || uint32_t(tracked[i]-before[i])!=delta)scalar_fault_park();
   s[8+i]=before[i];s[16+i]=tracked[i];
  }
  for(unsigned i=0;i<16;i++)s[24+i]=d[16+i];
  for(unsigned i=0;i<8;i++)s[40+i]=d[32+i];
  s[1]=request;s[2]=operation;s[3]=d[48];s[4]=d[49];s[5]=6;
  s[48]=d[40];s[49]=d[15];s[50]=d[3];s[51]=d[4];s[52]=d[5];s[53]=d[6];s[54]=d[46];s[55]=d[47];
  Fence();s[0]=L::Magic;Fence();
  last=request;phase=operation;Words(L::Status)[5]=phase;
  Words(L::Status)[4]=0;Fence();Words(L::Status)[1]=last;Fence();
 }
}
