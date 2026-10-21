/* SPDX-License-Identifier: MIT */
#include <array>
#include <cassert>
#include <cstdio>
#include <vector>
#include "../PioneerQ38NativeQ8Data.h"
int main() {
	for(unsigned byte=0;byte<256;byte++) {
		uint8_t sm=0x55,tc=0xaa;bool valid=PioneerQ38NativeQ8Data::Encode(uint8_t(byte),sm);
		assert(valid==(byte!=128));if(!valid){assert(sm==0x55);continue;}
		int expected=int(byte)-int((byte&128)*2),actual=int(sm&127)*((sm&128)?-1:1);
		assert(actual==expected && PioneerQ38NativeQ8Data::Decode(sm,tc) && tc==byte);
	}
	uint8_t untouched=0x55;assert(!PioneerQ38NativeQ8Data::Decode(128,untouched) && untouched==0x55);
	uint32_t seed=0x38a801;unsigned quantValues=0;
	for(unsigned trial=0;trial<32;trial++) {
		std::vector<uint8_t> tape(576*129);for(unsigned i=0;i<tape.size();i++) {
			seed^=seed<<13;seed^=seed>>17;seed^=seed<<5;unsigned value=seed%255;if(value>=128)value++;
			tape[i]=uint8_t((i%576>=32 && i%576<544)?value:seed);
		}
		for(uint64_t offset=0;offset<tape.size();offset+=4096) {
			std::array<uint8_t,4096> input{};std::array<uint8_t,4098> out;out.fill(0xdd);
			unsigned bytes=unsigned(tape.size()-offset<4096?tape.size()-offset:4096);
			for(unsigned i=0;i<bytes;i++)input[i]=tape[size_t(offset)+i];
			assert(PioneerQ38NativeQ8Data::PreparePage(input.data(),offset,bytes,out.data()+1,4096));
			assert(out.front()==0xdd && out.back()==0xdd);
			for(unsigned i=0;i<4096;i++) {
				unsigned at=unsigned((offset+i)%576);uint8_t expected=input[i];
				if(i<bytes && at>=32 && at<544) {
					int q=int(input[i])-int((input[i]&128)*2);
					expected=uint8_t(q<0?128+unsigned(-q):unsigned(q));quantValues++;
					uint8_t decoded=0;assert(PioneerQ38NativeQ8Data::Decode(out[i+1],decoded) && decoded==input[i]);
				}
				assert(out[i+1]==expected); // includes ALL metadata/padding and last-page zeros
			}
		}
	}
	for(unsigned fault=0;fault<10;fault++) {
		std::array<uint8_t,4096> input{},out;out.fill(0xdd);const uint8_t* src=input.data();uint8_t* dst=out.data();
		uint64_t offset=0;unsigned bytes=4096,capacity=4096;
		if(fault==0)src=nullptr;if(fault==1)dst=nullptr;if(fault==2)bytes=0;if(fault==3)bytes=4097;
		if(fault==4)capacity=4095;if(fault==5)offset=1;if(fault==6)src=out.data();
		if(fault==7)input[32]=128;if(fault==8)bytes=1;if(fault==9)offset=UINT64_MAX-4095;
		assert(!PioneerQ38NativeQ8Data::PreparePage(src,offset,bytes,dst,capacity));for(auto b:out)assert(b==0xdd);
	}
	std::array<uint8_t,4096> input{},out;input[576]=1;out.fill(0xdd);
	assert(!PioneerQ38NativeQ8Data::PreparePage(input.data(),0,576,out.data(),4096));for(auto b:out)assert(b==0xdd);
	for(unsigned at:{32U,543U,4095U}) {
		input.fill(0);input[at]=128;out.fill(0xdd);
		assert(!PioneerQ38NativeQ8Data::PreparePage(input.data(),0,4096,out.data(),4096));for(auto b:out)assert(b==0xdd);
	}
	printf("PASS OFFLINE native-Q8 DATA:ALL255 TC8<->SM7 exact,minus128/noncanonical-zero refusal,%u quant values across ALL9 page/tile phases+partial pages,metadata/padding/canaries exact,bounds/alias/overflow/late-domain refusal before publication;SAME_FOOTPRINT_NO_NATIVE_OR_SPEED_QUALIFICATION\n",quantValues);
}
