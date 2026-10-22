/* SPDX-License-Identifier: MIT */
// OFFLINE ROOT0 compact-output -> private FFN GDDR scatter stage ONLY.
// Not selected by any driver. No host/PCI destination or CPU numeric work.
// Future coordinator must prove EVERY matrix participant BEFORE entry and
// preserve exclusive ownership of source payload/map/workspace throughout.
#define PIONEER_ATTENTION_FREESTANDING
#include "../../src/add-ons/kernel/drivers/misc/PioneerQ38FfnOutputShape.h"
extern "C" void scalar_fault_park() __attribute__((noreturn));
namespace FfnOutput {
static volatile uint32_t* Words(unsigned a){return reinterpret_cast<volatile uint32_t*>(a);}
static void Fence(){asm volatile("fence" ::: "memory");}
static unsigned Counter(unsigned i){const unsigned o[]={0x208,0x20c,0x210,0x214,0x204,0x220,0x210,0x228};return o[i];}
static unsigned Source(unsigned i){return !i?0x100:i<=6?0x118+(i-1)*4:i<=12?0x130+(i-7)*4:i==13?0x150:i==14?0x154:0x16c;}
static bool Sealed(const uint32_t* d,const uint32_t map[119][4]) {
 bool same=true;for(unsigned i=0;i<16;i++)same &= *Words(0xffb20000+Source(i))==d[16+i];
 for(unsigned i=0;i<64;i++)same &= Words(TT_Q38_FFN_RESIDENT_DESCRIPTOR)[i]==d[i];
 for(unsigned w=0;w<119;w++)for(unsigned i=0;i<4;i++)same &= Words(TT_Q38_FFN_OUTPUT_MAP)[w*4+i]==map[w][i];
 return same;
}
static bool Idle() {
 bool idle=true;for(unsigned i=0;i<4;i++)idle &= !*Words(0xffb20040+i*0x800);
 for(unsigned i=0;i<32;i++)idle &= !(*Words(0xffb20240+i*4)&255);
 return idle;
}
static void Packet(unsigned worker,const uint32_t* d,const uint32_t map[119][4]) {
 if(worker>=d[7] || !Sealed(d,map) || !Idle())scalar_fault_park();
 const uint32_t* p=map[worker];unsigned bytes=p[1]*4;
 const uint32_t command[]={TT_Q38_FFN_OUTPUT_PACKED+p[2]*4,0,129,d[4]+d[10]+p[0]*4,0,d[8],0,0x212,bytes,0};
 const unsigned delta[]={0,0,1,0,1,bytes/64,1,1};uint32_t expected[8];
 for(unsigned i=0;i<8;i++)expected[i]=*Words(0xffb20000+Counter(i))+delta[i];
 for(unsigned i=0;i<10;i++)Words(0xffb20000)[i]=command[i];
 for(unsigned i=0;i<10;i++)if(Words(0xffb20000)[i]!=command[i])scalar_fault_park();
 Fence();*Words(0xffb20040)=1;(void)*Words(0xffb20040);Fence();
 for(unsigned attempt=0;attempt<65536;attempt++) {
  bool complete=!*Words(0xffb20040);for(unsigned i=0;i<8;i++)complete &= *Words(0xffb20000+Counter(i))==expected[i];
  if(complete){if(!Sealed(d,map) || !Idle())scalar_fault_park();
   Words(TT_Q38_FFN_RESIDENT_SUMMARY)[2]++;Words(TT_Q38_FFN_RESIDENT_SUMMARY)[3]+=bytes/64;Fence();return;}
 }
 scalar_fault_park(); // UNKNOWN terminal:no subsequent read/packet/retry/cleanup
}
}
extern "C" unsigned scalar_main()
{
 using namespace FfnOutput;uint32_t d[64],map[119][4];
 for(unsigned i=0;i<64;i++)d[i]=Words(TT_Q38_FFN_RESIDENT_DESCRIPTOR)[i];
 if(!PioneerQ38FfnOutputShape(d))return 0;
 for(unsigned w=0;w<119;w++)for(unsigned i=0;i<4;i++)map[w][i]=Words(TT_Q38_FFN_OUTPUT_MAP)[w*4+i];
 if(!PioneerQ38FfnOutputMap(d,map))return 0; // ENTIRE map checked BEFORE NoC access
 // ALL finite numeric words checked BEFORE the first write,not lazily per
 // chunk. This does not validate matrix arithmetic or replace peer proofs.
 for(unsigned i=0;i<d[11];i++)if((Words(TT_Q38_FFN_OUTPUT_PACKED)[i]&0x7f800000)==0x7f800000)return 0;
 if(!Sealed(d,map) || !Idle())scalar_fault_park();
 volatile uint32_t* s=Words(TT_Q38_FFN_RESIDENT_SUMMARY);for(unsigned i=0;i<32;i++)s[i]=0;
 s[5]=d[12];s[6]=d[6];s[7]=TT_Q38_FFN_OUTPUT_MARKER;s[24]=d[5];s[25]=d[3];s[26]=d[4];
 s[27]=d[10];s[28]=d[8];s[29]=d[42];s[30]=129;s[31]=d[7];
 for(unsigned i=0;i<8;i++)s[8+i]=*Words(0xffb20000+Counter(i));
 for(unsigned w=0;w<d[7];w++)Packet(w,d,map);
 if(!Sealed(d,map) || !Idle())scalar_fault_park();
 for(unsigned i=0;i<8;i++)s[16+i]=*Words(0xffb20000+Counter(i));
 s[4]=d[11];Fence();return d[11]; // DATA sink only,NOT whole FFN/model completion
}
