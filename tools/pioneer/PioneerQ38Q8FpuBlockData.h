/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_Q8_FPU_BLOCK_DATA_H
#define PIONEER_Q38_Q8_FPU_BLOCK_DATA_H
#if defined(_KERNEL_MODE) || defined(PIONEER_ATTENTION_FREESTANDING) || defined(PIONEER_Q38_WORK_INFERENCE) || defined(PIONEER_Q38_CPU_VALIDATION) || defined(__FAST_MATH__)
#error Q8 FPU data is a separate CPU preparation recipe only
#endif
#include "PioneerQ38BinaryScale.h"
// NEW CPU DATA recipe only,NOT any installed cache/source/owner capability.
// Original32 rows x8 Q8_0 blocks ->8192 UNCHANGED TC8 quant bytes+1024 exact
// FP32 original half scales. Includes-128;NO SM7 recoding/clipping,weight
// dequantization,BF16 conversion or learned products. Explicit16-row tail
// pads ONLY absent output rows with+0;caller must source-bind real row count.
struct PioneerQ38Q8FpuBlockData {
	static constexpr unsigned Version=1,RowBytes=8*34,OriginalBytes=32*RowBytes,Bytes=9216;
	static bool PrepareRows(const uint8_t* original,unsigned count,unsigned rows,uint8_t* output,unsigned capacity) {
		if(!original || !output || (rows!=16 && rows!=32) || count!=rows*RowBytes || capacity<Bytes)return false;
		uintptr_t a=reinterpret_cast<uintptr_t>(original),b=reinterpret_cast<uintptr_t>(output),limit=~uintptr_t(0);
		if(a>limit-count || b>limit-Bytes || !(a+count<=b || b+Bytes<=a))return false;
		uint8_t stage[Bytes]={};
		for(unsigned row=0;row<rows;row++)for(unsigned g=0;g<8;g++) {
			const uint8_t* p=original+row*RowBytes+g*34;
			for(unsigned i=0;i<32;i++)stage[(row/16)*4096+(g*32+i)*16+row%16]=p[2+i];
			uint32_t bits;if(!PioneerQ38BinaryScale(uint16_t(p[0]|(unsigned(p[1])<<8)),1,bits))return false;
			unsigned at=8192+(g*2+row/16)*64+(row%16)*4;
			for(unsigned byte=0;byte<4;byte++)stage[at+byte]=uint8_t(bits>>(byte*8));
		}
		for(unsigned i=0;i<Bytes;i++)output[i]=stage[i];
		return true;
	}
	static bool Prepare(const uint8_t* original,unsigned count,uint8_t* output,unsigned capacity) {
		return PrepareRows(original,count,32,output,capacity);
	}
};
#endif
