/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_NATIVE_BFP_BAND_LAYOUT_H
#define PIONEER_NATIVE_BFP_BAND_LAYOUT_H
#include <stdint.h>
// Fixed geometry DATA shared by the separate full-K owner/adapter. No
// instructions,model data,allocation,route or arbitrary-address capability.
struct PioneerNativeBfpBandLayout {
	static constexpr unsigned Weight=0x90000,Input=0x124000,Output=0x130000;
	static constexpr unsigned WeightPayload=1088,WeightStride=1104,InputPayload=64,InputStride=80;
	static constexpr bool Width(unsigned k){return k==5120 || k==6144 || k==17408;}
	static constexpr unsigned Tiles(unsigned k){return Width(k)?k/32:0;}
	static constexpr unsigned WeightEnd(unsigned k){return Weight-16+Tiles(k)*WeightStride;}
	static constexpr unsigned InputEnd(unsigned k){return Input-16+Tiles(k)*InputStride;}
};
static_assert(PioneerNativeBfpBandLayout::WeightEnd(17408)<=PioneerNativeBfpBandLayout::Input-16
	&& PioneerNativeBfpBandLayout::InputEnd(17408)<=PioneerNativeBfpBandLayout::Output
	&& PioneerNativeBfpBandLayout::Output+256<=0x16c000,"disjoint fixed full-K owned L1 extents");
#endif
