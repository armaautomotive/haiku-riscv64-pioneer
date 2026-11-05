/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_EXACT_Q4_TWO_PRODUCT_H
#define PIONEER_Q38_EXACT_Q4_TWO_PRODUCT_H
#include "PioneerQ38ExactQ4Affine.h"
// OFFBOARD arithmetic candidate only. No installed/native/Work selector.
// Original q=z+7+escape. Two integer FPU products return dot(z+escape,x).
// The already-owned K16 input sum restores +7*sum EXACTLY in FP32 before
// the unchanged original affine. All integers are <2^15,so no rounding is
// introduced by that addition. This is NOT K32 reassociation/requantizing.
// Same private L1/source-bank ownership obligations as the original helper.
struct PioneerQ38ExactQ4TwoProductDot {
 template<class E> static bool Setup(E& e){
  if(!PioneerQ38ExactQ4Primary::Setup(e))return false;
  e.Config(0,0x42b5);e.Config(1,0x60aa0000);e.Drain();return true;
 }
 template<class E> static bool Run(E& e){
  for(unsigned pair=0;pair<8;pair++){
   const unsigned bits=e.Mask(pair);
   e.Escape(pair*2,PioneerQ38ExactQ4Corrected::Spread(bits));
   e.Escape(pair*2+1,PioneerQ38ExactQ4Corrected::Spread(bits>>16));
  }
  e.SealData();e.Op(0x3700000f);e.Op(0x10180000);
  e.Op(0x5e23fc00);e.Op(0x5e41fc00);e.Op(0x5160000b);e.Op(0x5460000f);
  e.Config(64,0x01000017);e.Config(72,0x27);e.Config(50,0x181);
  e.Config(84,0x00400040);e.Config(76,0x90000/16-1);
  e.Op(0x42000081);e.Drain(); // primary rows0..15,NOT a published partial bank
  e.Config(64,0x0100001f);e.Config(72,15);e.Config(50,0x17f);
  e.Config(84,0x00400140);e.Config(76,0x90200/16-1);
  e.Op(0x420000c1);e.Op(0x428000c1);e.Drain(); // escape rows16..31;publish both
  e.Op(0x26004020);
  e.Op(0x37040201);e.Op(0x37040201);e.Op(0x37000008);
  e.Op(0x26c04020);e.Drain(); // second product releases BOTH source banks
  return true;
 }
};
struct PioneerQ38ExactQ4TwoProductAffine:PioneerQ38ExactQ4Affine {
 template<class E> static void Affine(E& e,unsigned scale,int sum){
  e.Op(0x3700000f);e.Op(0x8a000002);
  for(unsigned parity=0;parity<4;parity+=2){
   e.Op(0x70030000|32|parity);
   e.Op(0x70130000|96|parity);e.Op(0x70230000|100|parity);e.Op(0x70530000|108|parity);
   PioneerSfpuLoad32(e,3,scale);
   PioneerSfpuLoad32(e,4,(sum<0?0x80000000U:0)|unsigned(sum<0?-sum:sum));
   const int offset=sum*7;
   PioneerSfpuLoad32(e,6,(offset<0?0x80000000U:0)|unsigned(offset<0?-offset:offset));
   e.Op(0x90000660); // exact canonical SM32 -> FP32,not a float bit cast
   PioneerQ38VectorAffineAdd(e,0,6,0); // original q dot reconstructed BEFORE affine
   PioneerSfpuLoad32(e,6,0x7fffffff);PioneerSfpuAnd(e,7,0,6);
   PioneerSfpuAdd(e,7,6);PioneerSfpuShift(e,7,-31,true);PioneerSfpuAnd(e,0,0,7);
   PioneerQ38VectorAffineFloatDot(e);e.Op(0x72030000|108|parity);
  }
  e.Drain();
 }
 template<class E> static bool Run(E& e,unsigned k){
  if(!Width(k))return false;
  if(!PioneerQ38ExactQ4TwoProductDot::Setup(e))return false;
  e.ZeroData();e.SealData();FloatUnpack(e,16);
  e.Config(76,Zero/16-1);e.Config(84,0x00400000+(96+4)*16);
  e.Op(0x42000091);e.Drain();
  e.Op(0xb2050004);e.Config(1,0x60aa0000);e.Drain();PanelEmitter<E> panel(e);
  for(unsigned p=0;p<k/16;p++){
   unsigned scale=0;int sum=0;if(!e.StagePanel(p,scale,sum))return false;
   const unsigned magnitude=scale&0x7fffffff;
   if(sum<-2048 || sum>2048 || (magnitude && (magnitude<0x2f800000 || magnitude>0x47800000)))return false;
   e.Config(65,0x00010001);e.Config(73,0x000f010f);e.Config(86,0x01000100);
   if(!PioneerQ38ExactQ4TwoProductDot::Run(panel))return false;
   if(!(p&1))Metadata(e);
   Affine(e,scale,sum);
  }
  e.Drain();return true;
 }
};
#endif
