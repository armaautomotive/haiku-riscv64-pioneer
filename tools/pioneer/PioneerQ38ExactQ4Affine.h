/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_EXACT_Q4_AFFINE_H
#define PIONEER_Q38_EXACT_Q4_AFFINE_H
#if defined(_KERNEL_MODE) || defined(PIONEER_Q38_WORK_INFERENCE) || defined(PIONEER_Q38_CPU_VALIDATION)
#error New full-K arithmetic prototype requires separate native integration
#endif
#include "PioneerQ38ExactQ4Corrected.h"
#include "PioneerSfpuDivide.h"
#include "PioneerQ38VectorAffine.h"
// OFFBOARD full-K integration,NOT an installed/native/model backend.
// Input is the ORIGINAL int8 activation represented EXACTLY as BF16.
// Each K16 dot is complete before its ORIGINAL non-fused FP32 affine:
// old + ((coefficient*scale)*dot + (negativeMinimum*scale)*sum).
// Adjacent panels share metadata but retain TWO separate affine updates;
// combining K32 dots would change the original rounding order.
// Caller owns immutable/source-authenticated packets and private L1/banks.
// These addresses/width checks grant NO mapping,NoC or device authority.
struct PioneerQ38ExactQ4Affine {
 static constexpr unsigned Coefficient=0x94000,Minimum=0x94080,Zero=0x96000;
 static constexpr unsigned Source=0xa0000,PacketBytes=352,Output=0x92000;
 static constexpr bool Width(unsigned k){return k==5120 || k==6144 || k==17408;}
 // Preserve Dst32 rows96..111 (physical192..223) for metadata/accumulator.
 // ZEROACC mode1 faces4+5 invalidates ONLY physical64..95 (Dst32 32..47).
 // Official ZEROACC/Dst32 addressing,not a broad clear followed by repair.
 template<class E> struct PanelEmitter {
  E& e;explicit PanelEmitter(E& value):e(value){}
  void Op(unsigned word){if(word==0x10180000){e.Op(0x10080004);e.Op(0x10080005);}else e.Op(word);}
  void Config(unsigned i,unsigned v){e.Config(i,v);}
  void Drain(){e.Drain();}
  unsigned Mask(unsigned i){return e.Mask(i);}
  void Escape(unsigned i,unsigned v){e.Escape(i,v);}
  void SealData(){e.SealData();}
 };
 template<class E> static void FloatUnpack(E& e,unsigned rows) {
  e.Op(0xb2050000);e.Drain();e.Config(1,0x20200000);
  e.Config(50,0x100);e.Config(64,0x100010);e.Config(65,rows);
  e.Config(72,0);e.Config(73,0x13);e.Config(86,0x100010);
  e.Op(rows==16?0x5e23fc00:0x5e203c00);e.Op(0x5120000a);e.Op(0x5420000f);
 }
 template<class E> static void Metadata(E& e) {
  FloatUnpack(e,1);
  for(unsigned kind=0;kind<2;kind++) {
   e.Config(76,(kind?Minimum:Coefficient)/16-1);
   e.Config(84,0x00400000+(96+kind*4+4)*16);
   e.Op(0x42000081);e.Drain(); // no SrcA publication/flip
  }
  e.Op(0xb2050004);e.Config(1,0x60aa0000);e.Op(0x3700000f);e.Drain();
 }
 template<class E> static void Affine(E& e,unsigned scale,int sum) {
  e.Op(0x3700000f);e.Op(0x8a000002); // ALL32 lanes,not inherited predicates
  for(unsigned parity=0;parity<4;parity+=2) {
   e.Op(0x70030000|32|parity); // FP32 INTEGER dot;NEVER integer-encoded load/cast
   e.Op(0x70130000|96|parity);e.Op(0x70230000|100|parity);e.Op(0x70530000|108|parity);
   PioneerSfpuLoad32(e,3,scale);
   PioneerSfpuLoad32(e,4,(sum<0?0x80000000U:0)|unsigned(sum<0?-sum:sum));
   // Canonicalize signed zero only;all nonzero exact FP32 dots unchanged.
   PioneerSfpuLoad32(e,6,0x7fffffff);PioneerSfpuAnd(e,7,0,6);
   PioneerSfpuAdd(e,7,6);PioneerSfpuShift(e,7,-31,true);PioneerSfpuAnd(e,0,0,7);
   PioneerQ38VectorAffineFloatDot(e);
   e.Op(0x72030000|108|parity);
  }
  e.Drain(); // actual stores complete before any following format/data change
 }
 template<class E> static bool Run(E& e,unsigned k) {
  if(!Width(k))return false; // invalid geometry is inert
  if(!PioneerQ38ExactQ4Corrected::Setup(e))return false;
  e.ZeroData();e.SealData();FloatUnpack(e,16);
  e.Config(76,Zero/16-1);e.Config(84,0x00400000+(96+4)*16);
  e.Op(0x42000091);e.Drain(); // FULL96..111 initialization,including every padding column
  e.Op(0xb2050004);e.Config(1,0x60aa0000);e.Drain();
  PanelEmitter<E> panel(e);
  for(unsigned p=0;p<k/16;p++) {
   unsigned scale=0;int sum=0;
   if(!e.StagePanel(p,scale,sum))return false;
   const unsigned magnitude=scale&0x7fffffff;
   if(sum<-2048 || sum>2048 || (magnitude && (magnitude<0x2f800000 || magnitude>0x47800000)))return false;
   // Restore the actual corrected-panel descriptor after metadata imports.
   e.Config(65,0x00010001);e.Config(73,0x000f010f);e.Config(86,0x01000100);
   if(!PioneerQ38ExactQ4Corrected::Run(panel))return false;
   if(!(p&1))Metadata(e); // ORIGINAL shared K32 coefficients,not shared affine update
   Affine(e,scale,sum);
  }
  e.Drain();return true;
 }
};
static_assert(PioneerQ38ExactQ4Affine::Source+17408/16*PioneerQ38ExactQ4Affine::PacketBytes<0x170000,
 "bounded PRIVATE offboard full-K packet storage,not a source-bank allocation");
#endif
