/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_QWEN38_PACKED_DOT_H
#define PIONEER_QWEN38_PACKED_DOT_H
#include "PioneerQwen38Hybrid.h"
typedef __UINT8_TYPE__ PioneerQ38Byte;
typedef __INT8_TYPE__ PioneerQ38Int8;
static_assert(sizeof(int)==4,"packed integer reductions require signed32");
static inline int PioneerQ38SignedByte(const PioneerQ38Byte* address)
{
	unsigned value;
#if defined(PIONEER_ATTENTION_FREESTANDING)
	// Keep the ALREADY qualified LBU + integer operations. A normal int8_t
	// access made GCC select LB,which the strict instruction model refuses.
	// Do NOT weaken that model or implicitly qualify a new memory opcode.
	asm volatile("lbu %0,0(%1)" : "=r"(value) : "r"(address) : "memory");
#else
	value=*address;
#endif
	return int(value)-int((value&128)*2);
}

// OFFLINE packed-block arithmetic prerequisite,NOT a driver operation or a
// CPU inference fallback. Integer reductions preserve packed scales/minima
// and avoid expanding weights to BF16/F32. This scalar oracle/image is NOT
// the final high-throughput Tensix matrix kernel. Activation scale is an
// explicit F32 value (possibly negative,as in Q8_K),NOT an FP16 rewrite.
static inline bool PioneerQ38PackedDot(unsigned type,const PioneerQ38Byte* weights,unsigned bytes,
	const PioneerQ38Int8* activation,unsigned count,float scale,float* output)
{
	unsigned requiredBytes=type==8?34:type==12?144:type==14?210:0;
	unsigned requiredCount=type==8?32:256;
	if (!requiredBytes || bytes!=requiredBytes || count!=requiredCount || !weights || !activation || !output
		|| !PioneerQ38Finite(scale)) return false;
	typedef __UINTPTR_TYPE__ Address;
	Address destination=reinterpret_cast<Address>(output),w=reinterpret_cast<Address>(weights),a=reinterpret_cast<Address>(activation),limit=~Address(0);
	if (destination>limit-4 || w>limit-bytes || a>limit-count
		|| !(destination+4<=w || w+bytes<=destination) || !(destination+4<=a || a+count<=destination)) return false;
	unsigned at=type==14?208:0;
	float d=PioneerAttentionHalf(uint16_t(weights[at])|(uint16_t(weights[at+1])<<8));
	if (!PioneerQ38Finite(d)) return false;
	int sum=0,minimumSum=0; float minimum=0;
	if (type==8) {
		for (unsigned j=0;j<32;j++) sum+=PioneerQ38SignedByte(weights+2+j)
			*PioneerQ38SignedByte(reinterpret_cast<const PioneerQ38Byte*>(activation)+j);
	} else if (type==12) {
		minimum=PioneerAttentionHalf(uint16_t(weights[2])|(uint16_t(weights[3])<<8));
		if (!PioneerQ38Finite(minimum)) return false;
		for (unsigned group=0;group<8;group++) {
			unsigned factor,offset;
			if (group<4) { factor=weights[4+group]&63; offset=weights[8+group]&63; }
			else { factor=(weights[8+group]&15)|((weights[group]>>6)<<4);
				offset=(weights[8+group]>>4)|((weights[4+group]>>6)<<4); }
			int dot=0,inputSum=0;
			for (unsigned lane=0;lane<32;lane++) {
				unsigned code=weights[16+(group/2)*32+lane],q=(group&1)?code>>4:code&15;
				int value=PioneerQ38SignedByte(reinterpret_cast<const PioneerQ38Byte*>(activation)+group*32+lane);
				dot+=int(q)*value; inputSum+=value;
			}
			sum+=int(factor)*dot; minimumSum+=int(offset)*inputSum;
		}
	} else {
		for (unsigned i=0;i<256;i++) {
			unsigned half=i/128,plane=(i%128)/32,lane=i%32;
			unsigned lo=(weights[half*64+(plane&1)*32+lane]>>((plane/2)*4))&15;
			unsigned hi=(weights[128+half*32+lane]>>(plane*2))&3;
			int factor=PioneerQ38SignedByte(weights+192+i/16);
			sum+=factor*(int(lo|(hi<<4))-32)*PioneerQ38SignedByte(reinterpret_cast<const PioneerQ38Byte*>(activation)+i);
		}
	}
	// Worst integer magnitudes:Q4<=30,965,760;Q6<=134,217,728;min<=2,064,384.
	// All stay within signed32,including -128 activation/scales.
	float coefficient=d*scale,correction=minimum*scale;
	if (!PioneerQ38Finite(coefficient) || !PioneerQ38Finite(correction)) return false;
	float value=coefficient*float(sum)-correction*float(minimumSum);
	if (!PioneerQ38Finite(value)) return false;
	*output=value; return true;
}
#endif
