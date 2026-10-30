/* SPDX-License-Identifier: MIT */
// LOCAL arithmetic candidate only. Recheck roles even on late inclusion.
#if !defined(PIONEER_Q38_PRECISE_ACTIVATION_CHUNK_IMAGE) || PIONEER_Q38_PRECISE_ACTIVATION_CHUNK_IMAGE != 1 \
 || !defined(PIONEER_Q38_SIGMOID_PRECISION_IMAGE) || PIONEER_Q38_SIGMOID_PRECISION_IMAGE != 1
#error Exact isolated precise activation chunk and sigmoid roles required
#endif
#if defined(_KERNEL_MODE) || defined(__FAST_MATH__) || defined(PIONEER_Q38_CPU_VALIDATION) \
 || defined(PIONEER_Q38_WORK_INFERENCE) || defined(TT_PIONEER_Q38_WORK_MODE) \
 || defined(PIONEER_Q38_MIXED_LIVE_NATIVE_CONTEXT) || defined(PIONEER_Q38_MIXED_LIVE_VALIDATION_NATIVE) \
 || defined(PIONEER_Q38_MIXED_LIVE_VALIDATION_CLIENT) || defined(PIONEER_DRIVER_BUILD_TAG) \
 || defined(PIONEER_Q38_TOKEN_NATIVE_CONTEXT) || defined(PIONEER_Q38_GLOBAL_WORKER_IMAGE) \
 || defined(PIONEER_Q38_GLOBAL_TOKEN_ROOT_IMAGE) || defined(PIONEER_Q38_GLOBAL_CHAIN_COMPONENT_IMAGE) \
 || defined(PIONEER_Q38_FULL_MODEL_CPU_REFERENCE) || defined(PIONEER_Q38_TOKEN_CONTEXT_COMPILE_ONLY) \
 || defined(PIONEER_Q38_SILU_PRECISION_IMAGE) || defined(PIONEER_Q38_SILU_LEGACY_CONTROL_IMAGE)
#error Isolated activation chunk is not kernel, native, CPU reference or Work
#endif
#ifndef PIONEER_Q38_PRECISE_ACTIVATION_CHUNK_H
#define PIONEER_Q38_PRECISE_ACTIVATION_CHUNK_H
#include "PioneerQ38SigmoidPrecision.h"
#include "PioneerQ38LearnedRms.h"
#include "PioneerQ38ProjectionInput.h"

// One of the 68 independent 256-value FFN chunks. This does NOT launch
// workers or grant a producer/source/lifetime/native handle. Preserve the
// current ROOT's exact order: (gate * precise_sigmoid(gate)) * up, with
// -ffp-contract=off, and its existing RmsDomain for BOTH inputs and output.
// Packing uses the unchanged projection quantizer (Q8_0 vs Q8_K semantics).
// All four LOCAL buffers must be disjoint; both outputs publish only after
// all 256 values AND the complete quantized chunk pass. No partial result
// on late refusal, CPU fallback, NoC/PCI/DRAM access or old worker selection.
static __attribute__((noinline,unused)) bool PioneerQ38PreciseActivationChunk(unsigned type,
 const float* gate,const float* up,unsigned count,float* activated,PioneerQ38ProjectionInput* output)
{
 (void)&PioneerQ38LearnedRms; // shared header's unused non-inline helper is not called
 if(count!=256 || (type!=8 && type!=12 && type!=14))return false;
 const float* buffers[]={gate,up,activated,reinterpret_cast<const float*>(output)};
 const unsigned words[]={256,256,256,144};
 for(unsigned a=0;a<4;a++){
  if(!buffers[a] || (reinterpret_cast<__UINTPTR_TYPE__>(buffers[a])&3))return false;
  for(unsigned b=a+1;b<4;b++)if(!PioneerQ38Disjoint(buffers[a],words[a],buffers[b],words[b]))return false;
 }
 float stage[256];PioneerQ38ProjectionInput packed;
 for(unsigned i=0;i<256;i++){
  const float a=gate[i],b=up[i];float sigmoid;
  if(!PioneerQ38RmsDomain(a) || !PioneerQ38RmsDomain(b) || !PioneerQ38SigmoidPrecision(a,sigmoid))return false;
  const float value=(a*sigmoid)*b;
  if(!PioneerQ38RmsDomain(value))return false;
  stage[i]=value;
 }
 if(!PioneerQ38ProjectionQuantize(type,stage,256,&packed))return false;
 for(unsigned i=0;i<256;i++)activated[i]=stage[i];
 for(unsigned i=0;i<128;i++)output->operand[i]=packed.operand[i];
 for(unsigned i=0;i<16;i++)output->scale[i]=packed.scale[i];
 return true;
}
#endif
