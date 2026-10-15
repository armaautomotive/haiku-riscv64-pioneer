/* SPDX-License-Identifier: MIT */
#ifndef TENSTORRENT_GGUF_QUANT_H
#define TENSTORRENT_GGUF_QUANT_H
#include "TenstorrentQ4K.h"
static inline bool PioneerDecodeQ8_0(const uint8_t* block,float output[32])
{
	float d=PioneerHalfFloat(uint16_t(block[0])|(uint16_t(block[1])<<8));
	if(!isfinite(d)) return false;
	for(unsigned i=0;i<32;i++) {
		int quant=block[2+i]; if(quant>=128) quant-=256;
		output[i]=d*quant;
	}
	return true;
}
static inline bool PioneerDecodeQ6K(const uint8_t* block,float output[256])
{
	float d=PioneerHalfFloat(uint16_t(block[208])|(uint16_t(block[209])<<8));
	if(!isfinite(d)) return false;
	for(unsigned i=0;i<256;i++) {
		unsigned half=i/128,group=(i%128)/32,lane=i%32;
		unsigned low=(block[half*64+(group&1)*32+lane]>>((group/2)*4))&15;
		unsigned high=(block[128+half*32+lane]>>(group*2))&3;
		int quant=int(low|(high<<4))-32;
		int scale=block[192+i/16]; if(scale>=128) scale-=256;
		output[i]=(d*scale)*quant;
	}
	return true;
}
#endif
