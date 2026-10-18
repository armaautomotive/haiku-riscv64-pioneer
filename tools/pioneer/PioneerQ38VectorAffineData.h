/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_VECTOR_AFFINE_DATA_H
#define PIONEER_Q38_VECTOR_AFFINE_DATA_H
#include "PioneerSfpuDivide.h"
#include "PioneerQ38VectorAffine.h"
// OFFLINE ONLY data-loading integration. No native image/driver selects it.
// AFTER final matrix drain and scalar validation of ALL16 integer dots.
// Current unpack/Math SrcA bank is Unpacker-owned;NO FLIP in any metadata
// unpack. Fixed owned local coefficients91200,minima91240,accumulator84000.
// Dst32:96..99 coef,100..103 min,104..107 old,108..111 result. ALL unused
// rows explicitly zeroed,disjoint from raw/decoded weights and matrix dots.
// Caller must copy exactly16 result words from RISC FP32 Dst row108 then
// restore Config3=10000. This function never issues NoC/DMA/host accesses.
template<class E> static bool PioneerQ38VectorAffineData(E& e,unsigned group,uint32_t scale,int sum)
{
	unsigned magnitude=scale&0x7fffffff;
	if (group>=16 || sum<-2048 || sum>2048 || (magnitude && (magnitude<0x2f800000 || magnitude>0x47800000))) return false;
	e.Drain(); e.Op(0x8a000002); e.Op(0x3700000f);
	e.Op(0x71720000);
	for (unsigned block=96;block<108;block+=4) for (unsigned parity=0;parity<4;parity+=2) e.Op(0x72770000|block|parity);
	e.Drain();
	e.Op(0x5e203c00); e.Op(0x5120000a); e.Op(0x5420000f); //16 FP32,zero Y/Z/W
	e.Op(0xb2050000); e.Drain();
	e.Config(1,0xa0200000); e.Config(49,0); e.Config(64,0x100010); e.Config(65,1);
	e.Config(72,0); e.Config(73,0x13);
	for (unsigned kind=0;kind<3;kind++) {
		unsigned row=96+kind*4,address=kind==0?0x91200:kind==1?0x91240:0x84000+group*64;
		e.Config(84,0x00400000+(row+4)*16); e.Config(76,(address-16)/16);
		e.Op(0x42000081); e.Drain(); // no publication/bank flip;UnpackToDst clobbers available SrcA only
	}
	e.Config(1,0xa0220000); e.Config(64,0x100011); e.Config(65,16); e.Config(72,1);
	e.Config(73,3); e.Config(76,0x7070); e.Config(84,0x00400040);
	e.Op(0xb2050004); e.Op(0x5e23fc00); e.Drain(); e.Op(0x3700000f);
	for (unsigned parity=0;parity<4;parity+=2) {
		e.Op(0x70040000|32|parity); // bounded SM32 integer dots
		e.Op(0x70130000|96|parity); e.Op(0x70230000|100|parity); e.Op(0x70530000|104|parity);
		PioneerSfpuLoad32(e,3,scale);
		PioneerSfpuLoad32(e,4,(sum<0?0x80000000U:0)|unsigned(sum<0?-sum:sum));
		// Canonicalize a possible hardware SM32 negative-zero dot without
		// changing any nonzero bounded value. Sum came from a scalar int.
		PioneerSfpuLoad32(e,6,0x7fffffff); PioneerSfpuAnd(e,7,0,6);
		PioneerSfpuAdd(e,7,6); PioneerSfpuShift(e,7,-31,true); PioneerSfpuAnd(e,0,0,7);
		PioneerQ38VectorAffine(e);
		e.Op(0x72030000|108|parity); // IEEE FP32->Dst32 conversion,not opaque integer storage
	}
	e.Drain(); e.Config(3,0); // caller's direct RISC read converts Dst FP32 to IEEE bits
	return true;
}
#endif
