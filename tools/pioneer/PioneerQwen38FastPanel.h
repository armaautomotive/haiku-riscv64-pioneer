/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_QWEN38_FAST_PANEL_H
#define PIONEER_QWEN38_FAST_PANEL_H
#include "PioneerQwen38Panel.h"
// OFFLINE candidate only: decode FOUR output rows per integer word,without
// zero-fill/RMW/byte-at-a-time unpack. Same exact integer overlay and affine
// bits as the qualified scalar helper;no re-quantization or SM7 clipping.
static inline uint32_t PioneerQ38FastWord(const uint8_t* p)
{
#ifdef PIONEER_ATTENTION_FREESTANDING
	uint32_t value; asm volatile("lw %0,0(%1)":"=r"(value):"r"(p):"memory"); return value;
#else
	uint32_t value; __builtin_memcpy(&value,p,4);
#if __BYTE_ORDER__ != __ORDER_LITTLE_ENDIAN__
	value=__builtin_bswap32(value);
#endif
	return value;
#endif
}
static inline uint32_t PioneerQ38FastPair(uint32_t magnitude,uint32_t sign)
{
	return 0x40004000 | (magnitude&255) | ((magnitude&0xff00)<<8)
		| ((sign&128)<<8) | ((sign&0x8000)<<16);
}
static inline bool PioneerQ38FastPanel(unsigned type,const uint8_t* tile,unsigned bytes,unsigned panel,PioneerQ38Panel* output)
{
	unsigned required=PioneerQ38PanelTileBytes(type),panels=type==8?2:16;
	if (!required || !tile || !output || bytes!=required || panel>=panels) return false;
	uintptr_t first=reinterpret_cast<uintptr_t>(tile),last=reinterpret_cast<uintptr_t>(output),limit=~uintptr_t(0);
	if ((first&3) || (last&3) || first>limit-bytes || last>limit-sizeof(*output)
		|| !(first+bytes<=last || last+sizeof(*output)<=first)) return false;
	PioneerQ38Panel stage;
	for (unsigned row=0;row<16;row++) {
		stage.minimum[row]=0;
		unsigned at=type==14?208:0;
		uint16_t half=uint16_t(PioneerQ38PanelByte(tile,at*16+row)|(PioneerQ38PanelByte(tile,(at+1)*16+row)<<8));
		int factor=1;
		if (type==12) {
			unsigned group=panel/2,minimum;
			if (group<4) { factor=PioneerQ38PanelByte(tile,(4+group)*16+row)&63; minimum=PioneerQ38PanelByte(tile,(8+group)*16+row)&63; }
			else { factor=int((PioneerQ38PanelByte(tile,(8+group)*16+row)&15)|((PioneerQ38PanelByte(tile,group*16+row)>>6)<<4));
				minimum=(PioneerQ38PanelByte(tile,(8+group)*16+row)>>4)|((PioneerQ38PanelByte(tile,(4+group)*16+row)>>6)<<4); }
			uint16_t minHalf=uint16_t(PioneerQ38PanelByte(tile,32+row)|(PioneerQ38PanelByte(tile,48+row)<<8));
			if (!PioneerQ38PanelScale(minHalf,-int(minimum),stage.minimum[row])) return false;
		} else if (type==14) {
			unsigned value=PioneerQ38PanelByte(tile,(192+panel)*16+row); factor=int(value)-int((value&128)*2);
		}
		if (!PioneerQ38PanelScale(half,factor,stage.coefficient[row])) return false;
	}
	for (unsigned lane=0;lane<16;lane++) for (unsigned row=0;row<16;row+=4) {
		uint32_t magnitude,sign;
		if (type==8) {
			uint32_t q=PioneerQ38FastWord(tile+(2+panel*16+lane)*16+row),negative=(q>>7)&0x01010101;
			magnitude=(q^(negative*255))+negative; sign=q&0x80808080;
		} else if (type==12) {
			unsigned group=panel/2,at=16+(group/2)*32+(panel%2)*16+lane;
			magnitude=(PioneerQ38FastWord(tile+at*16+row)>>((group&1)*4))&0x0f0f0f0f; sign=0;
		} else {
			unsigned half=panel/8,plane=(panel%8)/2,at=(panel%2)*16+lane;
			uint32_t lo=(PioneerQ38FastWord(tile+(half*64+(plane&1)*32+at)*16+row)>>((plane/2)*4))&0x0f0f0f0f;
			uint32_t hi=(PioneerQ38FastWord(tile+(128+half*32+at)*16+row)>>(plane*2))&0x03030303;
			uint32_t q=lo|(hi<<4),negative=((~q)>>5)&0x01010101;
			magnitude=((q&0x1f1f1f1f)^(negative*31))+negative; sign=negative<<7;
		}
		unsigned word=(lane*16+row)/2;
		stage.operand[word]=PioneerQ38FastPair(magnitude,sign);
		stage.operand[word+1]=PioneerQ38FastPair(magnitude>>16,sign>>16);
	}
	for (unsigned w=0;w<128;w++) output->operand[w]=stage.operand[w];
	for (unsigned row=0;row<16;row++) { output->coefficient[row]=stage.coefficient[row]; output->minimum[row]=stage.minimum[row]; }
	return true;
}
#endif
