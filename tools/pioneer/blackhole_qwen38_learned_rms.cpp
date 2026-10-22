/* SPDX-License-Identifier: MIT */
// QRN1 fixed hidden learned RMS/residual backup. OFFLINE until a real
// native owner consumes the SAME opaque input producer and retains ALL
// memory/source/clock/host/learned owners through physical restore CLOSE.
#define PIONEER_ATTENTION_FREESTANDING
#include "PioneerQ38LearnedRms.h"
#include "../../src/add-ons/kernel/drivers/misc/PioneerQ38LearnedRmsShape.h"
extern "C" void scalar_fault_park() __attribute__((noreturn));
namespace LearnedRms {
static volatile uint32_t* Words(unsigned a){return reinterpret_cast<volatile uint32_t*>(a);}
static void Fence(){asm volatile("fence" ::: "memory");}
static unsigned Counter(unsigned i){const unsigned o[]={0x208,0x20c,0x210,0x214,0x204,0x220,0x210,0x228};return o[i];}
static unsigned Source(unsigned i){return !i?0x100:i<=6?0x118+(i-1)*4:i<=12?0x130+(i-7)*4:i==13?0x150:i==14?0x154:0x16c;}
static bool Seal(const uint32_t* d){
 for(unsigned i=0;i<16;i++)if(*Words(0xffb20000+Source(i))!=d[16+i])return false;
 for(unsigned i=0;i<64;i++)if(Words(TT_Q38_FFN_RESIDENT_DESCRIPTOR)[i]!=d[i])return false;
 return true;
}
static bool Quiet(){
 for(unsigned i=0;i<4;i++)if(*Words(0xffb20040+i*0x800))return false;
 for(unsigned i=0;i<32;i++)if(*Words(0xffb20240+i*4)&255)return false;
 return true;
}
static void Packet(unsigned index,const uint32_t* d){
 if(index>=8 || !Seal(d) || !Quiet())scalar_fault_park();
 bool write=index>=4,learned=index>=2 && index<4,normalized=index>=6;
 unsigned piece=index&1,offset=piece*16384,bytes=piece?4096:16384;
 unsigned local=(write?(normalized?0x90000:0x80000):learned?0x88000:0x80000)+offset;
 unsigned remote=(learned?d[11]:d[4])+offset+(write && !normalized?TT_Q38_RMS_BACKUP:0);
 unsigned route=learned?d[12]:d[8];
 const unsigned delta[]={write?0U:1U,write?0U:bytes/64,1,write?0U:1U,write?1U:0U,write?bytes/64:0U,1,write?1U:0U};
 uint32_t expected[8];for(unsigned i=0;i<8;i++){expected[i]=*Words(0xffb20000+Counter(i))+delta[i];if(i==6 && expected[6]!=expected[2])scalar_fault_park();}
 const uint32_t command[]={write?local:remote,0,write?129U:route,write?remote:local,0,write?route:129U,0,write?0x212U:0,bytes,0};
 for(unsigned i=0;i<10;i++)Words(0xffb20000)[i]=command[i];
 for(unsigned i=0;i<10;i++)if(Words(0xffb20000)[i]!=command[i])scalar_fault_park();
 Fence();*Words(0xffb20040)=1;(void)*Words(0xffb20040);Fence();
 for(unsigned poll=0;poll<65536;poll++){
  bool complete=!*Words(0xffb20040);for(unsigned i=0;i<8;i++)complete &= *Words(0xffb20000+Counter(i))==expected[i];
  if(complete){if(!Seal(d) || !Quiet())scalar_fault_park();Words(TT_Q38_FFN_RESIDENT_SUMMARY)[write?2:0]++;
   Words(TT_Q38_FFN_RESIDENT_SUMMARY)[write?3:1]+=bytes/64;Fence();return;}
 }
 scalar_fault_park();
}
}
extern "C" unsigned scalar_main(){
 using namespace LearnedRms;uint32_t d[64];for(unsigned i=0;i<64;i++)d[i]=Words(TT_Q38_FFN_RESIDENT_DESCRIPTOR)[i];
 if(!PioneerQ38LearnedRmsShape(d))return 0;
 if(!Seal(d) || !Quiet())scalar_fault_park();
 volatile uint32_t* s=Words(TT_Q38_FFN_RESIDENT_SUMMARY);for(unsigned i=0;i<32;i++)s[i]=0;
 s[5]=20480;s[6]=d[6];s[7]=TT_Q38_RMS_MARKER;s[24]=d[3];s[25]=d[4];s[26]=d[5];s[27]=d[7];
 s[28]=d[10];s[29]=d[11];s[30]=d[40];s[31]=TT_Q38_RMS_BACKUP;
 for(unsigned i=0;i<8;i++){s[8+i]=*Words(0xffb20000+Counter(i));if(i==6 && s[8+i]!=s[10])scalar_fault_park();}
 for(unsigned i=0;i<4;i++)Packet(i,d);
 if(!PioneerQ38LearnedRms(reinterpret_cast<const float*>(0x80000),reinterpret_cast<const float*>(0x88000),reinterpret_cast<float*>(0x90000)))scalar_fault_park();
 // BOTH original residual backup and normalized input are device-local.
 // ALL5120 math/domain checks finish BEFORE FIRST GDDR write/publication.
 for(unsigned i=4;i<8;i++)Packet(i,d);
 if(!Seal(d) || !Quiet())scalar_fault_park();
 const unsigned delta[]={4,640,8,4,4,640,8,4};
 for(unsigned i=0;i<8;i++){s[16+i]=*Words(0xffb20000+Counter(i));if(uint32_t(s[16+i]-s[8+i])!=delta[i])scalar_fault_park();if(i==6 && s[16+i]!=s[18])scalar_fault_park();}
 s[4]=5120;Fence();return 5120;
}
