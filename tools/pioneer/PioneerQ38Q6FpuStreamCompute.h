/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_Q6_FPU_STREAM_COMPUTE_H
#define PIONEER_Q38_Q6_FPU_STREAM_COMPUTE_H
#include "PioneerQ38MixedFpuDecode.h"
#include "PioneerQ38NativeBfpMatvec.h"
// NEW OFFBOARD persistent32-row accumulator. Begin ONCE/full-K job,Group
// adds32 K. NO scalar weight/activation reads,host callback or fallback.
// High-plane rawDst16 is320..383,NOT64..127 (which aliases matrix FP32
// accumulators32/40). Decoded FP32 rows64..127 and metadata128..143 have
// distinct PHYSICAL halves. Caller owes actual L1 producer/owner/drains.
struct PioneerQ38Q6FpuStreamCompute {
	static constexpr unsigned Low=0x90000,High=0x92000,Metadata=0x94000,Input=0x96000;
	static constexpr unsigned RawStride=320,MetadataStride=128,InputStride=128,BufferStride=0x10000;
	static constexpr unsigned Zero=0x97000,Output=0x98000,Header=64,PreparedBytes=8192;
private:
	template<class E> static void FloatUnpack(E& e,unsigned rows) {
		e.Op(0xb2050000);e.Drain();e.Config(1,0x20200000);
		e.Config(64,0x100010);e.Config(65,rows);e.Config(72,0);e.Config(73,0x13);
		e.Config(86,0x100010);e.Op(rows==16?0x5e23fc00:0x5e203c00);
		e.Op(0x5120000a);e.Op(0x5420000f);e.Drain();
	}
	template<unsigned Face,unsigned Parity,class E> static void MetadataLoad(E& e) {
		static_assert(Face<4 && (Parity==0 || Parity==2),"fixed Q6 face/parity");
		// Face's K half selects one of TWO independent K16 scales.
		constexpr unsigned slot=(Face%2)*2+Face/2;
		e.Op(0x70030000|(128+slot*4)|Parity);
		PioneerSfpuMove(e,1,0);PioneerSfpuMove(e,2,0);PioneerSfpuMove(e,3,0);
		PioneerSfpuLoad32(e,4,0);PioneerSfpuLoad32(e,5,0);PioneerSfpuLoad32(e,6,0);PioneerSfpuLoad32(e,7,0);
		e.Op(0x8c000000);PioneerSfpuMove(e,1,0);PioneerQ38Q6FpuPrepare(e);
	}
	template<class E> static __attribute__((noinline)) void Decode(E& e,unsigned group) {
		// Internal loop-derived selector only;Group already bounds0..7.
		// Shared leaf prevents8 face/parity x4 duplicate decoder bodies.
		switch(group) {
			case 0:PioneerQ38Q6FpuDecode<0>(e);break;
			case 1:PioneerQ38Q6FpuDecode<1>(e);break;
			case 2:PioneerQ38Q6FpuDecode<2>(e);break;
			case 3:PioneerQ38Q6FpuDecode<3>(e);break;
		}
	}
	template<unsigned Face,unsigned Block,unsigned Parity,class E> static __attribute__((always_inline)) inline void Chunk(E& e,unsigned group) {
		e.Op(0x700d0000|(Face*16+Block)|Parity);
		e.Op(0x703d0000|(320+Face*16+Block)|Parity);
		Decode(e,group);e.Op(0x72030000|(64+Face*16+Block)|Parity);
	}
	template<unsigned Face,unsigned Parity,class E> static __attribute__((noinline)) void FaceDecode(E& e,unsigned group) {
		MetadataLoad<Face,Parity>(e);
		Chunk<Face,0,Parity>(e,group);Chunk<Face,4,Parity>(e,group);
		Chunk<Face,8,Parity>(e,group);Chunk<Face,12,Parity>(e,group);
	}
	template<unsigned Face,class E> static void MatrixFace(E& e) {
		if(Face){e.Op(0x37040201);e.Op(0x37040201);}
		e.Op(Face&1?0x3700040a:0x3700000a);
		e.Op(0x26084000|(Face<2?32:40));e.Op(0x26084000|(Face<2?32:40));
		e.Op(0x26084000|(Face<2?32:40));
		e.Op(0x26084000|(Face<2?32:40)|(Face==3?0x00c00000:0));
	}
public:
	template<class E> static void Begin(E& e) {
		PioneerQ38NativeBfpMatvec::Setup(e,8);e.Config(0,0x42b5);
		e.Config(112,0x00200015);e.Op(0x3700000f);e.Op(0x10180000);e.Drain();
		FloatUnpack(e,16);e.Config(76,Zero/16-1);
		for(unsigned row=64;row<=128;row+=16){e.Config(84,0x00400000+(row+4)*16);e.Op(0x42000091);e.Drain();}
	}
	template<class E> static bool Group(E& e,unsigned buffer,unsigned group) {
		if(buffer>=2 || group>=8)return false; // ZERO IO
		e.Op(0x3700000f);FloatUnpack(e,1);
		for(unsigned slot=0;slot<4;slot++) {
			e.Config(76,(Metadata+buffer*BufferStride+(group*4+slot)*MetadataStride)/16-1);
			e.Config(84,0x00400000+(128+slot*4+4)*16);e.Op(0x42000081);e.Drain();
		}
		e.Config(1,0x203c8000);e.Config(64,0x10001e);e.Config(65,16);e.Config(72,14);e.Op(0x5e23fc00);e.Drain();
		for(unsigned face=0;face<4;face++) {
			e.Config(76,(Low+buffer*BufferStride+((group/4*2+(group&1))*4+face)*RawStride)/16-1);
			e.Config(84,0x00400000+(face*16+4)*16);e.Op(0x42000081);e.Drain();
		}
		for(unsigned face=0;face<4;face++) {
			e.Config(76,(High+buffer*BufferStride+((group/4)*4+face)*RawStride)/16-1);
			e.Config(84,0x00400000+(320+face*16+4)*16);e.Op(face==3?0x420000c1:0x42000081);e.Drain();
		}
		e.Op(0xb2050004);e.Drain();e.Config(1,0x60aa0000);
		e.Config(124,(Input+buffer*BufferStride+group*InputStride)/16-1);
		e.Op(0x5e407c00);e.Op(0x5160000b);e.Op(0x5460000f);e.Op(0x428000c1);e.Drain();
		e.Op(0x3700000f);e.Op(0x8a000002);
		FaceDecode<0,0>(e,group%4);FaceDecode<0,2>(e,group%4);
		FaceDecode<1,0>(e,group%4);FaceDecode<1,2>(e,group%4);
		FaceDecode<2,0>(e,group%4);FaceDecode<2,2>(e,group%4);
		FaceDecode<3,0>(e,group%4);FaceDecode<3,2>(e,group%4);
		for(unsigned row=0;row<64;row+=4)e.Op(0x08002000|(row<<17)|(64+row));
		e.Drain();MatrixFace<0>(e);MatrixFace<1>(e);MatrixFace<2>(e);MatrixFace<3>(e);e.Drain();
		return true;
	}
};
static_assert(PioneerQ38Q6FpuStreamCompute::Low+16*320<=0x92000-64
	&& 0x92000+8*320<=0x94000-64 && 0x94000+32*128<=0x96000-64
	&& 0x96000+8*128<=0x97000-64 && 0x97000+1024<=0x98000-64
	&& 0x98000+256<0xa0000-64,"Q6 L1 planes/metadata/input/zero/output are disjoint");
#endif
