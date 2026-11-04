/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_Q4_FPU_BLOCK_DATA_H
#define PIONEER_Q38_Q4_FPU_BLOCK_DATA_H
#if defined(_KERNEL_MODE) || defined(PIONEER_ATTENTION_FREESTANDING) || defined(PIONEER_Q38_WORK_INFERENCE) || defined(PIONEER_Q38_CPU_VALIDATION)
#error Q4 FPU prepared data is a separate CPU-only storage recipe,not a driver/Work source
#endif
#include "PioneerQ38BinaryScale.h"
// NEW CPU PREPARATION DATA ONLY. No IO/driver/source/allocation authority.
// Original32 row-major Q4_K blocks ->4096 unchanged byte-major quant bytes
// +2048B EXACT FP32 coefficients/negative minima. No weight dequantization,
// BF16 conversion,learned products or original-cache-format substitution.
// Every component is64B aligned;future GDDR->L1 owner adds private zero
// headers,not duplicated global weights. Version must be bound separately.
struct PioneerQ38Q4FpuBlockData {
	static constexpr unsigned Version=1,OriginalBytes=32*144,Bytes=6144;
	static bool Prepare(const uint8_t* original,unsigned count,uint8_t* output,unsigned capacity) {
		if(!original || !output || count!=OriginalBytes || capacity<Bytes)return false;
		uintptr_t a=reinterpret_cast<uintptr_t>(original),b=reinterpret_cast<uintptr_t>(output),limit=~uintptr_t(0);
		if(a>limit-count || b>limit-Bytes || !(a+count<=b || b+Bytes<=a))return false;
		uint8_t stage[Bytes];
		for(unsigned row=0;row<32;row++) {
			const uint8_t* q=original+row*144;
			for(unsigned byte=0;byte<128;byte++)stage[(row/16)*2048+byte*16+row%16]=q[16+byte];
			uint16_t d=uint16_t(q[0]|(unsigned(q[1])<<8)),dmin=uint16_t(q[2]|(unsigned(q[3])<<8));
			for(unsigned group=0;group<8;group++) {
				unsigned scale,minimum;
				if(group<4){scale=q[4+group]&63;minimum=q[8+group]&63;}
				else{scale=(q[8+group]&15)|((q[group]>>6)<<4);minimum=(q[8+group]>>4)|((q[4+group]>>6)<<4);}
				uint32_t coefficient,negative;
				if(!PioneerQ38BinaryScale(d,int(scale),coefficient) || !PioneerQ38BinaryScale(dmin,int(minimum),negative))return false;
				negative^=0x80000000; // negate the PRODUCT,including minimum==0
				for(unsigned kind=0;kind<2;kind++) {
					unsigned at=4096+(group*4+(row/16)*2+kind)*64+(row%16)*4;
					uint32_t bits=kind?negative:coefficient;
					for(unsigned byte=0;byte<4;byte++)stage[at+byte]=uint8_t(bits>>(8*byte));
				}
			}
		}
		for(unsigned i=0;i<Bytes;i++)output[i]=stage[i];
		return true;
	}
};
#endif
