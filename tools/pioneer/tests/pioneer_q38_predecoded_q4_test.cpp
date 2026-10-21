/* SPDX-License-Identifier: MIT */
#include <array>
#include <cassert>
#include <cstdio>
#include <cstring>
#include "../PioneerQ38PredecodedQ4.h"
#include "../PioneerQwen38Panel.h"
int main() {
	std::array<uint8_t,2304> original,recovered;std::array<uint8_t,4354> prepared;uint32_t seed=0x384732;
	for(unsigned trial=0;trial<1024;trial++) {
		for(auto& b:original){seed^=seed<<13;seed^=seed>>17;seed^=seed<<5;b=uint8_t(seed);}
		// Independent panel unpack also validates its metadata HALF values.
		for(unsigned row=0;row<16;row++)for(unsigned kind=0;kind<2;kind++)original[kind*32+row+16]&=0xfb;
		prepared.fill(0xdd);assert(PioneerQ38PredecodedQ4::Prepare(original.data(),2304,prepared.data()+1,4352));
		assert(prepared.front()==0xdd && prepared.back()==0xdd);
		assert(!memcmp(original.data(),prepared.data()+1,256));
		for(unsigned p=0;p<16;p++) {
			PioneerQ38Panel panel;assert(PioneerQ38UnpackPanel(12,original.data(),2304,p,&panel));
			for(unsigned e=0;e<256;e++) {
				uint16_t w=uint16_t(panel.operand[e/2]>>((e%2)*16));
				assert(!(w&0x8000) && (w&1023)<=15 && prepared[1+256+p*256+e]==(w&1023));
			}
		}
		assert(PioneerQ38PredecodedQ4::Recover(prepared.data()+1,4352,recovered.data(),2304) && recovered==original);
	}
	for(unsigned at:{256U,511U,4351U}) {
		auto broken=prepared;broken[1+at]=16;recovered.fill(0xee);
		assert(!PioneerQ38PredecodedQ4::Recover(broken.data()+1,4352,recovered.data(),2304));
		for(auto b:recovered)assert(b==0xee);
	}
	for(unsigned fault=0;fault<6;fault++) {
		prepared.fill(0xdd);const uint8_t* in=original.data();uint8_t* out=prepared.data()+1;unsigned size=2304,cap=4352;
		if(fault==0)in=nullptr;if(fault==1)out=nullptr;if(fault==2)size=2303;if(fault==3)cap=4351;
		if(fault==4)in=out;if(fault==5)in=out+4000;
		assert(!PioneerQ38PredecodedQ4::Prepare(in,size,out,cap));for(auto b:prepared)assert(b==0xdd);
	}
	puts("PASS OFFLINE predecoded-Q4 DATA:1024 lossless round trips+ALL16 panels/256 quant values match independent integer unpack,metadata/canaries exact,invalid value/null/size/capacity/overlap before publication;NO_IMAGE_LAYOUT_OWNER_NATIVE_OR_SPEED_QUALIFICATION");
}
