/* SPDX-License-Identifier: MIT */
#define PIONEER_NATIVE_BFP_IMAGE_ONLY
#include "pioneer_q38_native_bfp_image_test.cpp"
#include "pioneer_blackhole_arithmetic_observer.h"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerNativeBfpFixture.h"
struct ArithmeticSingleImage:NativeBfpRV32 {
	using NativeBfpRV32::NativeBfpRV32;
	void Instruction(uint32_t op) override {
		if((op>>24)==0x26)PioneerBlackholeArithmeticObserver::Matrix(native,op,false,[&]{NativeBfpRV32::Instruction(op);});
		else NativeBfpRV32::Instruction(op);
	}
};
int main(int argc,char** argv) {
	assert(argc==3);const unsigned bits=unsigned(std::stoul(argv[2]));assert(bits==4 || bits==8);
	uint32_t expected[32];for(auto& word:expected)assert(scanf("%x",&word)==1);
	std::array<uint8_t,1088> packed{};std::array<uint16_t,32> input{};
	for(unsigned i=0;i<PioneerNativeBfpFixture::WeightBytes(bits);i++)packed[i]=PioneerNativeBfpFixture::WeightByte(bits,i);
	for(unsigned i=0;i<32;i++)input[i]=PioneerNativeBfpFixture::InputBits(i);
	ArithmeticSingleImage image(argv[1]);image.native.Stage(packed.data(),PioneerNativeBfpFixture::WeightBytes(bits),input);
	image.native.delay[0]=100003;image.native.delay[1]=7;assert(image.Execute());
	for(unsigned pass=0;pass<2;pass++)for(unsigned row=0;row<32;row++)assert(image.l1[image.Index(0x92000+pass*128+row*4)]==expected[row]);
	if(bits==8)assert(expected[0]==0x4180b500); // only recorded BFP8 hardware word
	printf("PASS separate arithmetic ACTUAL single-tile RV32 bits%u,two jobs/all64 words exact vs independent Fraction predictions;ONE_PRIOR_BFP8_WORD_EXPLAINED_OTHER63_NOT_OBSERVED,NO_HARDWARE_ACCEPTANCE_OR_SPEED_PROOF\n",bits);
}
