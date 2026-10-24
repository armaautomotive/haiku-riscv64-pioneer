/* SPDX-License-Identifier: MIT */
// OFFLINE fused raw-input->QRJ1->paired raw-ready/NoC->gated norm->GDDR.
// No CPU arithmetic,host intermediate,stop/reload or native/Work selection.
#if !defined(PIONEER_Q38_GDN_GATED_RESIDENT_IMAGE) || PIONEER_Q38_GDN_GATED_RESIDENT_IMAGE != 1
#error Exact separate offline fused-resident image role required
#endif
#define PIONEER_ATTENTION_FREESTANDING
#define PIONEER_Q38_GDN_GATED_NORM_IMAGE 1
#define PIONEER_Q38_GDN_PREPARE_IMAGE 1
#include "PioneerQ38GdnGatedNorm.h"
#include "../../src/add-ons/kernel/drivers/misc/PioneerQ38GdnGatedShape.h"
extern "C" unsigned q38_resident_gdn_recurrent_stage();
extern "C" void scalar_fault_park() __attribute__((noreturn));
extern "C" void* memset(void* target,int value,__SIZE_TYPE__ bytes){
 volatile unsigned char* out=static_cast<volatile unsigned char*>(target);
 for(__SIZE_TYPE__ i=0;i<bytes;i++)out[i]=static_cast<unsigned char>(value);
 return target;
}
namespace Gated {
static volatile uint32_t* Words(unsigned at){return reinterpret_cast<volatile uint32_t*>(at);}
static void Fence(){asm volatile("fence" ::: "memory");}
// No per-poll stack-table memcpy:bounded waits must remain bounded actual
// instruction work too,not rebuild eight constant tables each iteration.
static unsigned Counter(unsigned i){return i==0?0x208:i==1?0x20c:(i==2 || i==6)?0x210:i==3?0x214:i==4?0x204:i==5?0x220:0x228;}
static unsigned Source(unsigned i){return !i?0x100:i<=6?0x118+(i-1)*4:i<=12?0x130+(i-7)*4:i==13?0x150:i==14?0x154:0x16c;}
static bool Quiet(){
 for(unsigned i=0;i<4;i++)if(*Words(0xffb20040+i*0x800))return false;
 for(unsigned i=0;i<32;i++)if(*Words(0xffb20240+i*4)&255)return false;
 return true;
}
static bool Bound(const uint32_t* d){
 for(unsigned i=0;i<16;i++)if(*Words(0xffb20000+Source(i))!=d[16+i])return false;
 for(unsigned i=0;i<64;i++)if(Words(TT_Q38_GDN_GATED_DESCRIPTOR)[i]!=d[i])return false;
 return true;
}
static bool Seal(const uint32_t* d){
 if(!Bound(d))return false;
 // Peer may advance to gated output,but MUST retain this separate raw
 // readiness seal and raw LOCAL output until the future owner holds ALL.
 for(unsigned i=0;i<16;i++)if(Words(TT_Q38_GDN_GATED_READY)[i]!=PioneerQ38GdnGatedReadyWord(d,i,false))return false;
 return true;
}
static __attribute__((noinline)) void Packet(unsigned p,const uint32_t* d,uint32_t* tracked){
 if(p>4 || !Seal(d) || !Quiet())scalar_fault_park();
 bool write=p==4;
 const unsigned size[]={64,256,512,512,256};
 const unsigned remote[]={d[44],d[45],d[12],d[4]+d[42],d[4]+d[43]};
 const unsigned local[]={TT_Q38_GDN_GATED_PEER_READY,0x80000+((d[7]^1)&1)*256,0x80200,0x80400,0x8ea00};
 const unsigned route[]={d[15],d[15],d[13],d[8],d[8]};
 if(!write)for(unsigned i=0;i<size[p]/4;i++){
  unsigned poison=0x7fc70000+p*1024+i;Words(local[p])[i]=poison;if(Words(local[p])[i]!=poison)scalar_fault_park();
 }
 uint32_t expected[8];const unsigned delta[]={write?0U:1U,write?0U:size[p]/64,1,write?0U:1U,write?1U:0U,write?size[p]/64:0U,1,write?1U:0U};
 for(unsigned i=0;i<8;i++){
  if(*Words(0xffb20000+Counter(i))!=tracked[i])scalar_fault_park();
  expected[i]=tracked[i]+delta[i];
 }
 const unsigned command[]={write?local[p]:remote[p],0,write?d[9]:route[p],write?remote[p]:local[p],0,write?route[p]:d[9],0,write?0x212U:0,size[p],0};
 for(unsigned i=0;i<10;i++)Words(0xffb20000)[i]=command[i];
 for(unsigned i=0;i<10;i++)if(Words(0xffb20000)[i]!=command[i])scalar_fault_park();
 Fence();*Words(0xffb20040)=1;(void)*Words(0xffb20040);Fence();
 for(unsigned attempt=0;attempt<65536;attempt++){
  bool complete=!*Words(0xffb20040);
  for(unsigned i=0;i<8;i++)complete &= *Words(0xffb20000+Counter(i))==expected[i];
  if(complete){
   if(!Seal(d) || !Quiet())scalar_fault_park();
   for(unsigned i=0;i<10;i++)if(Words(0xffb20000)[i]!=command[i])scalar_fault_park();
   if(p && !write)for(unsigned i=0;i<size[p]/4;i++)if(!PioneerQ38PrepareScalar(PioneerAttentionFloat(Words(local[p])[i])))scalar_fault_park();
   for(unsigned i=0;i<8;i++)tracked[i]=expected[i];
   Words(TT_Q38_GDN_GATED_SUMMARY)[write?2:0]++;Words(TT_Q38_GDN_GATED_SUMMARY)[write?3:1]+=size[p]/64;Fence();return;
  }
 }
 scalar_fault_park(); // UNKNOWN packet:never retry/reset/restore/publish
}
}
extern "C" unsigned scalar_main(){
 using namespace Gated;uint32_t d[64],input[64],state[64],tracked[8];
 for(unsigned i=0;i<64;i++){
  d[i]=Words(TT_Q38_GDN_GATED_DESCRIPTOR)[i];input[i]=Words(TT_Q38_GDN_INPUT_DESCRIPTOR)[i];state[i]=Words(TT_Q38_RGDN_DESCRIPTOR)[i];
 }
 if(!PioneerQ38GdnGatedShape(d,input,state))return 0; // NO MMIO before ALL paired DATA checks
 // Future owner must clear ALL peers' cells BEFORE releasing ANY. A stale
 // cell is a refusal,not permission to clear it and hide a reused epoch.
 for(unsigned i=0;i<16;i++)if(Words(TT_Q38_GDN_GATED_READY)[i])return 0;
 if(q38_resident_gdn_recurrent_stage()!=64)scalar_fault_park();
 for(unsigned i=0;i<64;i++)if(Words(TT_Q38_GDN_GATED_DESCRIPTOR)[i]!=d[i]
  || Words(TT_Q38_GDN_INPUT_DESCRIPTOR)[i]!=input[i] || Words(TT_Q38_RGDN_DESCRIPTOR)[i]!=state[i])scalar_fault_park();
 for(unsigned i=0;i<8;i++){
  tracked[i]=*Words(0xffb20000+Counter(i));if(tracked[i]!=Words(TT_Q38_RGDN_SUMMARY)[16+i])scalar_fault_park();
 }
 if(tracked[2]!=tracked[6] || !Bound(d) || !Quiet())scalar_fault_park();
 for(unsigned i=0;i<16;i++)if(Words(TT_Q38_GDN_GATED_READY)[i])scalar_fault_park();
 volatile uint32_t* s=Words(TT_Q38_GDN_GATED_SUMMARY);for(unsigned i=0;i<32;i++)s[i]=0;
 s[6]=d[6];s[7]=TT_Q38_GDN_GATED_MARKER;
 const unsigned tail[]={d[7],d[5],d[40],d[15],d[4]+d[43],d[3],d[11],d[12]};
 for(unsigned i=0;i<8;i++){s[8+i]=tracked[i];s[24+i]=tail[i];}
 // Publish ONLY after both actual NEXT writes and their command/counter
 // receipts. Raw output and this seal stay immutable through final hold.
 for(unsigned i=1;i<16;i++)Words(TT_Q38_GDN_GATED_READY)[i]=PioneerQ38GdnGatedReadyWord(d,i,false);
 Fence();Words(TT_Q38_GDN_GATED_READY)[0]=TT_Q38_GDN_GATED_MARKER;Fence();
 if(!Seal(d))scalar_fault_park();
 bool ready=false;
 for(unsigned poll=0;poll<TT_Q38_GDN_GATED_POLLS;poll++){
  Packet(0,d,tracked);s[5]++;
  if(!Words(TT_Q38_GDN_GATED_PEER_READY)[0])continue;
  for(unsigned i=0;i<16;i++)if(Words(TT_Q38_GDN_GATED_PEER_READY)[i]!=PioneerQ38GdnGatedReadyWord(d,i,true))scalar_fault_park();
  ready=true;break;
 }
 if(!ready)scalar_fault_park(); // bounded known-complete polls;no host wait or fallback
 for(unsigned i=0;i<64;i++)Words(0x80000+(d[7]&1)*256)[i]=Words(d[45])[i];
 Packet(1,d,tracked);Packet(2,d,tracked);Packet(3,d,tracked);
 if(!PioneerQ38GdnGatedNormHalf(reinterpret_cast<const float*>(0x80000),reinterpret_cast<const float*>(0x80200),
  reinterpret_cast<const float*>(0x80400),d[7]&1,reinterpret_cast<float*>(0x8ea00)))scalar_fault_park();
 for(unsigned i=0;i<64;i++)if(Words(TT_Q38_GDN_INPUT_DESCRIPTOR)[i]!=input[i] || Words(TT_Q38_RGDN_DESCRIPTOR)[i]!=state[i])scalar_fault_park();
 Packet(4,d,tracked);
 for(unsigned i=0;i<8;i++)s[16+i]=tracked[i];
 if(!Seal(d) || !Quiet())scalar_fault_park();
 Fence();s[4]=64;Fence();return 64; // private output DATA;NOT current-slot/token publication
}
