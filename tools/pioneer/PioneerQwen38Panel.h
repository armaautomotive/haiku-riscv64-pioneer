/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_QWEN38_PANEL_H
#define PIONEER_QWEN38_PANEL_H
#ifdef PIONEER_ATTENTION_FREESTANDING
typedef __UINT8_TYPE__ uint8_t;
typedef __UINT32_TYPE__ uint32_t;
typedef __UINTPTR_TYPE__ uintptr_t;
#else
#include <stdint.h>
#endif
#include "../../src/add-ons/kernel/drivers/misc/PioneerInt8Operand.h"

// One LOCAL16x16 integer matrix operand plus exact FP32 affine scales.
// Original lossless byte-major GGUF tiles stay packed in GDDR. ONLY this
// bounded640B local operand expands; no whole-weight FP16/F32 conversion,
// re-quantization, SM7 -128 clipping, host inference, or route authority.
// Operand order is K-major/output-column-minor, matching the qualified
// Tensix integer-overlay SrcA path (not the native SM7-only byte path).
struct PioneerQ38Panel {
	uint32_t operand[128];
	uint32_t coefficient[16],minimum[16];
};
static_assert(sizeof(PioneerQ38Panel)==640,"fixed local panel footprint");
static inline unsigned PioneerQ38PanelTileBytes(unsigned type)
{
	return type==8?576:type==12?2304:type==14?3392:0;
}
static inline unsigned PioneerQ38PanelByte(const uint8_t* data,unsigned at)
{
#ifdef PIONEER_ATTENTION_FREESTANDING
	unsigned value; asm volatile("lbu %0,0(%1)":"=r"(value):"r"(data+at):"memory"); return value;
#else
	return data[at];
#endif
}
// Half times small integer is EXACTLY representable in FP32 (<=18 useful
// bits), including HALF subnormals/zeros. Integer-only implementation means
// no scalar floating multiply/denormal flush, runtime lib, or approximation.
static inline bool PioneerQ38PanelScale(uint16_t half,int factor,uint32_t& output)
{
	unsigned exponent=(half>>10)&31;
	if (exponent==31 || factor<-128 || factor>127) return false;
	uint32_t sign=(uint32_t(half&0x8000)<<16)^(factor<0?0x80000000U:0);
	unsigned magnitude=exponent?1024+(half&1023):half&1023;
	uint32_t product=magnitude*unsigned(factor<0?-factor:factor);
	if (!product) { output=sign; return true; }
	int encoded=exponent?int(exponent)-25+150:-24+150;
	while (product<0x800000) { product<<=1; encoded--; }
	output=sign|(uint32_t(encoded)<<23)|(product&0x7fffff); return true;
}
static inline bool PioneerQ38UnpackPanel(unsigned type,const uint8_t* tile,unsigned bytes,unsigned panel,PioneerQ38Panel* output)
{
	unsigned required=PioneerQ38PanelTileBytes(type),panels=type==8?2:16;
	if (!required || !tile || !output || bytes!=required || panel>=panels) return false;
	uintptr_t first=reinterpret_cast<uintptr_t>(tile),last=reinterpret_cast<uintptr_t>(output),limit=~uintptr_t(0);
	if ((last&3) || first>limit-bytes || last>limit-sizeof(*output)
		|| !(first+bytes<=last || last+sizeof(*output)<=first)) return false;
	PioneerQ38Panel stage;
	// Explicit word stores keep the freestanding image independent of a
	// compiler-emitted memset/libc call; no byte/halfword stores are needed.
	volatile uint32_t* zero=stage.operand;
	for (unsigned w=0;w<128;w++) zero[w]=0;
	for (unsigned row=0;row<16;row++) {
		stage.minimum[row]=0;
		unsigned at=type==14?208:0;
		uint16_t half=uint16_t(PioneerQ38PanelByte(tile,at*16+row)|(PioneerQ38PanelByte(tile,(at+1)*16+row)<<8));
		int factor=1; unsigned minimum=0;
		if (type==12) {
			unsigned group=panel/2;
			if (group<4) { factor=PioneerQ38PanelByte(tile,(4+group)*16+row)&63;
				minimum=PioneerQ38PanelByte(tile,(8+group)*16+row)&63; }
			else { factor=int((PioneerQ38PanelByte(tile,(8+group)*16+row)&15)|((PioneerQ38PanelByte(tile,group*16+row)>>6)<<4));
				minimum=(PioneerQ38PanelByte(tile,(8+group)*16+row)>>4)|((PioneerQ38PanelByte(tile,(4+group)*16+row)>>6)<<4); }
			uint16_t minHalf=uint16_t(PioneerQ38PanelByte(tile,2*16+row)|(PioneerQ38PanelByte(tile,3*16+row)<<8));
			if (!PioneerQ38PanelScale(minHalf,-int(minimum),stage.minimum[row])) return false;
		} else if (type==14) {
			unsigned value=PioneerQ38PanelByte(tile,(192+panel)*16+row);
			factor=int(value)-int((value&128)*2);
		}
		if (!PioneerQ38PanelScale(half,factor,stage.coefficient[row])) return false;
		for (unsigned lane=0;lane<16;lane++) {
			unsigned i=panel*16+lane; int value;
			if (type==8) {
				unsigned code=PioneerQ38PanelByte(tile,(2+i)*16+row);
				value=int(code)-int((code&128)*2);
			} else if (type==12) {
				unsigned group=i/32,code=PioneerQ38PanelByte(tile,(16+(group/2)*32+i%32)*16+row);
				value=int((group&1)?code>>4:code&15);
			} else {
				unsigned halfIndex=i/128,plane=(i%128)/32;
				unsigned lo=(PioneerQ38PanelByte(tile,(halfIndex*64+(plane&1)*32+i%32)*16+row)>>((plane/2)*4))&15;
				unsigned hi=(PioneerQ38PanelByte(tile,(128+halfIndex*32+i%32)*16+row)>>(plane*2))&3;
				value=int(lo|(hi<<4))-32;
			}
			unsigned element=lane*16+row;
			stage.operand[element/2]|=uint32_t(PioneerInt8Operand(value))<<((element%2)*16);
		}
	}
	// Publish only after ALL scales/bytes are validated. Word stores only.
	for (unsigned w=0;w<128;w++) output->operand[w]=stage.operand[w];
	for (unsigned row=0;row<16;row++) { output->coefficient[row]=stage.coefficient[row]; output->minimum[row]=stage.minimum[row]; }
	return true;
}
#endif
