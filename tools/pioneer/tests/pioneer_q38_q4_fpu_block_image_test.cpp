/* SPDX-License-Identifier: MIT */
#define PIONEER_TENSIX_MACHINE_ONLY
#include "pioneer_tensix_image_test.cpp"
#include "pioneer_q38_q4_fpu_pipeline_machine.h"
#define PIONEER_Q4_PIPELINE_FIXTURE_ONLY
#include "pioneer_q38_q4_fpu_pipeline_test.cpp"
#include <string>
// Actual compiled NEW image with the UNCHANGED strict RV32I Step decoder.
// Only its code/private ABI stack/status/results and explicit MMIO exist.
// No scalar weight/metadata/activation/GDDR/NoC/host access is accepted.
struct Q4FpuBlockRV32:Machine {
	Q4FpuPipelineMachine native;std::vector<uint8_t> code;
	unsigned minSP=0x70000,minStore=0x70000,stores=0,acks=0,dropAckAt=0,portWrites=0,directIssues=0;
	unsigned dropConfig=~0U;bool awaiting=false,stackSeen=false;
	explicit Q4FpuBlockRV32(const char* folder) {
		std::string root=folder;code=Read((root+"/text.bin").c_str());auto boot=Read((root+"/boot.bin").c_str());
		assert(boot.size()==4 && code.size()<=8192 && !code.empty() && !(code.size()&3));
		std::fill(std::begin(l1),std::end(l1),0xdeadbeef);l1[0]=Word(boot,0);
		for(unsigned i=0;i<code.size()/4;i++)l1[1+i]=Word(code,i*4);
		for(unsigned i=0;i<3;i++)l1[Index(0x17ffc0+i*4)]=0;
	}
	static bool Range(uint32_t a,uint32_t first,unsigned bytes){return a>=first && a-first<bytes;}
	unsigned Index(uint32_t a)override {
		assert(!(a&3));if(a==0x6000)return 0;
		if(Range(a,0x70000,unsigned(code.size())))return 1+(a-0x70000)/4;
		if(Range(a,0x6e000,8192))return 2049+(a-0x6e000)/4;
		if(Range(a,0x17ffc0,12))return 4097+(a-0x17ffc0)/4;
		assert(Range(a,0x98000,256));return 4100+(a-0x98000)/4;
	}
	bool CodeAddress(uint32_t a)const override{return a==0x6000 || Range(a,0x70000,unsigned(code.size()));}
	bool InlineTensix()const override{return true;}
	unsigned StepLimit()const override{return 10000000;}
	void Instruction(uint32_t op)override {
		if(op==0xa2101816){assert(!gpr63 && !awaiting);awaiting=true;}
		else if(op==0x4500017e) {
			assert(awaiting && !gpr63);acks++;
			if(acks!=dropAckAt){native.Drain();gpr63=1;awaiting=false;}
		} else {directIssues++;native.Op(op);}
	}
	void Store(uint32_t a,uint32_t value)override {
		assert(!(a&3));
		if(a==0xffe40000){portWrites++;Instruction(value);return;}
		if(a==0xffe000fc){assert(!value);gpr63=0;return;}
		if(Range(a,0xffef0000,224*4)){unsigned i=(a-0xffef0000)/4;if(i!=dropConfig)native.Config(i,value);return;}
		assert(Range(a,0x6e000,8192) || Range(a,0x17ffc0,12) || Range(a,0x98000,256));
		if(Range(a,0x6e000,8192))minStore=std::min(minStore,unsigned(a));
		if(Range(a,0x98000,256)){assert(native.Clean() && !awaiting);stores++;}l1[Index(a)]=value;
	}
	uint32_t Load(uint32_t a)override {
		assert(!(a&3));if(a==0xffe000fc)return gpr63;
		if(Range(a,0xffef0000,224*4)){assert(!native.Pending() && !native.mathPending);return native.config[(a-0xffef0000)/4];}
		if(Range(a,0xffbd8800,64) || Range(a,0xffbd8a00,64)) {
			if(!native.Clean() || awaiting)throw Q4FpuPipelineMachine::Fault{};
			assert(!native.config[3]);unsigned row=(a-0xffbd8000)/64,column=(a%64)/4;assert(row==32 || row==40);
			native.outputReads++;return native.FloatDst(row,column);
		}
		assert(CodeAddress(a) || Range(a,0x6e000,8192) || Range(a,0x17ffc0,12));return l1[Index(a)];
	}
	bool Execute() {
		while(pc!=0x70038 && pc!=0x70058){Step();if(regs[2])stackSeen=true;if(stackSeen){assert(regs[2]>=0x6e000 && regs[2]<=0x70000 && !(regs[2]&15));minSP=std::min(minSP,regs[2]);}}
		assert(l1[Index(pc)]==0x0000006f && minStore>=0x6e000);
		if(pc==0x70058){assert(l1[Index(0x17ffc8)]==0x45504651);return false;}
		assert(l1[Index(0x17ffc0)]==0x7000c && l1[Index(0x17ffc4)]==0x04000100 && l1[Index(0x17ffc8)]==0x31504651);
		assert(native.Clean() && stores==64 && native.outputReads==64 && native.matrixOps==256
			&& native.rawLoads==64 && native.metadataLoads==64 && native.zeroLoads==10 && native.inputLoads==16);
		for(unsigned phase=0;phase<4;phase++)assert(native.visits[phase]==64);return true;
	}
	bool Matches(const std::array<uint32_t,32>& expected) {
		for(unsigned pass=0;pass<2;pass++)for(unsigned row=0;row<32;row++)if(l1[Index(0x98000+pass*128+row*4)]!=expected[row])return false;
		return true;
	}
};
#ifndef PIONEER_Q4_FPU_BLOCK_IMAGE_ONLY
int main(int argc,char** argv)
{
	assert(argc==2);unsigned cases=0,maxStack=0,maxTouch=0,acks=0;
	for(unsigned trial=0;trial<4;trial++) {
		Q4PipelineFixture data(trial);Q4FpuBlockRV32 image(argv[1]);image.native.Stage(data.prepared,data.input);image.native.delay[0]=100003;image.native.delay[1]=7;
		assert(image.Execute() && image.Matches(data.expected));cases++;
		maxStack=std::max(maxStack,0x70000-image.minSP);maxTouch=std::max(maxTouch,0x70000-image.minStore);acks=image.acks;
		assert(image.portWrites<1024 && image.directIssues>6000);
		if(!trial)for(unsigned pass=0;pass<2;pass++)for(unsigned row=0;row<32;row++)printf("Q4_FPU_PIPELINE_REFERENCE trial0 pass%u row%u word%08x\n",pass,row,image.l1[image.Index(0x98000+pass*128+row*4)]);
	}
	Q4PipelineFixture data(0);unsigned refused=0;
	for(unsigned fault:{1U,2U,3U}) {
		Q4FpuBlockRV32 bad(argv[1]);bad.native.Stage(data.prepared,data.input);
		if(fault==1)bad.dropConfig=72;if(fault==2)bad.dropAckAt=1;if(fault==3)bad.dropAckAt=acks/2;
		assert(!bad.Execute() && !bad.stores && !bad.native.outputReads);refused++;
	}
	for(unsigned missing:{1U,2U}) {
		Q4FpuBlockRV32 bad(argv[1]);bad.native.Stage(data.prepared,data.input);bad.native.dropProducer=missing;bool unknown=false;
		try{bad.Execute();}catch(uint32_t marker){unknown=marker==NativeBfpMachine::Unknown;}
		assert(unknown && !bad.stores && !bad.native.outputReads);refused++;
	}
	for(unsigned fault:{1U,2U}) {
		Q4FpuBlockRV32 bad(argv[1]);bad.native.Stage(data.prepared,data.input);if(fault==1)bad.native.skipZero=true;else bad.native.skipMove=1;bool unknown=false;
		try{bad.Execute();}catch(Q4FpuPipelineMachine::Fault){unknown=true;}
		assert(unknown && !bad.stores && !bad.native.outputReads);refused++;
	}
	for(unsigned phase=0;phase<4;phase++) {
		Q4FpuBlockRV32 bad(argv[1]);bad.native.Stage(data.prepared,data.input);bad.native.dropPhase=phase;
		assert(bad.Execute() && !bad.Matches(data.expected));refused++;
	}
	for(unsigned fault:{1U,2U}) {
		Q4FpuBlockRV32 bad(argv[1]);bad.native.Stage(data.prepared,data.input);if(fault==1)bad.native.skipTranspose=1;else bad.native.l1[0x94003]^=0x80;
		assert(bad.Execute() && !bad.Matches(data.expected));refused++;
	}
	{
		// Transpose reads DEFINED padding but only its original first
		// subvector reaches coef/min registers. Padding values are irrelevant.
		Q4FpuBlockRV32 padded(argv[1]);padded.native.Stage(data.prepared,data.input);padded.native.dirtyPadding=true;
		assert(padded.Execute() && padded.Matches(data.expected));cases++;
	}
	printf("PASS actual RV32I packed-Q4/SFPU/BF16/FPU K256 pipeline:%u two-job trials/all64 words exact,%u failure/numerical controls,strict unchanged Step,shared L1/physical Dst,SFPTRANSP/MOVD2A,all4 phases;SP%u/touched%u;NO scalar payload/NoC/GDDR/native driver/hardware/full-model quality/speed proof\n",cases,refused,maxStack,maxTouch);
}
#endif
