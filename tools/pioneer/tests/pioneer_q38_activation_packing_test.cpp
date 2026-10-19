/* SPDX-License-Identifier: MIT */
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <vector>
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38ActivationPacking.h"
static uint16_t Read(const std::vector<uint32_t>& data,unsigned i) { return uint16_t(data.at(i/2)>>((i%2)*16)); }
int main() {
	std::vector<uint32_t> input(17408/2,0x40014001);
	for (unsigned raw=0;raw<65536;raw++) for (unsigned lane=0;lane<3;lane++) {
		input[0]=input[1]=0x40014001;
		unsigned shift=(lane%2)*16; input[lane/2]=(input[lane/2]&~(0xffffU<<shift))|(raw<<shift);
		uint32_t packed=0xdeadbeef; bool valid=PioneerInt8OperandValid(uint16_t(raw));
		assert(PioneerQ38PackActivationWord(input.data(),5120,0,packed)==valid);
		if (valid) assert(PioneerQ38ExpandActivation(packed>>(lane*9))==raw && !(packed&0xf8000000U));
		else assert(packed==0xdeadbeef);
	}
	for (unsigned k:{5120U,6144U,17408U}) {
		input.assign(k/2,0);
		for (unsigned i=0;i<k;i++) {
			unsigned state=(i*29+17)%257;
			uint16_t v=state==256?uint16_t(0xc000):PioneerInt8Operand(int(state)-128);
			input[i/2]|=uint32_t(v)<<((i%2)*16);
		}
		std::vector<uint32_t> image(k/2+2,0xdeadbeef);
		unsigned words=PioneerQ38PackedActivationWords(k);
		for (unsigned w=0;w<words;w++) assert(PioneerQ38PackActivationWord(input.data(),k,w,image[w]));
		if (k%3) assert(!(image[words-1]>>(9*(k%3))));
		// Independent scalar unpack,EXACT backwards in-place overlap pattern.
		for (unsigned word=k/2;word--;) {
			unsigned i=word*2,lo=(image[i/3]>>((i%3)*9))&511,hi=(image[(i+1)/3]>>(((i+1)%3)*9))&511;
			auto decode=[](unsigned v) { return uint16_t(0x4000|(v%256)|(v/256)*0x8000); };
			image[word]=uint32_t(decode(lo))|(uint32_t(decode(hi))<<16);
		}
		for (unsigned i=0;i<k;i++) assert(Read(image,i)==Read(input,i));
		assert(image[k/2]==0xdeadbeef && image[k/2+1]==0xdeadbeef);
		uint32_t result=0xdeadbeef;
		assert(!PioneerQ38PackActivationWord(input.data(),k,words,result) && result==0xdeadbeef);
		assert(!PioneerQ38PackActivationWord(nullptr,k,0,result));
	}
	puts("PASS lossless activation packing:ALL65536 halfwords in EACH triple lane,all257 valid states including negative zero/-128,5120/6144/17408 backwards in-place overlap+tail/canaries,invalid inputs leave output untouched;NO native qualification");
}
