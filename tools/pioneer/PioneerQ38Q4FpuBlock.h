/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_Q4_FPU_BLOCK_H
#define PIONEER_Q38_Q4_FPU_BLOCK_H
#include "PioneerQ38Q4FpuDecode.h"
#include "PioneerQ38NativeBfpMatvec.h"
// NEW OFFBOARD32-output/K256 pipeline,not a driver/Work/layout authority.
// Owned canonical L1:16 raw256B panels/32 exact FP32-affine64B panels/
// eight BF16 input64B vectors,each with its OWN zero64B header. Parent
// supplies an owned immutable1024B zero buffer with zero64B header too.
// No scalar weight/activation IO,host learned products or CPU fallback.
// Complete GDDR streaming/native lease/model quality still unqualified.
struct PioneerQ38Q4FpuBlock {
	static constexpr unsigned Raw=0x90000,RawStride=320,Metadata=0x94000,MetadataStride=128;
	static constexpr unsigned Input=0x96000,InputStride=128,Zero=0x97000,Output=0x98000;
	static constexpr unsigned Header=64,Rows=32,Width=256,PreparedBytes=6144;
private:
	template<class E> static void FloatUnpack(E& e,unsigned rows) {
		e.Op(0xb2050000);e.Drain();e.Config(1,0x20200000);
		e.Config(64,0x100010);e.Config(65,rows);e.Config(72,0);e.Config(73,0x13);
		e.Config(86,0x100010);e.Op(rows==16?0x5e23fc00:0x5e203c00);
		e.Op(0x5120000a);e.Op(0x5420000f);e.Drain();
	}
	template<class E> static void Init(E& e) {
		PioneerQ38NativeBfpMatvec::Setup(e,8);e.Config(0,0x42b5);
		e.Config(112,0x00200015);e.Op(0x3700000f);e.Op(0x10180000);e.Drain();
		FloatUnpack(e,16);e.Config(76,Zero/16-1);
		// Define BOTH physical FP32 halves of every partial-write row.
		// Raw Dst16 rows0..63 and matrix Dst32 rows32/40 do not overlap.
		for(unsigned row=64;row<=128;row+=16) {
			e.Config(84,0x00400000+(row+4)*16);e.Op(0x42000091);e.Drain();
		}
	}
	template<unsigned Half,unsigned Parity,class E> static __attribute__((always_inline)) inline void MetadataLoad(E& e) {
		static_assert(Half<2 && (Parity==0 || Parity==2),"fixed original output half/parity");
		e.Op(0x70030000|(128+Half*8)|Parity); // coef row;other three rows zero
		e.Op(0x70430000|(132+Half*8)|Parity); // NEGATIVE minimum
		PioneerSfpuMove(e,1,0);PioneerSfpuMove(e,2,0);PioneerSfpuMove(e,3,0);
		PioneerSfpuMove(e,5,4);PioneerSfpuMove(e,6,4);PioneerSfpuMove(e,7,4);
		// ISA4a73a712 SFPTRANSP:transpose4x4 subvectors in BOTH register
		// groups. All8 registers/all32 lanes defined;no undefined padding.
		e.Op(0x8c000000);PioneerSfpuMove(e,1,0);PioneerSfpuMove(e,2,4);
		PioneerSfpuLoad32(e,4,15);PioneerSfpuLoad32(e,5,1);
		PioneerSfpuLoad32(e,6,0x7fff);PioneerSfpuLoad32(e,7,0xffff0000);
	}
	template<unsigned Face,unsigned Block,unsigned Parity,bool Upper,class E>
	static __attribute__((always_inline)) inline void Chunk(E& e) {
		e.Op(0x700d0000|(Face*16+Block)|Parity);
		PioneerQ38Q4FpuDecode<E,Upper>(e);
		e.Op(0x72030000|(64+Face*16+Block)|Parity);
	}
	template<unsigned Half,unsigned Parity,bool Upper,class E>
	static __attribute__((noinline)) void HalfDecode(E& e) {
		MetadataLoad<Half,Parity>(e);
		Chunk<Half*2,0,Parity,Upper>(e);Chunk<Half*2,4,Parity,Upper>(e);
		Chunk<Half*2,8,Parity,Upper>(e);Chunk<Half*2,12,Parity,Upper>(e);
		Chunk<Half*2+1,0,Parity,Upper>(e);Chunk<Half*2+1,4,Parity,Upper>(e);
		Chunk<Half*2+1,8,Parity,Upper>(e);Chunk<Half*2+1,12,Parity,Upper>(e);
	}
	template<unsigned Face,class E> static __attribute__((always_inline)) inline void MatrixFace(E& e) {
		if(Face){e.Op(0x37040201);e.Op(0x37040201);}
		e.Op(Face&1?0x3700040a:0x3700000a);
		e.Op(0x26084000|(Face<2?32:40));e.Op(0x26084000|(Face<2?32:40));
		e.Op(0x26084000|(Face<2?32:40));
		e.Op(0x26084000|(Face<2?32:40)|(Face==3?0x00c00000:0));
	}
public:
	template<class E> static void Run(E& e) {
		Init(e);
		for(unsigned group=0;group<8;group++) {
			e.Op(0x3700000f);FloatUnpack(e,1);
			for(unsigned slot=0;slot<4;slot++) {
				e.Config(76,(Metadata+(group*4+slot)*MetadataStride)/16-1);
				e.Config(84,0x00400000+(128+slot*4+4)*16);
				e.Op(0x42000081);e.Drain(); // no SrcA publication
			}
			e.Config(1,0x203c8000);e.Config(64,0x10001e);e.Config(65,16);e.Config(72,14);
			e.Op(0x5e23fc00);e.Drain();
			for(unsigned face=0;face<4;face++) {
				e.Config(76,(Raw+((group/2)*4+face)*RawStride)/16-1);
				e.Config(84,0x00400000+(face*16+4)*16);
				e.Op(face==3?0x420000c1:0x42000081);e.Drain();
			}
			e.Op(0xb2050004);e.Drain();e.Config(1,0x60aa0000);
			e.Config(124,(Input+group*InputStride)/16-1);
			e.Op(0x5e407c00);e.Op(0x5160000b);e.Op(0x5460000f);
			e.Op(0x428000c1);e.Drain();e.Op(0x3700000f);e.Op(0x8a000002);
			if(group&1) {
				HalfDecode<0,0,true>(e);HalfDecode<0,2,true>(e);
				HalfDecode<1,0,true>(e);HalfDecode<1,2,true>(e);
			} else {
				HalfDecode<0,0,false>(e);HalfDecode<0,2,false>(e);
				HalfDecode<1,0,false>(e);HalfDecode<1,2,false>(e);
			}
			for(unsigned row=0;row<64;row+=4)e.Op(0x08002000|(row<<17)|(64+row));
			e.Drain(); // SFPU stores/MOVD2A complete before any MVMUL
			MatrixFace<0>(e);MatrixFace<1>(e);MatrixFace<2>(e);MatrixFace<3>(e);
			e.Drain(); // REAL completion before next group changes formats/Dst
		}
	}
};
static_assert(PioneerQ38Q4FpuBlock::Raw+16*PioneerQ38Q4FpuBlock::RawStride<=PioneerQ38Q4FpuBlock::Metadata-64
	&& PioneerQ38Q4FpuBlock::Metadata+32*128<=PioneerQ38Q4FpuBlock::Input-64
	&& PioneerQ38Q4FpuBlock::Input+8*128<=PioneerQ38Q4FpuBlock::Zero-64
	&& PioneerQ38Q4FpuBlock::Zero+1024<=PioneerQ38Q4FpuBlock::Output-64,"separate bounded L1 operands");
#endif
