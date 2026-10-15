/* SPDX-License-Identifier: MIT */
#define PIONEER_TENSIX_MACHINE_ONLY
#include "pioneer_tensix_image_test.cpp"
#include "../../../src/add-ons/kernel/drivers/misc/BlackholeResidentMathImage.h"
struct ResidentMachine:Machine {
	unsigned requests=0,responses=0,outstanding=0,waitReads=0,delay=0;
	bool pending=false,doorbellRead=false,dropRead=false,corrupt=false,suppressResponse=false;
	ResidentMachine():Machine(false,24) {
		for (unsigned i=0;i<TT_RESIDENT_MATH_IMAGE_WORDS;i++) l1[i]=BlackholeResidentMathImageWord(i);
	}
	unsigned Index(uint32_t address) override {
		for (unsigned i=0;i<TT_RESIDENT_MATH_IMAGE_WORDS;i++) if (BlackholeResidentMathAddress(i)==address) return i;
		assert(false); return 0;
	}
	unsigned StepLimit() const override { return 1000000; }
	bool CodeAddress(uint32_t address) const override {
		return address==0x6000 || (address>=0x70000 && address<0x70500)
			|| (address>=0x70a40 && address<0x70ab0);
	}
	void Store(uint32_t address,uint32_t value) override {
		if (address==0xffb20040) {
			assert(value==1 && !pending && outstanding==0 && drained && requests<2);
			requests++; outstanding=1; pending=true; doorbellRead=false; waitReads=0; return;
		}
		if (address==0x17ffc8 && value==0x4e4f4346) {
			// Reached only by bounded failure cases. Reuse Run's stop address
			// after verifying the failure can never be interpreted as success.
			assert(dropRead || suppressResponse);
			assert(matrixInstructions==0 && requests==1);
			throw value;
		}
		Machine::Store(address,value);
	}
	uint32_t Load(uint32_t address) override {
		if (address==0xffb20040) {
			doorbellRead=true;
			if (pending && ++waitReads>delay && !dropRead) {
				for (unsigned w=0;w<TT_RESIDENT_MATH_GDDR_WORDS;w++)
					l1[Index(0x70500+w*4)]=BlackholeResidentMathGddrWord(w)^(corrupt?0x00800080:0);
				pending=false; outstanding=0;
				if (!suppressResponse) responses++;
			}
			return pending?1:0;
		}
		if (address==0xffb20208) { assert(!requests || doorbellRead); return responses; }
		if (address==0xffb20240) { assert(doorbellRead); return outstanding; }
		return Machine::Load(address);
	}
	bool DenseCorrect() {
		for (unsigned i=0;i<256;i++) if (l1[Index(0x17f800+i*4)]!=BlackholeNonuniformExpected(21,i)) return false;
		return true;
	}
};
int main(int argc,char** argv)
{
	assert(argc==3);
	auto boot=Read(argv[1]),code=Read(argv[2]);
	assert(boot.size()==4 && Word(boot,0)==BlackholeResidentMathImageWord(0));
	assert(code.size()==TT_RESIDENT_MATH_CODE_WORDS*4);
	for (unsigned w=0;w<TT_RESIDENT_MATH_CODE_WORDS;w++) assert(Word(code,w*4)==BlackholeResidentMathImageWord(w+1));
	for (unsigned delay:{0U,1U,32U,100U}) {
		ResidentMachine m; m.delay=delay; m.Run();
		assert(m.DenseCorrect() && m.requests==2 && m.responses==2 && !m.pending && m.matrixInstructions==8);
	}
	ResidentMachine broken; broken.corrupt=true; broken.Run(); assert(!broken.DenseCorrect());
	for (bool lostResponse:{false,true}) {
		ResidentMachine m; m.dropRead=!lostResponse; m.suppressResponse=lostResponse;
		bool fault=false;
		try { m.Run(); } catch (uint32_t value) { fault=value==0x4e4f4346; }
		assert(fault && m.requests==1 && m.matrixInstructions==0);
	}
	assert(BlackholeResidentMathAddress(TT_RESIDENT_MATH_IMAGE_WORDS)==UINT32_MAX);
	printf("PASS: assembled fixed resident-weight matrix image, independent RV32I/FPU/NoC simulation, delayed/corrupt/missing responses; NOT a hardware speed result\n");
}
