/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_RUNTIME_TEST_MACHINE_INCLUDED
#define PIONEER_RUNTIME_TEST_MACHINE_INCLUDED
#define PIONEER_TENSIX_MACHINE_ONLY
#include "pioneer_tensix_image_test.cpp"
#include "../../../src/add-ons/kernel/drivers/misc/BlackholeRuntimeMatvecImage.h"
#include "../../../src/add-ons/kernel/drivers/misc/BlackholeRuntimeFp16MatvecImage.h"
#include "../../../src/add-ons/kernel/drivers/misc/BlackholeRuntimeInt8MatvecImage.h"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerInt8Operand.h"
struct RuntimeMachine:Machine {
	unsigned inputs,outputs,requests=0,responses=0,delay=0,waits=0,faultAt=0;
	uint32_t base,target;
	bool pending=false,doorbellRead=false,corrupt=false;
	bool fp16;
	bool integer;
	bool int8;
#ifdef PIONEER_Q38_TEST_SHARED_WORKER_STORAGE
	::PioneerQ38SharedTestArray<0x80000,8192> buffers;
#else
	uint32_t buffers[8192]={};
#endif
	std::vector<uint32_t> weights;
	std::vector<uint16_t> vector;
	RuntimeMachine(unsigned k,unsigned n,unsigned trial=0,uint32_t address=0x04000000,bool half=false,bool integerBlock=false,bool integerEngine=false)
		:Machine(false,24),inputs(k),outputs(n),base(address),target(address),fp16(half),integer(integerBlock),int8(integerEngine),weights(PioneerMatvecWeightBytes(k,n)/4),vector(k) {
		assert(!int8 || (integer && !fp16));
		assert(PioneerMatvecShape(k,n)); l1[0]=0x0006a06f;
		for (unsigned i=0;i<1024;i++) l1[1+i]=int8?BlackholeRuntimeInt8MatvecCodeWord(i):half?BlackholeRuntimeFp16MatvecCodeWord(i):BlackholeRuntimeMatvecCodeWord(i);
		l1[Index(0x70500)]=base; l1[Index(0x70504)]=k/16; l1[Index(0x70508)]=n/16;
		for (unsigned i=0;i<k;i++) {
			vector[i]=uint16_t((0x3e00+(i*5+trial*13)%512)|((i&2)?0x8000:0));
			if (integer) vector[i]=uint16_t(Bits(i<32?float(int((i*17+trial*29)%255)-127):0)>>16);
			if (int8) vector[i]=PioneerInt8Operand(i<32?int((i*17+trial*29)%255)-127:0);
			buffers[i/2]|=uint32_t(vector[i])<<(16*(i&1));
		}
		for (unsigned row=0;row<n;row++) for (unsigned i=0;i<k;i++) {
			uint16_t value=uint16_t((0x3c00+(i*3+row*7)%512)|((i&1)?0x8000:0));
			if (integer) value=uint16_t(Bits(i<32?float(int((i*13+row*7)%256)-128):0)>>16);
			if (int8) value=PioneerInt8Operand(i<32?int((i*13+row*7)%256)-128:0);
			uint64_t byte=PioneerMatvecWeightElement(k,i,row);
			weights[byte/4]|=uint32_t(value)<<(8*(byte&3));
		}
		for (unsigned i=4096;i<8192;i++) buffers[i]=0x7fc00000;
	}
	bool Fp16Operands() const override { return fp16 || int8; }
	bool Int8Arithmetic() const override { return int8; }
	float Operand16(uint16_t half) const override {
		if (int8) { assert(!half || PioneerInt8OperandValid(half)); return float(half&1023)*((half&0x8000)?-1:1); }
		if (!fp16) return Float(uint32_t(half)<<16);
		unsigned e=(half>>10)&31; assert(e!=31);
		return e?std::ldexp(float(1024+(half&1023)),int(e)-25)*((half&0x8000)?-1:1):0;
	}
	unsigned Index(uint32_t address) override {
		assert((address&3)==0);
		if (address==0x6000) return 0;
		if (address>=0x70000 && address<0x71000) return 1+(address-0x70000)/4;
		if (address>=0x17ffc0 && address<=0x17ffc8) return 1025+(address-0x17ffc0)/4;
		assert(false); return 0;
	}
	unsigned StepLimit() const override { return 10000000; }
	uint32_t ParkPc() const override { return TT_RUNTIME_MATVEC_PARK_PC; }
	uint32_t FinalPhase() const override { return TT_RUNTIME_MATVEC_FINAL_PHASE; }
	unsigned ExpectedFixtures() const override { return (inputs/16)*(outputs/16); }
	bool CodeAddress(uint32_t address) const override {
		return address==0x6000 || (address>=0x70000 && address<0x70500)
			|| (address>=0x70540 && address<0x70580) || (address>=0x70600 && address<0x70660)
			|| (address>=0x70a40 && address<0x70ac0);
	}
	void Store(uint32_t address,uint32_t value) override {
		if (address>=0x84000 && address<0x88000) { assert(drained); buffers[(address-0x80000)/4]=value; return; }
		if (address==0xffb20000) {
			if (pending || !drained || value!=base+requests*576) fprintf(stderr,"NoC address pc=%x pending=%u drained=%u value=%x expected=%x requests=%u s4=%u\n",pc,pending,drained,value,base+requests*576,requests,regs[20]);
			assert(!pending && drained && value==base+requests*576); target=value; return;
		}
		if (address==0xffb20040) {
			assert(value==1 && !pending && drained && target==base+requests*576 && requests<ExpectedFixtures());
			requests++; pending=true; doorbellRead=false; waits=0; return;
		}
		if (address==0x17ffc8 && value==0x4e4f4346) { assert(requests==faultAt); throw value; }
		Machine::Store(address,value);
	}
	uint32_t Load(uint32_t address) override {
		if (address>=0x80000 && address<0x88000) return buffers[(address-0x80000)/4];
		if (address>=0x70500 && address<0x70a20) return l1[Index(address)];
		if (address==0xffb20000) return target;
		if (address==0xffb20040) {
			doorbellRead=true;
			if (pending && ++waits>delay && requests!=faultAt) {
				for (unsigned w=0;w<144;w++) l1[Index(0x70700+w*4)]=weights[(requests-1)*144+w]^((corrupt && w==4)?0x00800080:0);
				pending=false; responses++;
			}
			return pending?1:0;
		}
		if (address==0xffb20208) { assert(!requests || doorbellRead); return responses; }
		if (address==0xffb20240) { assert(doorbellRead); return pending?1:0; }
		return Machine::Load(address);
	}
	bool CorrectRuntime() const {
		for (unsigned row=0;row<outputs;row++) {
			float sum=0,absolute=0;
			// Independent output-major host matrix, not the device tile layout.
			for (unsigned i=0;i<inputs;i++) {
				uint16_t weight=uint16_t((0x3c00+(i*3+row*7)%512)|((i&1)?0x8000:0));
				if (integer) weight=uint16_t(Bits(i<32?float(int((i*13+row*7)%256)-128):0)>>16);
				if (int8) weight=PioneerInt8Operand(i<32?int((i*13+row*7)%256)-128:0);
				float product=Operand16(fp16?weight&~1U:weight)*Operand16(vector[i]);
				sum+=product; absolute+=fabsf(product);
			}
			if (int8) { if (int32_t(buffers[4096+row])!=int32_t(sum)) return false; }
			else if (!isfinite(Float(buffers[4096+row])) || (integer?Float(buffers[4096+row])!=sum:fabsf(Float(buffers[4096+row])-sum)>1e-5f+absolute*1e-5f)) return false;
		}
		for (unsigned row=outputs;row<4096;row++) if (buffers[4096+row]!=0x7fc00000) return false;
		return true;
	}
};
#ifndef PIONEER_RUNTIME_MACHINE_ONLY
int main(int argc,char** argv)
{
	assert(argc==5); auto boot=Read(argv[1]),code=Read(argv[2]),fp16=Read(argv[3]),int8=Read(argv[4]);
	assert(boot.size()==4 && Word(boot,0)==0x0006a06f && code.size()==4096);
	for (unsigned w=0;w<1024;w++) assert(Word(code,w*4)==BlackholeRuntimeMatvecCodeWord(w));
	assert(fp16.size()==4096); unsigned differences=0;
	for (unsigned w=0;w<1024;w++) {
		assert(Word(fp16,w*4)==BlackholeRuntimeFp16MatvecCodeWord(w));
		differences+=Word(fp16,w*4)!=Word(code,w*4);
	}
	assert(differences==6);
	assert(int8.size()==4096); differences=0;
	for (unsigned w=0;w<1024;w++) {
		assert(Word(int8,w*4)==BlackholeRuntimeInt8MatvecCodeWord(w)); differences+=Word(int8,w*4)!=Word(fp16,w*4);
	}
	assert(differences==2);
	for (unsigned k:{1024U,2048U,3072U,5120U,6144U}) {
		RuntimeMachine m(k,k==1024?32:16); m.delay=3; m.Run();
		assert(m.CorrectRuntime() && m.requests==m.ExpectedFixtures() && m.matrixInstructions==4*m.ExpectedFixtures() && m.outputReads==m.outputs);
	}
	for (unsigned trial=0;trial<4;trial++) {
		RuntimeMachine block(1024,32,trial,0x04000000,false,true); block.Run();
		assert(block.CorrectRuntime() && block.requests==128 && block.matrixInstructions==512);
		RuntimeMachine engine(1024,32,trial,0x04000000,false,true,true); engine.Run();
		assert(engine.CorrectRuntime() && engine.requests==128 && engine.matrixInstructions==512);
	}
	RuntimeMachine alternate(1024,64,1,0x04800000); alternate.Run(); assert(alternate.CorrectRuntime());
	for (unsigned k:{1024U,2048U,3072U,5120U,6144U}) {
		RuntimeMachine half(k,32,1,0x04000000,true); half.delay=3; half.Run();
		assert(half.CorrectRuntime() && half.requests==half.ExpectedFixtures() && half.matrixInstructions==4*half.ExpectedFixtures());
	}
	RuntimeMachine stale(1024,32); stale.dropClear=true;
	for (auto& row:stale.dst) for (float& value:row) value=3;
	stale.Run(); assert(!stale.CorrectRuntime());
	RuntimeMachine integerStale(1024,32,0,0x04000000,false,true,true); integerStale.dropClear=true;
	for (auto& row:integerStale.dstInt) for (int32_t& value:row) value=7;
	integerStale.Run(); assert(!integerStale.CorrectRuntime());
	RuntimeMachine corrupt(1024,32); corrupt.corrupt=true; corrupt.Run(); assert(!corrupt.CorrectRuntime());
	for (unsigned fault:{1U,65U,128U}) {
		RuntimeMachine m(1024,32); m.faultAt=fault; bool failed=false;
		try { m.Run(); } catch (uint32_t marker) { failed=marker==0x4e4f4346; }
		assert(failed && m.requests==fault && m.responses==fault-1 && m.matrixInstructions==4*(fault-1));
	}
	assert(!PioneerMatvecShape(0,16) && !PioneerMatvecShape(1024,0) && !PioneerMatvecShape(1024,4097));
	assert(PioneerMatvecWeightRange(2*1024*1024,4096,1024,32) && !PioneerMatvecWeightRange(4096,0,1024,16));
	printf("PASS: independently assembled BF16/FP16 runtime images, retained weights, separate vector, K/N loops, phase-aware dot references, bounded NoC faults; NO hardware\n");
}
#endif
#endif
