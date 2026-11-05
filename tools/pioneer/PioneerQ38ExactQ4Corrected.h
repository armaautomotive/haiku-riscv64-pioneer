/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_EXACT_Q4_CORRECTED_H
#define PIONEER_Q38_EXACT_Q4_CORRECTED_H
#include "PioneerQ38ExactQ4Primary.h"
// OFFBOARD ONLY. Complete INTEGER dot(q,x),not fullK/affine/model/ownership.
// q=z+7+escape is reconstructed by THREE FPU matrix operations before any
// original FP32 affine update. No scalar/CPU dot or weight requantization.
// Same integer-input/private-idle entry obligations as ExactQ4Primary.
// Private aligned L1:primary90000/128B,constant7 90100/128B,escape90200/64B,
// BF16 input91000/256B,original1-bit mask90300/32B. Each unpack payload has
// a separate16B preceding header. The caller separately owns/authenticates
// these extents;these constants do NOT grant a native memory capability.
// UNP0 puts z/7/escape into SrcA rows0/16/32 of the SAME bank. Only its last
// unpack hands that bank over. Explicit matrix BF16 format ignores implied
// input formats. All three phase0 integer products are EXACT;only final
// MVMUL releases both banks. Every config rewrite follows real completion.
struct PioneerQ38ExactQ4Corrected {
 static unsigned Spread(unsigned v) {
  v&=65535;v=(v|(v<<8))&0x00ff00ff;v=(v|(v<<4))&0x0f0f0f0f;
  v=(v|(v<<2))&0x33333333;return (v|(v<<1))&0x55555555;
 } // pure bit permutation:16 one-bit mask values ->16 native SM1 values
 template<class E> static bool Setup(E& e) {
  if(!PioneerQ38ExactQ4Primary::Setup(e))return false;
  e.Config(0,0x42b5);e.Config(1,0x60aa0000);e.Drain();
  for(unsigned i=0;i<32;i++)e.Seven(i,0x77777777);
  e.SealData();return true;
 }
 template<class E> static bool Run(E& e) {
  for(unsigned pair=0;pair<8;pair++) {
   const unsigned bits=e.Mask(pair);
   e.Escape(pair*2,Spread(bits));e.Escape(pair*2+1,Spread(bits>>16));
  }
  e.SealData(); // real RV fence before unpack reads newly written L1
  e.Op(0x3700000f);e.Op(0x10180000);
  e.Op(0x5e23fc00);e.Op(0x5e41fc00);e.Op(0x5160000b);e.Op(0x5460000f);
  e.Config(64,0x01000017);e.Config(72,0x27);e.Config(50,0x181);
  e.Config(84,0x00400040);e.Config(76,0x90000/16-1);
  e.Op(0x42000081);e.Drain(); // primary:DO NOT give partial SrcA to matrix
  e.Config(84,0x00400140);e.Config(76,0x90100/16-1);
  e.Op(0x42000081);e.Drain(); // constant7,still SAME unpacker-owned bank
  e.Config(64,0x0100001f);e.Config(72,15);e.Config(50,0x17f);
  e.Config(84,0x00400240);e.Config(76,0x90200/16-1);
  e.Op(0x420000c1);e.Op(0x428000c1);e.Drain(); // BFP2_B ID15,NOT BFP2_A11
  e.Op(0x26004020); // primary z
  e.Op(0x37040201);e.Op(0x37040201);e.Op(0x37000008);
  e.Op(0x26004020); // +7*sum(x),still no source release
  e.Op(0x37040201);e.Op(0x37040201);e.Op(0x37000008);
  e.Op(0x26c04020);e.Drain(); // +escape dot,release BOTH;exact original q dot
  return true;
 }
};
#endif
