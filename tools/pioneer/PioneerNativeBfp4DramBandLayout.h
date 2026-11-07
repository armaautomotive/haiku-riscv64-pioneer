/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_NATIVE_BFP4_DRAM_BAND_LAYOUT_H
#define PIONEER_NATIVE_BFP4_DRAM_BAND_LAYOUT_H
// Fixed synthetic diagnostic geometry only. Prior C64 BFP8 diagnostic
// covered these windows, but this layout DOES NOT grant ownership of them.
// Packed BFP4 payloads are576 bytes, each L1 tile has a separate64B header.
struct PioneerNativeBfp4DramBandLayout {
 static constexpr unsigned Weight=0x90000,Input=0x12c000,Output=0x140000;
 static constexpr unsigned WeightPayload=576,WeightStride=640,InputPayload=64,InputStride=128,Header=64;
 static constexpr unsigned DramWeight=0x04000000,DramInput=0x04100000;
 static constexpr bool Width(unsigned k){return k==5120 || k==6144 || k==17408;}
 static constexpr unsigned Tiles(unsigned k){return Width(k)?k/32:0;}
 static constexpr unsigned WeightEnd(unsigned k){return Weight-Header+Tiles(k)*WeightStride;}
 static constexpr unsigned InputEnd(unsigned k){return Input-Header+Tiles(k)*InputStride;}
 struct Read {unsigned source,destination,bytes;};
 static bool Packet(unsigned k,unsigned operand,unsigned tile,Read& result){
  if(!Width(k) || operand>1 || tile>=Tiles(k))return false;
  Read next={operand?DramInput+tile*InputPayload:DramWeight+tile*WeightPayload,
   operand?Input+tile*InputStride:Weight+tile*WeightStride,operand?InputPayload:WeightPayload};
  result=next;return true;
 }
};
static_assert(PioneerNativeBfp4DramBandLayout::WeightStride==PioneerNativeBfp4DramBandLayout::WeightPayload+PioneerNativeBfp4DramBandLayout::Header
 && PioneerNativeBfp4DramBandLayout::InputStride==PioneerNativeBfp4DramBandLayout::InputPayload+PioneerNativeBfp4DramBandLayout::Header,"no header gaps");
static_assert(!(PioneerNativeBfp4DramBandLayout::Weight&63) && !(PioneerNativeBfp4DramBandLayout::Input&63)
 && !(PioneerNativeBfp4DramBandLayout::WeightStride&63) && !(PioneerNativeBfp4DramBandLayout::InputStride&63)
 && !(PioneerNativeBfp4DramBandLayout::WeightPayload&63) && !(PioneerNativeBfp4DramBandLayout::InputPayload&63),"C64 packed GDDR geometry");
static_assert(PioneerNativeBfp4DramBandLayout::WeightEnd(17408)<=PioneerNativeBfp4DramBandLayout::Input-PioneerNativeBfp4DramBandLayout::Header
 && PioneerNativeBfp4DramBandLayout::InputEnd(17408)<=PioneerNativeBfp4DramBandLayout::Output
 && PioneerNativeBfp4DramBandLayout::Output+256<=0x16c000
 && PioneerNativeBfp4DramBandLayout::DramWeight+544*576<=PioneerNativeBfp4DramBandLayout::DramInput
 && PioneerNativeBfp4DramBandLayout::DramInput+544*64<=0x04800000,"bounded disjoint L1/GDDR DATA spans");
#endif
