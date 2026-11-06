/* SPDX-License-Identifier: MIT */
// BUILD-only functions,not bootable/native/Work. Caller owes a NEW owner.
#if !defined(__riscv) || __riscv_xlen != 32 || !defined(PIONEER_Q38_Q4_FACTORED_TILE_BUILD) || PIONEER_Q38_Q4_FACTORED_TILE_BUILD != 1
#error Exact separate factored-tile RV32 BUILD-only selection required
#endif
using uint32_t=unsigned;
#define PIONEER_ATTENTION_FREESTANDING
#define PIONEER_Q38_Q4_FACTORED_BFP8_OFFBOARD 1
#include "PioneerQ38Q4FactoredTile.h"
extern "C" __attribute__((noreturn)) void pioneer_q4_factored_tile_fault(){for(;;)asm volatile("wfi");}
static volatile unsigned* Words(unsigned address){return reinterpret_cast<volatile unsigned*>(address);}
struct FactoredTileIO {
 void Write(unsigned a,unsigned v){*Words(a)=v;}
 unsigned Read(unsigned a){return *Words(a);}
};
struct FactoredTileEmitter {
 __attribute__((always_inline)) inline void Op(unsigned word){
#ifdef PIONEER_Q38_Q4_FACTORED_BFP8_INLINE
  if(__builtin_constant_p(word))asm volatile(".word %0"::"i"((word<<2)|(word>>30)):"memory");
  else *Words(0xffe40000)=word;
#else
  *Words(0xffe40000)=word;
#endif
 }
 void Config(unsigned i,unsigned v){auto* p=Words(0xffef0000+i*4);*p=v;if(*p!=v)pioneer_q4_factored_tile_fault();}
 void Drain(){
  *Words(0xffe000fc)=0;(void)*Words(0xffe000fc);asm volatile("fence":::"memory");
  Op(0xa2101816);Op(0x4500017e);
  for(unsigned n=0;n<1000000;n++)if(*Words(0xffe000fc)){asm volatile("fence":::"memory");return;}
  pioneer_q4_factored_tile_fault();
 }
};
extern "C" void pioneer_q4_factored_tile_constants(){FactoredTileIO io;PioneerQ38Q4FactoredTile::Constants(io);}
extern "C" void pioneer_q4_factored_tile_begin(){FactoredTileEmitter e;PioneerQ38Q4FactoredTile::Begin(e);}
extern "C" unsigned pioneer_q4_factored_tile_group(unsigned group,unsigned* result){
 FactoredTileEmitter e;FactoredTileIO io;unsigned sum;
 const bool okay=PioneerQ38Q4FactoredTile::Group(e,io,group,sum);
 if(okay)*result=sum;
 return okay;
}
