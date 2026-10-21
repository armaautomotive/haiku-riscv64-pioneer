/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_PREPARED_Q4_H
#define PIONEER_Q38_PREPARED_Q4_H
#include "PioneerQ38AffineOnly.h"
// OFFLINE FORMAT EXPERIMENT ONLY. No native reader,registry or driver
// selects it;only a separately flagged offline image. NEVER label it as
// the existing byte-major GGUF layout.
// Preserve all2048 packed quant bytes;replace256B of HALF/6-bit metadata
// with768B of EXACT precomputed FP32-affine values in a canonical24-bit
// encoding. No learned product/requantization/host inference/device IO.
// HALF * unsigned6 has at most17 significand bits and exponent[-24,21].
// FP32's low7 fraction bits are zero;biased exponents103..148 fit6 bits.
// Code63 means signed zero. Other unused codes/high bit MUST refuse.
struct PioneerQ38PreparedQ4 {
	static constexpr unsigned Version=1,OriginalBytes=2304,QuantBytes=2048,MetadataBytes=768,Bytes=2816;
	static bool Encode(uint32_t bits,uint32_t& packed) {
		uint32_t sign=(bits&0x80000000U)?0x400000U:0;
		if (!(bits&0x7fffffff)) { packed=sign|0x3f0000; return true; }
		unsigned exponent=(bits>>23)&255;
		if (exponent<103 || exponent>148 || (bits&127)) return false;
		packed=sign|((exponent-103)<<16)|((bits&0x7fffff)>>7); return true;
	}
	static bool Decode(uint32_t packed,uint32_t& bits) {
		if (packed&0xff800000U) return false;
		uint32_t sign=(packed&0x400000U)?0x80000000U:0;
		unsigned exponent=(packed>>16)&63,mantissa=packed&65535;
		if (exponent==63 && !mantissa) { bits=sign; return true; }
		if (exponent>45) return false;
		bits=sign|((exponent+103)<<23)|(mantissa<<7); return true;
	}
	static bool Separate(uintptr_t a,unsigned n,uintptr_t b,unsigned m) {
		uintptr_t limit=~uintptr_t(0);
		return a<=limit-n && b<=limit-m && (a+n<=b || b+m<=a);
	}
	// CPU loading-time metadata preparation ONLY. Full fixed tile publication
	// after ALL128 coefficient/minimum pairs validate. Caller must separately
	// authenticate the complete source and bind a NEW layout/version owner.
	static bool Prepare(const uint8_t* original,unsigned originalBytes,uint8_t* output,unsigned capacity) {
		if (!original || !output || originalBytes!=OriginalBytes || capacity<Bytes
			|| !Separate(reinterpret_cast<uintptr_t>(original),originalBytes,reinterpret_cast<uintptr_t>(output),Bytes)) return false;
		uint8_t stage[Bytes];
		for (unsigned byte=0;byte<QuantBytes;byte++) stage[byte]=original[256+byte];
		PioneerQ38Panel metadata;
		for (unsigned group=0;group<8;group++) {
			if (!PioneerQ38AffineOnly(12,original,originalBytes,group*2,&metadata)) return false;
			for (unsigned kind=0;kind<2;kind++) for (unsigned row=0;row<16;row++) {
				uint32_t packed=0;
				if (!Encode(kind?metadata.minimum[row]:metadata.coefficient[row],packed)) return false;
				unsigned at=QuantBytes+(group*32+kind*16+row)*3;
				for (unsigned byte=0;byte<3;byte++) stage[at+byte]=uint8_t(packed>>(byte*8));
			}
		}
		for (unsigned byte=0;byte<Bytes;byte++) output[byte]=stage[byte];
		return true;
	}
	// Separate OFFLINE consumer. No installed/pinned image selects it.
#ifdef PIONEER_Q38_PREPARED_Q4_BLOCK
	static uint32_t AlignedWord(const uint8_t* at) {
#ifdef PIONEER_ATTENTION_FREESTANDING
		uint32_t value;asm volatile("lw %0,0(%1)":"=r"(value):"r"(at):"memory");return value;
#else
		uint32_t value;__builtin_memcpy(&value,at,4);return value;
#endif
	}
	// OFFLINE-only block consumer:decode each exact compact value ONCE,
	// then atomically stage ALL8 original FP32 coefficient/minimum groups.
	// No source/driver/layout authority or changed arithmetic is implied.
	__attribute__((noinline)) static bool Block(const uint8_t* prepared,unsigned bytes,volatile uint32_t* scratch,unsigned words) {
		if(!prepared || bytes!=Bytes || !scratch || words!=256 || (reinterpret_cast<uintptr_t>(scratch)&3) || (reinterpret_cast<uintptr_t>(prepared)&3)
			|| !Separate(reinterpret_cast<uintptr_t>(prepared),bytes,reinterpret_cast<uintptr_t>(scratch),1024))return false;
		uint32_t stage[256];
		for(unsigned group=0;group<64;group++) {
			const uint8_t* at=prepared+QuantBytes+group*12;
			uint32_t a=AlignedWord(at),b=AlignedWord(at+4),c=AlignedWord(at+8);
			const uint32_t packed[4]={a&0xffffff,((a>>24)|(b<<8))&0xffffff,((b>>16)|(c<<16))&0xffffff,c>>8};
			for(unsigned word=0;word<4;word++)if(!Decode(packed[word],stage[group*4+word]))return false;
		}
		for(unsigned group=0;group<8;group++)for(unsigned row=0;row<16;row++) {
			scratch[group*32+row]=stage[group*32+row];scratch[group*32+16+row]=stage[group*32+16+row];
		}
		return true;
	}
#endif
	__attribute__((noinline)) static bool Panel(const uint8_t* prepared,unsigned bytes,unsigned panel,PioneerQ38Panel* output) {
		if (!prepared || bytes!=Bytes || panel>=16 || !output || (reinterpret_cast<uintptr_t>(output)&3)
			|| !Separate(reinterpret_cast<uintptr_t>(prepared),bytes,reinterpret_cast<uintptr_t>(output),sizeof(*output))) return false;
		uint32_t stage[32];
		for (unsigned word=0;word<32;word++) {
			unsigned at=QuantBytes+((panel/2)*32+word)*3;
			uint32_t packed=PioneerQ38PanelByte(prepared,at)|(PioneerQ38PanelByte(prepared,at+1)<<8)|(PioneerQ38PanelByte(prepared,at+2)<<16);
			if (!Decode(packed,stage[word])) return false;
		}
		volatile uint32_t* coefficient=output->coefficient;
		volatile uint32_t* minimum=output->minimum;
		for (unsigned row=0;row<16;row++) { coefficient[row]=stage[row]; minimum[row]=stage[16+row]; }
		return true;
	}
};
static_assert(PioneerQ38PreparedQ4::Bytes%64==0,"exact bounded NoC tile,NO authority");
#endif
