/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_PREDECODED_Q4_H
#define PIONEER_Q38_PREDECODED_Q4_H
#ifndef PIONEER_ATTENTION_FREESTANDING
#include <stdint.h>
#endif
// OFFLINE FORMAT STUDY ONLY. NO native/source/layout selector consumes it.
// Keep ALL original256 metadata bytes;expand2048 packed quant bytes into
// 4096 unsigned INT8 values in sixteen original K-major/output-row panels.
// Every value0..15 is preserved;NO learned products/requantization,model
// authentication,buffer ownership or device authority comes from this helper.
struct PioneerQ38PredecodedQ4 {
	static constexpr unsigned Version=1,OriginalBytes=2304,MetadataBytes=256,Bytes=4352;
	static bool Separate(uintptr_t a,unsigned n,uintptr_t b,unsigned m) {
		uintptr_t limit=~uintptr_t(0);return a<=limit-n && b<=limit-m && (a+n<=b || b+m<=a);
	}
	static bool Prepare(const uint8_t* original,unsigned bytes,uint8_t* output,unsigned capacity) {
		if(!original || !output || bytes!=OriginalBytes || capacity<Bytes
			|| !Separate(reinterpret_cast<uintptr_t>(original),OriginalBytes,reinterpret_cast<uintptr_t>(output),Bytes))return false;
		uint8_t stage[Bytes];
		for(unsigned i=0;i<MetadataBytes;i++)stage[i]=original[i];
		for(unsigned p=0;p<16;p++)for(unsigned i=0;i<256;i++) {
			unsigned at=MetadataBytes+(p/4)*512+(p%2)*256+i;
			stage[MetadataBytes+p*256+i]=uint8_t((original[at]>>((p%4>=2)?4:0))&15);
		}
		for(unsigned i=0;i<Bytes;i++)output[i]=stage[i];
		return true;
	}
	// Diagnostic inverse,not an installed reader or a source-authentication
	// shortcut. Refuse any out-of-domain expanded byte BEFORE publication.
	static bool Recover(const uint8_t* prepared,unsigned bytes,uint8_t* output,unsigned capacity) {
		if(!prepared || !output || bytes!=Bytes || capacity<OriginalBytes
			|| !Separate(reinterpret_cast<uintptr_t>(prepared),Bytes,reinterpret_cast<uintptr_t>(output),OriginalBytes))return false;
		uint8_t stage[OriginalBytes]={};
		for(unsigned i=0;i<MetadataBytes;i++)stage[i]=prepared[i];
		for(unsigned p=0;p<16;p++)for(unsigned i=0;i<256;i++) {
			unsigned value=prepared[MetadataBytes+p*256+i];if(value>15)return false;
			unsigned at=MetadataBytes+(p/4)*512+(p%2)*256+i;
			stage[at]|=uint8_t(value<<((p%4>=2)?4:0));
		}
		for(unsigned i=0;i<OriginalBytes;i++)output[i]=stage[i];
		return true;
	}
};
static_assert(PioneerQ38PredecodedQ4::Bytes%64==0,"bounded complete sixteen-row NoC block;NOT native authority");
#endif
