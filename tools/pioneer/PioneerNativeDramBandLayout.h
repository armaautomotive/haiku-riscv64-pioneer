/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_NATIVE_DRAM_BAND_LAYOUT_H
#define PIONEER_NATIVE_DRAM_BAND_LAYOUT_H
#include "../../src/add-ons/kernel/drivers/misc/PioneerNativeDramLoadVariant.h"
// Separate direct-GDDR layout DATA. Existing qualified1104/80 layout stays
// unchanged. Packed GDDR/cache tiles remain1088/64 bytes. ONLY the private
// L1 strides grow,so both addresses are congruent mod32 on EVERY read.
// These fixed diagnostic offsets grant no physical range/route/owner lease.
// Explicit diagnostic83/84 compile options use C64 starts/64B headers.
// It changes L1 placement ONLY,not packed GDDR/cache bytes or fidelity.
struct PioneerNativeDramBandLayout {
	using V=PioneerNativeDramLoadVariant;
	static constexpr unsigned Weight=0x90000,Input=V::Input,Output=V::Output;
	static constexpr unsigned WeightPayload=1088,WeightStride=V::WeightStride,InputPayload=64,InputStride=V::InputStride,Header=V::Header;
	static constexpr unsigned DramWeight=0x04000000,DramInput=0x04100000;
	static constexpr bool Width(unsigned k){return k==5120 || k==6144 || k==17408;}
	static constexpr unsigned Tiles(unsigned k){return Width(k)?k/32:0;}
	static constexpr unsigned WeightEnd(unsigned k){return Weight-Header+Tiles(k)*WeightStride;}
	static constexpr unsigned InputEnd(unsigned k){return Input-Header+Tiles(k)*InputStride;}
	struct Read {unsigned source,destination,bytes;};
	static bool Packet(unsigned k,unsigned operand,unsigned tile,Read& result) {
		if(!Width(k) || operand>1 || tile>=Tiles(k))return false;
		Read next={operand?DramInput+tile*InputPayload:DramWeight+tile*WeightPayload,
			operand?Input+tile*InputStride:Weight+tile*WeightStride,operand?InputPayload:WeightPayload};
		result=next;return true;
	}
};
static_assert(PioneerNativeDramBandLayout::WeightStride==PioneerNativeDramBandLayout::WeightPayload+PioneerNativeDramBandLayout::Header
	&& PioneerNativeDramBandLayout::InputStride==PioneerNativeDramBandLayout::InputPayload+PioneerNativeDramBandLayout::Header,"complete header/payload extents,NO unchecked placement gap");
static_assert(!(PioneerNativeDramBandLayout::Weight&31) && !(PioneerNativeDramBandLayout::Input&31)
	&& !(PioneerNativeDramBandLayout::WeightStride&31) && !(PioneerNativeDramBandLayout::InputStride&31)
	&& !(PioneerNativeDramBandLayout::WeightPayload&63) && !(PioneerNativeDramBandLayout::InputPayload&63),"C32 direct GDDR reads,whole64B flits");
static_assert(PioneerNativeDramLoadVariant::Role<83 || (!(PioneerNativeDramBandLayout::Weight&63)
	&& !(PioneerNativeDramBandLayout::Input&63) && !(PioneerNativeDramBandLayout::WeightStride&63)
	&& !(PioneerNativeDramBandLayout::InputStride&63) && PioneerNativeDramBandLayout::Header==64
	&& PioneerNativeDramLoadVariant::L1Words==55372 && PioneerNativeDramLoadVariant::HeaderWords==5120),"exact C64 placement/journal control");
static_assert(PioneerNativeDramBandLayout::WeightEnd(17408)<=PioneerNativeDramBandLayout::Input-PioneerNativeDramBandLayout::Header
	&& PioneerNativeDramBandLayout::InputEnd(17408)<=PioneerNativeDramBandLayout::Output
	&& PioneerNativeDramBandLayout::Output+256<=0x16c000
	&& PioneerNativeDramBandLayout::DramWeight+544*1088<=PioneerNativeDramBandLayout::DramInput
	&& PioneerNativeDramBandLayout::DramInput+544*64<=0x04800000,"bounded disjoint L1/GDDR DATA spans");
#endif
