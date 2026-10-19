/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_SFPU_ACCUMULATOR_H
#define PIONEER_Q38_SFPU_ACCUMULATOR_H
#include "PioneerQ38VectorAffine.h"
#include "PioneerSfpuDivide.h"
// OFFLINE ONLY. AFTER every matrix drain+ALL16 finite/bounded dot checks.
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
template<class E,bool Integer=false> PIONEER_Q38_ACCUMULATOR_SIZE
static bool PioneerQ38SfpuAccumulator(E& e,unsigned type,bool first,bool metadata,uint32_t scale,int sum)
{
	unsigned mag=scale&0x7fffffff;
	if ((type!=8 && type!=12 && type!=14) || sum<-2048 || sum>2048 || (first && !metadata)
		|| (mag && (mag<0x2f800000 || mag>0x47800000))) return false;
	e.Op(0x3700000f);
	if (first) {
		e.Op(0x71720000);
		for (unsigned slot=0;slot<3;slot++) for (unsigned parity=0;parity<4;parity+=2)
			e.Op(0x72770000|(slot==2?108U:96U+slot*4)|parity);
		e.Drain();
	}
	if (metadata) {
		e.Op(0x5e203c00); e.Op(0x5120000a); e.Op(0x5420000f); e.Op(0xb2050000); e.Drain();
		e.Config(1,Integer?0xa0200000:0x20200000); e.Config(64,0x100010); e.Config(65,1); e.Config(72,0);
		for (unsigned kind=0;kind<(type==14?1U:2U);kind++) {
			unsigned row=96+kind*4,address=0x91200+kind*64;
			e.Config(84,0x00400000+(row+4)*16); e.Config(76,(address-16)/16);
			e.Op(0x42000081); e.Drain(); // no bank publication or flip
		}
		e.Config(1,Integer?0xa03c8000:0x203c8000); e.Config(64,0x10001e); e.Config(65,16); e.Config(72,14); e.Config(84,0x00400040);
		e.Op(0xb2050004); e.Op(0x5e23fc00); e.Drain(); e.Op(0x3700000f);
	}
	for (unsigned parity=0;parity<4;parity+=2) {
		e.Op((Integer?0x70040000:0x70030000)|32|parity); // canonical SM32 or exact FP32 integer dot
		e.Op(0x70130000|96|parity); e.Op(0x70230000|100|parity); e.Op(0x70530000|108|parity);
		PioneerSfpuLoad32(e,3,scale);
		PioneerSfpuLoad32(e,4,(sum<0?0x80000000U:0)|unsigned(sum<0?-sum:sum));
		// Canonicalize ONLY signed zero (SM32 or IEEE);all nonzero bits stay.
		PioneerSfpuLoad32(e,6,0x7fffffff); PioneerSfpuAnd(e,7,0,6);
		PioneerSfpuAdd(e,7,6); PioneerSfpuShift(e,7,-31,true); PioneerSfpuAnd(e,0,0,7);
		if (Integer) PioneerQ38VectorAffine(e); else PioneerQ38VectorAffineFloatDot(e);
		e.Op(0x72030000|108|parity);
	}
	e.Drain(); return true;
}
#undef PIONEER_Q38_ACCUMULATOR_SIZE
#endif
