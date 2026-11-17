/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_TWO_TOKEN_Q4_AFFINE_H
#define PIONEER_Q38_TWO_TOKEN_Q4_AFFINE_H
#if defined(_KERNEL_MODE) || defined(PIONEER_Q38_FULL_OFFLOAD_NATIVE) \
 || defined(PIONEER_Q38_WORK_INFERENCE) || defined(PIONEER_Q38_CPU_VALIDATION) \
 || defined(PIONEER_Q38_FULL_MODEL_CPU_REFERENCE) || defined(__FAST_MATH__)
#error Paired vector Q4 affine needs a separately qualified test image
#endif
#include "PioneerQ38VectorQ4Metadata.h"

template<class E> struct PioneerQ38TwoTokenQ4MetadataEmitter {
	E& inner;
	void Op(uint32_t word) { inner.Op(word); }
	void Config(unsigned index,uint32_t value) { inner.Config(index,value); }
	void Drain() { inner.Drain(); }
};
template<class E> struct PioneerQ38VectorQ4MetadataPairRows<PioneerQ38TwoTokenQ4MetadataEmitter<E>> {
	static constexpr bool Enabled=true;
};

// Local arithmetic only. The caller owes exclusive Dst ownership, a
// completed nonbroadcast integer MVMUL with rows0/1 independent and
// rows2/3 zero, and preserved scratch across subsequent dot-only clears.
// No cross-token reduction, activation rounding, or inference selection.
struct PioneerQ38TwoTokenQ4Affine {
	enum { Dot=32, Acc=108, Scale=112, Sum=116, Metadata=128 };
	static_assert(Dot+16<=Acc && Acc+4==Scale && Scale+4==Sum && Sum+4<=Metadata,
		"two-token accumulator and scratch must survive dot-only clears");

	static bool ValidScale(uint32_t bits)
	{
		const uint32_t mag=bits&0x7fffffff;
		return !mag || (mag>=0x2f800000 && mag<=0x47800000);
	}

	template<class E> static void Begin(E& e)
	{
		e.Op(0x3700000f);
		e.Op(0x8a000002);
		e.Op(0x71720000);
		e.Op(0x72730000|Acc);e.Op(0x72730000|Acc|2);
		e.Drain();
	}

	template<class E> static bool Block(E& e,unsigned raw,uint32_t a,uint32_t b)
	{
		if (!ValidScale(a) || !ValidScale(b)) return false;
		// One original packed metadata prefix. FiniteBlock is a separate
		// required caller check, as in the single-row vector worker.
		PioneerQ38TwoTokenQ4MetadataEmitter<E> metadata={e};
		if (!PioneerQ38VectorQ4Metadata::Block(metadata,raw,a)) return false;
		Rows<Scale,false>(e,a,b);
		e.Drain();
		return true;
	}

	template<class E> static bool Panel(E& e,unsigned panel,int a,int b)
	{
		if (panel>=16 || a<-2048 || a>2048 || b<-2048 || b>2048) return false;
		e.Op(0x3700000f);
		Rows<Sum,true>(e,Signed(a),Signed(b));
		const unsigned row=Metadata+(panel/2)*8;
		for (unsigned parity=0;parity<4;parity+=2) {
			e.Op(0x70040000|Dot|parity); // original SM32 dot, possibly signed zero
			e.Op(0x70130000|row|parity);
			e.Op(0x70230000|(row+4)|parity);
			e.Op(0x70330000|Scale|parity);
			e.Op(0x70430000|Sum|parity); // already exact FP32
			e.Op(0x70530000|Acc|parity);
			// Same signed-zero canonicalization as checked vector Q4.
			PioneerSfpuLoad32(e,6,0x7fffffff);PioneerSfpuAnd(e,7,0,6);
			PioneerSfpuAdd(e,7,6);PioneerSfpuShift(e,7,-31,true);PioneerSfpuAnd(e,0,0,7);
			e.Op(0x90000000);
			PioneerQ38VectorAffineMul(e,1,3,6);
			PioneerQ38VectorAffineMul(e,2,3,7);
			PioneerQ38VectorAffineMul(e,6,0,0);
			PioneerQ38VectorAffineMul(e,7,4,4);
			PioneerQ38VectorAffineAdd(e,0,4,0);
			PioneerQ38VectorAffineAdd(e,5,0,0);
			e.Op(0x72030000|Acc|parity);
		}
		e.Drain();
		return true;
	}

private:
	static uint32_t Signed(int v) { return v<0?0x80000000U|unsigned(-v):unsigned(v); }

	template<unsigned Slot,bool Cast,class E> static void Rows(E& e,uint32_t a,uint32_t b)
	{
		PioneerSfpuLoad32(e,0,a);PioneerSfpuLoad32(e,1,b);
		for (unsigned r=2;r<8;r++) PioneerSfpuLoad32(e,r,0);
		e.Op(0x8c000000); // LReg0 subrows now a,b,0,0; parity-independent
		if (Cast) e.Op(0x90000000); // store FP32, never flush a raw small integer as denormal
		e.Op(0x72030000|Slot);e.Op(0x72030000|Slot|2);
	}
};
#endif
