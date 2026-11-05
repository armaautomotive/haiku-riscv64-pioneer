/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_EXACT_Q4_TWO_PRODUCT_REPLAY_H
#define PIONEER_Q38_EXACT_Q4_TWO_PRODUCT_REPLAY_H
#include "PioneerQ38ExactQ4TwoProduct.h"
// OFFBOARD ONLY. Private per-thread replay slots12..24;caller owns the
// whole worker and records afresh at the first panel of EVERY output group.
// Only the13 register-only original FP32 affine instructions are replayed.
// No loads/stores,config,source handoff,NoC,drain or nested REPLAY is recorded.
// The preceding EXACT dot+7*sum SFPADD restores ORIGINAL q dot and also
// canonicalizes zero:sum0 has offset+0;opposite nonzero exact integers cancel
// to+0. Thus the old six-word zero cleanup is redundant HERE,not generally.
// Both explicit MAD latency slots and every original affine rounding remain.
struct PioneerQ38ExactQ4TwoProductReplay:PioneerQ38ExactQ4TwoProductAffine {
 enum {ReplayFirst=12,ReplayCount=13};
 template<class E> static __attribute__((always_inline)) inline void Affine(E& e,unsigned scale,int sum,bool record){
  e.Op(0x3700000f);e.Op(0x8a000002);
  for(unsigned parity=0;parity<4;parity+=2){
   e.Op(0x70030000|32|parity);
   e.Op(0x70130000|96|parity);e.Op(0x70230000|100|parity);e.Op(0x70530000|108|parity);
   PioneerSfpuLoad32(e,3,scale);
   PioneerSfpuLoad32(e,4,(sum<0?0x80000000U:0)|unsigned(sum<0?-sum:sum));
   const int offset=sum*7;
   PioneerSfpuLoad32(e,6,(offset<0?0x80000000U:0)|unsigned(offset<0?-offset:offset));
   e.Op(0x90000660);PioneerQ38VectorAffineAdd(e,0,6,0);
   const unsigned replay=0x04000000|(ReplayFirst<<14)|(ReplayCount<<4);
   if(record && !parity){e.Op(replay|3);PioneerQ38VectorAffineFloatDot(e);}
   else e.Op(replay);
   e.Op(0x72030000|108|parity);
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
   Affine(e,scale,sum,p==0);
  }
  e.Drain();return true;
 }
};
#endif
