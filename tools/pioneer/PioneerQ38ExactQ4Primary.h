/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_EXACT_Q4_PRIMARY_H
#define PIONEER_Q38_EXACT_Q4_PRIMARY_H
#include "PioneerQ38NativeBfpMatvec.h"
// OFFBOARD prototype ONLY. NOT a full Q4 dot: produces dot(z,x); the escape
// correction and7*sum(x) MUST be added as integers before original affine
// arithmetic. No installed owner/worker/Work mode selects this producer.
// Entry:private reset/idle Config0,FPU/unpackers/source banks/Dst and owned
// immutable aligned L1. Setup DOES NOT establish/prove that ownership.
// A:128 packed bytes at90000,one K16xN16 face,K-major,canonical SM3[-7,7].
// B:128 BF16 values at91000,eight repetitions of16 EXACT INTEGER x[-128,127].
// Payloads have private16-byte headers. UNP0 forces exponent129,no stream,
// no transpose/tileize;SrcA rows0..15/columnN. BF16 inputs have at most7
// significant bits;A at most3,so phase0 retains their ENTIRE product. This
// is NOT permission to truncate fidelity for arbitrary BF16/float inputs.
// Outputs:Dst32..39,each row is the same16 integer primary sums. Real final
// completion is mandatory before reads/reuse. No SFPU decoder,MOVD2A,
// packer,TDMA,NoC,host data,GGUF read,CPU dot or lifecycle authority here.
struct PioneerQ38ExactQ4Primary {
 template<class E> static bool Setup(E& e) {
  if(!PioneerQ38NativeBfpMatvec::Setup(e,4))return false;
  e.Config(50,0x181); // existing add-dest-counter bit8 + forced exponent129
  e.Config(64,0x01000017);e.Config(65,0x00010001); // A:X256,Y1,Z1
  e.Config(72,0x27);e.Config(73,0x000f010f); // x4,NO transpose,force shared exp
  e.Config(112,0x00800015); // B:X128 BF16:8 identical rows
  e.Drain();return true;
 }
 template<class E> static bool Run(E& e) {
  e.Op(0x3700000f);e.Op(0x10180000); // all RWCs/fidelity0;invalidate private Dst
  e.Op(0x5e23fc00);e.Op(0x5e41fc00); // A256/B128 datums
  e.Op(0x5160000b);e.Op(0x5460000f);
  e.Op(0x420000c1);e.Op(0x428000c1);e.Drain();
  e.Op(0x26c04020); // sole exact integer-input phase,release BOTH source banks
  e.Drain();return true;
 }
};
#endif
