/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_NATIVE_BFP4_BAND_H
#define PIONEER_Q38_NATIVE_BFP4_BAND_H
#if defined(_KERNEL_MODE) || defined(PIONEER_Q38_WORK_INFERENCE) || defined(PIONEER_Q38_CPU_VALIDATION) \
 || defined(PIONEER_Q38_FULL_MODEL_CPU_REFERENCE) || defined(__FAST_MATH__)
#error Native BFP4 band is an unqualified shader prototype, not driver, Work or CPU validation
#endif
#include "PioneerQ38NativeBfpMatvec.h"

// Distinct approximate-weight prototype: one 32-row band, complete K.
// Source weights have already been REQUANTIZED to BFP4_B; this is not the
// original Q4_K arithmetic or a model-quality/speed/device-ownership grant.
// The caller must authenticate and own every weight/input/header/output byte
// and exclusive idle Tensix configuration/source banks before invocation.
// BFP4 x BF16 requires fidelity terms 0 and 2; omitting either is not allowed.
struct PioneerQ38NativeBfp4Band {
 static constexpr unsigned Weight=0x90000,Input=0x124000,Output=0x130000;
 static constexpr unsigned WeightPayload=576,WeightStride=592,InputPayload=64,InputStride=80;
 static constexpr bool Width(unsigned k){return k==5120 || k==6144 || k==17408;}
 static constexpr unsigned Tiles(unsigned k){return Width(k)?k/32:0;}
 static constexpr unsigned WeightEnd(unsigned k){return Weight-16+Tiles(k)*WeightStride;}
 static constexpr unsigned InputEnd(unsigned k){return Input-16+Tiles(k)*InputStride;}
private:
 template<unsigned Face,class E> static __attribute__((always_inline)) inline void FaceRun(E& e){
  static_assert(Face<4,"four native faces");
  if(Face){e.Op(0x37040201);e.Op(0x37040201);}
  e.Op(Face&1?0x3700040a:0x3700000a);
  e.Op(0x26084000|(Face<2?32:40)); // phase0; retain both source banks
  e.Op(0x26084000|(Face<2?32:40)|(Face==3?0x00c00000:0)); // phase2; final face releases
 }
public:
 template<class E> static bool Run(E& e,unsigned k){
  if(!Width(k))return false; // no instruction, config or memory IO
  if(!PioneerQ38NativeBfpMatvec::Setup(e,4))return false;
  e.Config(112,0x00200015); // 32 BF16 values, broadcast by MVMUL
  e.Op(0x3700000f);e.Op(0x10180000);e.Drain(); // clear full-band accumulator once
  unsigned weight=Weight/16-1,input=Input/16-1;
  for(unsigned tile=0;tile<Tiles(k);tile++,weight+=WeightStride/16,input+=InputStride/16){
   e.Config(76,weight);e.Config(124,input);
   e.Op(0x3700000f);e.Op(0x5e2ffc00);e.Op(0x5e407c00);
   e.Op(0x5160000b);e.Op(0x5460000f);
   e.Op(0x420000c1);e.Op(0x428000c1);e.Drain();
   FaceRun<0>(e);FaceRun<1>(e);FaceRun<2>(e);FaceRun<3>(e);
   e.Drain(); // complete both source-bank releases before descriptor reuse
  }
  return true;
 }
};
static_assert(PioneerQ38NativeBfp4Band::WeightEnd(17408)<=PioneerQ38NativeBfp4Band::Input-16
 && PioneerQ38NativeBfp4Band::InputEnd(17408)<=PioneerQ38NativeBfp4Band::Output
 && PioneerQ38NativeBfp4Band::Output+256<=0x16c000,"BFP4 prototype L1 extents must remain disjoint");
#endif
