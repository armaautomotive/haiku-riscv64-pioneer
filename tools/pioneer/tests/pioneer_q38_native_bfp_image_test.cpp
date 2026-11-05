/* SPDX-License-Identifier: MIT */
#define PIONEER_NATIVE_BFP_FIXTURE_ONLY
#include "pioneer_q38_native_bfp_matvec_test.cpp"
#define PIONEER_TENSIX_MACHINE_ONLY
#include "pioneer_tensix_image_test.cpp"
#include <string>

// Reuse the UNCHANGED project's strict RV32I Step decoder. Override ONLY
// this NEW image's fixed memory/MMIO map and independent BFP FPU observer.
template<class Arithmetic=NativeBfpMachine,unsigned Columns=32,unsigned Variant=(Columns==16?1:0)> struct NativeBfpRV32Image:Machine {
	static_assert(Columns==16 || Columns==32,"explicit prototype image geometry");
	static_assert(Variant<=2 && (Columns==32)==(Variant==0),"explicit prototype variant");
	Arithmetic native;std::vector<uint8_t> code;
	unsigned minStack=0x70000,minSP=0x70000,resultStores=0,ackRequests=0,dropAckAt=0;bool dropAck=false,awaitingAck=false,stackSeen=false;
	unsigned dropConfig=~0U;
	NativeBfpRV32Image(const char* directory):Machine() {
		const std::string root=directory;code=Read((root+"/text.bin").c_str());auto boot=Read((root+"/boot.bin").c_str());
		assert(boot.size()==4 && code.size()<=4096 && !(code.size()&3));
		std::fill(std::begin(l1),std::end(l1),0xdeadbeef);
		l1[0]=Word(boot,0);
		for(unsigned i=0;i<code.size()/4;i++)l1[1+i]=Word(code,i*4);
		for(unsigned i=0;i<3;i++)l1[Index(0x17ffc0+i*4)]=0;
	}
	static bool Range(uint32_t a,uint32_t first,unsigned size) { return a>=first && a-first<size; }
	unsigned Index(uint32_t a) override {
		assert(!(a&3));
		if(a==0x6000)return 0;
		if(Range(a,0x70000,unsigned(code.size())))return 1+(a-0x70000)/4;
		if(Range(a,0x6e000,8192))return 1025+(a-0x6e000)/4;
		if(Range(a,0x17ffc0,12))return 3073+(a-0x17ffc0)/4;
		if(Range(a,0x92000,256))return 3076+(a-0x92000)/4;
		assert(false && "native BFP scalar address outside owned map");return 0;
	}
	bool CodeAddress(uint32_t a) const override { return a==0x6000 || Range(a,0x70000,unsigned(code.size())); }
	unsigned StepLimit() const override { return 10000000; }
	void Instruction(uint32_t op) override {
		if(op==0xa2101816) { assert(!gpr63 && !awaitingAck);awaitingAck=true; }
		else if(op==0x4500017e) {
			assert(awaitingAck && !gpr63);
			ackRequests++;
			if(!dropAck && ackRequests!=dropAckAt) { native.Drain();gpr63=1;awaitingAck=false; }
		} else native.Op(op);
	}
	void Store(uint32_t a,uint32_t value) override {
		assert(!(a&3));
		if(a==0xffe40000) { Instruction(value);return; }
		if(a==0xffe000fc) { assert(!value);gpr63=0;return; }
		if(Range(a,0xffef0000,224*4)) {
			unsigned w=(a-0xffef0000)/4;if(w!=dropConfig)native.Config(w,value);return;
		}
		assert(Range(a,0x6e000,8192) || Range(a,0x17ffc0,12) || Range(a,0x92000,256));
		if(Range(a,0x6e000,8192))minStack=std::min(minStack,unsigned(a));
		if(Range(a,0x92000,256)) { assert(native.Clean());resultStores++; }
		l1[Index(a)]=value;
	}
	uint32_t Load(uint32_t a) override {
		assert(!(a&3));
		if(a==0xffe000fc)return gpr63;
		if(Range(a,0xffef0000,224*4)) { assert(!native.Pending() && !native.mathPending);return native.config[(a-0xffef0000)/4]; }
		if(Range(a,0xffbd8800,64) || Range(a,0xffbd8a00,64)) {
			assert(native.Clean() && native.config[3]==0);unsigned row=(a-0xffbd8000)/64,column=(a%64)/4;
			assert(native.valid[row]);native.outputReads++;return Bits(native.destination[row][column]);
		}
		assert(CodeAddress(a) || Range(a,0x6e000,8192) || Range(a,0x17ffc0,12));
		return l1[Index(a)];
	}
	virtual void BeforeStep() {} // observer extension only;strict RV decoder unchanged
	bool Execute() {
		while(pc!=0x70038 && pc!=0x70058) {
			BeforeStep();Step();if(regs[2])stackSeen=true;
			if(stackSeen) { assert(regs[2]>=0x6e000 && regs[2]<=0x70000 && !(regs[2]&15));minSP=std::min(minSP,regs[2]); }
		}
		assert(l1[Index(pc)]==0x0000006f && minStack>=0x6e000);
		if(pc==0x70058) { assert(l1[Index(0x17ffc8)]==(Variant==2?0x46433451U:Variant==1?0x46503451U:0x4642464eU) && !resultStores);return false; }
		assert(l1[Index(0x17ffc0)]==0x7000c && l1[Index(0x17ffc4)]==2 && l1[Index(0x17ffc8)]==(Variant==2?0x31433451U:Variant==1?0x31503451U:0x3142464eU));
		assert(native.Clean() && resultStores==Columns*2 && native.outputReads==Columns*2 && native.unpacks[0]==(Variant==2?6U:2U) && native.unpacks[1]==2);
		assert(!native.unpackBank[0] && !native.unpackBank[1] && !native.matrixBank[0] && !native.matrixBank[1]);return true;
	}
	void Exact(const std::array<float,Columns>& expected) {
		for(unsigned pass=0;pass<2;pass++) {
			for(unsigned row=0;row<Columns;row++)assert(l1[Index(0x92000+pass*Columns*4+row*4)]==Bits(expected[row]));
		}
	}
};
using NativeBfpRV32=NativeBfpRV32Image<>;
#ifndef PIONEER_NATIVE_BFP_IMAGE_ONLY
int main(int argc,char** argv) {
	assert(argc==3);unsigned maxStack=0,maxTouched=0;
	for(unsigned bits:{4U,8U})for(unsigned trial=0;trial<16;trial++) {
		NativeBfpRV32 image(argv[bits==4?1:2]);
		std::array<uint8_t,1088> packed;std::array<uint16_t,32> input;
		const auto expected=Fixture(bits,trial,packed,input);
		image.native.Stage(packed.data(),Kernel::WeightBytes(bits),input);
		image.native.delay[0]=trial&1?1000003:1;image.native.delay[1]=trial&1?1:100003;
		assert(image.Execute());image.Exact(expected);
		assert(image.native.matrixOps==(bits==4?16U:32U));
		maxStack=std::max(maxStack,0x70000-image.minSP);maxTouched=std::max(maxTouched,0x70000-image.minStack);checked++;
	}
	for(unsigned fault=0;fault<2;fault++) {
		NativeBfpRV32 image(argv[1]);if(!fault)image.dropConfig=72;else image.dropAck=true;
		assert(!image.Execute() && !image.native.matrixOps && !image.native.outputReads && !image.resultStores);checked++;
	}
	for(unsigned missing:{1U,2U}) {
		NativeBfpRV32 image(argv[2]);std::array<uint8_t,1088> p;std::array<uint16_t,32> v;Fixture(8,3,p,v);
		image.native.Stage(p.data(),1088,v);image.native.dropProducer=missing;bool unknown=false;
		try { image.Execute(); } catch(uint32_t marker) { unknown=marker==NativeBfpMachine::Unknown; }
		assert(unknown && !image.native.matrixOps && !image.resultStores && !image.native.outputReads);checked++;
	}
	{
		NativeBfpRV32 image(argv[1]);std::array<uint8_t,1088> p;std::array<uint16_t,32> v;Fixture(4,2,p,v);
		image.native.Stage(p.data(),576,v);image.dropAckAt=5;
		assert(!image.Execute() && image.native.matrixOps==8 && !image.native.outputReads && !image.resultStores && image.native.mathPending);checked++;
	}
	assert(maxStack==48 && maxTouched<=maxStack);
	printf("PASS OFFBOARD actual RV32I BFP4_B/BFP8_B kernels:%u checks;both formats/all32 outputs/two real bank alternations/unchanged strict RV32 decoder/independent descriptor+fidelity observer/readback+completion+producer failures;fixed memory map,NO NoC/GDDR/CPU math/raw GGUF decode/reference/fallback;observed SP allocation%u/touched%u;NOT hardware,model quality,full runtime or timing proof\n",checked,maxStack,maxTouched);
}
#endif
