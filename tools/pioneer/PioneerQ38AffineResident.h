/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_AFFINE_RESIDENT_H
#define PIONEER_Q38_AFFINE_RESIDENT_H
#include "PioneerQ38AffineOnly.h"
// OFFLINE ONLY. Stronger contract than AffineReuse:ONE fixed output panel
// per immutable fetched block,with its coefficient/minimum words exclusively
// read-only between Panel calls. Operand words remain caller-owned. This
// eliminates BOTH redundant cache copies and odd-panel Q8/Q4 publication.
// Card vector path alone meets this contract:raw NoC ends below91000,
// vector operands live in Dst,activation staging70920,results84000. No
// metadata destination may be rebound or reused after any refusal.
class PioneerQ38AffineResident {
	const uint8_t* tile=nullptr;
	PioneerQ38Panel* bound=nullptr;
	unsigned type=0,bytes=0,next=0;
	uint32_t attempted=0,ready=0,failed=0,base[16];
	bool Refuse() { failed=true; ready=false; return false; }
	static float Float(uint32_t b) { float f; __builtin_memcpy(&f,&b,4); return f; }
	static uint32_t Bits(float f) { uint32_t b; __builtin_memcpy(&b,&f,4); return b; }
	static bool Separate(uintptr_t a,unsigned n,uintptr_t b,unsigned m) {
		uintptr_t limit=~uintptr_t(0);
		return a<=limit-n && b<=limit-m && (a+n<=b || b+m<=a);
	}
public:
	PioneerQ38AffineResident()=default;
	PioneerQ38AffineResident(const PioneerQ38AffineResident&)=delete;
	PioneerQ38AffineResident& operator=(const PioneerQ38AffineResident&)=delete;
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
			|| (reinterpret_cast<uintptr_t>(output)&3) || (bound && bound!=output)
			|| !Separate(reinterpret_cast<uintptr_t>(output),sizeof(*output),reinterpret_cast<uintptr_t>(tile),bytes)
			|| !Separate(reinterpret_cast<uintptr_t>(output),sizeof(*output),reinterpret_cast<uintptr_t>(this),sizeof(*this))) return Refuse();
		if (type==14) {
			uint32_t stage[16];
			for (unsigned row=0;row<16;row++) {
				unsigned raw=PioneerQ38PanelByte(tile,(192+panel)*16+row);
				stage[row]=Bits(Float(base[row])*float(int(raw)-int((raw&128)*2)));
				if ((stage[row]&0x7f800000)==0x7f800000) return Refuse();
			}
			for (unsigned row=0;row<16;row++) { output->coefficient[row]=stage[row]; output->minimum[row]=0; }
		} else if (!(panel&1) && !PioneerQ38AffineOnly(type,tile,bytes,panel,output)) return Refuse();
		bound=output; next++; return true;
	}
};
#endif
