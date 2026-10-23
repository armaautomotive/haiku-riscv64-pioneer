/* SPDX-License-Identifier: MIT */
// OFFLINE QCV1 parallel16channel consumer candidate,NO native selection.
// Reads resident QPS1 QKV/original F32 weights/previous state on CARD;
// writes only private next state+convolution result. No PCI/DMA/host math.
#define PIONEER_ATTENTION_FREESTANDING
#include "PioneerQ38ConvMath.h"
#include "../../src/add-ons/kernel/drivers/misc/PioneerQ38ConvShape.h"
#ifdef PIONEER_Q38_CONV_BATCH_IMAGE
#if PIONEER_Q38_CONV_BATCH_IMAGE != 1
#error Exact separate QCB1 offline image only
#endif
#include "../../src/add-ons/kernel/drivers/misc/PioneerQ38ConvBatchShape.h"
#endif
extern "C" void scalar_fault_park() __attribute__((noreturn));
namespace Conv {
static volatile uint32_t* Words(unsigned a){return reinterpret_cast<volatile uint32_t*>(a);}
static void Fence(){asm volatile("fence" ::: "memory");}
static unsigned Counter(unsigned i){const unsigned o[]={0x208,0x20c,0x210,0x214,0x204,0x220,0x210,0x228};return o[i];}
static unsigned Source(unsigned i){return !i?0x100:i<=6?0x118+(i-1)*4:i<=12?0x130+(i-7)*4:i==13?0x150:i==14?0x154:0x16c;}
static bool Seal(const uint32_t* d
#ifdef PIONEER_Q38_CONV_BATCH_IMAGE
 ,const uint32_t* table
#endif
 ){
 for(unsigned i=0;i<16;i++)if(*Words(0xffb20000+Source(i))!=d[16+i])return false;
#ifdef PIONEER_Q38_CONV_BATCH_IMAGE
 for(unsigned i=0;i<384;i++)if(Words(TT_Q38_CONV_BATCH_TABLE)[i]!=table[i])return false;
#else
 for(unsigned i=0;i<64;i++)if(Words(TT_Q38_FFN_RESIDENT_DESCRIPTOR)[i]!=d[i])return false;
#endif
 return true;
}
static bool Quiet(){
 for(unsigned i=0;i<4;i++)if(*Words(0xffb20040+i*0x800))return false;
 for(unsigned i=0;i<32;i++)if(*Words(0xffb20240+i*4)&255)return false;
 return true;
}
static void Packet(unsigned index,const uint32_t* d
#ifdef PIONEER_Q38_CONV_BATCH_IMAGE
 ,const uint32_t* table,uint32_t* tracked
#endif
 ){
 if(index>=5 || !Seal(d
#ifdef PIONEER_Q38_CONV_BATCH_IMAGE
  ,table
#endif
  ) || !Quiet())scalar_fault_park();
 const bool write=index>=3;
 const unsigned locals[]={0x80000,0x80400,0x80800,0x80c00,0x81000},sizes[]={64,256,192,192,64};
 const unsigned remotes[]={d[4]+d[45],d[12],d[15],d[4]+d[42],d[4]+d[43]},routes[]={d[8],d[13],d[41],d[8],d[8]};
 unsigned local=locals[index],bytes=sizes[index],remote=remotes[index],route=routes[index];
 if(!write)for(unsigned i=0;i<bytes/4;i++){
  unsigned poison=0x7fc50000+index*1024+i;Words(local)[i]=poison;if(Words(local)[i]!=poison)scalar_fault_park();
 }
 const unsigned delta[]={write?0U:1U,write?0U:bytes/64,1,write?0U:1U,write?1U:0U,write?bytes/64:0U,1,write?1U:0U};
 uint32_t expected[8];for(unsigned i=0;i<8;i++){
#ifdef PIONEER_Q38_CONV_BATCH_IMAGE
  if(*Words(0xffb20000+Counter(i))!=tracked[i])scalar_fault_park();
  expected[i]=tracked[i]+delta[i];if(i==6 && expected[6]!=expected[2])scalar_fault_park();
#else
  expected[i]=*Words(0xffb20000+Counter(i))+delta[i];if(i==6 && expected[6]!=expected[2])scalar_fault_park();
#endif
 }
 const uint32_t command[]={write?local:remote,0,write?d[9]:route,write?remote:local,0,write?route:d[9],0,write?0x212U:0,bytes,0};
 for(unsigned i=0;i<10;i++)Words(0xffb20000)[i]=command[i];
 for(unsigned i=0;i<10;i++)if(Words(0xffb20000)[i]!=command[i])scalar_fault_park();
 Fence();*Words(0xffb20040)=1;(void)*Words(0xffb20040);Fence();
 for(unsigned poll=0;poll<65536;poll++){
  bool complete=!*Words(0xffb20040);for(unsigned i=0;i<8;i++)complete &= *Words(0xffb20000+Counter(i))==expected[i];
  if(complete){if(!Seal(d
#ifdef PIONEER_Q38_CONV_BATCH_IMAGE
    ,table
#endif
    ) || !Quiet())scalar_fault_park();
#ifdef PIONEER_Q38_CONV_BATCH_IMAGE
   for(unsigned i=0;i<10;i++)if(Words(0xffb20000)[i]!=command[i])scalar_fault_park();
#endif
   if(!write)for(unsigned i=0;i<bytes/4;i++)if(!PioneerQ38ConvDomain(PioneerAttentionFloat(Words(local)[i])))scalar_fault_park();
#ifdef PIONEER_Q38_CONV_BATCH_IMAGE
   for(unsigned i=0;i<8;i++)tracked[i]=expected[i]; // ONLY after exact completion/data/source/command checks
#endif
   Words(TT_Q38_FFN_RESIDENT_SUMMARY)[write?2:0]++;
   Words(TT_Q38_FFN_RESIDENT_SUMMARY)[write?3:1]+=bytes/64;Fence();return;}
 }
 scalar_fault_park(); // UNKNOWN ->inert fault terminal,NO later packet/retry
}
}
#ifndef PIONEER_Q38_CONV_BATCH_IMAGE
extern "C" unsigned scalar_main(){
 using namespace Conv;uint32_t d[64];for(unsigned i=0;i<64;i++)d[i]=Words(TT_Q38_FFN_RESIDENT_DESCRIPTOR)[i];
 if(!PioneerQ38ConvShape(d))return 0; // malformed DATA does NO NoC I/O
 if(!Seal(d) || !Quiet())scalar_fault_park();
 volatile uint32_t* s=Words(TT_Q38_FFN_RESIDENT_SUMMARY);for(unsigned i=0;i<32;i++)s[i]=0;
 s[5]=256;s[6]=d[6];s[7]=TT_Q38_CONV_MARKER;
 const unsigned tail[]={d[7],d[5],d[40],d[10],d[11],d[14],d[42],d[43]};for(unsigned i=0;i<8;i++)s[24+i]=tail[i];
 for(unsigned i=0;i<8;i++){s[8+i]=*Words(0xffb20000+Counter(i));if(i==6 && s[8+i]!=s[10])scalar_fault_park();}
 for(unsigned i=0;i<3;i++)Packet(i,d);
 if(!PioneerQ38Conv16(reinterpret_cast<const float*>(0x80000),reinterpret_cast<const float*>(0x80400),
  reinterpret_cast<const float*>(0x80800),reinterpret_cast<float*>(0x80c00),reinterpret_cast<float*>(0x81000)))scalar_fault_park();
 // ALL local results/next state verified BEFORE FIRST private GDDR write.
 Packet(3,d);Packet(4,d);
 if(!Seal(d) || !Quiet())scalar_fault_park();
 const unsigned delta[]={3,8,5,3,2,4,5,2};
 for(unsigned i=0;i<8;i++){s[16+i]=*Words(0xffb20000+Counter(i));if(uint32_t(s[16+i]-s[8+i])!=delta[i]
  || (i==6 && s[16+i]!=s[18]))scalar_fault_park();}
 s[4]=16;Fence();return 16;
}
#else
extern "C" unsigned scalar_main(){
 using namespace Conv;uint32_t table[384],tracked[8];
 for(unsigned i=0;i<384;i++)table[i]=Words(TT_Q38_CONV_BATCH_TABLE)[i];
 // ALL active records and inactive zero records are checked BEFORE ANY
 // NoC I/O. No caller-selected table address or mutable job-count hint.
 if(!PioneerQ38ConvBatchShape(table))return 0;
 unsigned count=PioneerQ38ConvBatchCount(table[10]);
 if(!Seal(table,table) || !Quiet())scalar_fault_park();
 volatile uint32_t* s=Words(TT_Q38_FFN_RESIDENT_SUMMARY);for(unsigned i=0;i<32;i++)s[i]=0;
 s[5]=count*256;s[6]=table[6];s[7]=TT_Q38_CONV_BATCH_MARKER;
 const unsigned tail[]={table[10],table[5],table[40],count,table[7],table[(count-1)*64+7],TT_Q38_CONV_NEXT,TT_Q38_CONV_OUTPUT};
 for(unsigned i=0;i<8;i++)s[24+i]=tail[i];
 for(unsigned i=0;i<8;i++){
  tracked[i]=s[8+i]=*Words(0xffb20000+Counter(i));if(i==6 && tracked[6]!=tracked[2])scalar_fault_park();
 }
 for(unsigned job=0;job<count;job++){
  const auto* d=table+job*64;
  for(unsigned i=0;i<3;i++)Packet(i,d,table,tracked);
  if(!PioneerQ38Conv16(reinterpret_cast<const float*>(0x80000),reinterpret_cast<const float*>(0x80400),
   reinterpret_cast<const float*>(0x80800),reinterpret_cast<float*>(0x80c00),reinterpret_cast<float*>(0x81000)))scalar_fault_park();
  Packet(3,d,table,tracked);Packet(4,d,table,tracked);
 }
 if(!Seal(table,table) || !Quiet())scalar_fault_park();
 const unsigned delta[]={3*count,8*count,5*count,3*count,2*count,4*count,5*count,2*count};
 for(unsigned i=0;i<8;i++){
  s[16+i]=*Words(0xffb20000+Counter(i));if(s[16+i]!=tracked[i] || uint32_t(s[16+i]-s[8+i])!=delta[i]
   || (i==6 && s[16+i]!=s[18]))scalar_fault_park();
 }
 s[4]=16*count;Fence();return 16*count;
}
#endif
