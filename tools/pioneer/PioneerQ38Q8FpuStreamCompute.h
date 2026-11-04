/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_Q8_FPU_STREAM_COMPUTE_H
#define PIONEER_Q38_Q8_FPU_STREAM_COMPUTE_H
#include "PioneerQ38MixedFpuDecode.h"
#include "PioneerQ38NativeBfpMatvec.h"
// NEW OFFBOARD32-row full-K accumulator. Raw original TC8 bytes are UNPACKed
// unsigned to physicalDst16,decoded on SFPU (including-128),BF16-RNE stored,
// moved to SrcA then multiplied through ALL four FPU phases. Begin ONCE/job.
// NO scalar learned-payload reads/host products/reset/fallback or ownership.
struct PioneerQ38Q8FpuStreamCompute {
	static constexpr unsigned Raw=0x90000,Metadata=0x94000,Input=0x96000;
	static constexpr unsigned RawStride=320,MetadataStride=128,InputStride=128,BufferStride=0x10000;
	static constexpr unsigned Zero=0x97000,Output=0x98000,Header=64,PreparedBytes=9216;
private:
	template<class E> static void FloatUnpack(E& e,unsigned rows) {
		e.Op(0xb2050000);e.Drain();e.Config(1,0x20200000);
		e.Config(64,0x100010);e.Config(65,rows);e.Config(72,0);e.Config(73,0x13);
		e.Config(86,0x100010);e.Op(rows==16?0x5e23fc00:0x5e203c00);
		e.Op(0x5120000a);e.Op(0x5420000f);e.Drain();
	}
	template<unsigned Half,unsigned Parity,class E> static void MetadataLoad(E& e) {
		static_assert(Half<2 && (Parity==0 || Parity==2),"fixed Q8 half/parity");
		e.Op(0x70030000|(128+Half*4)|Parity);
		PioneerSfpuMove(e,1,0);PioneerSfpuMove(e,2,0);PioneerSfpuMove(e,3,0);
		PioneerSfpuLoad32(e,4,0);PioneerSfpuLoad32(e,5,0);PioneerSfpuLoad32(e,6,0);PioneerSfpuLoad32(e,7,0);
		e.Op(0x8c000000);PioneerSfpuMove(e,1,0);PioneerQ38Q8FpuPrepare(e);
	}
	template<class E> static __attribute__((noinline)) void Decode(E& e){PioneerQ38Q8FpuDecode(e);}
	template<unsigned Face,unsigned Block,unsigned Parity,class E> static __attribute__((always_inline)) inline void Chunk(E& e) {
		e.Op(0x700d0000|(Face*16+Block)|Parity);Decode(e);
		e.Op(0x72030000|(64+Face*16+Block)|Parity);
	}
	template<unsigned Half,unsigned Parity,class E> static __attribute__((noinline)) void HalfDecode(E& e) {
		MetadataLoad<Half,Parity>(e);
		Chunk<Half*2,0,Parity>(e);Chunk<Half*2,4,Parity>(e);Chunk<Half*2,8,Parity>(e);Chunk<Half*2,12,Parity>(e);
		Chunk<Half*2+1,0,Parity>(e);Chunk<Half*2+1,4,Parity>(e);Chunk<Half*2+1,8,Parity>(e);Chunk<Half*2+1,12,Parity>(e);
	}
	template<unsigned Face,class E> static void MatrixFace(E& e) {
		if(Face){e.Op(0x37040201);e.Op(0x37040201);}
		e.Op(Face&1?0x3700040a:0x3700000a);
		e.Op(0x26084000|(Face<2?32:40));e.Op(0x26084000|(Face<2?32:40));
		e.Op(0x26084000|(Face<2?32:40));e.Op(0x26084000|(Face<2?32:40)|(Face==3?0x00c00000:0));
	}
public:
	template<class E> static void Begin(E& e) {
		PioneerQ38NativeBfpMatvec::Setup(e,8);e.Config(0,0x42b5);e.Config(112,0x00200015);
		e.Op(0x3700000f);e.Op(0x10180000);e.Drain();FloatUnpack(e,16);e.Config(76,Zero/16-1);
		for(unsigned row=64;row<=128;row+=16){e.Config(84,0x00400000+(row+4)*16);e.Op(0x42000091);e.Drain();}
	}
	template<class E> static bool Group(E& e,unsigned buffer,unsigned group) {
		if(buffer>=2 || group>=8)return false; // ZERO IO
		e.Op(0x3700000f);FloatUnpack(e,1);
		for(unsigned slot=0;slot<2;slot++) {
			e.Config(76,(Metadata+buffer*BufferStride+(group*2+slot)*MetadataStride)/16-1);
			e.Config(84,0x00400000+(128+slot*4+4)*16);e.Op(0x42000081);e.Drain();
		}
		e.Config(1,0x203c8000);e.Config(64,0x10001e);e.Config(65,16);e.Config(72,14);e.Op(0x5e23fc00);e.Drain();
		for(unsigned face=0;face<4;face++) {
			e.Config(76,(Raw+buffer*BufferStride+(group*4+face)*RawStride)/16-1);
			e.Config(84,0x00400000+(face*16+4)*16);e.Op(face==3?0x420000c1:0x42000081);e.Drain();
		}
		e.Op(0xb2050004);e.Drain();e.Config(1,0x60aa0000);
		e.Config(124,(Input+buffer*BufferStride+group*InputStride)/16-1);
		e.Op(0x5e407c00);e.Op(0x5160000b);e.Op(0x5460000f);e.Op(0x428000c1);e.Drain();
		e.Op(0x3700000f);e.Op(0x8a000002);
		HalfDecode<0,0>(e);HalfDecode<0,2>(e);HalfDecode<1,0>(e);HalfDecode<1,2>(e);
		for(unsigned row=0;row<64;row+=4)e.Op(0x08002000|(row<<17)|(64+row));
		e.Drain();MatrixFace<0>(e);MatrixFace<1>(e);MatrixFace<2>(e);MatrixFace<3>(e);e.Drain();return true;
	}
};
static_assert(0x90000+32*320<=0x94000-64 && 0x94000+16*128<=0x96000-64
	&& 0x96000+8*128<=0x97000-64 && 0x97000+1024<=0x98000-64
	&& 0x98000+256<0xa0000-64,"Q8 raw/metadata/input/zero/output are disjoint");
#endif
