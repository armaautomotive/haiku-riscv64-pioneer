/* SPDX-License-Identifier: MIT */
#define PIONEER_NATIVE_BFP_BAND_IMAGE_ONLY
#include "pioneer_q38_native_bfp_band_image_test.cpp"
#include "pioneer_blackhole_arithmetic_observer.h"
struct ArithmeticBandImage:BandRV32 {
	using BandRV32::BandRV32;
	void Instruction(uint32_t op) override {
		if((op>>24)==0x26)PioneerBlackholeArithmeticObserver::Matrix(native,op,true,[&]{BandRV32::Instruction(op);});
		else BandRV32::Instruction(op);
	}
};
int main(int argc,char** argv) {
	assert(argc==5);const unsigned width=unsigned(std::stoul(argv[2])),phases=unsigned(std::stoul(argv[3]));
	assert(!strcmp(argv[4],"mmio") || !strcmp(argv[4],"direct"));const bool direct=!strcmp(argv[4],"direct");
	uint32_t expected[32];for(auto& word:expected)assert(scanf("%x",&word)==1);
	const auto data=NativeBfpBandMachine::Data(width);ArithmeticBandImage image(argv[1],width,phases,direct);
	image.native.Stage(data);image.native.delay[0]=7;image.native.delay[1]=100003;assert(image.Execute());unsigned idealDifferences=0;
	for(unsigned row=0;row<32;row++) {
		for(unsigned pass=0;pass<2;pass++)assert(image.l1[image.Index(0x130000+pass*128+row*4)]==expected[row]);
		idealDifferences+=expected[row]!=NativeBfpMachine::Bits((phases==4?data.full:data.fast)[row]);
	}
	assert(idealDifferences);
	printf("PASS separate arithmetic ACTUAL full-K RV32 K%u phases%u %s,two jobs/all64 words exact vs independent Fraction predictions,ideal_different_rows%u;NO_PHYSICAL_SILICON_OR_MODEL_QUALITY_OR_SPEED_PROOF\n",width,phases,direct?"direct":"mmio",idealDifferences);
	if(width==5120 && phases==4 && !direct) {
		unsigned failures=0;
		for(unsigned fault=0;fault<3;fault++) {
			ArithmeticBandImage bad(argv[1],width,phases,false);bad.native.Stage(data);
			if(fault==0)bad.dropConfig=72;if(fault==1)bad.dropAckAt=1;if(fault==2)bad.dropAckAt=4+2*bad.native.tiles;
			assert(!bad.Execute() && !bad.stores && !bad.native.outputReads);failures++;
		}
		for(unsigned missing:{1U,2U,3U}) {
			ArithmeticBandImage bad(argv[1],width,phases,false);bad.native.Stage(data);bool refused=false;
			if(missing<3)bad.native.dropProducer=missing;else bad.native.omitFlip=true;
			try{bad.Execute();}catch(uint32_t marker){refused=marker==NativeBfpMachine::Unknown;}
			assert(refused && !bad.stores && !bad.native.outputReads);failures++;
		}
		for(unsigned phase:{1U,2U,3U}) {
			ArithmeticBandImage bad(argv[1],width,phases,false);bad.native.Stage(data);bad.native.dropPhase=phase;
			assert(bad.Execute());bool difference=false;
			for(unsigned row=0;row<32;row++)difference|=bad.l1[bad.Index(0x130000+row*4)]!=expected[row];
			assert(difference);failures++;
		}
		printf("PASS separate arithmetic wrapper failures:%u actual RV32 cases;config/completion/producer/bank failures NO output,all3 omitted fidelity terms differ from independent Fraction prediction;NO_HARDWARE_AUTHORITY\n",failures);
	}
}
