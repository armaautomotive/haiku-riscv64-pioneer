/* SPDX-License-Identifier: MIT */
#define PIONEER_TENSIX_MACHINE_ONLY
#include "pioneer_tensix_image_test.cpp"
#include "../../../src/add-ons/kernel/drivers/misc/BlackholeResidentGemvImage.h"
struct GemvMachine:Machine {
	const unsigned inputs;
	unsigned requests=0,responses=0,outstanding=0,waitReads=0,delay=0,faultAt=0;
	uint32_t target=0x04000000;
	bool pending=false,doorbellRead=false,corrupt=false,clearBetween=false;
	GemvMachine(unsigned count):Machine(false,24),inputs(count) {
		assert(BlackholeResidentGemvShape(inputs));
		for (unsigned i=0;i<TT_RESIDENT_MATH_IMAGE_WORDS;i++) l1[i]=BlackholeResidentGemvImageWord(i,inputs);
	}
	unsigned Index(uint32_t address) override {
		assert((address&3)==0);
		if (address==0x6000) return 0;
		if (address>=0x70000 && address<0x71000) return 1+(address-0x70000)/4;
		if (address>=0x17f800 && address<0x17fc00) return 1+1024+(address-0x17f800)/4;
		if (address>=0x17ffc0 && address<=0x17ffc8) return TT_RESIDENT_MATH_STATUS_INDEX+(address-0x17ffc0)/4;
		assert(false); return 0;
	}
	unsigned StepLimit() const override { return 3000000; }
	uint32_t ParkPc() const override { return TT_RESIDENT_GEMV_PARK_PC; }
	uint32_t FinalPhase() const override { return TT_RESIDENT_GEMV_FINAL_PHASE; }
	unsigned ExpectedFixtures() const override { return inputs/16; }
	bool CodeAddress(uint32_t address) const override {
		return address==0x6000 || (address>=0x70000 && address<0x70500)
			|| (address>=0x70a40 && address<0x70ad0);
	}
	void Store(uint32_t address,uint32_t value) override {
		if (address==0xffb20000) {
			assert(!pending && outstanding==0 && drained && value==0x04000000+requests*1088);
			target=value; return;
		}
		if (address==0xffb20040) {
			assert(value==1 && !pending && outstanding==0 && drained && requests<inputs/16);
			assert(target==0x04000000+requests*1088);
			requests++; outstanding=1; pending=true; doorbellRead=false; waitReads=0; return;
		}
		if (address==0x17ffc8 && value==0x4e4f4346) {
			assert(faultAt==requests && matrixInstructions==4*(requests-1)); throw value;
		}
		Machine::Store(address,value);
	}
	uint32_t Load(uint32_t address) override {
		if (address==0xffb20000) return target;
		if (address==0xffb20040) {
			doorbellRead=true;
			if (pending && ++waitReads>delay && requests!=faultAt) {
				for (unsigned w=0;w<TT_RESIDENT_MATH_GDDR_WORDS;w++) {
					uint32_t value=BlackholeResidentGemvGddrWord((requests-1)*TT_RESIDENT_MATH_GDDR_WORDS+w,inputs);
					if (corrupt && w==0x210/4) value^=0x00800080;
					l1[Index(0x70500+w*4)]=value;
				}
				if (clearBetween && requests>1) for (unsigned i=0;i<64;i++) valid[i]=false;
				pending=false; outstanding=0; responses++;
			}
			return pending?1:0;
		}
		if (address==0xffb20208) { assert(!requests || doorbellRead); return responses; }
		if (address==0xffb20240) { assert(doorbellRead); return outstanding; }
		return Machine::Load(address);
	}
	bool CorrectGemv() {
		for (unsigned output=0;output<128;output++) {
			int sum=0;
			// Independent host dot product: do not decode/reuse the staged
			// GDDR layout or the fixed-image expected-word function.
			if (output<16) for (unsigned k=0;k<inputs;k++)
				sum+=(int((k*7+k/8)%17)-8)*(int((k*17+output*3+(k/16)*output)%13)-6);
			uint32_t expected=Bits(float(sum));
			assert(expected==BlackholeResidentGemvExpected(output,inputs));
			if (l1[Index(0x17f800+output*4)]!=expected) return false;
		}
		return true;
	}
};
int main(int argc,char** argv)
{
	assert(argc==5); auto boot=Read(argv[1]); assert(boot.size()==4 && Word(boot,0)==0x0006a06f);
	unsigned argument=2;
	for (unsigned inputs:{1024U,5120U,6144U}) {
		auto code=Read(argv[argument++]); assert(code.size()==4096);
		for (unsigned w=0;w<1024;w++) assert(Word(code,w*4)==BlackholeResidentGemvImageWord(w+1,inputs));
		GemvMachine m(inputs); m.delay=3; m.Run();
		assert(m.CorrectGemv() && m.requests==inputs/16 && m.responses==inputs/16
			&& m.matrixInstructions==4*(inputs/16) && m.outputReads==128);
	}
	GemvMachine stale(1024); stale.dropClear=true;
	for (auto& row:stale.dst) for (float& value:row) value=3;
	stale.Run(); assert(!stale.CorrectGemv());
	GemvMachine reset(1024); reset.clearBetween=true; reset.Run(); assert(!reset.CorrectGemv());
	GemvMachine corrupt(1024); corrupt.corrupt=true; corrupt.Run(); assert(!corrupt.CorrectGemv());
	for (unsigned fault:{1U,17U,64U}) {
		GemvMachine m(1024); m.faultAt=fault; bool failed=false;
		try { m.Run(); } catch (uint32_t marker) { failed=marker==0x4e4f4346; }
		assert(failed && m.requests==fault && m.responses==fault-1);
	}
	assert(!BlackholeResidentGemvShape(0) && !BlackholeResidentGemvShape(16) && !BlackholeResidentGemvShape(6145));
	printf("PASS: 16-output resident GEMV, K=1024/5120/6144, independent full dot products, accumulated Dst, NoC ordering/fault bounds; NO hardware operation\n");
}
