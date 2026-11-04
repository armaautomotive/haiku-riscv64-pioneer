/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_NATIVE_DRAM_BAND_LAYOUT_H
#define PIONEER_NATIVE_DRAM_BAND_LAYOUT_H
// Separate direct-GDDR layout DATA. Existing qualified1104/80 layout stays
// unchanged. Packed GDDR/cache tiles remain1088/64 bytes. ONLY the private
// L1 strides grow,so both addresses are congruent mod32 on EVERY read.
// These fixed diagnostic offsets grant no physical range/route/owner lease.
struct PioneerNativeDramBandLayout {
	static constexpr unsigned Weight=0x90000,Input=0x126000,Output=0x134000;
	static constexpr unsigned WeightPayload=1088,WeightStride=1120,InputPayload=64,InputStride=96,Header=32;
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
static_assert(!(PioneerNativeDramBandLayout::Weight&31) && !(PioneerNativeDramBandLayout::Input&31)
	&& !(PioneerNativeDramBandLayout::WeightStride&31) && !(PioneerNativeDramBandLayout::InputStride&31)
	&& !(PioneerNativeDramBandLayout::WeightPayload&63) && !(PioneerNativeDramBandLayout::InputPayload&63),"C32 direct GDDR reads,whole64B flits");
static_assert(PioneerNativeDramBandLayout::WeightEnd(17408)<=PioneerNativeDramBandLayout::Input-32
	&& PioneerNativeDramBandLayout::InputEnd(17408)<=PioneerNativeDramBandLayout::Output
	&& PioneerNativeDramBandLayout::Output+256<=0x16c000
	&& PioneerNativeDramBandLayout::DramWeight+544*1088<=PioneerNativeDramBandLayout::DramInput
	&& PioneerNativeDramBandLayout::DramInput+544*64<=0x04800000,"bounded disjoint L1/GDDR DATA spans");
#endif
