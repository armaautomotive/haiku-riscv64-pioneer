/* SPDX-License-Identifier: MIT */
// OFFLINE standalone instruction image ONLY. No driver selects this image
// or its NEW256B Delta extent. Fixed local L1/import/Dst/CSR;NO NoC/DMA/PCI.
typedef __UINT32_TYPE__ uint32_t;
#define PIONEER_ATTENTION_FREESTANDING
#define PIONEER_Q38_WIDE_DELTA_LOCAL_IMAGE 1
#include "PioneerQ38WideDeltaMath.h"
#include "PioneerQ38WideDeltaLocalLayout.h"
namespace W=PioneerQ38WideDeltaLocalLayout;
extern "C" void scalar_fault_park() __attribute__((noreturn));
static volatile uint32_t* Words(uint32_t at){return reinterpret_cast<volatile uint32_t*>(at);}
static inline void Fence(){asm volatile("fence" ::: "memory");}
static inline void Emit(uint32_t op){*Words(0xffe40000)=op;}
static void Drain()
{
 volatile uint32_t* ack=Words(0xffe000fc);*ack=0;(void)*ack;Fence();
 Emit(0xa2101816);Emit(0x4500017e);
 for(unsigned attempt=0;attempt<1000000;attempt++)if(*ack){Fence();return;}
 scalar_fault_park();
}
static void Config(unsigned index,uint32_t value)
{
 volatile uint32_t* at=Words(0xffef0000+index*4);*at=value;if(*at!=value)scalar_fault_park();
}
static bool Finite(uint32_t x){return (x&0x7f800000U)!=0x7f800000U;}
static bool Domain(uint32_t x,uint32_t bound)
{
 const uint32_t magnitude=x&0x7fffffffU;
 return magnitude<=bound && (!magnitude || (magnitude&0x7f800000U));
}
static void Setup()
{
 Emit(0xb2000000);Drain();
 for(unsigned i=1;i<68;i++)Emit(0xb2000000|(i<<16));
 Emit(0xb2020001);Emit(0xb2030001);Emit(0xb2050000);Drain();
 static const unsigned index[]={0,1,2,3,6,7,220,20,24,28,49,50,56,59,64,65,72,73,76,84,86,112,113,120,121,124};
 static const uint32_t value[]={0x4231,0xa0200000,0,0,0,0,0,0,0xffff,0x100,0,0x100,
  0x100000,0x100,0x100010,16,0,0x13,0x8fff,0x400040,0x100010,0x100011,8,1,3,0x7091};
 for(unsigned i=0;i<sizeof(index)/sizeof(index[0]);i++)Config(index[i],value[i]);
}
static void Import(uint32_t source,unsigned row,unsigned words)
{
 Emit(0xb2050000);Drain();
 Config(76,(source-16)/16);Config(84,0x400000+(row+4)*16);
 Emit(0x5e200000|((words-1)<<10));Emit(0x5120000a);Emit(0x5420000f);
 Emit(0x42000081);Drain();
}
static void Math(){Emit(0xb2050004);Drain();}
struct Emitter {void Op(uint32_t op){Emit(op);}void Drain(){::Drain();}};
static void Dot(Emitter& e,unsigned parity,bool prediction,uint32_t decay)
{
 PioneerSfpuLoad32(e,6,0);PioneerSfpuLoad32(e,7,0);
 for(unsigned key=0;key<128;key++){
  Emit(0x70030000|(key*4)|parity);
  if(prediction){PioneerSfpuLoad32(e,1,decay);PioneerQ38VectorAffineMul(e,0,1,0);}
  PioneerSfpuLoad32(e,1,Words(prediction?W::Key:W::Query)[key]);
  PioneerQ38WideDeltaProduct(e);PioneerQ38WideDeltaAccumulate(e);
 }
 PioneerQ38VectorAffineAdd(e,6,7,6);
}
extern "C" unsigned scalar_main()
{
 const volatile uint32_t* h=Words(W::Descriptor);
 if(h[0]!=W::Magic || h[1]!=128 || h[2]!=64 || h[3])return 0;
 const uint32_t decay=*Words(W::Decay),beta=*Words(W::Beta);
 if(decay>0x3f800000U || beta>0x3f800000U || !Domain(decay,0x3f800000U) || !Domain(beta,0x3f800000U))return 0;
 for(unsigned i=0;i<8192;i++)if(!Domain(Words(W::Prior)[i],0x49800000U))return 0;
 for(unsigned i=0;i<256;i++)if(!Domain(Words(W::Query)[i],0x40800000U))return 0;
 for(unsigned i=0;i<64;i++)if(!Domain(Words(W::Value)[i],0x49800000U))return 0;
 Setup();
 for(unsigned chunk=0;chunk<8;chunk++)Import(W::Prior+chunk*4096,chunk*64,1024);
 Math();Emit(0x8a000002);Emitter e;
 for(unsigned parity=0;parity<4;parity+=2){
  Dot(e,parity,true,decay);
  // Only AFTER all128 prior rows have contributed may V replace key0.
  Import(W::Value,0,64);Math();Emit(0x70030000|parity);
  PioneerQ38WideDeltaSub(e,0,6,0);PioneerSfpuLoad32(e,1,beta);
  PioneerQ38VectorAffineMul(e,0,1,0);Emit(0x72030000|parity);Drain();
  for(unsigned v=parity/2;v<64;v+=2){
   const uint32_t x=Words(0xffbd8000)[v];if(!Finite(x))scalar_fault_park();Words(W::Delta)[v]=x;
  }
  // Restore key0 before the other prediction parity. After the SECOND
  // parity,Delta will immediately replace key0,so another prior reload
  // there is dead work. The final reload below precedes ALL state updates.
  if(!parity){Import(W::Prior,0,64);Math();}
 }
 Import(W::Delta,0,64);Math();Emit(0x70630000);Emit(0x70730002);Drain();
 Import(W::Prior,0,64);Math();PioneerSfpuLoad32(e,2,decay);
 for(unsigned key=0;key<128;key++){
  PioneerSfpuLoad32(e,3,Words(W::Key)[key]);
  for(unsigned parity=0;parity<4;parity+=2){
   Emit(0x70030000|(key*4)|parity);PioneerQ38VectorAffineMul(e,0,2,0);
   PioneerQ38VectorAffineMul(e,3,parity?7:6,1);PioneerQ38VectorAffineAdd(e,0,1,0);
   Emit(0x72030000|(key*4)|parity);
  }
 }
 Drain();
 // Preserve every next-state word BEFORE output overwrites key0 Dst.
 for(unsigned i=0;i<8192;i++){
  const uint32_t x=Words(0xffbd8000)[i];if(!Domain(x,0x4d800000U))scalar_fault_park();Words(W::Next)[i]=x;
 }
 for(unsigned parity=0;parity<4;parity+=2){
  Dot(e,parity,false,0);PioneerSfpuLoad32(e,1,0x3db504f3U);
  PioneerQ38VectorAffineMul(e,6,1,6);Emit(0x72630000|parity);Drain();
 }
 for(unsigned v=0;v<64;v++){
  const uint32_t x=Words(0xffbd8000)[v];if(!Domain(x,0x51800000U))scalar_fault_park();Words(W::Output)[v]=x;
 }
 Fence();return 64;
}
