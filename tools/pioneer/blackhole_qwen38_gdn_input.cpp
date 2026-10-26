/* SPDX-License-Identifier: MIT */
// OFFLINE resident raw-input -> local GDN preparation. Not a native grant.
// Reuse the qualified QGP1 math IN THIS worker,not a host stop/reload path.
#define PIONEER_Q38_GDN_INPUT_IMAGE 1
#include "PioneerQ38MixedCapsuleHook.h"
#define scalar_main q38_local_gdn_prepare
#include "blackhole_qwen38_gdn_prepare.cpp"
#undef scalar_main
#include "../../src/add-ons/kernel/drivers/misc/PioneerQ38GdnInputShape.h"
namespace GdnInput {
static unsigned Counter(unsigned i){const unsigned o[]={0x208,0x20c,0x210,0x214,0x204,0x220,0x210,0x228};return o[i];}
static unsigned Source(unsigned i){return !i?0x100:i<=6?0x118+(i-1)*4:i<=12?0x130+(i-7)*4:i==13?0x150:i==14?0x154:0x16c;}
static void Fence(){asm volatile("fence" ::: "memory");}
static bool Seal(const uint32_t* d){
 PIONEER_Q38_MIXED_SEAL();
 for(unsigned i=0;i<16;i++)if(*Words(0xffb20000+Source(i))!=d[16+i])return false;
 for(unsigned i=0;i<64;i++)if(Words(TT_Q38_GDN_INPUT_DESCRIPTOR)[i]!=d[i])return false;
 return true;
}
static bool Quiet(){
 for(unsigned i=0;i<4;i++)if(*Words(0xffb20040+i*0x800))return false;
 for(unsigned i=0;i<32;i++)if(*Words(0xffb20240+i*4)&255)return false;
 return true;
}
static __attribute__((noinline)) void Read(unsigned p,const uint32_t* d,uint32_t* tracked){
 if(p>=7 || !Seal(d) || !Quiet())scalar_fault_park();
 const unsigned local[]={0x80000,0x80200,0x80400,0x80800,0x80840,0x80880,0x808c0},size[]={512,512,256,64,64,64,64};
 const unsigned remote[]={d[4]+d[42],d[4]+d[43],d[4]+d[44],d[4]+d[45],d[4]+d[46],d[12],d[15]};
 const unsigned route[]={d[8],d[8],d[8],d[8],d[8],d[13],d[41]};
 for(unsigned i=0;i<size[p]/4;i++){
  unsigned poison=0x7fc60000+p*1024+i;Words(local[p])[i]=poison;if(Words(local[p])[i]!=poison)scalar_fault_park();
 }
 uint32_t expected[8];const unsigned delta[]={1,size[p]/64,1,1,0,0,1,0};
 for(unsigned i=0;i<8;i++){
  if(*Words(0xffb20000+Counter(i))!=tracked[i])scalar_fault_park();
  expected[i]=tracked[i]+delta[i];if(i==6 && expected[2]!=expected[6])scalar_fault_park();
 }
 const unsigned command[]={remote[p],0,route[p],local[p],0,d[9],0,0,size[p],0};
 for(unsigned i=0;i<10;i++)Words(0xffb20000)[i]=command[i];
 for(unsigned i=0;i<10;i++)if(Words(0xffb20000)[i]!=command[i])scalar_fault_park();
 Fence();*Words(0xffb20040)=1;(void)*Words(0xffb20040);Fence();
 for(unsigned attempt=0;attempt<65536;attempt++){
  bool complete=!*Words(0xffb20040);
  for(unsigned i=0;i<8;i++)complete &= *Words(0xffb20000+Counter(i))==expected[i];
  if(complete){
   if(!Seal(d) || !Quiet())scalar_fault_park();
   for(unsigned i=0;i<10;i++)if(Words(0xffb20000)[i]!=command[i])scalar_fault_park();
   for(unsigned i=0;i<size[p]/4;i++)if(!PioneerQ38PrepareNormal(PioneerAttentionFloat(Words(local[p])[i])))scalar_fault_park();
   for(unsigned i=0;i<8;i++)tracked[i]=expected[i];
   Words(TT_Q38_GDN_INPUT_SUMMARY)[0]++;Words(TT_Q38_GDN_INPUT_SUMMARY)[1]+=size[p]/64;Fence();return;
  }
 }
 scalar_fault_park(); // UNKNOWN,NO later packet/reset/restore/retry
}
}
#ifdef PIONEER_Q38_GDN_INPUT_COMBINED_STAGE
#if PIONEER_Q38_GDN_INPUT_COMBINED_STAGE != 1
#error Exact offline combined-input stage role required
#endif
extern "C" bool q38_resident_gdn_input_shape(const uint32_t*);
extern "C" unsigned q38_resident_gdn_input_stage(){
#else
extern "C" unsigned scalar_main(){
#endif
 using namespace GdnInput;uint32_t d[64],tracked[8];
 for(unsigned i=0;i<64;i++)d[i]=Words(TT_Q38_GDN_INPUT_DESCRIPTOR)[i];
#ifdef PIONEER_Q38_GDN_INPUT_COMBINED_STAGE
 if(!q38_resident_gdn_input_shape(d))return 0;
#else
 if(!PioneerQ38GdnInputShape(d))return 0;
#endif
 // malformed DATA:NO NoC I/O
 if(!Seal(d) || !Quiet())scalar_fault_park();
 volatile uint32_t* s=Words(TT_Q38_GDN_INPUT_SUMMARY);for(unsigned i=0;i<32;i++)s[i]=0;
 s[5]=1536;s[6]=d[6];s[7]=TT_Q38_GDN_INPUT_MARKER;
 const unsigned tail[]={d[7],d[5],d[40],d[10],d[11],d[14],d[42],d[44]};
 for(unsigned i=0;i<8;i++)s[24+i]=tail[i];
 for(unsigned i=0;i<8;i++){tracked[i]=s[8+i]=*Words(0xffb20000+Counter(i));if(i==6 && tracked[6]!=tracked[2])scalar_fault_park();}
 for(unsigned p=0;p<7;p++)Read(p,d,tracked);
 for(unsigned i=0;i<4;i++)Words(0x80500)[i]=Words(0x80800+i*64)[d[47]];
 for(unsigned i=0;i<32;i++)Words(0x74600)[i]=i==0?0x31504751:i==1?128:i==2?64:i==3?d[7]:0;
 if(q38_local_gdn_prepare()!=64 || !Seal(d) || !Quiet())scalar_fault_park();
 const unsigned delta[]={7,24,7,7,0,0,7,0};
 for(unsigned i=0;i<8;i++){
  s[16+i]=*Words(0xffb20000+Counter(i));if(s[16+i]!=tracked[i] || uint32_t(s[16+i]-s[8+i])!=delta[i])scalar_fault_park();
 }
 s[4]=322;Fence();return 64; // LOCAL data only,NOT GDN state read/commit or native producer
}
