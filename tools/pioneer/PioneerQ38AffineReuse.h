/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_AFFINE_REUSE_H
#define PIONEER_Q38_AFFINE_REUSE_H
#include "PioneerQ38AffineOnly.h"
// OFFLINE prerequisite ONLY. Caller owns ONE immutable fetched quantization
// block throughout all2/16 panels;never reuse across a NoC fetch. Q8 has
// identical affine metadata for both panels;Q4 for each adjacent pair.
// Q6 half scale is constant across all16 panels:preconvert it ONCE,then
// half-as-FP32 * signed8 factor is EXACT (<=18 useful bits,never an FP32
// subnormal). No approximation,host math,NoC or scalar operand decoding.
class PioneerQ38AffineReuse {
	const uint8_t* tile=nullptr;
	unsigned type=0,bytes=0,next=0;
	// Word-sized states retain the image's existing LW/SW-only stack writes.
	uint32_t attempted=0,ready=0,failed=0;
	uint32_t base[16],coefficient[16],minimum[16];
	bool Refuse() { failed=true; ready=false; return false; }
	static float Float(uint32_t bits) { float f; __builtin_memcpy(&f,&bits,4); return f; }
	static uint32_t Bits(float f) { uint32_t bits; __builtin_memcpy(&bits,&f,4); return bits; }
	static bool Separate(uintptr_t a,unsigned n,uintptr_t b,unsigned m) {
		uintptr_t limit=~uintptr_t(0);
		return a<=limit-n && b<=limit-m && (a+n<=b || b+m<=a);
	}
public:
	PioneerQ38AffineReuse()=default;
	PioneerQ38AffineReuse(const PioneerQ38AffineReuse&)=delete;
	PioneerQ38AffineReuse& operator=(const PioneerQ38AffineReuse&)=delete;
	bool Prepare(unsigned kind,const uint8_t* data,unsigned count) {
		if (attempted || failed) return Refuse();
		attempted=true;
		unsigned expected=PioneerQ38PanelTileBytes(kind);
		if (!expected || !data || count!=expected
			|| !Separate(reinterpret_cast<uintptr_t>(data),count,reinterpret_cast<uintptr_t>(this),sizeof(*this))) return Refuse();
		if (kind==14) for (unsigned row=0;row<16;row++) {
			uint16_t half=uint16_t(PioneerQ38PanelByte(data,208*16+row)|(PioneerQ38PanelByte(data,209*16+row)<<8));
			if (!PioneerQ38BinaryScale(half,1,base[row])) return Refuse();
		}
		tile=data; type=kind; bytes=count; ready=true; return true;
	}
	bool Panel(unsigned panel,PioneerQ38Panel* output) {
		if (!ready || failed || !output || panel!=next || panel>=(type==8?2U:16U)
			|| (reinterpret_cast<uintptr_t>(output)&3)
			|| !Separate(reinterpret_cast<uintptr_t>(output),sizeof(*output),reinterpret_cast<uintptr_t>(tile),bytes)
			|| !Separate(reinterpret_cast<uintptr_t>(output),sizeof(*output),reinterpret_cast<uintptr_t>(this),sizeof(*this))) return Refuse();
		if (type==14) {
			uint32_t stage[16];
			for (unsigned row=0;row<16;row++) {
				unsigned code=PioneerQ38PanelByte(tile,(192+panel)*16+row);
				int factor=int(code)-int((code&128)*2);
				stage[row]=Bits(Float(base[row])*float(factor));
				if ((stage[row]&0x7f800000)==0x7f800000) return Refuse();
			}
			for (unsigned row=0;row<16;row++) { coefficient[row]=stage[row]; minimum[row]=0; }
		} else if (!(panel&1)) {
			if (!PioneerQ38AffineOnly(type,tile,bytes,panel,output)) return Refuse();
			for (unsigned row=0;row<16;row++) { coefficient[row]=output->coefficient[row]; minimum[row]=output->minimum[row]; }
		}
		for (unsigned row=0;row<16;row++) { output->coefficient[row]=coefficient[row]; output->minimum[row]=minimum[row]; }
		next++; return true;
	}
};
#endif
