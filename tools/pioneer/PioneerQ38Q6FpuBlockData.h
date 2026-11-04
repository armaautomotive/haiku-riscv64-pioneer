/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_Q6_FPU_BLOCK_DATA_H
#define PIONEER_Q38_Q6_FPU_BLOCK_DATA_H
#if defined(_KERNEL_MODE) || defined(PIONEER_ATTENTION_FREESTANDING) || defined(PIONEER_Q38_WORK_INFERENCE) || defined(PIONEER_Q38_CPU_VALIDATION) || defined(__FAST_MATH__)
#error Q6 FPU data is a separate CPU preparation recipe only
#endif
#include "PioneerQ38BinaryScale.h"
// NEW DATA only,NOT source/allocation/driver authority or an installed cache
// recipe. Original32 row-major Q6 blocks ->4096 UNCHANGED ql bytes+2048
// UNCHANGED qh bytes+2048 exact FP32 half*signed8 coefficients. No expanded
// weights,BF16 conversion,learned products or clipping. Metadata is K16.
struct PioneerQ38Q6FpuBlockData {
	static constexpr unsigned Version=1,OriginalBytes=32*210,Bytes=8192;
	static bool Prepare(const uint8_t* original,unsigned count,uint8_t* output,unsigned capacity) {
		if(!original || !output || count!=OriginalBytes || capacity<Bytes)return false;
		uintptr_t a=reinterpret_cast<uintptr_t>(original),b=reinterpret_cast<uintptr_t>(output),limit=~uintptr_t(0);
		if(a>limit-count || b>limit-Bytes || !(a+count<=b || b+Bytes<=a))return false;
		uint8_t stage[Bytes];
		for(unsigned row=0;row<32;row++) {
			const uint8_t* p=original+row*210;
			for(unsigned i=0;i<128;i++)stage[(row/16)*2048+i*16+row%16]=p[i];
			for(unsigned i=0;i<64;i++)stage[4096+(row/16)*1024+i*16+row%16]=p[128+i];
			uint16_t half=uint16_t(p[208]|(unsigned(p[209])<<8));
			for(unsigned g=0;g<16;g++) {
				int scale=p[192+g];if(scale>=128)scale-=256;uint32_t bits;
				if(!PioneerQ38BinaryScale(half,scale,bits))return false;
				unsigned at=6144+(g*2+row/16)*64+(row%16)*4;
				for(unsigned byte=0;byte<4;byte++)stage[at+byte]=uint8_t(bits>>(byte*8));
			}
		}
		for(unsigned i=0;i<Bytes;i++)output[i]=stage[i];
		return true;
	}
};
#endif
