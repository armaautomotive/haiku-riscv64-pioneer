/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_FFN_ACTIVATION_H
#define PIONEER_Q38_FFN_ACTIVATION_H
#include "PioneerQwen38Activation.h"

// OFFLINE local arithmetic prerequisite ONLY,not a fused backend or native
// launch permission. One256-lane Q4_K block:gate/up -> SiLU(gate)*up -> Q8_K.
// Inputs and outputs are disjoint caller-owned LOCAL buffers. No external
// address,NoC/PCI/DRAM/owner/reset path. Publish BOTH outputs only on true.
//
// This deliberately bounded FIRST candidate accepts signed zero,or normal
// gate magnitudes [2^-40,32] and up magnitudes [2^-40,2^20]. Within this box
// every nonzero SiLU/product/scale/quantizer intermediate stays normal under
// BabyRISCV's flush-to-zero arithmetic. Merely rejecting a subnormal RESULT
// after FPU execution is insufficient:it might already have flushed to zero.
// These bounds are NOT evidence that arbitrary model activations qualify.
// Its output is packed GGML Q8_K,NOT the existing native matrix operand
// wire. Native packing and the matrix scale-range contract remain separate
// prerequisites;this candidate does not relax either one.
// Compile with -ffp-contract=off;the fast polynomial's error is tested against
// independent host expf. This is not a replacement for pinned CPU semantics.
static inline bool PioneerQ38FfnActivation(const float* gate,const float* up,unsigned count,
 float* activated,PioneerQ38ActivationK* quantized)
{
 if(count!=256)return false;
 const float* buffers[]={gate,up,activated,reinterpret_cast<const float*>(quantized)};
 const unsigned words[]={256,256,256,73};
 for(unsigned a=0;a<4;a++) {
  if(!buffers[a] || (reinterpret_cast<__UINTPTR_TYPE__>(buffers[a])&3))return false;
  for(unsigned b=a+1;b<4;b++)if(!PioneerQ38Disjoint(buffers[a],words[a],buffers[b],words[b]))return false;
 }
 for(unsigned i=0;i<256;i++) {
  const uint32_t g=PioneerAttentionBits(gate[i])&0x7fffffff,u=PioneerAttentionBits(up[i])&0x7fffffff;
  if((g && (g<0x2b800000 || g>0x42000000)) || (u && (u<0x2b800000 || u>0x49800000)))return false;
 }
 float stage[256];PioneerQ38ActivationK q;
 for(unsigned i=0;i<256;i++)stage[i]=PioneerGraphSiluFast(gate[i])*up[i];
 if(!PioneerQ38QuantizeK(stage,256,&q))return false;
 for(unsigned i=0;i<256;i++)activated[i]=stage[i];
 for(unsigned i=0;i<73;i++)quantized->words[i]=q.words[i];
 return true;
}
#endif
