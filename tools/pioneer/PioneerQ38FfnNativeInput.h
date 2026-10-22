/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_FFN_NATIVE_INPUT_H
#define PIONEER_Q38_FFN_NATIVE_INPUT_H
#include "PioneerQ38FfnActivation.h"
#include "../../src/add-ons/kernel/drivers/misc/PioneerInt8Operand.h"

// OFFLINE local arithmetic/packing ONLY;not a device handle or launch ABI.
// Exact existing matrix operand interpretation:256 sign-magnitude16 lanes
// plus ONE signedF32 scale. Each local block has a separate64B scale line,
// with15 zero padding words,so future NoC gathering never reads past it or
// needs concurrent sub-line writes to the shared compact scale vector.
// Owner must gather scales into that compact vector separately. No supplied
// address,NoC/PCI/DRAM/reset/driver path;no native lifecycle permission here.
struct PioneerQ38FfnNativeInput {uint32_t operand[128],scale[16];};
static_assert(sizeof(PioneerQ38FfnNativeInput)==576,"512B operands +64B scale line");

static inline bool PioneerQ38FfnPackNative(const PioneerQ38ActivationK* input,PioneerQ38FfnNativeInput* output)
{
 if(!input || !output || (reinterpret_cast<__UINTPTR_TYPE__>(input)&3)
  || (reinterpret_cast<__UINTPTR_TYPE__>(output)&3)
  || !PioneerQ38Disjoint(reinterpret_cast<const float*>(input),73,reinterpret_cast<const float*>(output),144))return false;
 const uint32_t scale=input->words[0],magnitude=scale&0x7fffffff;
 // EXACT unchanged native matrix contract,not a broadened arithmetic range.
 if((magnitude && (magnitude<0x2f800000 || magnitude>0x47800000)) || scale==0x80000000)return false;
 PioneerQ38FfnNativeInput stage;
 for(unsigned i=0;i<128;i++)stage.operand[i]=0;
 for(unsigned i=0;i<16;i++)stage.scale[i]=0;
 stage.scale[0]=scale;
 for(unsigned group=0;group<16;group++) {
  int sum=0;
  for(unsigned lane=0;lane<16;lane++) {
   const unsigned i=group*16+lane,raw=(input->words[1+i/4]>>((i%4)*8))&255;
   const int value=int(raw)-int((raw&128)*2);
   if(value==-128 || (!magnitude && value))return false; // Q8_K uses[-127,127]
   stage.operand[i/2]|=uint32_t(PioneerInt8Operand(value))<<((i%2)*16);sum+=value;
  }
  const unsigned encoded=(input->words[65+group/2]>>((group%2)*16))&65535;
  const int supplied=int(encoded)-int((encoded&32768)*2);
  if(supplied!=sum)return false;
 }
 for(unsigned i=0;i<128;i++)output->operand[i]=stage.operand[i];
 for(unsigned i=0;i<16;i++)output->scale[i]=stage.scale[i];
 return true;
}

// Local fused producer. Both outputs remain unchanged on ANY bad input,
// bounded-SiLU refusal or scale/packing refusal. Stage activations on-card,
// never publish raw intermediates to a host or silently execute CPU fallback.
static inline bool PioneerQ38FfnNativeActivation(const float* gate,const float* up,unsigned count,
 float* activated,PioneerQ38FfnNativeInput* output)
{
 if(count!=256)return false;
 const float* buffers[]={gate,up,activated,reinterpret_cast<const float*>(output)};
 const unsigned words[]={256,256,256,144};
 for(unsigned a=0;a<4;a++) {
  if(!buffers[a] || (reinterpret_cast<__UINTPTR_TYPE__>(buffers[a])&3))return false;
  for(unsigned b=a+1;b<4;b++)if(!PioneerQ38Disjoint(buffers[a],words[a],buffers[b],words[b]))return false;
 }
 float stage[256];PioneerQ38ActivationK quantized;PioneerQ38FfnNativeInput native;
 if(!PioneerQ38FfnActivation(gate,up,256,stage,&quantized) || !PioneerQ38FfnPackNative(&quantized,&native))return false;
 for(unsigned i=0;i<256;i++)activated[i]=stage[i];
 for(unsigned i=0;i<128;i++)output->operand[i]=native.operand[i];
 for(unsigned i=0;i<16;i++)output->scale[i]=native.scale[i];
 return true;
}
#endif
