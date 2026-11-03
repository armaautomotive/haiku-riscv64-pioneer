/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_NATIVE_BFP_BAND_H
#define PIONEER_Q38_NATIVE_BFP_BAND_H
#if defined(_KERNEL_MODE) || defined(PIONEER_Q38_WORK_INFERENCE) || defined(PIONEER_Q38_CPU_VALIDATION)
#error Native BFP band is an unqualified offboard prototype, never driver/Work/validation
#endif
#include "PioneerQ38NativeBfpMatvec.h"
// One COMPLETE32-output-row band, all K tiles, entirely on-card after launch.
// Caller owes exclusive idle FPU/unpacker/banks/Config0 and immutable owned L1.
// All payloads must already be validated canonical BFP8 and finite normal/zero
// BF16 inputs, with correct extents/zero headers; this producer does NOT validate
// or authenticate scalar data, addresses, model/cache keys or allocations.
// Every tile has its OWN16-byte header BEFORE the payload: no overlapping header
// assumptions. No source of file/DRAM/NoC/device/lifecycle authority exists here.
// Phases4=all BF16 fidelity; Phases1=EXPLICIT reduced-precision phase0 ONLY.
// The two are never numerical substitutes. Neither is selected by installed Work.
// Broadcast reads ONLY Dst rows32/40, whose bit2 is clear and valid bit defined;
// other broadcast rows can remain undefined/contain stale bits and are NOT output.
struct PioneerQ38NativeBfpBand {
	static constexpr unsigned Weight=0x90000,Input=0x124000,Output=0x130000;
	static constexpr unsigned WeightPayload=1088,WeightStride=1104,InputPayload=64,InputStride=80;
	static constexpr bool Width(unsigned k){return k==5120 || k==6144 || k==17408;}
	static constexpr unsigned Tiles(unsigned k){return Width(k)?k/32:0;}
	static constexpr unsigned WeightEnd(unsigned k){return Weight-16+Tiles(k)*WeightStride;}
	static constexpr unsigned InputEnd(unsigned k){return Input-16+Tiles(k)*InputStride;}
private:
	template<unsigned Face,unsigned Phases,class E> static __attribute__((always_inline)) inline void FaceRun(E& e) {
		static_assert(Face<4 && (Phases==1 || Phases==4),"fixed native face/fidelity variant");
		if(Face){e.Op(0x37040201);e.Op(0x37040201);} // next16 SrcA rows
		e.Op(Face&1?0x3700040a:0x3700000a); // B row1/0, fidelity0, no implied broadcast alignment
		if(Phases==4) { // retain ALL four terms in the high-fidelity prototype
			e.Op(0x26084000|(Face<2?32:40));
			e.Op(0x26084000|(Face<2?32:40));
			e.Op(0x26084000|(Face<2?32:40));
		}
		e.Op(0x26084000|(Face<2?32:40)|(Face==3?0x00c00000:0)); // ONLY final face/phase releases both banks
	}
public:
	template<unsigned Phases,class E> static bool Run(E& e,unsigned k) {
		static_assert(Phases==1 || Phases==4,"explicit separate precision variant required");
		if(!Width(k))return false; // no command/config access on invalid geometry
		if(!PioneerQ38NativeBfpMatvec::Setup(e,8))return false;
		e.Config(112,0x00200015); // exactly32 contiguous BF16 vector values, not8 duplicate rows
		e.Op(0x3700000f);e.Op(0x10180000);e.Drain(); // clear Dst ONCE for this complete band
		unsigned weight=Weight/16-1,input=Input/16-1;
		for(unsigned tile=0;tile<Tiles(k);tile++,weight+=WeightStride/16,input+=InputStride/16) {
			e.Config(76,weight);e.Config(124,input);
			e.Op(0x3700000f); // per-tile source/fidelity RWCs only; NEVER clear accumulated Dst
			e.Op(0x5e2ffc00);e.Op(0x5e407c00); // U0=1024 packed weights;U1=32 BF16 inputs
			e.Op(0x5160000b);e.Op(0x5460000f);
			e.Op(0x420000c1);e.Op(0x428000c1);e.Drain();
			FaceRun<0,Phases>(e);FaceRun<1,Phases>(e);FaceRun<2,Phases>(e);FaceRun<3,Phases>(e);
			e.Drain(); // REAL completion before descriptor rewrite or final scalar read
		}
		return true;
	}
};
static_assert(PioneerQ38NativeBfpBand::WeightEnd(17408)<=PioneerQ38NativeBfpBand::Input-16
	&& PioneerQ38NativeBfpBand::InputEnd(17408)<=PioneerQ38NativeBfpBand::Output
	&& PioneerQ38NativeBfpBand::Output+256<=0x16c000,"private prototype L1 extents disjoint and bounded");
#endif
