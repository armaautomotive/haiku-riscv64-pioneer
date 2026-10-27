/* SPDX-License-Identifier: MIT */
// Returning on-card Conv->GDN coordinator for SAME retained QMW1 workers.
// Separate offline image;original/init/clock/native/lifecycle ownership owed.
#include "PioneerQ38MixedRecurrentCohortRootImageRole.h"
#define PIONEER_ATTENTION_FREESTANDING
#include "PioneerAttentionMath.h"
#include "PioneerQ38MixedRecurrentCohortLayout.h"
extern "C" void scalar_fault_park() __attribute__((noreturn));
extern "C" unsigned mixed_recurrent_cohort_wait(unsigned);
extern "C" unsigned q38_mixed_recurrent_root_conv();
extern "C" __attribute__((noinline)) void q38_mixed_root_seal();
extern "C" __attribute__((noinline)) const uint32_t* q38_mixed_recurrent_frame();
extern "C" void* memset(void* target,int value,__SIZE_TYPE__ bytes){
 if(value || ((reinterpret_cast<unsigned>(target)|bytes)&3))scalar_fault_park();
 volatile uint32_t* out=static_cast<volatile uint32_t*>(target);
 for(unsigned i=0;i<bytes/4;i++)out[i]=0;
 return target;
}
namespace MixedRecurrentCohort {
namespace L=PioneerQ38MixedRecurrentCohortLayout;
namespace M=PioneerQ38MixedWorkerLayout;
namespace Dispatch=PioneerQ38MixedRecurrentDispatch;
static volatile uint32_t* Words(unsigned a){return reinterpret_cast<volatile uint32_t*>(a);}
static const uint32_t* Data(unsigned a){return reinterpret_cast<const uint32_t*>(a);}
static const uint32_t* Frame(){return q38_mixed_recurrent_frame();}
static const uint32_t* Conv(unsigned w){return Data(L::ConvShadow+w*1536);}
static const uint32_t* Gdn(unsigned w){return Data(L::GdnShadow+(w-1)*768);}
static const uint32_t* Boot(unsigned w){return Data(L::PeerBoots+w*256);}
static void Fence(){asm volatile("fence" ::: "memory");}
static unsigned Counter(unsigned i){return i==0?0x208:i==1?0x20c:(i==2 || i==6)?0x210:i==3?0x214:i==4?0x204:i==5?0x220:0x228;}
static unsigned Source(unsigned i){return !i?0x100:i<=6?0x118+(i-1)*4:i<=12?0x130+(i-7)*4:i==13?0x150:i==14?0x154:0x16c;}
static bool Quiet(){
 for(unsigned i=0;i<4;i++)if(*Words(0xffb20040+i*0x800))return false;
 for(unsigned i=0;i<32;i++)if(*Words(0xffb20240+i*4)&255)return false;
 return true;
}
static void Copy(unsigned to,unsigned from,unsigned n){for(unsigned i=0;i<n;i++)Words(to)[i]=Words(from)[i];}
static void Audit(){
 q38_mixed_root_seal();
 for(unsigned i=0;i<L::ConvWords;i++)if(Words(L::Conv)[i]!=Words(L::ConvShadow)[i])scalar_fault_park();
 for(unsigned i=0;i<L::GdnWords;i++)if(Words(L::Gdn)[i]!=Words(L::GdnShadow)[i])scalar_fault_park();
}
static bool Csr(unsigned at){
 if(at==0xffb121b0 || at==0xffb13140)return true;
 for(unsigned i=0;i<16;i++)if(at==0xffb20000+Source(i))return true;
 for(unsigned i=0;i<8;i++)if(at==0xffb20000+Counter(i))return true;
 return false;
}
static void Packet(uint32_t* tracked,unsigned w,unsigned at,unsigned bytes,bool write){
 bool permit=write?(at==M::Job && bytes==256) || (at==M::Doorbell && bytes==4)
  || (at==TT_Q38_CONV_BATCH_TABLE && bytes==1536)
  || ((at==TT_Q38_GDN_INPUT_DESCRIPTOR || at==TT_Q38_RGDN_DESCRIPTOR || at==TT_Q38_GDN_GATED_DESCRIPTOR) && bytes==256)
  || (at==TT_Q38_GDN_GATED_READY && bytes==64)
  :(at==M::Boot && bytes==256) || (at==M::Job && bytes==256) || (at==M::Status && bytes==64)
  || (at==M::Doorbell && bytes==4) || (at==0x83f00 && bytes==256)
  || ((at==TT_Q38_FFN_RESIDENT_SUMMARY || at==TT_Q38_GDN_INPUT_SUMMARY || at==TT_Q38_RGDN_SUMMARY || at==TT_Q38_GDN_GATED_SUMMARY) && bytes==128)
  || ((at==TT_Q38_GDN_GATED_READY || at==TT_Q38_GDN_RENDEZVOUS_CLOCK) && bytes==64) || (Csr(at) && bytes==4);
 if(!permit || w>=119 || ((at|bytes)&3) || bytes>1536)scalar_fault_park();
 q38_mixed_root_seal();if(!Quiet())scalar_fault_park();
 unsigned local=L::Scratch+(at&31),peer=M::Peer(w+1);uint32_t expected[8];
 for(unsigned i=0;i<8;i++){
  if(*Words(0xffb20000+Counter(i))!=tracked[i])scalar_fault_park();
  unsigned delta=i==2 || i==6?1U:i==(write?5U:1U)?(bytes+63)>>6:i==(write?4U:0U) || i==(write?7U:3U)?1U:0U;
  expected[i]=tracked[i]+delta;if(i==6 && expected[6]!=expected[2])scalar_fault_park();
 }
 const uint32_t command[]={write?local:at,0,write?129U:peer,write?at:local,0,write?peer:129U,0,write?0x212U:0,bytes,0};
 for(unsigned i=0;i<10;i++)Words(0xffb20000)[i]=command[i];
 for(unsigned i=0;i<10;i++)if(Words(0xffb20000)[i]!=command[i])scalar_fault_park();
 Fence();*Words(0xffb20040)=1;(void)*Words(0xffb20040);Fence();
 for(unsigned poll=0;poll<65536;poll++){
  bool done=!*Words(0xffb20040);for(unsigned i=0;i<8;i++)done &= *Words(0xffb20000+Counter(i))==expected[i];
  if(done){
   q38_mixed_root_seal();if(!Quiet())scalar_fault_park();
   for(unsigned i=0;i<10;i++)if(Words(0xffb20000)[i]!=command[i])scalar_fault_park();
   for(unsigned i=0;i<8;i++)tracked[i]=expected[i];
   Words(L::Summary)[8]++;Words(L::Summary)[9]+=(bytes+63)>>6;return;
  }
 }
 scalar_fault_park(); // UNKNOWN:no later packet/retry/reset/restore/END
}
static void Read(uint32_t* t,unsigned w,unsigned at,uint32_t* out,unsigned words){
 Packet(t,w,at,words*4,false);for(unsigned i=0;i<words;i++)out[i]=Words(L::Scratch+(at&31))[i];
}
static void Write(uint32_t* t,unsigned w,unsigned at,const uint32_t* in,unsigned words){
 for(unsigned i=0;i<words;i++)Words(L::Scratch+(at&31))[i]=in[i];
 Packet(t,w,at,words*4,true);
}
static unsigned Register(uint32_t* t,unsigned w,unsigned at){uint32_t v;Read(t,w,at,&v,1);return v;}
static unsigned Wait(uint32_t* t,unsigned w,unsigned target,unsigned operation,bool capture){
 const auto* d=Frame();
 for(unsigned poll=0;poll<65536;poll++){
  // The reset observation precedes PC. A speculative PC alone grants nothing.
  if(Register(t,w,0xffb121b0)!=0x46800)scalar_fault_park();
  uint32_t s[16];Read(t,w,M::Status,s,16);
  if(s[2] || s[3]!=M::Magic || s[0]>=0x7fffffff || s[1]>s[0] || s[5]
   || s[6]>=64 || s[7]>=8192 || (s[4] && s[4]!=operation))scalar_fault_park();
  for(unsigned i=8;i<16;i++)if(s[i])scalar_fault_park();
  if(capture){if(!s[0] || s[0]!=s[1] || s[4])scalar_fault_park();target=s[1];}
  else if(s[0]<target-1 || s[0]>target || s[1]<target-1 || s[1]>target)scalar_fault_park();
  unsigned pc=Register(t,w,0xffb13140);
  if(s[0]!=target || s[1]!=target || s[4])continue;
  if(s[6]!=d[3] || s[7]!=d[4])scalar_fault_park();
  if(pc==0x2004c || pc==0x20050)return target;
 }
 scalar_fault_park();
}
static void Sources(uint32_t* t,unsigned w){
 for(unsigned i=0;i<16;i++)if(Register(t,w,0xffb20000+Source(i))!=Boot(w)[16+i])scalar_fault_park();
}
static void BootSeal(uint32_t* t,unsigned w){
 uint32_t b[64];Read(t,w,M::Boot,b,64);for(unsigned i=0;i<64;i++)if(b[i]!=Boot(w)[i])scalar_fault_park();Sources(t,w);
}
static void Closed(uint32_t* t,unsigned w){
 uint32_t j[64],s[16];Read(t,w,M::Job,j,64);
 for(unsigned i=0;i<64;i++)if(j[i]!=Words(L::PeerJobs+w*256)[i])scalar_fault_park();
 unsigned n=Words(L::Nonces)[w];if(Register(t,w,M::Doorbell)!=n)scalar_fault_park();
 Read(t,w,M::Status,s,16);
 if(s[0]!=n || s[1]!=n || s[2] || s[3]!=M::Magic || s[4] || s[5] || s[6]!=j[5] || s[7]!=j[6])scalar_fault_park();
 for(unsigned i=8;i<16;i++)if(s[i])scalar_fault_park();
}
static void CounterSeal(uint32_t* t,unsigned w,const uint32_t* after){
 for(unsigned i=0;i<8;i++){
  unsigned v=Register(t,w,0xffb20000+Counter(i));if(v!=after[i] || (i==6 && v!=after[2]))scalar_fault_park();
 }
 for(unsigned i=0;i<8;i++)Words(L::Counters+w*32)[i]=after[i];
}
static void Admit(uint32_t* t,unsigned w){
 const auto* d=Frame();unsigned n=Wait(t,w,0,0,true);Words(L::Nonces)[w]=n;
 if(n>=d[5] || n>0x7ffffffb)scalar_fault_park(); // Conv,GDN,subsequent matrix and STOP remain representable
 uint32_t b[64],j[64],p[64];Read(t,w,M::Boot,b,64);
 if(!M::BootShape(b) || b[3]!=w+1 || b[4]!=M::Peer(w+1) || !Dispatch::Binding(b,Conv(w+1)))scalar_fault_park();
 for(unsigned i=0;i<64;i++)Words(L::PeerBoots+w*256)[i]=b[i];
 Sources(t,w);
 Read(t,w,M::Job,j,64);
 if(!M::JobShape(j,b,n-1,0) || j[1]!=n || j[2]!=1 || j[5]!=d[3] || j[6]!=d[4]
  || j[16]!=0x31483851 || j[17]!=8 || j[18]!=576 || j[19] || j[20] || j[21]!=5120
  || !j[23] || j[23]>16 || j[25]!=1 || j[26]!=1)scalar_fault_park();
 for(unsigned i=0;i<64;i++)Words(L::PeerJobs+w*256)[i]=j[i];
 Closed(t,w);
 Read(t,w,0x83f00,p,64);unsigned groups=j[23],reads=groups*160+2,flits=groups*1440+170;
 if(p[16]!=groups*320 || p[17]!=n || p[18]!=w+1 || p[19]!=TT_Q38_FFN_PERSISTENT_MARKER || p[30]!=b[16] || p[31]!=b[4])scalar_fault_park();
 for(unsigned i=0;i<16;i++)if(p[32+i]!=b[16+i])scalar_fault_park();
 for(unsigned i=48;i<64;i++)if(p[i])scalar_fault_park();
 const uint32_t command[]={j[22]+(groups*160-1)*576,0,j[24],0x90000,0,b[4],0,0,576,0};
 for(unsigned i=0;i<10;i++)if(p[20+i]!=command[i])scalar_fault_park();
 for(unsigned i=0;i<8;i++){
  unsigned delta=i==1?flits:i==0 || i==2 || i==3 || i==6?reads:0;
  if(uint32_t(p[8+i]-p[i])!=delta)scalar_fault_park();
 }
 CounterSeal(t,w,p+8);BootSeal(t,w);
}
static void Capture(){
 const auto* d=Frame();if(!L::Shape(d))scalar_fault_park();
 Copy(L::ConvShadow,L::Conv,L::ConvWords);Copy(L::GdnShadow,L::Gdn,L::GdnWords);
 for(unsigned w=0;w<120;w++){
  if(!L::ConvShape(d,Conv(w),w))scalar_fault_park();
  if(w && w<=96 && !L::GdnShape(d,Gdn(w),Conv(w),w))scalar_fault_park();
 }
 Audit(); // complete geometry BEFORE the first peer packet
}
static void Job(unsigned w,unsigned operation,uint32_t* j){
 unsigned n=Words(L::Nonces)[w];bool okay=operation==7?Dispatch::Conv(Boot(w),Conv(w+1),n,j):
  Dispatch::Gdn(Boot(w),Gdn(w+1),Gdn(w+1)+64,Gdn(w+1)+128,n,j);
 if(!okay)scalar_fault_park();
}
static void Finish(uint32_t* t,unsigned w,unsigned operation){
 unsigned n=Words(L::Nonces)[w];Wait(t,w,n,operation,false);BootSeal(t,w);Closed(t,w);
 uint32_t p[64],j[16],actual[8];Read(t,w,0x83f00,p,64);
 Dispatch::Internal(operation,Frame()[operation==7?5:6],w+1,n,j);
 const auto* before=Data(L::Counters+w*32);bool okay;
 if(operation==7){
  uint32_t summary[32];Read(t,w,TT_Q38_FFN_RESIDENT_SUMMARY,summary,32);
  okay=PioneerQ38ResidentConvServiceProof(p,j,Conv(w+1),Boot(w)+16,w+1,M::Peer(w+1),before,p+8,summary);
 }else{
  uint32_t first[32],state[32],gated[32],clock[16];
  Read(t,w,TT_Q38_GDN_INPUT_SUMMARY,first,32);Read(t,w,TT_Q38_RGDN_SUMMARY,state,32);
  Read(t,w,TT_Q38_GDN_GATED_SUMMARY,gated,32);Read(t,w,TT_Q38_GDN_RENDEZVOUS_CLOCK,clock,16);
  const auto* in=Gdn(w+1);
  okay=PioneerQ38ResidentGdnServiceProof(p,j,in+128,in,in+64,Boot(w)+16,w+1,M::Peer(w+1),before,p+8,first,state,gated,clock);
 }
 if(!okay)scalar_fault_park();
 for(unsigned i=0;i<8;i++)actual[i]=p[8+i];
 CounterSeal(t,w,actual);
}
static void Release(uint32_t* t,unsigned count){
 for(unsigned w=0;w<count;w++){unsigned next=Words(L::Nonces)[w]+1;Write(t,w,M::Doorbell,&next,1);Words(L::Nonces)[w]=next;}
}
static void StageConv(uint32_t* t){
 for(unsigned w=0;w<119;w++)Admit(t,w);
 Audit();
 for(unsigned w=0;w<119;w++){
  uint32_t j[64];Job(w,7,j);Write(t,w,TT_Q38_CONV_BATCH_TABLE,Conv(w+1),384);
  Write(t,w,M::Job,j,64);for(unsigned i=0;i<64;i++)Words(L::PeerJobs+w*256)[i]=j[i];
 }
 Audit();Release(t,119);
 // ROOT0's six groups run while the119 workers are already computing.
 Copy(TT_Q38_CONV_BATCH_TABLE,L::ConvShadow,384);uint32_t before[8],after[8],summary[32];
 for(unsigned i=0;i<8;i++){if(*Words(0xffb20000+Counter(i))!=t[i])scalar_fault_park();before[i]=t[i];}
 if(q38_mixed_recurrent_root_conv()!=96 || !Quiet())scalar_fault_park();
 Audit();
 for(unsigned i=0;i<8;i++)after[i]=*Words(0xffb20000+Counter(i));
 for(unsigned i=0;i<32;i++)summary[i]=Words(TT_Q38_FFN_RESIDENT_SUMMARY)[i];
 if(!PioneerQ38ConvBatchSummary(summary,Conv(0),before,after))scalar_fault_park();
 for(unsigned i=0;i<8;i++)t[i]=after[i];
 Words(L::Summary)[8]+=summary[0]+summary[2];Words(L::Summary)[9]+=summary[1]+summary[3];Words(L::Summary)[5]=6;
 for(unsigned w=0;w<119;w++)Finish(t,w,7);
 Audit();Words(L::Summary)[4]=120; // ALL640 actual outputs precede ANY GDN
}
static void StageGdn(uint32_t* t){
 for(unsigned w=0;w<96;w++){
  Wait(t,w,Words(L::Nonces)[w],0,true);BootSeal(t,w);Closed(t,w);
  CounterSeal(t,w,Data(L::Counters+w*32));
  const auto* in=Gdn(w+1);uint32_t j[64],zero[16]={};Job(w,8,j);
  Write(t,w,TT_Q38_GDN_INPUT_DESCRIPTOR,in,64);Write(t,w,TT_Q38_RGDN_DESCRIPTOR,in+64,64);
  Write(t,w,TT_Q38_GDN_GATED_DESCRIPTOR,in+128,64);Write(t,w,M::Job,j,64);
  for(unsigned i=0;i<64;i++)Words(L::PeerJobs+w*256)[i]=j[i];
  Write(t,w,TT_Q38_GDN_GATED_READY,zero,16);
 }
 for(unsigned w=0;w<96;w++){
  uint32_t ready[16];Read(t,w,TT_Q38_GDN_GATED_READY,ready,16);
  for(unsigned i=0;i<16;i++)if(ready[i])scalar_fault_park();
 }
 Audit();Release(t,96);
 for(unsigned w=0;w<96;w++)Finish(t,w,8);
 Audit();Words(L::Summary)[6]=96;
}
}
extern "C" unsigned q38_mixed_recurrent_cohort(){
 using namespace MixedRecurrentCohort;Capture();uint32_t tracked[8],before[8];
 for(unsigned i=0;i<64;i++)Words(L::Summary)[i]=0;
 for(unsigned i=0;i<8;i++)before[i]=tracked[i]=*Words(0xffb20000+Counter(i));
 if(tracked[2]!=tracked[6] || !Quiet())scalar_fault_park();
 StageConv(tracked);StageGdn(tracked);Audit();if(!Quiet())scalar_fault_park();
 const auto* d=Frame();auto* s=Words(L::Summary);
 for(unsigned i=0;i<8;i++){if(*Words(0xffb20000+Counter(i))!=tracked[i])scalar_fault_park();s[16+i]=tracked[i];s[24+i]=before[i];s[48+i]=d[32+i];}
 for(unsigned i=0;i<16;i++)s[32+i]=d[16+i];
 s[1]=d[48];s[2]=d[3];s[3]=d[4];s[7]=6144;s[56]=d[5];s[57]=d[6];s[58]=d[14];
 Fence();s[0]=L::Magic;Fence();return 6144; // private NEXT/gated data,NOT state-slot/token commit
}
extern "C" void q38_mixed_root_seal(){
 using namespace MixedRecurrentCohort;const L::PrivateSeal* p;asm volatile("mv %0,s11":"=r"(p));
 if(reinterpret_cast<unsigned>(p)<0x6e000 || reinterpret_cast<unsigned>(p)>0x70000-sizeof(*p))scalar_fault_park();
 for(unsigned i=0;i<64;i++)if(Words(L::Boot)[i]!=p->boot[i] || Words(L::Header)[i]!=p->frame[i])scalar_fault_park();
 if(*Words(L::Doorbell)!=p->frame[48])scalar_fault_park();
 for(unsigned i=0;i<16;i++)if(*Words(0xffb20000+Source(i))!=p->boot[16+i])scalar_fault_park();
}
extern "C" const uint32_t* q38_mixed_recurrent_frame(){
 using namespace MixedRecurrentCohort;const L::PrivateSeal* p;asm volatile("mv %0,s11":"=r"(p));
 if(reinterpret_cast<unsigned>(p)<0x6e000 || reinterpret_cast<unsigned>(p)>0x70000-sizeof(*p))scalar_fault_park();
 return p->frame;
}
extern "C" void mixed_recurrent_cohort_main(){
 using namespace MixedRecurrentCohort;L::PrivateSeal seal;uint32_t counters[8];
 for(unsigned i=0;i<64;i++)seal.boot[i]=Words(L::Boot)[i];
 if(!L::BootShape(seal.boot) || *Words(L::Doorbell) || !Quiet())scalar_fault_park();
 for(unsigned i=0;i<16;i++)if(*Words(0xffb20000+Source(i))!=seal.boot[16+i])scalar_fault_park();
 for(unsigned i=0;i<8;i++)counters[i]=*Words(0xffb20000+Counter(i));
 if(counters[2]!=counters[6])scalar_fault_park();
 asm volatile("mv s11,%0"::"r"(&seal):"s11","memory");
 for(unsigned i=0;i<16;i++)Words(L::Status)[i]=0;
 Words(L::Status)[3]=L::Magic;Fence();unsigned previous=0;
 for(;;){
  unsigned n=mixed_recurrent_cohort_wait(previous);for(unsigned i=0;i<64;i++)seal.frame[i]=Words(L::Header)[i];
  const auto* d=seal.frame;if(n!=previous+1 || d[48]!=n || !L::Shape(d))scalar_fault_park();
  for(unsigned i=0;i<16;i++)if(d[16+i]!=seal.boot[16+i])scalar_fault_park();
  for(unsigned i=0;i<8;i++)if(d[32+i]!=seal.boot[8+i] || *Words(0xffb20000+Counter(i))!=counters[i])scalar_fault_park();
  q38_mixed_root_seal();Words(L::Status)[0]=n;Words(L::Status)[4]=1;Fence();
  if(q38_mixed_recurrent_cohort()!=6144)scalar_fault_park();
  q38_mixed_root_seal();
  for(unsigned i=0;i<8;i++)counters[i]=*Words(0xffb20000+Counter(i));
  previous=n;Words(L::Status)[6]=d[3];Words(L::Status)[7]=d[4];Words(L::Status)[4]=0;Fence();Words(L::Status)[1]=n;Fence();
 }
}
