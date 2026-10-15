/* SPDX-License-Identifier: MIT */
#include "../PioneerFloat16.h"
#include <cassert>
#include <cmath>
#include <limits>
#include <initializer_list>
int main() {
	assert(PioneerFloat16(0)==0 && PioneerFloat16(-0.0f)==0x8000);
	assert(PioneerFloat16(1)==0x3c00 && PioneerFloat16(-2)==0xc000);
	assert(PioneerFloat16(65504)==0x7bff && PioneerFloat16(65520)==0x7c00);
	assert(PioneerFloat16(std::ldexp(1.0f,-14))==0x400);
	assert(PioneerFloat16(std::ldexp(1.0f,-15))==0);
	assert(PioneerFloat16(std::ldexp(1.0f,-14)-std::ldexp(1.0f,-25))==0x400);
	assert(PioneerFloat16(std::ldexp(1.0f,-14)-std::ldexp(1.0f,-24))==0);
	assert(PioneerFloat16(1+std::ldexp(1.0f,-11))==0x3c00);
	assert(PioneerFloat16(1+3*std::ldexp(1.0f,-11))==0x3c02);
	assert(std::isinf(PioneerFloat16Value(PioneerFloat16(std::numeric_limits<float>::infinity()))));
	assert(std::isnan(PioneerFloat16Value(PioneerFloat16(std::numeric_limits<float>::quiet_NaN()))));
	for (unsigned sign=0;sign<2;sign++) for (unsigned e=1;e<31;e++) for (unsigned m=0;m<1024;m++) {
		uint16_t half=uint16_t((sign<<15)|(e<<10)|m);
		float independent=std::ldexp(float(1024+m),int(e)-25)*(sign?-1:1);
		assert(PioneerFloat16(independent)==half && PioneerFloat16Value(half)==independent);
		// Independent compiler conversion agrees over the finite normal
		// range; subnormals are deliberately flushed by this candidate.
		for (float delta:{-.49f,0.0f,.49f}) {
			float x=independent+delta*std::ldexp(1.0f,int(e)-25);
			_Float16 native=(_Float16)x; uint16_t bits; memcpy(&bits,&native,2);
			if (!(bits&0x7c00)) bits&=0x8000;
			assert(PioneerFloat16(x)==bits);
		}
	}
}
