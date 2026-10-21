/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_CACHED_AFFINE_H
#define PIONEER_Q38_CACHED_AFFINE_H
#include "PioneerQ38AffineOnly.h"
// OFFLINE candidate. ONE immutable fetched block and ONE exclusive metadata
// destination, as for AffineResident. Convert its HALF bases ONCE, not on
// every even Q4 panel. HALF*signed8 is exactly representable in FP32,
// including signed zeros and HALF subnormals; preserve every original bit.
// No learned product, device IO, physical address or host capability here.
class PioneerQ38CachedAffine {
	const uint8_t* tile=nullptr;
	PioneerQ38Panel* bound=nullptr;
	unsigned type=0,bytes=0,next=0;
	uint32_t attempted=0,ready=0,failed=0; // keep the existing qualified word-store subset
	uint32_t base[16],minimumBase[16];
	bool Refuse() { failed=true; ready=false; return false; }
	static float Float(uint32_t b) { float f; __builtin_memcpy(&f,&b,4); return f; }
	static uint32_t Bits(float f) { uint32_t b; __builtin_memcpy(&b,&f,4); return b; }
	static bool Separate(uintptr_t a,unsigned n,uintptr_t b,unsigned m) {
		uintptr_t limit=~uintptr_t(0);
		return a<=limit-n && b<=limit-m && (a+n<=b || b+m<=a);
	}
public:
	PioneerQ38CachedAffine()=default;
	PioneerQ38CachedAffine(const PioneerQ38CachedAffine&)=delete;
	PioneerQ38CachedAffine& operator=(const PioneerQ38CachedAffine&)=delete;
#if defined(PIONEER_Q38_PREDECODED_Q4) && defined(__GNUC__) && !defined(__clang__)
	__attribute__((noinline,optimize("Os")))
#else
	__attribute__((noinline))
#endif
	bool Prepare(unsigned kind,const uint8_t* data,unsigned count
#ifdef PIONEER_Q38_PREDECODED_Q4
		,bool predecoded=false
#endif
	) {
		if (attempted || failed) return Refuse();
		attempted=true;
		if (!data || !PioneerQ38PanelTileBytes(kind) || count!=
#ifdef PIONEER_Q38_PREDECODED_Q4
			(predecoded && kind==12?4352U:PioneerQ38PanelTileBytes(kind)) || (predecoded && kind!=12)
#else
			PioneerQ38PanelTileBytes(kind)
#endif
			|| !Separate(reinterpret_cast<uintptr_t>(data),count,reinterpret_cast<uintptr_t>(this),sizeof(*this))) return Refuse();
		for (unsigned row=0;row<16;row++) {
			unsigned at=(kind==14?208:0)*16+row;
			uint16_t half=uint16_t(PioneerQ38PanelByte(data,at)|(PioneerQ38PanelByte(data,at+16)<<8));
			if (!PioneerQ38BinaryScale(half,1,base[row])) return Refuse();
			if (kind==12) {
				half=uint16_t(PioneerQ38PanelByte(data,32+row)|(PioneerQ38PanelByte(data,48+row)<<8));
				if (!PioneerQ38BinaryScale(half,1,minimumBase[row])) return Refuse();
			}
		}
		tile=data; type=kind; bytes=count; ready=true; return true;
	}
	__attribute__((noinline)) bool Panel(unsigned panel,PioneerQ38Panel* output) {
		if (!ready || failed || !output || panel!=next || panel>=(type==8?2U:16U)
			|| (reinterpret_cast<uintptr_t>(output)&3) || (bound && bound!=output)
			|| !Separate(reinterpret_cast<uintptr_t>(output),sizeof(*output),reinterpret_cast<uintptr_t>(tile),bytes)
			|| !Separate(reinterpret_cast<uintptr_t>(output),sizeof(*output),reinterpret_cast<uintptr_t>(this),sizeof(*this))) return Refuse();
		if (type==14 || !(panel&1)) {
			uint32_t coefficients[16],minima[16];
			for (unsigned row=0;row<16;row++) {
				int factor=1; unsigned minimum=0;
				if (type==12) {
					unsigned group=panel/2;
					if (group<4) { factor=PioneerQ38PanelByte(tile,(4+group)*16+row)&63; minimum=PioneerQ38PanelByte(tile,(8+group)*16+row)&63; }
					else { factor=int((PioneerQ38PanelByte(tile,(8+group)*16+row)&15)|((PioneerQ38PanelByte(tile,group*16+row)>>6)<<4));
						minimum=(PioneerQ38PanelByte(tile,(8+group)*16+row)>>4)|((PioneerQ38PanelByte(tile,(4+group)*16+row)>>6)<<4); }
				} else if (type==14) { unsigned raw=PioneerQ38PanelByte(tile,(192+panel)*16+row); factor=int(raw)-int((raw&128)*2); }
				coefficients[row]=type==8?base[row]:Bits(Float(base[row])*float(factor));
				minima[row]=type==12?Bits(Float(minimumBase[row])*float(-int(minimum))):0;
				if ((coefficients[row]&0x7f800000)==0x7f800000 || (minima[row]&0x7f800000)==0x7f800000) return Refuse();
			}
			// Volatile fixed metadata stores prevent a freestanding compiler
			// from introducing libc copies. All rows validate BEFORE any write.
			volatile uint32_t* coefficient=output->coefficient;
			volatile uint32_t* minimum=output->minimum;
			for (unsigned row=0;row<16;row++) { coefficient[row]=coefficients[row]; minimum[row]=minima[row]; }
		}
		bound=output; next++; return true;
	}
#ifdef PIONEER_Q38_BLOCK_AFFINE_DST
	// Separate OFFLINE candidate. ALL original per-panel FP32 values and
	// rounding points are retained;only their DATA staging is moved earlier.
	// Caller must independently bind the fixed scratch and Dst ownership.
	__attribute__((noinline)) bool BlockQ4(PioneerQ38Panel* panel,volatile uint32_t* scratch,unsigned words) {
		if (!ready || failed || type!=12 || next || bound || !panel || !scratch || words!=256
			|| (reinterpret_cast<uintptr_t>(panel)&3) || (reinterpret_cast<uintptr_t>(scratch)&3)
			|| !Separate(reinterpret_cast<uintptr_t>(scratch),1024,reinterpret_cast<uintptr_t>(tile),bytes)
			|| !Separate(reinterpret_cast<uintptr_t>(scratch),1024,reinterpret_cast<uintptr_t>(this),sizeof(*this))
			|| !Separate(reinterpret_cast<uintptr_t>(scratch),1024,reinterpret_cast<uintptr_t>(panel),sizeof(*panel))) return Refuse();
		for (unsigned group=0;group<8;group++) {
			uint32_t coefficients[16],minima[16];
			for (unsigned row=0;row<16;row++) {
				unsigned factor,minimum;
				if (group<4) { factor=PioneerQ38PanelByte(tile,(4+group)*16+row)&63; minimum=PioneerQ38PanelByte(tile,(8+group)*16+row)&63; }
				else { factor=(PioneerQ38PanelByte(tile,(8+group)*16+row)&15)|((PioneerQ38PanelByte(tile,group*16+row)>>6)<<4);
					minimum=(PioneerQ38PanelByte(tile,(8+group)*16+row)>>4)|((PioneerQ38PanelByte(tile,(4+group)*16+row)>>6)<<4); }
				coefficients[row]=Bits(Float(base[row])*float(factor));
				minima[row]=Bits(Float(minimumBase[row])*float(-int(minimum)));
				if ((coefficients[row]&0x7f800000)==0x7f800000 || (minima[row]&0x7f800000)==0x7f800000) return Refuse();
			}
			for (unsigned row=0;row<16;row++) { scratch[group*32+row]=coefficients[row]; scratch[group*32+16+row]=minima[row]; }
		}
		bound=panel; next=16; return true;
	}
#endif
};
#endif
