/* SPDX-License-Identifier: MIT */
// OFFLINE one complete resident Conv->GDN cohort. NO public/native selector.
// ALL actual source/initialized state/clock/private producer ownership OWED.
#include "PioneerQ38ResidentCohortImageRole.h"
#define PIONEER_ATTENTION_FREESTANDING
#include "../../src/add-ons/kernel/drivers/misc/PioneerQ38ResidentCohortShape.h"
#include "../../src/add-ons/kernel/drivers/misc/PioneerQ38PeerTerminal.h"
extern "C" unsigned q38_cohort_conv_once();
extern "C" void cohort_fault_park() __attribute__((noreturn));
extern "C" void* memset(void* target,int value,__SIZE_TYPE__ bytes){
 volatile unsigned char* out=static_cast<volatile unsigned char*>(target);
 for(__SIZE_TYPE__ i=0;i<bytes;i++)out[i]=static_cast<unsigned char>(value);
 return target;
}
extern "C" void* memcpy(void* target,const void* source,__SIZE_TYPE__ bytes){
 volatile unsigned char* out=static_cast<volatile unsigned char*>(target);
 const volatile unsigned char* in=static_cast<const volatile unsigned char*>(source);
 for(__SIZE_TYPE__ i=0;i<bytes;i++)out[i]=in[i];
 return target;
}
namespace Cohort {
static volatile uint32_t* Words(unsigned at){return reinterpret_cast<volatile uint32_t*>(at);}
static const uint32_t* Manifest(){return reinterpret_cast<const uint32_t*>(TT_Q38_COHORT_SHADOW);}
static const uint32_t* Source(unsigned w){return PioneerQ38CohortSource(Manifest(),w);}
static void Fence(){asm volatile("fence" ::: "memory");}
static unsigned Counter(unsigned i){return i==0?0x208:i==1?0x20c:(i==2 || i==6)?0x210:i==3?0x214:i==4?0x204:i==5?0x220:0x228;}
static unsigned SourceOffset(unsigned i){return !i?0x100:i<=6?0x118+(i-1)*4:i<=12?0x130+(i-7)*4:i==13?0x150:i==14?0x154:0x16c;}
static bool Quiet(){
 for(unsigned i=0;i<4;i++)if(*Words(0xffb20040+i*0x800))return false;
 for(unsigned i=0;i<32;i++)if(*Words(0xffb20240+i*4)&255)return false;
 return true;
}
static bool Current(){
 for(unsigned i=0;i<16;i++)if(*Words(0xffb20000+SourceOffset(i))!=Manifest()[16+i])return false;
 return true;
}
static bool Audit(){
 for(unsigned i=0;i<TT_Q38_COHORT_WORDS;i++)if(Words(TT_Q38_COHORT_MANIFEST)[i]!=Words(TT_Q38_COHORT_SHADOW)[i])return false;
 return Current();
}
static bool Csr(unsigned remote){
 if(remote==0xffb13140 || remote==0xffb121b0)return true;
 for(unsigned i=0;i<16;i++)if(remote==0xffb20000+SourceOffset(i))return true;
 for(unsigned i=0;i<8;i++)if(remote==0xffb20000+Counter(i))return true;
 return false;
}
static void Packet(bool write,unsigned w,unsigned local,unsigned remote,unsigned bytes){
 bool permit=write?(remote==0x74000 && bytes==64) || (remote==0x74100 && bytes==4)
  || (remote==TT_Q38_CONV_BATCH_TABLE && (bytes==1536 || bytes==256))
  || ((remote==TT_Q38_RGDN_DESCRIPTOR || remote==TT_Q38_GDN_GATED_DESCRIPTOR) && bytes==256)
  || (remote==TT_Q38_GDN_GATED_READY && bytes==64)
  :(remote==0x74140 && bytes==64) || (remote==0x83f00 && bytes==256)
  || ((remote==TT_Q38_FFN_RESIDENT_SUMMARY || remote==TT_Q38_GDN_INPUT_SUMMARY || remote==TT_Q38_GDN_GATED_SUMMARY) && bytes==128)
  || ((remote==TT_Q38_GDN_RENDEZVOUS_CLOCK || remote==TT_Q38_GDN_GATED_READY) && bytes==64)
  || (Csr(remote) && bytes==4);
 if(!permit || !w || w>119 || local<0x75000 || bytes>2048 || local>0x75800-bytes
  || ((local^remote)&31) || !Current() || !Quiet())cohort_fault_park();
 uint32_t expected[8];unsigned flits=(bytes+63)/64;
 for(unsigned i=0;i<8;i++){
  unsigned delta=i==2 || i==6?1U:i==0 || i==3?unsigned(!write):i==1?(!write?flits:0U):i==5?(write?flits:0U):unsigned(write);
  expected[i]=*Words(0xffb20000+Counter(i))+delta;if(i==6 && expected[6]!=expected[2])cohort_fault_park();
 }
 const uint32_t command[]={write?local:remote,0,write?129U:PioneerQ38ConvPeer(w),write?remote:local,0,
  write?PioneerQ38ConvPeer(w):129U,0,write?0x212U:0,bytes,0};
 for(unsigned i=0;i<10;i++)Words(0xffb20000)[i]=command[i];
 for(unsigned i=0;i<10;i++)if(Words(0xffb20000)[i]!=command[i])cohort_fault_park();
 Fence();*Words(0xffb20040)=1;(void)*Words(0xffb20040);Fence();
 for(unsigned poll=0;poll<65536;poll++){
  bool complete=!*Words(0xffb20040);for(unsigned i=0;i<8;i++)complete &= *Words(0xffb20000+Counter(i))==expected[i];
  if(complete){
   if(!Current() || !Quiet())cohort_fault_park();
   for(unsigned i=0;i<10;i++)if(Words(0xffb20000)[i]!=command[i])cohort_fault_park();
   Words(TT_Q38_COHORT_STATS)[write?1:0]++;Words(TT_Q38_COHORT_STATS)[write?3:2]+=flits;return;
  }
 }
 cohort_fault_park(); // UNKNOWN:NO further packet/reset/restore/STOP/retry
}
static void Read(unsigned w,unsigned at,uint32_t* p,unsigned words){
 unsigned local=0x75000+(at&31);Packet(false,w,local,at,words*4);
 for(unsigned i=0;i<words;i++)p[i]=Words(local)[i];
}
static void Write(unsigned w,unsigned at,const uint32_t* p,unsigned words){
 unsigned local=0x75000+(at&31);for(unsigned i=0;i<words;i++)Words(local)[i]=p[i];
 Packet(true,w,local,at,words*4);
}
static void Sources(unsigned w){
 uint32_t value;
 for(unsigned i=0;i<16;i++){Read(w,0xffb20000+SourceOffset(i),&value,1);if(value!=Source(w)[i])cohort_fault_park();}
}
static void Counters(unsigned w,uint32_t* p,const uint32_t* expected){
 for(unsigned i=0;i<8;i++){
  Read(w,0xffb20000+Counter(i),p+i,1);
  if((i==6 && p[i]!=p[2]) || (expected && p[i]!=expected[i]))cohort_fault_park();
 }
}
static bool WaitPc(unsigned pc){return pc==0x7204c || pc==0x72050;} // exact companion symbols pinned by builder
static bool Physical(unsigned w){
 uint32_t pc[2],reset;
 for(unsigned i=0;i<2;i++){
  Read(w,0xffb13140,pc+i,1);
  if((pc[i]&3) || !((pc[i]>=0x64000 && pc[i]<0x68000) || (pc[i]>=0x68000 && pc[i]<0x6e000)
    || (pc[i]>=0x70b00 && pc[i]<0x74000) || (pc[i]>=0xb8000 && pc[i]<0xc8000))
   || pc[i]==TT_Q38_PEER_FAULT_ENTRY || pc[i]==TT_Q38_PEER_FAULT_PARK || PioneerQ38PeerTerminalPc(pc[i]))cohort_fault_park();
 }
 Read(w,0xffb121b0,&reset,1);if(reset!=0x46800)cohort_fault_park();
 return WaitPc(pc[0]) && WaitPc(pc[1]); // never infer WAIT from a running/held hint
}
static void Wait(unsigned w,unsigned sequence,unsigned operation,bool initial){
 for(unsigned poll=0;poll<65536;poll++){
  uint32_t s[16];Read(w,0x74140,s,16);Words(TT_Q38_COHORT_STATS)[4]++;
  unsigned old=initial?sequence:sequence-1;
  if(s[2] || s[3]!=TT_Q38_FFN_PERSISTENT_MARKER || s[0]<old || s[0]>sequence || s[1]<old || s[1]>s[0]
   || (s[4] && s[4]!=operation))cohort_fault_park();
  for(unsigned i=5;i<16;i++)if(s[i])cohort_fault_park();
  if(s[0]==sequence && s[1]==sequence && !s[4] && Physical(w))return;
 }
 cohort_fault_park();
}
static void Before(unsigned count,unsigned operation){
 for(unsigned w=1;w<=count;w++){
  Wait(w,Words(TT_Q38_COHORT_SEQUENCE)[w-1],operation,true);Sources(w);
  uint32_t before[8];Counters(w,before,nullptr);for(unsigned i=0;i<8;i++)Words(TT_Q38_COHORT_COUNTERS)[(w-1)*8+i]=before[i];
 }
}
static void StageConv(){
 Before(119,7);
 for(unsigned w=1;w<120;w++){
  uint32_t j[16];PioneerQ38CohortJob(Manifest(),w,7,j);
  Write(w,TT_Q38_CONV_BATCH_TABLE,PioneerQ38CohortConv(Manifest(),w),384);Write(w,0x74000,j,16);
 }
 if(!Audit())cohort_fault_park();
 for(unsigned w=1;w<120;w++){uint32_t sequence=Words(TT_Q38_COHORT_SEQUENCE)[w-1]+1;Write(w,0x74100,&sequence,1);}
 // ROOT0 contributes the six missing groups. Workers already run;ROOT's
 // local arithmetic shares their GDDR workspace without any host callback.
 const auto* table=PioneerQ38CohortConv(Manifest(),0);uint32_t before[8],after[8],summary[32];
 for(unsigned i=0;i<384;i++)Words(TT_Q38_CONV_BATCH_TABLE)[i]=table[i];
 for(unsigned i=0;i<8;i++)before[i]=*Words(0xffb20000+Counter(i));
 if(!Current() || !Quiet() || q38_cohort_conv_once()!=96 || !Audit() || !Quiet())cohort_fault_park();
 for(unsigned i=0;i<8;i++)after[i]=*Words(0xffb20000+Counter(i));
 for(unsigned i=0;i<32;i++)summary[i]=Words(TT_Q38_FFN_RESIDENT_SUMMARY)[i];
 if(!PioneerQ38ConvBatchSummary(summary,table,before,after))cohort_fault_park();
 Words(TT_Q38_COHORT_STATS)[5]=1;
 for(unsigned w=1;w<120;w++){
  uint32_t p[64],s[32],j[16],actual[8];unsigned seq=Words(TT_Q38_COHORT_SEQUENCE)[w-1]+1;
  Wait(w,seq,7,false);Read(w,0x83f00,p,64);Read(w,TT_Q38_FFN_RESIDENT_SUMMARY,s,32);
  PioneerQ38CohortJob(Manifest(),w,7,j);
  const auto* baseline=reinterpret_cast<const uint32_t*>(TT_Q38_COHORT_COUNTERS+(w-1)*32);
  if(!PioneerQ38ResidentConvServiceProof(p,j,PioneerQ38CohortConv(Manifest(),w),Source(w),w,PioneerQ38ConvPeer(w),baseline,p+8,s))cohort_fault_park();
  Sources(w);Counters(w,actual,p+8);Words(TT_Q38_COHORT_SEQUENCE)[w-1]=seq;Words(TT_Q38_COHORT_STATS)[5]++;
 }
 if(!Audit())cohort_fault_park(); // ALL640 outputs precede ANY GDN release
}
static void StageGdn(){
 Before(96,8);
 for(unsigned w=1;w<=96;w++){
  const auto* in=PioneerQ38CohortGdn(Manifest(),w);uint32_t j[16],zero[16]={};
  PioneerQ38CohortJob(Manifest(),w,8,j);
  Write(w,TT_Q38_GDN_INPUT_DESCRIPTOR,in,64);Write(w,TT_Q38_RGDN_DESCRIPTOR,in+64,64);
  Write(w,TT_Q38_GDN_GATED_DESCRIPTOR,in+128,64);Write(w,0x74000,j,16);
  Write(w,TT_Q38_GDN_GATED_READY,zero,16);
 }
 // Cohort barrier:clear AND read back ALL96 cells before releasing ANY
 // worker. A late clear can otherwise erase a legitimately published seal.
 for(unsigned w=1;w<=96;w++){
  uint32_t ready[16];Read(w,TT_Q38_GDN_GATED_READY,ready,16);
  for(unsigned i=0;i<16;i++)if(ready[i])cohort_fault_park();
 }
 if(!Audit())cohort_fault_park();
 for(unsigned w=1;w<=96;w++){uint32_t sequence=Words(TT_Q38_COHORT_SEQUENCE)[w-1]+1;Write(w,0x74100,&sequence,1);}
 for(unsigned w=1;w<=96;w++){
  uint32_t p[64],first[32],recurrent[32],gated[32],clock[16],j[16],actual[8];
  unsigned seq=Words(TT_Q38_COHORT_SEQUENCE)[w-1]+1;Wait(w,seq,8,false);
  Read(w,0x83f00,p,64);Read(w,TT_Q38_GDN_INPUT_SUMMARY,first,32);Read(w,TT_Q38_RGDN_SUMMARY,recurrent,32);
  Read(w,TT_Q38_GDN_GATED_SUMMARY,gated,32);Read(w,TT_Q38_GDN_RENDEZVOUS_CLOCK,clock,16);
  const auto* in=PioneerQ38CohortGdn(Manifest(),w);PioneerQ38CohortJob(Manifest(),w,8,j);
  const auto* baseline=reinterpret_cast<const uint32_t*>(TT_Q38_COHORT_COUNTERS+(w-1)*32);
  if(!PioneerQ38ResidentGdnServiceProof(p,j,in+128,in,in+64,Source(w),w,PioneerQ38ConvPeer(w),baseline,p+8,first,recurrent,gated,clock))cohort_fault_park();
  Sources(w);Counters(w,actual,p+8);Words(TT_Q38_COHORT_SEQUENCE)[w-1]=seq;Words(TT_Q38_COHORT_STATS)[6]++;
 }
 if(!Audit())cohort_fault_park();
}
}
extern "C" unsigned q38_resident_cohort_main(){
 using namespace Cohort;
 for(unsigned i=0;i<TT_Q38_COHORT_WORDS;i++)Words(TT_Q38_COHORT_SHADOW)[i]=Words(TT_Q38_COHORT_MANIFEST)[i];
 if(!PioneerQ38ResidentCohortShape(Manifest()) || !Audit() || !Quiet())cohort_fault_park();
 for(unsigned i=0;i<64;i++)Words(TT_Q38_COHORT_STATS)[i]=0;
 for(unsigned w=1;w<120;w++)Words(TT_Q38_COHORT_SEQUENCE)[w-1]=Manifest()[64+w-1];
 StageConv();StageGdn();
 return 6144; // private NEXT/output ONLY;NO state-slot or model-token commit
}
