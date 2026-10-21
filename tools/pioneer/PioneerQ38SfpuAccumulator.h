/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_SFPU_ACCUMULATOR_H
#define PIONEER_Q38_SFPU_ACCUMULATOR_H
#include "PioneerQ38VectorAffine.h"
#include "PioneerSfpuDivide.h"
#if defined(PIONEER_Q38_WORK_DOT_ZERO_ELISION) && (!defined(PIONEER_Q38_WORK_DEFER_AFFINE) || !defined(PIONEER_Q38_SFPU_INTEGER) || !defined(PIONEER_Q38_WORK_ZERO_MINIMUM) || !defined(PIONEER_Q38_BLOCK_SCALE_REUSE))
#error Work dot-zero elision requires the separately proved exact integer Work accumulator chain
#endif
#ifdef PIONEER_Q38_WORK_AFFINE_REPLAY
#if !defined(PIONEER_Q38_BLOCK_AFFINE_DST) || !defined(PIONEER_Q38_BLOCK_SCALE_REUSE) || !defined(PIONEER_Q38_WORK_ZERO_MINIMUM) || !defined(PIONEER_Q38_WORK_DEFER_AFFINE) || !defined(PIONEER_Q38_NATIVE_OPERAND_STORE)
#error Affine replay requires its separate exact scale-cache/native-operand Work chain
#endif
#include "PioneerQ38AffineReplay.h"
#endif
#ifdef PIONEER_Q38_WORK_ZERO_MINIMUM
#include "PioneerQ38ZeroMinimumAffine.h"
#endif
// AFTER every matrix drain. Checked jobs also read/check ALL16 dot bounds;
// the separately gated Work selector may rely on the fixed integer-input
// proof instead. No address/length/source proof depends on numeric dots.
// The qualified vector unpack has ALREADY enabled ALL32 lanes;matrix/RISC checks do not
// alter predication or issue new asynchronous work. Do NOT repeat those
// two setup operations here;keep EVERY actual matrix/metadata/final wait.
// Owned Dst32:coef96..99,min100..103,accumulator108..111. Raw/decoded
// weights and the FP32 dot-only ZEROACC32..47 do NOT overlap these.
// Caller FIRST defines ALL metadata rows with full-row UNPACK. Initialize ALL padding
// each group. Reload immutable Q8/Q4 metadata on even panels,Q6 each panel.
// Q6 minimum is ALWAYS+0 and remains the explicitly initialized zero.
// Metadata UNPACK NEVER publishes/flips SrcA. Integer=true preserves the
// fixed INTEGER matrix/FP32 destination fields, loads SM32 integer dots
// and explicitly casts before the SAME non-fused FP32 affine sequence.
// Integer=false is the older OFFLINE BF16 research path, NOT a fallback.
// Blackhole mode4 reads sign-magnitude Integer32 WITHOUT a two's-complement
// conversion: https://github.com/tenstorrent/tt-isa-documentation/blob/main/BlackholeA0/TensixTile/TensixCoprocessor/SFPLOAD.md
// Restore the matching raw format
// and Thread5=4 before any following SrcB operation. No NoC/host authority.
#if defined(__GNUC__) && !defined(__clang__)
#define PIONEER_Q38_ACCUMULATOR_SIZE __attribute__((noinline,optimize("Os")))
#else
#define PIONEER_Q38_ACCUMULATOR_SIZE
#endif
#ifdef PIONEER_Q38_BLOCK_AFFINE_DST
template<bool Cached> struct PioneerQ38AffineRow {
	template<class E> static unsigned Get(E&) { return 96; }
};
template<> struct PioneerQ38AffineRow<true> {
	template<class E> static unsigned Get(E& e) { return e.BlockAffineRow(); }
};
#endif
template<class E,bool Integer=false
#ifdef PIONEER_Q38_WORK_ZERO_MINIMUM
	,bool ZeroMinimum=false
#endif
#ifdef PIONEER_Q38_WORK_DEFER_AFFINE
	,bool Defer=false
#endif
#ifdef PIONEER_Q38_BLOCK_AFFINE_DST
	,bool BlockMetadata=false
#endif
> PIONEER_Q38_ACCUMULATOR_SIZE
static bool PioneerQ38SfpuAccumulator(E& e,unsigned type,bool first,bool metadata,uint32_t scale,int sum)
{
	unsigned mag=scale&0x7fffffff;
	unsigned metadataRow=96;
#ifdef PIONEER_Q38_BLOCK_AFFINE_DST
	if (BlockMetadata) {
		static_assert(!BlockMetadata || (Integer && Defer && !ZeroMinimum),"exact Q4 Work cache only");
		metadataRow=PioneerQ38AffineRow<BlockMetadata>::Get(e);
		if (type!=12 || metadataRow<128 || metadataRow>184 || (metadataRow&7)) return false;
	}
#endif
	const int sumBound=
#ifdef PIONEER_Q38_Q4_GROUP32
		BlockMetadata?4096:
#endif
		2048;
	if ((type!=8 && type!=12 && type!=14) || sum<-sumBound || sum>sumBound || (first && !metadata)
		|| (mag && (mag<0x2f800000 || mag>0x47800000))) return false;
#ifdef PIONEER_Q38_WORK_ZERO_MINIMUM
	static_assert(!ZeroMinimum || Integer,"zero-minimum specialization is exact INTEGER Work only");
	const bool zeroMinimum=ZeroMinimum;
#ifdef PIONEER_Q38_NATIVE_Q8
	const bool directNativeQ8=ZeroMinimum && type==8;
#endif
	if (ZeroMinimum && type==12) return false;
	// ALL accumulator lanes are explicitly +0 at first group panel below.
	// Q8/Q6 metadata minima are +0,ALL products remain finite normal/zero.
	// RN addition from +0 or normal cannot produce -0. The required +0
	// induction therefore holds across EVERY original panel of this group.
#endif
	e.Op(0x3700000f);
	if (first) {
		e.Op(0x71720000);
		for (unsigned slot=0;slot<3;slot++) for (unsigned parity=0;parity<4;parity+=2)
			e.Op(0x72770000|(slot==2?108U:96U+slot*4)|parity);
		e.Drain();
	}
	if (metadata
#ifdef PIONEER_Q38_BLOCK_AFFINE_DST
		&& !BlockMetadata
#endif
	) {
		e.Op(0x5e203c00); e.Op(0x5120000a); e.Op(0x5420000f); e.Op(0xb2050000); e.Drain();
#ifdef PIONEER_Q38_NATIVE_Q8
		if(directNativeQ8)e.Config(73,0x13); // FP32 metadata UNPACK-to-Dst,not SrcA publication
#endif
		e.Config(1,Integer?0xa0200000:0x20200000); e.Config(64,0x100010); e.Config(65,1); e.Config(72,0);
		for (unsigned kind=0;kind<(
#ifdef PIONEER_Q38_WORK_ZERO_MINIMUM
			zeroMinimum ||
#endif
			type==14?1U:2U);kind++) {
			unsigned row=96+kind*4,address=0x91200+kind*64;
			e.Config(84,0x00400000+(row+4)*16); e.Config(76,(address-16)/16);
			e.Op(0x42000081); e.Drain(); // no bank publication or flip
		}
		e.Config(1,
#ifdef PIONEER_Q38_NATIVE_Q8
			directNativeQ8?0xa03c0000:
#endif
			Integer?0xa03c8000:0x203c8000); e.Config(64,0x10001e); e.Config(65,16); e.Config(72,14); e.Config(84,0x00400040);
#ifdef PIONEER_Q38_NATIVE_Q8
		if(directNativeQ8)e.Config(73,3);
#endif
		e.Op(0xb2050004); e.Op(0x5e23fc00); e.Drain(); e.Op(0x3700000f);
	}
#ifdef PIONEER_Q38_WORK_AFFINE_REPLAY
	const bool replayAffine=BlockMetadata || (ZeroMinimum && type==8);
	// Disjoint scale register is stable through BOTH parity updates. Q4
	// still uses its exact once/block scale;Q8 keeps original scale bits.
	if(replayAffine && !BlockMetadata)PioneerSfpuLoad32(e,3,scale);
#endif
	for (unsigned parity=0;parity<4;parity+=2) {
		e.Op((Integer?0x70040000:0x70030000)|32|parity); // canonical SM32 or exact FP32 integer dot
		e.Op(0x70130000|metadataRow|parity);
#ifdef PIONEER_Q38_WORK_ZERO_MINIMUM
		if (!zeroMinimum)
#endif
		e.Op(0x70230000|(metadataRow+4)|parity);
		e.Op(0x70530000|108|parity);
#ifdef PIONEER_Q38_BLOCK_SCALE_REUSE
		if (!BlockMetadata
#ifdef PIONEER_Q38_WORK_AFFINE_REPLAY
			&& !replayAffine
#endif
		)
#endif
		PioneerSfpuLoad32(e,3,scale);
#ifdef PIONEER_Q38_WORK_ZERO_MINIMUM
		if (!zeroMinimum)
#endif
		PioneerSfpuLoad32(e,4,(sum<0?0x80000000U:0)|unsigned(sum<0?-sum:sum));
#ifdef PIONEER_Q38_WORK_AFFINE_REPLAY
		if(replayAffine)PioneerQ38AffineReplay(e,zeroMinimum,first && !parity);
		else {
#endif
		// Canonicalize ONLY signed zero (SM32 or IEEE);all nonzero bits stay.
#ifdef PIONEER_Q38_WORK_DOT_ZERO_ELISION
		if(!Defer) { // checked retains canonicalization;Work's +0 induction covers Q8/Q4/Q6
#endif
		PioneerSfpuLoad32(e,6,0x7fffffff); PioneerSfpuAnd(e,7,0,6);
		PioneerSfpuAdd(e,7,6); PioneerSfpuShift(e,7,-31,true); PioneerSfpuAnd(e,0,0,7);
#ifdef PIONEER_Q38_WORK_DOT_ZERO_ELISION
		}
#endif
#ifdef PIONEER_Q38_WORK_ZERO_MINIMUM
		if (zeroMinimum) PioneerQ38ZeroMinimumAffine(e); else
#endif
		if (Integer) PioneerQ38VectorAffine(e); else PioneerQ38VectorAffineFloatDot(e);
#ifdef PIONEER_Q38_WORK_AFFINE_REPLAY
		}
#endif
		e.Op(0x72030000|108|parity);
	}
#ifdef PIONEER_Q38_WORK_DEFER_AFFINE
	// OFFLINE candidate:Work's next raw-unpack entry already drains BEFORE
	// any CSR/config change. Only disjoint L1/NoC staging may overlap these
	// accumulator stores. Caller MUST drain BEFORE final group Dst export.
	if (!Defer) e.Drain();
#else
	e.Drain();
#endif
	return true;
}
#undef PIONEER_Q38_ACCUMULATOR_SIZE
#endif
