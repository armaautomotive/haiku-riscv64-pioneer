/* SPDX-License-Identifier: MIT */
#if !defined(PIONEER_Q38_PREPARED_BF16_Q4_OFFBOARD) || PIONEER_Q38_PREPARED_BF16_Q4_OFFBOARD != 1
#error Separate prepared BF16 Q4 arithmetic selection required
#endif
#include "PioneerQ38Q4FactoredCompute.h" // repeats the native/Work/reference exclusions
#include "PioneerQ38ExactQ4Corrected.h"
#ifndef PIONEER_Q38_PREPARED_BF16_Q4_COMPUTE_H
#define PIONEER_Q38_PREPARED_BF16_Q4_COMPUTE_H
// Distinct arithmetic producer, not native ownership or a Work selector.
// Consume EXISTING native-cache K-major SM3+escape faces directly. No raw
// nibble decode, learned floating products on the controller, PACK, global
// BFP8 expansion or changed cache recipe. Original metadata is unchanged.
//
// The two original2304B cache primaries remain CONTIGUOUS in L1; no face
// scatter/copy/header insertion. Forced-exponent UNPACK skips its16B header
// at address-16, as in the existing prepared integer worker, without parsing
// the preceding metadata/face bytes. Only each NoC span's guard is zeroed.
// Dense masks
// are expanded by the EXISTING sparse-mask reader before Group. MaskWord
// reads their four current faces; EscapeWord writes only the private BFP2
// scratch. These bit permutations are NOT model inference on the host.
// Inputs are ORIGINAL finite normal/zero BF16, not converted integer x.
//
// Numeric contract (separate from old per-weight BF16 and integer engine):
// Z=FPU(z,x), E=FPU(escape,x), S=FPU(ones,x), Q=(Z+E)+7*S,
// then acc += coefficient*Q + negativeMinimum*S, separately rounded.
// Z's two K16 halves precede E's two halves. Every product uses phases0/2.
// Full-BF16 reassociation is NOT exact integer reconstruction; whole-model
// quality must be tested separately. The independent original-byte oracle
// is PioneerQ38PreparedBf16Q4Reference, NEVER a Work fallback.
struct PioneerQ38PreparedBf16Q4Compute: PioneerQ38Q4FactoredCompute {
 using Base=PioneerQ38Q4FactoredCompute;
 enum : unsigned {Primary=0x90000,TileBytes=2304,Escape=0x8a000,EscapeStride=128,
  Ones=0x88800,BufferStride=0x10000,InputStride=64};
 static constexpr unsigned Face(unsigned buffer,unsigned group,unsigned outputHalf,unsigned inputHalf){
  return Primary+buffer*BufferStride+outputHalf*TileBytes+256+(group*2+inputHalf)*128;
 }
 template<class IO> static void Constants(IO& io){
  for(unsigned at=Zero-64;at<Zero+1024;at+=4)io.Write(at,0);
  for(unsigned at=Ones-64;at<Ones;at+=4)io.Write(at,0);
  for(unsigned at=Ones;at<Ones+128;at+=4)io.Write(at,0x11111111);
  for(unsigned face=0;face<4;face++)for(unsigned at=Escape+face*EscapeStride-64;at<Escape+face*EscapeStride;at+=4)io.Write(at,0);
 }
private:
 template<class E> struct MetadataSource {
  E& e;unsigned buffer;
  void Op(unsigned v){e.Op(v);}void Drain(){e.Drain();}
  void Config(unsigned i,unsigned v){
   if(i==76)for(unsigned half=0;half<2;half++)if(v==(Metadata+buffer*BufferStride+half*320)/16-1){
    e.Config(i,(Primary+buffer*BufferStride+half*TileBytes)/16-1);return;
   }
   e.Config(i,v);
  }
 };
 template<class E> static void Format(E& e,bool escape){
  e.Config(50,escape?0x17f:0x181); // exponent127 for SM1,129 for SM3
  e.Config(64,escape?0x0100001f:0x01000017);e.Config(72,escape?15:0x27);
 }
 template<class E> static void Matrix(E& e,unsigned address,unsigned input,unsigned dst){
  e.Config(76,address/16-1);e.Config(124,input/16-1);
  e.Op(0x3700000f);e.Op(0x5e23fc00);e.Op(0x5e403c00); // A256/B16
  e.Op(0x5160000b);e.Op(0x5460000f);
  e.Op(0x420000c1);e.Op(0x428000c1);e.Drain();
  e.Op(0x26084000|dst);e.Op(0x26c84000|dst);e.Drain();
 }
public:
 // Original metadata import changes the unpack layout. Restore the common
 // direct descriptor ONCE per completed K256 block, not ten times/group.
 // This owner must remain exclusive until the next LoadMetadata/END.
 template<class E> static bool LoadMetadata(E& e,unsigned buffer){
  if(buffer>=2)return false;
  MetadataSource<E> source{e,buffer};if(!Base::LoadMetadata(source,buffer))return false;
  e.Op(0xb2050004);e.Op(0xb21d4000);e.Drain();
  e.Config(0,0x42b5);e.Config(1,0x60aa0000);
  e.Config(65,0x00010001);e.Config(66,1);e.Config(67,0);e.Config(70,8);e.Config(71,0);
  e.Config(73,0x000f010f);e.Config(84,0x00400040);e.Config(86,0x01000100);
  e.Config(112,0x00100015);e.Config(113,0x00010001);e.Config(114,1);e.Config(115,0);
  e.Config(120,0x25);e.Config(121,0x000f000f);return true;
 }
 template<class E> static bool Group(E& e,unsigned buffer,unsigned group){
  if(buffer>=2 || group>=8)return false; // invalid geometry has ZERO IO
  // The prior group has completed before these private scratch writes.
  for(unsigned face=0;face<4;face++)for(unsigned pair=0;pair<8;pair++){
   const unsigned bits=e.MaskWord(buffer,group,face,pair);
   e.EscapeWord(face,pair*2,PioneerQ38ExactQ4Corrected::Spread(bits));
   e.EscapeWord(face,pair*2+1,PioneerQ38ExactQ4Corrected::Spread(bits>>16));
  }
  e.SealData();
  Format(e,false);
  e.Op(0x100c0002); // only scratch32..47; retain original metadata and acc
  for(unsigned half=0;half<2;half++)Matrix(e,Ones,F::Input+buffer*BufferStride+group*InputStride+half*32,32);
  e.Op(0x8a001002);PioneerSfpuMove(e,7,15);PioneerSfpuAddI(e,7,-16);e.Op(0x7b000700);
  e.Op(0x70130020); // only defined first-row lanes0..7, genuine on-card S
  e.Op(0x100c0002);
  for(unsigned output=0;output<2;output++)for(unsigned half=0;half<2;half++)
   Matrix(e,Face(buffer,group,output,half),F::Input+buffer*BufferStride+group*InputStride+half*32,32+output*8);
  Format(e,true);
  for(unsigned output=0;output<2;output++)for(unsigned half=0;half<2;half++)
   Matrix(e,Escape+(output*2+half)*EscapeStride,F::Input+buffer*BufferStride+group*InputStride+half*32,32+output*8);
  for(unsigned half=0;half<2;half++)for(unsigned parity=0;parity<=2;parity+=2){
   const unsigned row=M::Row(group,half),acc=Accumulator+half*8;
   e.Op(0x70230000|row|parity);e.Op(0x70330000|(row+4)|parity);
   e.Op(0x70030000|(half?40:32)|parity);e.Op(0x70430000|acc|parity);
   PioneerSfpuLoad32(e,5,0x40e00000); // 7.0f; no changed affine coefficient
   e.Op(0x86051962);e.Op(0x8f000000); // r6=7*S, separate FP32 rounding
   e.Op(0x850a0600);e.Op(0x8f000000); // r0=(Z+E)+r6, BEFORE affine
   PioneerQ38Q4FactoredBfp8::GroupAffine(e);
   e.Op(0x72430000|acc|parity);
  }
  e.Op(0x8a000002);e.Drain();return true;
 }
};
static_assert(PioneerQ38PreparedBf16Q4Compute::Primary+2*2304<0x94000
 && PioneerQ38PreparedBf16Q4Compute::Escape+4*128<0x8b000,"distinct bounded L1 proposals, NOT ownership");
#endif
