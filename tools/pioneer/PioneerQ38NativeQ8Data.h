/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_NATIVE_Q8_DATA_H
#define PIONEER_Q38_NATIVE_Q8_DATA_H
#include <stdint.h>
// OFFLINE DATA format ONLY. Native signed INT8 unpack consumes SM7,not TC8.
// The authenticated PUBLIC model inventory proves all its Q8 weights are
// [-127,127]. Encode ONCE at load time,without clipping/requantization or
// learned products. A DIFFERENT layout/ABI/source owner is still required.
// No native image/driver/source selector consumes this format yet.
struct PioneerQ38NativeQ8Data {
	static constexpr unsigned Version=1,TileBytes=576,Header=32,QuantEnd=544,PageBytes=4096;
	static bool Encode(uint8_t tc,uint8_t& sm) {
		if(tc==128)return false;
		sm=tc&128?uint8_t(128U|(256U-unsigned(tc))):tc;return true;
	}
	static bool Decode(uint8_t sm,uint8_t& tc) {
		if(sm==128)return false; // noncanonical negative zero,not a generated value
		tc=sm&128?uint8_t(0U-unsigned(sm&127)):sm;return true;
	}
	static bool Separate(uintptr_t a,unsigned n,uintptr_t b,unsigned m) {
		uintptr_t limit=~uintptr_t(0);return a<=limit-n && b<=limit-m && (a+n<=b || b+m<=a);
	}
	static bool PreparePage(const uint8_t* input,uint64_t fragmentOffset,unsigned validBytes,uint8_t* output,unsigned capacity) {
		if(!input || !output || !validBytes || validBytes>PageBytes || capacity<PageBytes || fragmentOffset%PageBytes
			|| fragmentOffset>UINT64_MAX-validBytes || (validBytes<PageBytes && (fragmentOffset+validBytes)%TileBytes)
			|| !Separate(reinterpret_cast<uintptr_t>(input),PageBytes,reinterpret_cast<uintptr_t>(output),PageBytes))return false;
		uint8_t stage[PageBytes];
		for(unsigned i=0;i<PageBytes;i++) {
			unsigned at=unsigned((fragmentOffset+i)%TileBytes);uint8_t value=input[i];
			if(i>=validBytes) {if(value)return false;}
			else if(at>=Header && at<QuantEnd && !Encode(value,value))return false;
			stage[i]=value;
		}
		for(unsigned i=0;i<PageBytes;i++)output[i]=stage[i];
		return true;
	}
};
#endif
