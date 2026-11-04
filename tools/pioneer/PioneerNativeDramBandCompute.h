/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_NATIVE_DRAM_BAND_COMPUTE_H
#define PIONEER_NATIVE_DRAM_BAND_COMPUTE_H
#include "PioneerNativeDramBandLayout.h"
#include "PioneerQ38NativeBfpBand.h"
// Reuse the EXACT full4-phase producer,substituting ONLY private unpack
// descriptor addresses. Never edits the old hardware-qualified source/image.
// Not driver/Work authority;the surrounding native owner must qualify this
// new layout,image,NIU journal and complete lifecycle separately.
struct PioneerNativeDramBandCompute {
	template<class E> struct Descriptors {
		E& emitter;const unsigned tiles;unsigned weight=0,input=0;bool active=false,valid=true;
		explicit Descriptors(E& e,unsigned k):emitter(e),tiles(k/32){}
		void Op(unsigned value){if(valid)emitter.Op(value);}
		void Drain(){if(valid)emitter.Drain();}
		void Config(unsigned index,unsigned value) {
			if(!valid)return;
			if(index==112 && value==0x00200015)active=true; // after complete cold setup
			if(index==76) {
				if(active) {
					if(weight>=tiles || value!=PioneerQ38NativeBfpBand::Weight/16-1+weight*(PioneerQ38NativeBfpBand::WeightStride/16)){valid=false;return;}
					value=PioneerNativeDramBandLayout::Weight/16-1+weight++*(PioneerNativeDramBandLayout::WeightStride/16);
				} else {
					if(value!=PioneerQ38NativeBfpMatvec::WeightAddress/16-1){valid=false;return;}
					value=PioneerNativeDramBandLayout::Weight/16-1;
				}
			}
			if(index==124) {
				if(active) {
					if(input>=tiles || value!=PioneerQ38NativeBfpBand::Input/16-1+input*(PioneerQ38NativeBfpBand::InputStride/16)){valid=false;return;}
					value=PioneerNativeDramBandLayout::Input/16-1+input++*(PioneerNativeDramBandLayout::InputStride/16);
				} else {
					if(value!=PioneerQ38NativeBfpMatvec::InputAddress/16-1){valid=false;return;}
					value=PioneerNativeDramBandLayout::Input/16-1;
				}
			}
			emitter.Config(index,value);
		}
		bool Complete()const{return valid && active && weight==tiles && input==tiles;}
	};
	template<class E> static bool Run(E& emitter,unsigned k) {
		if(!PioneerNativeDramBandLayout::Width(k))return false;
		Descriptors<E> translated(emitter,k);
		return PioneerQ38NativeBfpBand::Run<4>(translated,k) && translated.Complete();
	}
};
#endif
