/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_VECTOR_Q4_METADATA_H
#define PIONEER_Q38_VECTOR_Q4_METADATA_H
#if defined(_KERNEL_MODE) || defined(PIONEER_Q38_WORK_INFERENCE) \
 || defined(PIONEER_Q38_CPU_VALIDATION) || defined(PIONEER_Q38_FULL_MODEL_CPU_REFERENCE) || defined(__FAST_MATH__)
#error Vector Q4 metadata is a separate shader arithmetic prerequisite,not a driver or CPU inference path
#endif
#include "PioneerSfpuDivide.h"
#include "PioneerQ38VectorAffine.h"

// ORIGINAL Q4_K metadata arithmetic in parallel lanes. No BF16 conversion,
// weight requantization,product upload,scalar metadata reads,IO or fallback.
// A separately selected worker candidate may use Block below. The arithmetic
// helpers alone do NOT establish raw-byte provenance or lifetime ownership.
struct PioneerQ38VectorQ4Metadata {
	static bool FiniteHalf(unsigned h) { return h<=65535 && (h&0x7c00)!=0x7c00; }
	static constexpr unsigned ScaleByte(unsigned group) { return group<4?4+group:8+group; }
	static constexpr unsigned MinimumByte(unsigned group) { return group<4?8+group:group; }
	static constexpr unsigned ExtraByte(unsigned group) { return group<4?0:4+group; }
	static bool RawBase(unsigned base)
	{
		return base==0x90000 || base==0x98000 || base==0x98900 || base==0x9a000 || base==0x9a900;
	}
	// Structural finite-domain check ONLY,not CPU inference/validation. Read
	// the eight ORIGINAL high-byte words (no scalar HALF conversion/products).
	// Bits2..6 of each high byte are the complete HALF exponent. This SWAR
	// test cannot cross byte boundaries at its four selected bit2 positions.
	static bool FiniteBlock(const volatile uint32_t* tile,unsigned bytes)
	{
		if(!tile || (reinterpret_cast<__UINTPTR_TYPE__>(tile)&3) || bytes!=2304)return false;
		for(unsigned base=4;base<=12;base+=8)for(unsigned word=0;word<4;word++) {
			const uint32_t high=tile[base+word];
			if(high&(high>>1)&(high>>2)&(high>>3)&(high>>4)&0x04040404)return false;
		}
		return true;
	}

	// LReg0:finite original HALF bits,NOT a float. Output0:EXACT FP32.
	// Preserve1;clobber2..7. HALF subnormals become NORMAL FP32,so the
	// Blackhole MAD's FP32-denormal flush cannot change any finite result.
	// sig=mantissa+(normal?1024:0), scale=2^(max(exponent,1)-25).
	// sig<=2047 and the product is exact;the sign lives in the scale,which
	// also preserves BOTH signed zeros through the qualified -0 MAD addend.
	template<class E> static void Half0(E& e)
	{
		PioneerSfpuLoad32(e,7,0x8000);PioneerSfpuAnd(e,2,0,7);PioneerSfpuShift(e,2,16);
		PioneerSfpuMove(e,3,0);PioneerSfpuShift(e,3,-10);
		PioneerSfpuLoad32(e,7,31);PioneerSfpuAnd(e,3,3,7);
		PioneerSfpuMove(e,4,3);PioneerSfpuAddI(e,4,-1);PioneerSfpuShift(e,4,-31,true); // -1 iff subnormal/zero
		PioneerSfpuLoad32(e,7,1023);PioneerSfpuAnd(e,5,0,7);
		PioneerSfpuMove(e,6,4);PioneerSfpuLoad32(e,7,0xffffffff);PioneerSfpuXor(e,6,7);
		PioneerSfpuLoad32(e,7,1024);PioneerSfpuAnd(e,6,6,7);PioneerSfpuAdd(e,5,6);
		PioneerSfpuShift(e,4,-31);PioneerSfpuAdd(e,3,4);PioneerSfpuAddI(e,3,102);
		PioneerSfpuShift(e,3,23);PioneerSfpuXor(e,3,2);
		e.Op(0x90000550); // exact positive SM32 significand -> FP32,not a new CAST mode
		PioneerQ38VectorAffineMul(e,5,3,0);
	}

	// Input0/1 are original d/dmin HALF bits. Output4/5 are EXACT FP32
	// bases,kept immutable through all eight groups of THIS block only.
	template<class E> static void Bases(E& e)
	{
		Half0(e);
		PioneerSfpuMove(e,7,0);PioneerSfpuMove(e,0,1);PioneerSfpuMove(e,1,7);
		Half0(e);
		PioneerSfpuMove(e,4,1);PioneerSfpuMove(e,5,0);
	}

	// Input0..3:the four original UNSIGNED byte planes d.lo,d.hi,
	// dmin.lo,dmin.hi for the same output lanes. No scalar weight read.
	template<class E> static void RawBases(E& e)
	{
		PioneerSfpuShift(e,1,8);PioneerSfpuAdd(e,0,1);
		PioneerSfpuShift(e,3,8);PioneerSfpuAdd(e,2,3);PioneerSfpuMove(e,1,2);
		Bases(e);
	}

	// Input0..2 are the original byte planes named by ScaleByte,
	// MinimumByte and ExtraByte. Low groups ignore2. Output0/1 are the
	// exact unsigned6-bit scale/minimum;preserve the FP32 bases4/5.
	template<unsigned Group,class E> static void Factors(E& e)
	{
		static_assert(Group<8,"only the eight original Q4_K metadata groups");
		if(Group<4) {
			PioneerSfpuLoad32(e,2,63);PioneerSfpuAnd(e,0,0,2);PioneerSfpuAnd(e,1,1,2);
		} else {
			PioneerSfpuMove(e,3,0);PioneerSfpuShift(e,3,-4);
			PioneerSfpuShift(e,2,-6);PioneerSfpuShift(e,2,4);PioneerSfpuAdd(e,3,2);
			PioneerSfpuLoad32(e,2,15);PioneerSfpuAnd(e,0,0,2);
			PioneerSfpuShift(e,1,-6);PioneerSfpuShift(e,1,4);PioneerSfpuAdd(e,0,1);
			PioneerSfpuMove(e,1,3);
		}
	}

	// Input0=scale,1=minimum,4=d,5=dmin. Output6/7 are the exact existing
	// CachedAffine::BlockQ4 coefficient and minimum. No new rounding point.
	// IMPORTANT:min==0 means float(-int(0))==+0,NOT unconditional -0.
	// Preserve both original base signs,including negative-zero HALF values.
	template<class E> static void Products(E& e)
	{
		PioneerSfpuMove(e,2,1);PioneerSfpuAddI(e,2,-1);PioneerSfpuShift(e,2,-31,true);
		PioneerSfpuLoad32(e,3,0xffffffff);PioneerSfpuXor(e,2,3);
		PioneerSfpuLoad32(e,3,0x80000000);PioneerSfpuAnd(e,2,2,3);PioneerSfpuXor(e,1,2);
		e.Op(0x90000000);e.Op(0x90000110); // exact signed-magnitude small factors -> FP32
		PioneerQ38VectorAffineMul(e,4,0,6);PioneerQ38VectorAffineMul(e,5,1,7);
	}

private:
	// Each product register contains FOUR groups in its four eight-lane
	// subvectors. Transpose with three explicitly +0 registers compacts each
	// group into its original first Dst row and defines ALL three padding
	// rows as +0. Both register groups and every lane are initialized BEFORE
	// SFPTRANSP (ISA4a73a712). No predication/lane-index trick or new opcode.
	template<unsigned First,unsigned Parity,class E> static void StoreGroups(E& e)
	{
		static_assert((First==0 || First==4) && (Parity==0 || Parity==2),"fixed groups/parity");
		PioneerSfpuMove(e,0,6);PioneerSfpuMove(e,4,7);
		PioneerSfpuLoad32(e,1,0);PioneerSfpuLoad32(e,2,0);PioneerSfpuLoad32(e,3,0);
		PioneerSfpuLoad32(e,5,0);PioneerSfpuLoad32(e,6,0);PioneerSfpuLoad32(e,7,0);
		e.Op(0x8c000000);
		e.Op(0x72030000|(128+First*8)|Parity);e.Op(0x72430000|(132+First*8)|Parity);
		e.Op(0x72130000|(136+First*8)|Parity);e.Op(0x72530000|(140+First*8)|Parity);
		e.Op(0x72230000|(144+First*8)|Parity);e.Op(0x72630000|(148+First*8)|Parity);
		e.Op(0x72330000|(152+First*8)|Parity);e.Op(0x72730000|(156+First*8)|Parity);
	}
	template<unsigned Parity,class E> static void ParityBlock(E& e)
	{
		static_assert(Parity==0 || Parity==2,"original even/odd output columns");
		// Raw prefix Dst16 rows0..15 retains byte-major ORIGINAL metadata.
		// Broadcast its first four byte planes (d.lo/hi,dmin.lo/hi) across
		// all four group subvectors,then convert the bases EXACTLY once.
		e.Op(0x700d0000|Parity);
		PioneerSfpuMove(e,1,0);PioneerSfpuMove(e,2,0);PioneerSfpuMove(e,3,0);
		PioneerSfpuLoad32(e,4,0);PioneerSfpuLoad32(e,5,0);PioneerSfpuLoad32(e,6,0);PioneerSfpuLoad32(e,7,0);
		e.Op(0x8c000000);RawBases(e);
		// Temporarily borrow THIS parity of group7's OWN output rows. They
		// are not consumed until this whole block completes. Upper factors
		// reload them BEFORE StoreGroups<4> replaces EVERY borrowed lane and
		// padding word with the final group7 values. No extra Dst/L1 scratch,
		// and the prior live accumulator108..111 is NEVER touched.
		e.Op(0x72430000|184|Parity);e.Op(0x72530000|188|Parity);
		e.Op(0x700d0000|4|Parity);e.Op(0x701d0000|8|Parity);
		Factors<0>(e);Products(e);StoreGroups<0,Parity>(e);
		e.Op(0x700d0000|12|Parity);e.Op(0x701d0000|4|Parity);e.Op(0x702d0000|8|Parity);
		e.Op(0x70430000|184|Parity);e.Op(0x70530000|188|Parity);
		Factors<4>(e);Products(e);StoreGroups<4,Parity>(e);
	}
public:
	// Entry:exclusive completed original Q4 tile,FixedSetup source format,
	// both source banks Unpacker-owned,prior deferred accumulator drained,
	// caller has accepted FiniteBlock. No SrcA flip/publication occurs here.
	// Exit:ALL original FP32 metadata/padding rows128..191 defined,original
	// accumulator/other Dst untouched,Thread5=4,activation scale3,real drain.
	template<class E> static
#ifdef __riscv
	__attribute__((noinline,section(".text.pioneer_q38_resident_service")))
#else
	__attribute__((noinline))
#endif
	bool Block(E& e,unsigned rawBase,uint32_t scale)
	{
		const unsigned magnitude=scale&0x7fffffff;
		if(!RawBase(rawBase)
			|| (magnitude && (magnitude<0x2f800000 || magnitude>0x47800000)))return false;
		e.Op(0x3700000f);e.Op(0x5e23fc00);e.Op(0x5120000a);e.Op(0x5420000f);
		e.Op(0xb2050000);e.Drain();
		e.Config(1,0xa03c8000);e.Config(64,0x10001e);e.Config(65,16);e.Config(72,14);e.Config(73,0x13);
		e.Config(84,0x00400040);e.Config(76,(rawBase-16)/16);
		e.Op(0x42000081);e.Drain(); // ONE original256B prefix,no SrcA flip
		e.Op(0xb2050004);e.Op(0x3700000f);e.Op(0x8a000002);e.Drain();
		ParityBlock<0>(e);ParityBlock<2>(e);
		PioneerSfpuLoad32(e,3,scale);e.Drain();
		return true;
	}
};
#endif
