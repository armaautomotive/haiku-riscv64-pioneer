/* SPDX-License-Identifier: MIT */
// Separate advance component. Preserve each ACTUAL QPD1 producer's source
// identity; unlike the legacy homogeneous-source gather, ROOT is not peer0.
#include "PioneerQ38ResidentAdvanceImageRole.h"
#define PIONEER_ATTENTION_FREESTANDING
#include "../../src/add-ons/kernel/drivers/misc/PioneerQ38ResidentAdvanceShape.h"
#include "../../src/add-ons/kernel/drivers/misc/PioneerQ38ProjectionGatherShape.h"
extern "C" void cohort_fault_park() __attribute__((noreturn));
namespace AdvanceGather {
static volatile uint32_t* Words(unsigned a){return reinterpret_cast<volatile uint32_t*>(a);}
static void Fence(){asm volatile("fence" ::: "memory");}
static unsigned Counter(unsigned i){const unsigned o[]={0x208,0x20c,0x210,0x214,0x204,0x220,0x210,0x228};return o[i];}
static unsigned Source(unsigned i){return !i?0x100:i<=6?0x118+(i-1)*4:i<=12?0x130+(i-7)*4:i==13?0x150:i==14?0x154:0x16c;}
static bool Seal(const uint32_t* d){
 for(unsigned i=0;i<16;i++)if(*Words(0xffb20000+Source(i))!=d[16+i])return false;
 for(unsigned i=0;i<64;i++)if(Words(TT_Q38_FFN_RESIDENT_DESCRIPTOR)[i]!=d[i])return false;
 for(unsigned w=1;w<=20;w++)for(unsigned i=0;i<16;i++){
  unsigned n=TT_Q38_COHORT_SOURCE_WORD+w*16+i;
  if(Words(TT_Q38_COHORT_MANIFEST)[n]!=Words(TT_Q38_COHORT_SHADOW)[n])return false;
 }
 return true;
}
static bool Quiet(){
 for(unsigned i=0;i<4;i++)if(*Words(0xffb20040+i*0x800))return false;
 for(unsigned i=0;i<32;i++)if(*Words(0xffb20240+i*4)&255)return false;
 return true;
}
static void Packet(unsigned p,const uint32_t* d){
 if(p>=22 || !Seal(d) || !Quiet())cohort_fault_park();
 bool write=p==21;unsigned local=p<20?0x90e00:write?0xab000:0xa0000;
 unsigned remote=d[4]+(p<20?TT_Q38_PROJECTION_LINES_OFFSET+p*128:write?unsigned(TT_Q38_PROJECTION_COMPACT_OFFSET):unsigned(TT_Q38_PROJECTION_NATIVE_OFFSET));
 unsigned bytes=p<20?128:write?640:10240;
 uint32_t expected[8];for(unsigned i=0;i<8;i++){
  unsigned delta=i==2 || i==6?1U:i==0 || i==3?unsigned(!write):i==1?(!write?bytes/64:0U):i==5?(write?bytes/64:0U):unsigned(write);
  expected[i]=*Words(0xffb20000+Counter(i))+delta;if(i==6 && expected[6]!=expected[2])cohort_fault_park();
 }
 const uint32_t command[]={write?local:remote,0,write?129U:d[8],write?remote:local,0,write?d[8]:129U,0,write?0x212U:0,bytes,0};
 for(unsigned i=0;i<10;i++)Words(0xffb20000)[i]=command[i];
 for(unsigned i=0;i<10;i++)if(Words(0xffb20000)[i]!=command[i])cohort_fault_park();
 Fence();*Words(0xffb20040)=1;(void)*Words(0xffb20040);Fence();
 for(unsigned attempt=0;attempt<65536;attempt++){
  bool complete=!*Words(0xffb20040);for(unsigned i=0;i<8;i++)complete &= *Words(0xffb20000+Counter(i))==expected[i];
  if(complete){
   if(!Seal(d) || !Quiet())cohort_fault_park();
   for(unsigned i=0;i<10;i++)if(Words(0xffb20000)[i]!=command[i])cohort_fault_park();
   Words(TT_Q38_FFN_RESIDENT_SUMMARY)[write?2:0]++;Words(TT_Q38_FFN_RESIDENT_SUMMARY)[write?3:1]+=bytes/64;return;
  }
 }
 cohort_fault_park(); // UNKNOWN: no following packet, compact sink, reset or retry
}
}
extern "C" unsigned q38_resident_advance_gather_once(){
 using namespace AdvanceGather;uint32_t d[64];for(unsigned i=0;i<64;i++)d[i]=Words(TT_Q38_FFN_RESIDENT_DESCRIPTOR)[i];
 if(!PioneerQ38ProjectionGatherShape(d) || d[5]!=20 || d[13]!=10240 || d[14]!=640 || d[15]!=8 || !d[42]
  || d[42]>=64 || d[42]%4==3 || d[43]!=5 || d[40]>=8192 || d[41]>=248320)return 0;
 if(!Seal(d) || !Quiet())cohort_fault_park();
 auto* s=Words(TT_Q38_FFN_RESIDENT_SUMMARY);for(unsigned i=0;i<32;i++)s[i]=0;
 s[5]=10240;s[6]=d[6];s[7]=TT_Q38_PROJECTION_GATHER_MARKER;s[24]=d[3];s[25]=d[4];s[26]=d[42];s[27]=5;s[28]=8;s[29]=640;s[30]=5120;s[31]=129;
 for(unsigned i=0;i<8;i++){s[8+i]=*Words(0xffb20000+Counter(i));if(i==6 && s[8+i]!=s[10])cohort_fault_park();}
 for(unsigned i=0;i<160;i++)Words(0xab000)[i]=0;
 for(unsigned chunk=0;chunk<20;chunk++){
  Packet(chunk,d);uint32_t line[32],peer[64];
  for(unsigned i=0;i<32;i++)line[i]=Words(0x90e00)[i];
  for(unsigned i=0;i<64;i++)peer[i]=d[i];
  peer[0]=TT_Q38_PROJECTION_INPUT_MARKER;peer[5]=chunk;peer[7]=chunk+1;peer[9]=PioneerQ38ProjectionPeer(chunk);
  peer[10]=chunk*1024;peer[11]=5120;peer[12]=TT_Q38_PROJECTION_NATIVE_OFFSET+chunk*512;peer[13]=TT_Q38_PROJECTION_LINES_OFFSET+chunk*128;peer[14]=8;peer[15]=20;
  for(unsigned i=0;i<16;i++)peer[16+i]=Words(TT_Q38_COHORT_SHADOW)[TT_Q38_COHORT_SOURCE_WORD+(chunk+1)*16+i];
  if(!peer[16] || !PioneerQ38ProjectionInputLine(line,peer))cohort_fault_park();
  for(unsigned i=0;i<8;i++)Words(0xab000)[chunk*8+i]=line[i];
 }
 // Every QPD1 line, including its distinct producer's source, must pass
 // BEFORE any native operand fetch or compact-scale publication.
 Packet(20,d);Packet(21,d);if(!Seal(d) || !Quiet())cohort_fault_park();
 const unsigned delta[]={21,200,22,21,1,10,22,1};
 for(unsigned i=0;i<8;i++){s[16+i]=*Words(0xffb20000+Counter(i));if(uint32_t(s[16+i]-s[8+i])!=delta[i])cohort_fault_park();}
 s[4]=20;Fence();return 20;
}
