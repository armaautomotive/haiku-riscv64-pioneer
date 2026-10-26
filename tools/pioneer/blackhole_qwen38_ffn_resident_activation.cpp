/* SPDX-License-Identifier: MIT */
// OFFLINE GDDR -> local fused activation/native packing -> GDDR ONLY.
// No driver selects it. Shape is not ownership;native use requires a NEW
// retained mutable-workspace owner and independently proven coordinator.
#define PIONEER_ATTENTION_FREESTANDING
#include "PioneerQ38MixedCapsuleHook.h"
#include "PioneerQ38FfnNativeInput.h"
#include "../../src/add-ons/kernel/drivers/misc/PioneerQ38FfnResidentShape.h"
extern "C" void scalar_fault_park() __attribute__((noreturn));
namespace FfnResident {
static volatile uint32_t* Words(unsigned a){return reinterpret_cast<volatile uint32_t*>(a);}
static void Fence(){asm volatile("fence" ::: "memory");}
static unsigned Counter(unsigned i){const unsigned o[]={0x208,0x20c,0x210,0x214,0x204,0x220,0x210,0x228};return o[i];}
static unsigned Source(unsigned i){return !i?0x100:i<=6?0x118+(i-1)*4:i<=12?0x130+(i-7)*4:i==13?0x150:i==14?0x154:0x16c;}
static bool Sealed(const uint32_t* d) {
 PIONEER_Q38_MIXED_SEAL();
 bool same=true;
 for(unsigned i=0;i<16;i++)same &= *Words(0xffb20000+Source(i))==d[16+i];
 for(unsigned i=0;i<64;i++)same &= Words(TT_Q38_FFN_RESIDENT_DESCRIPTOR)[i]==d[i];
 return same; // command word5 is NOT an independent physical identity
}
static bool Idle() {
 bool idle=true;
 for(unsigned i=0;i<4;i++)idle &= !*Words(0xffb20040+i*0x800);
 for(unsigned i=0;i<32;i++)idle &= !(*Words(0xffb20240+i*4)&255);
 return idle;
}
static void Packet(unsigned packet,const uint32_t* d) {
 if(packet>=4 || !Sealed(d) || !Idle())scalar_fault_park();
 const bool write=packet>=2;
 const unsigned local[]={0x90000,0x90400,0x90c00,0x90e00};
 const unsigned bytes[]={1024,1024,512,64};
 const unsigned size=bytes[packet],remote=d[4]+d[10+packet],here=local[packet];
 const unsigned delta[]={write?0U:1U,write?0U:size/64,1,write?0U:1U,write?1U:0U,write?size/64:0U,1,write?1U:0U};
 uint32_t expected[8];for(unsigned i=0;i<8;i++)expected[i]=*Words(0xffb20000+Counter(i))+delta[i];
 const uint32_t command[]={write?here:remote,0,write?d[9]:d[8],write?remote:here,0,write?d[8]:d[9],0,write?0x212U:0,size,0};
 for(unsigned i=0;i<10;i++)Words(0xffb20000)[i]=command[i];
 for(unsigned i=0;i<10;i++)if(Words(0xffb20000)[i]!=command[i])scalar_fault_park();
 Fence();*Words(0xffb20040)=1;(void)*Words(0xffb20040);Fence();
 for(unsigned attempt=0;attempt<65536;attempt++) {
  bool complete=!*Words(0xffb20040);
  for(unsigned i=0;i<8;i++)complete &= *Words(0xffb20000+Counter(i))==expected[i];
  if(complete) {
   if(!Idle() || !Sealed(d))scalar_fault_park();
   Words(TT_Q38_FFN_RESIDENT_SUMMARY)[write?2:0]++;
   Words(TT_Q38_FFN_RESIDENT_SUMMARY)[write?3:1]+=size/64;Fence();return;
  }
 }
 scalar_fault_park(); // UNKNOWN terminal:NO later packet/retry/cleanup
}
}
extern "C" unsigned scalar_main()
{
 using namespace FfnResident;
 uint32_t d[64];for(unsigned i=0;i<64;i++)d[i]=Words(TT_Q38_FFN_RESIDENT_DESCRIPTOR)[i];
 if(!PioneerQ38FfnResidentShape(d))return 0; // malformed shape:NO NoC MMIO
 if(!Sealed(d) || !Idle())scalar_fault_park();
 volatile uint32_t* s=Words(TT_Q38_FFN_RESIDENT_SUMMARY);for(unsigned i=0;i<32;i++)s[i]=0;
 s[5]=576;s[6]=d[6];s[7]=TT_Q38_FFN_RESIDENT_MARKER;
 s[24]=d[5];s[25]=d[3];s[26]=d[4];s[27]=d[8];s[28]=d[9];s[29]=d[42];s[30]=d[12];s[31]=d[13];
 for(unsigned i=0;i<8;i++)s[8+i]=*Words(0xffb20000+Counter(i));
 Packet(0,d);Packet(1,d);
 if(!PioneerQ38FfnNativeActivation(reinterpret_cast<const float*>(0x90000),reinterpret_cast<const float*>(0x90400),256,
  reinterpret_cast<float*>(0x90800),reinterpret_cast<PioneerQ38FfnNativeInput*>(0x90c00)))scalar_fault_park();
 // PRIVATE scale line,not native matrix padding. Root compacts ONLY word0
 // after all producer completions. Bind every line to this exact operation
 // so a prior token/layer/owner cannot be mistaken for fresh output.
 volatile uint32_t* line=Words(0x90e00);line[1]=TT_Q38_FFN_RESIDENT_MARKER;line[2]=d[6];line[3]=d[5];line[4]=d[42];
 line[5]=d[40];line[6]=d[41];for(unsigned i=0;i<8;i++)line[7+i]=d[32+i];line[15]=0;
 Packet(2,d);Packet(3,d);
 if(!Sealed(d) || !Idle())scalar_fault_park();
 for(unsigned i=0;i<8;i++)s[16+i]=*Words(0xffb20000+Counter(i));
 s[4]=256;Fence();return 256; // one block ONLY;root owes scale gather+down
}
