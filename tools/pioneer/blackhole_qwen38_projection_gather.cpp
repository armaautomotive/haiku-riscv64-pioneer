/* SPDX-License-Identifier: MIT */
// OFFLINE new ROOT gather:Q8_0 eight scales/chunk or Q8_K one. Native
// coordinator/registry/owner does NOT select this image. No CPU products.
#define PIONEER_ATTENTION_FREESTANDING
#include "../../src/add-ons/kernel/drivers/misc/PioneerQ38ProjectionGatherShape.h"
extern "C" void scalar_fault_park() __attribute__((noreturn));
namespace ProjectionGather {
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
// Private bounded modes ONLY:128B line,<=16KiB operand read,compact scales.
static void Packet(unsigned mode,unsigned index,const uint32_t* d){
 if(mode>2 || (mode==0 && index>=d[5]) || (mode==1 && index>=(d[13]+16383)/16384) || (mode==2 && index)
  || !Seal(d) || !Quiet())scalar_fault_park();
 bool write=mode==2;unsigned local,remote,bytes;
 if(!mode){local=0x90e00;remote=d[4]+d[11]+index*128;bytes=128;}
 else if(mode==1){local=0xa0000+index*16384;remote=d[4]+d[10]+index*16384;bytes=d[13]-index*16384;if(bytes>16384)bytes=16384;}
 else{local=0xab000;remote=d[4]+d[12];bytes=d[14];}
 const unsigned delta[]={write?0U:1U,write?0U:bytes/64,1,write?0U:1U,write?1U:0U,write?bytes/64:0U,1,write?1U:0U};
 uint32_t expected[8];for(unsigned i=0;i<8;i++){
  expected[i]=*Words(0xffb20000+Counter(i))+delta[i];if(i==6 && expected[6]!=expected[2])scalar_fault_park();
 }
 const uint32_t command[]={write?local:remote,0,write?d[9]:d[8],write?remote:local,0,write?d[8]:d[9],0,write?0x212U:0,bytes,0};
 for(unsigned i=0;i<10;i++)Words(0xffb20000)[i]=command[i];
 for(unsigned i=0;i<10;i++)if(Words(0xffb20000)[i]!=command[i])scalar_fault_park();
 Fence();*Words(0xffb20040)=1;(void)*Words(0xffb20040);Fence();
 for(unsigned attempt=0;attempt<65536;attempt++){
  bool complete=!*Words(0xffb20040);for(unsigned i=0;i<8;i++)complete &= *Words(0xffb20000+Counter(i))==expected[i];
  if(complete){if(!Seal(d) || !Quiet())scalar_fault_park();Words(TT_Q38_FFN_RESIDENT_SUMMARY)[write?2:0]++;
   Words(TT_Q38_FFN_RESIDENT_SUMMARY)[write?3:1]+=bytes/64;Fence();return;}
 }
 scalar_fault_park(); // UNKNOWN:ZERO next packet/retry/reset/restore
}
}
extern "C" unsigned scalar_main(){
 using namespace ProjectionGather;uint32_t d[64];for(unsigned i=0;i<64;i++)d[i]=Words(TT_Q38_FFN_RESIDENT_DESCRIPTOR)[i];
 if(!PioneerQ38ProjectionGatherShape(d))return 0;
 if(!Seal(d) || !Quiet())scalar_fault_park();
 volatile uint32_t* s=Words(TT_Q38_FFN_RESIDENT_SUMMARY);for(unsigned i=0;i<32;i++)s[i]=0;
 s[5]=d[13];s[6]=d[6];s[7]=TT_Q38_PROJECTION_GATHER_MARKER;s[24]=d[3];s[25]=d[4];s[26]=d[42];s[27]=d[43];
 s[28]=d[15];s[29]=d[14];s[30]=d[13]/2;s[31]=129;
 for(unsigned i=0;i<8;i++){s[8+i]=*Words(0xffb20000+Counter(i));if(i==6 && s[8+i]!=s[10])scalar_fault_park();}
 for(unsigned i=0;i<d[14]/4;i++)Words(0xab000)[i]=0;
 unsigned per=d[15]==8?8:1;
 for(unsigned chunk=0;chunk<d[5];chunk++){
  Packet(0,chunk,d);uint32_t line[32];for(unsigned i=0;i<32;i++)line[i]=Words(0x90e00)[i];
  if(!PioneerQ38ProjectionGatherLine(line,d,chunk))scalar_fault_park();
  for(unsigned i=0;i<per;i++)Words(0xab000)[chunk*per+i]=line[i];
 }
 // No operand read or compact sink until ALL chunk identities are fresh.
 for(unsigned chunk=0;chunk<(d[13]+16383)/16384;chunk++)Packet(1,chunk,d);
 Packet(2,0,d);if(!Seal(d) || !Quiet())scalar_fault_park();
 unsigned reads=d[5]+(d[13]+16383)/16384,flits=d[5]*2+d[13]/64;
 const unsigned delta[]={reads,flits,reads+1,reads,1,d[14]/64,reads+1,1};
 for(unsigned i=0;i<8;i++){
  s[16+i]=*Words(0xffb20000+Counter(i));if(uint32_t(s[16+i]-s[8+i])!=delta[i])scalar_fault_park();
  if(i==6 && s[16+i]!=s[18])scalar_fault_park();
 }
 s[4]=d[5];Fence();return d[5];
}
