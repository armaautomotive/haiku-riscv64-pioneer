/* SPDX-License-Identifier: MIT */
#define PIONEER_BULK_Q8_MACHINE_ONLY
#include "pioneer_runtime_q8_bulk_image_test.cpp"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerComputeFleet.h"
#include "../../../src/add-ons/kernel/drivers/misc/BlackholeFleetPersistentImage.h"
#include "../../../src/add-ons/kernel/drivers/misc/BlackholeRuntimeQ8InlineImage.h"
#include "../../../src/add-ons/kernel/drivers/misc/BlackholeRuntimeQ8PartialImage.h"
#include "../../../src/add-ons/kernel/drivers/misc/BlackholeRuntimeQ8PhaseImage.h"
struct PersistentMachine:BulkQ8Machine {
	unsigned worker,jobs=0,previousReads=0,inputReads=0,completedInputs=0;
	uint32_t command[10]={},source[4096]={};
	bool inputPending=false;
	bool inlineMath=false,partialMath=false;
	PersistentMachine(unsigned k,unsigned n,unsigned w):BulkQ8Machine(k,n,0),worker(w) {
		for (unsigned i=0;i<1024;i++) l1[6144+i]=BlackholeFleetPersistentCodeWord(i);
		l1[Index(TT_RUNTIME_Q8_PARK_PC)]=TT_PERSISTENT_MATH_RETURN;
		l1[Index(0x7050c)]=worker!=0;
		l1[Index(0x70510)]=PioneerWorkerX(worker)|(PioneerWorkerY(worker)<<6);
		pc=0x71000;
	}
	unsigned StepLimit() const override { return 50000000; }
	bool InlineTensix() const override { return inlineMath; }
	void Instruction(uint32_t op) override { if (op==0x8f000000) return; BulkQ8Machine::Instruction(op); }
	void Inline() {
		inlineMath=true;
		for (unsigned i=0;i<1024;i++) l1[1+i]=BlackholeRuntimeQ8InlineCodeWord(i);
		l1[Index(TT_RUNTIME_Q8_PARK_PC)]=TT_PERSISTENT_MATH_RETURN;
		l1[Index(0x70500)]=base; l1[Index(0x70504)]=inputs/16; l1[Index(0x70508)]=outputs/16;
		l1[Index(0x7050c)]=worker!=0; l1[Index(0x70510)]=PioneerWorkerX(worker)|(PioneerWorkerY(worker)<<6);
	}
	void Partial() {
		Inline(); partialMath=true;
		for (unsigned i=0;i<1024;i++) l1[1+i]=BlackholeRuntimeQ8PartialCodeWord(i);
		l1[Index(TT_RUNTIME_Q8_PARK_PC)]=TT_PERSISTENT_MATH_RETURN;
		l1[Index(0x70500)]=base; l1[Index(0x70504)]=inputs/16; l1[Index(0x70508)]=outputs/16;
		l1[Index(0x7050c)]=worker!=0; l1[Index(0x70510)]=PioneerWorkerX(worker)|(PioneerWorkerY(worker)<<6);
	}
	void Phase() {
		Inline();
		for (unsigned i=0;i<1024;i++) l1[1+i]=BlackholeRuntimeQ8PhaseCodeWord(i);
		l1[Index(TT_RUNTIME_Q8_PARK_PC)]=TT_PERSISTENT_MATH_RETURN;
		l1[Index(0x70500)]=base; l1[Index(0x70504)]=inputs/16; l1[Index(0x70508)]=outputs/16;
		l1[Index(0x7050c)]=worker!=0; l1[Index(0x70510)]=PioneerWorkerX(worker)|(PioneerWorkerY(worker)<<6);
	}
	bool CorrectExpected() const {
		if (!partialMath) return CorrectIeee();
		if (conditional) return false;
		for (unsigned row=0;row<outputs;row++) {
			float expected=0;
			for (unsigned block=0;block<inputs/32;block++) {
				int dot=0; for (unsigned i=0;i<32;i++) dot+=NativeWeight(row,block*32+i)*Input(block*32+i,trial);
				// Independent native partial-product oracle, never host fmaf.
				float scale=PioneerBlackholeFma(Half(WeightScale(row,block)),InputScale(block,trial),0);
				expected=PioneerBlackholeFma(float(dot),scale,expected);
			}
			if (Bits(expected)!=buffers[4096+row]) return false;
		}
		return true;
	}
	unsigned Index(uint32_t a) override {
		if (a>=0x71000 && a<0x72000) { assert(!(a&3)); return 6144+(a-0x71000)/4; }
		return BulkQ8Machine::Index(a);
	}
	bool CodeAddress(uint32_t a) const override {
		return (a>=0x71000 && a<0x71300) || (inlineMath && a>=0x70ea8 && a<0x70fac) || BulkQ8Machine::CodeAddress(a);
	}
	void Store(uint32_t a,uint32_t v) override {
		if (a>=0x71300 && a<0x71310) {
			assert(a==0x71304 || a==0x7130c); l1[Index(a)]=v; return;
		}
		if (a>=0xffb20000 && a<=0xffb20024) {
			assert(!pending && !inputPending); command[(a-0xffb20000)/4]=v;
			if (!command[2] || command[2]==786) { if (a==0xffb20000) target=v; }
			return;
		}
		if (a==0xffb20040 && command[2]==129) {
			const uint32_t expected[]={0x80000,0,129,0x80000,0,PioneerWorkerX(worker)|(PioneerWorkerY(worker)<<6),0,0,16384,0};
			assert(worker && v==1 && !inputPending && !pending && !memcmp(command,expected,sizeof(expected)));
			inputReads++; inputPending=true; return;
		}
		if (a==0xffb20040) {
			const uint32_t expected[]={base+requests*9216,0,786,0x88000,0,PioneerWorkerX(worker)|(PioneerWorkerY(worker)<<6),0,0,9216,0};
			assert(!inputPending && !memcmp(command,expected,sizeof(expected)));
		}
		if (a==0x17ffc8 && v==0x4e4f4346) throw v;
		BulkQ8Machine::Store(a,v);
	}
	uint32_t Load(uint32_t a) override {
		if (a>=0x71000 && a<0x72000) return l1[Index(a)];
		if (a>=0xffb20000 && a<=0xffb20024) return command[(a-0xffb20000)/4];
		if (a>=0xffb20208 && a<=0xffb20214) {
			unsigned index=(a-0xffb20208)/4;
			return index==1?(previousReads+responses)*144+completedInputs*256:previousReads+responses+completedInputs;
		}
		if (a==0xffb20240) return pending || inputPending;
		if (a==0xffb20040 && inputPending) {
			memcpy(buffers,source,sizeof(source)); inputPending=false; completedInputs++; return 0;
		}
		return BulkQ8Machine::Load(a);
	}
	void Until(uint32_t target) {
		unsigned began=steps;
		do { Step(); } while (pc!=target && steps-began<20000000);
		assert(pc==target && drained && !pending && !inputPending);
	}
	void Boot() {
		Until(TT_PERSISTENT_WAIT);
		assert(l1[Index(0x7130c)]==TT_PERSISTENT_READY && !requests && !matrixInstructions);
	}
	void Job(unsigned t) {
		assert(pc==TT_PERSISTENT_WAIT && l1[Index(0x71300)]==jobs && l1[Index(0x71304)]==jobs);
		previousReads+=responses; requests=responses=0; trial=t;
		for (unsigned k=0;k<inputs;k++) {
			if (!(k&1)) buffers[k/2]=0;
			buffers[k/2]|=uint32_t(PioneerInt8Operand(Input(k,t)))<<(16*(k&1));
		}
		for (unsigned b=0;b<inputs/32;b++) buffers[3072+b]=Bits(InputScale(b,t));
		memcpy(source,buffers,sizeof(source));
		if (worker) for (unsigned i=0;i<4096;i++) buffers[i]=0xcafebabe;
		l1[Index(0x71300)]=++jobs;
		Until(TT_PERSISTENT_WAIT);
		assert(l1[Index(0x71304)]==jobs && CorrectExpected() && Guards());
		assert(!memcmp(source,buffers,sizeof(source)) && inputReads==(worker?jobs:0));
	}
	void Stop() {
		unsigned matrixBefore=matrixInstructions;
		l1[Index(0x71308)]=1; l1[Index(0x71300)]=++jobs;
		Until(TT_PERSISTENT_STOP_PC);
		assert(l1[Index(0x71304)]==jobs && l1[Index(0x7130c)]==TT_PERSISTENT_STOPPED && matrixInstructions==matrixBefore);
	}
};
#ifndef PIONEER_PERSISTENT_MACHINE_ONLY
int main(int argc,char** argv) {
	assert(argc==2); auto code=Read(argv[1]); assert(code.size()==TT_PERSISTENT_BYTES);
	for (unsigned w=0;w<code.size()/4;w++) assert(Word(code,w*4)==BlackholeFleetPersistentCodeWord(w));
	for (unsigned worker=0;worker<120;worker++) {
		PersistentMachine m(1024,32,worker); m.Boot(); m.Job(0); m.Job(1); m.Job(9); m.Stop();
	}
	for (unsigned k:{1024U,2048U,3072U,5120U,6144U}) {
		PersistentMachine m(k,64,119); m.Boot(); m.Job(2); m.Job(3); m.Stop();
	}
	PersistentMachine full(1024,4096,1); full.Boot(); full.Job(0); full.Job(1); full.Stop();
	for (unsigned fault=0;fault<2;fault++) {
		PersistentMachine m(1024,32,1); m.Boot(); m.l1[m.Index(0x71300)]=fault?1:2; m.l1[m.Index(0x71308)]=fault?2:0;
		bool failed=false; try { m.Until(TT_PERSISTENT_FAULT); } catch (uint32_t marker) { failed=marker==0x4e4f4346; }
		assert(failed && !m.requests && !m.inputReads && !m.matrixInstructions);
	}
	puts("PASS OFFLINE assembled persistent RISC + patched exact math,120 workers,changed inputs across jobs WITHOUT coprocessor reset,all K,N4096,STOP and invalid mailbox fault parks; NOT hardware qualification");
}
#endif
