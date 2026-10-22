/* SPDX-License-Identifier: MIT */
// OFFLINE ROOT0 gather ONLY,not selected by any native driver. Hardware
// coordinator must first prove ALL20/68 producers+workspace owner independently.
// Leaves exact POST-EXPANSION native input in the existing root windows a0000/
// ab000 and compact scales in the SAME workspace. NO host/PCI numeric work.
// This is NOT compressed host staging or a legacy reuse-cache permission.
#define PIONEER_ATTENTION_FREESTANDING
#include "../../src/add-ons/kernel/drivers/misc/PioneerQ38FfnGather2Shape.h"
extern "C" void scalar_fault_park() __attribute__((noreturn));
namespace Gather {
static volatile uint32_t* Words(unsigned a){return reinterpret_cast<volatile uint32_t*>(a);}
static void Fence(){asm volatile("fence" ::: "memory");}
static unsigned Counter(unsigned i){const unsigned o[]={0x208,0x20c,0x210,0x214,0x204,0x220,0x210,0x228};return o[i];}
static unsigned Source(unsigned i){return !i?0x100:i<=6?0x118+(i-1)*4:i<=12?0x130+(i-7)*4:i==13?0x150:i==14?0x154:0x16c;}
static bool Sealed(const uint32_t* d) {
 bool same=true;for(unsigned i=0;i<16;i++)same &= *Words(0xffb20000+Source(i))==d[16+i];
 for(unsigned i=0;i<64;i++)same &= Words(TT_Q38_FFN_RESIDENT_DESCRIPTOR)[i]==d[i];
 return same;
}
static bool Idle() {
 bool idle=true;for(unsigned i=0;i<4;i++)idle &= !*Words(0xffb20040+i*0x800);
 for(unsigned i=0;i<32;i++)idle &= !(*Words(0xffb20240+i*4)&255);
 return idle;
}
// BOUNDED PRIVATE modes,never arbitrary local/remote addresses:
// mode0:one of20/68 scale lines;mode1:one of1/3 native chunks;mode2:final scales.
static void Packet(unsigned mode,unsigned index,const uint32_t* d) {
 if(mode>2 || (mode==0 && index>=d[5]) || (mode==1 && index>=(d[5]==20?1U:3U)) || (mode==2 && index)
  || !Sealed(d) || !Idle())scalar_fault_park();
 const bool write=mode==2;unsigned local,remote,bytes;
 if(!mode){local=0x90e00;remote=d[4]+d[11]+index*64;bytes=64;}
 else if(mode==1){local=0xa0000+index*16384;remote=d[4]+d[10]+index*16384;bytes=d[5]==20?10240U:index==2?2048U:16384U;}
 else{local=0xab000;remote=d[4]+d[12];bytes=d[14];}
 const unsigned delta[]={write?0U:1U,write?0U:bytes/64,1,write?0U:1U,write?1U:0U,write?bytes/64:0U,1,write?1U:0U};
 uint32_t expected[8];for(unsigned i=0;i<8;i++)expected[i]=*Words(0xffb20000+Counter(i))+delta[i];
 const uint32_t command[]={write?local:remote,0,write?d[9]:d[8],write?remote:local,0,write?d[8]:d[9],0,write?0x212U:0,bytes,0};
 for(unsigned i=0;i<10;i++)Words(0xffb20000)[i]=command[i];
 for(unsigned i=0;i<10;i++)if(Words(0xffb20000)[i]!=command[i])scalar_fault_park();
 Fence();*Words(0xffb20040)=1;(void)*Words(0xffb20040);Fence();
 for(unsigned attempt=0;attempt<65536;attempt++) {
  bool complete=!*Words(0xffb20040);for(unsigned i=0;i<8;i++)complete &= *Words(0xffb20000+Counter(i))==expected[i];
  if(complete){if(!Sealed(d) || !Idle())scalar_fault_park();Words(TT_Q38_FFN_RESIDENT_SUMMARY)[write?2:0]++;
   Words(TT_Q38_FFN_RESIDENT_SUMMARY)[write?3:1]+=bytes/64;Fence();return;}
 }
 scalar_fault_park(); // UNKNOWN terminal:NO later packet/retry/cleanup
}
}
extern "C" unsigned scalar_main()
{
 using namespace Gather;uint32_t d[64];for(unsigned i=0;i<64;i++)d[i]=Words(TT_Q38_FFN_RESIDENT_DESCRIPTOR)[i];
 if(!PioneerQ38FfnGather2Shape(d))return 0;
 if(!Sealed(d) || !Idle())scalar_fault_park();
 volatile uint32_t* s=Words(TT_Q38_FFN_RESIDENT_SUMMARY);for(unsigned i=0;i<32;i++)s[i]=0;
 s[5]=d[13];s[6]=d[6];s[7]=TT_Q38_FFN_GATHER2_MARKER;s[24]=d[3];s[25]=d[4];s[26]=d[10];s[27]=d[11];s[28]=d[12];s[29]=d[42];s[30]=129;s[31]=d[5];
 for(unsigned i=0;i<8;i++)s[8+i]=*Words(0xffb20000+Counter(i));
 for(unsigned i=0;i<d[14]/4;i++)Words(0xab000)[i]=0; // exact128/320B compact scales+zero padding
 for(unsigned block=0;block<d[5];block++) {
  Packet(0,block,d);uint32_t line[16];for(unsigned i=0;i<16;i++)line[i]=Words(0x90e00)[i];
  if(!(d[5]==20?PioneerQ38FfnInputScale(line,d,block):PioneerQ38FfnResidentScale(line,d,block)))scalar_fault_park();
  Words(0xab000)[block]=line[0];
 }
 // No operand read or compact-scale GDDR write until ALL fresh tags pass.
 for(unsigned chunk=0;chunk<(d[5]==20?1U:3U);chunk++)Packet(1,chunk,d);
 Packet(2,0,d);if(!Sealed(d) || !Idle())scalar_fault_park();
 for(unsigned i=0;i<8;i++)s[16+i]=*Words(0xffb20000+Counter(i));
 s[4]=d[5];Fence();return d[5]; // root input ready ONLY,NOT a matrix/FFN completion
}
