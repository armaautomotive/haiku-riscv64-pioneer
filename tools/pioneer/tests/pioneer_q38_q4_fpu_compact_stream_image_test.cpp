/* SPDX-License-Identifier: MIT */
#define PIONEER_TENSIX_MACHINE_ONLY
#include "pioneer_tensix_image_test.cpp"
#define PIONEER_Q4_COMPACT_STREAM_PIPELINE_ONLY
#include "pioneer_q38_q4_fpu_compact_stream_pipeline_test.cpp"
// UNCHANGED strict RV32 Step. Scalar code/stack/frame/status/output and ALL
// NoC/UNPACK operands share canonical native.l1. Immutable fetch cache mirrors
// ONLY staged ROM;no descriptor normalization or payload copies. Explicit
// observation of REAL FENCE words drives NoC producer-visibility witnesses.
struct CompactStreamRV32:Machine {
	CompactStreamPipeline native;StreamIO bus;std::vector<uint8_t> code;
	unsigned width,minSP=0x70000,minStore=0x70000,stores=0,acks=0,dropAckAt=0,portWrites=0,directIssues=0;
	bool awaiting=false,stackSeen=false;unsigned dropConfig=~0U;
	CompactStreamRV32(const char* folder,unsigned k,bool timing=false,bool pairIssue=false):bus(k,17|(12<<6),&native.l1),width(k) {
		std::string path=folder;code=Read((path+"/text.bin").c_str());auto boot=Read((path+"/boot.bin").c_str());
		assert((!pairIssue || timing) && boot.size()==4 && code.size()==(pairIssue?7820U:timing?7972U:7644U));native.blocks=k/256;native.bus=&bus;native.delay[0]=100003;native.delay[1]=7;
		std::fill(native.l1.begin(),native.l1.end(),0xa7);std::fill(std::begin(l1),std::end(l1),0xdeadbeef);
		WriteL1(0x6000,::Word(boot,0));for(unsigned i=0;i<code.size();i+=4)WriteL1(0x70000+i,::Word(code,i));
		unsigned frame[8]={0x31534351,1,k,bus.route,1,0,0,0};for(unsigned i=0;i<8;i++)WriteL1(SL::Frame+i*4,frame[i]);
		for(unsigned i=0;i<3;i++)WriteL1(0x17ffc0+i*4,0);
	}
	static bool Range(unsigned a,unsigned b,unsigned n){return a>=b && a-b<n;}
	unsigned Index(unsigned a)override {
		assert(!(a&3));if(a==0x6000)return 0;
		if(Range(a,0x70000,8192))return 1+(a-0x70000)/4;
		if(Range(a,SL::Frame,32))return 2049+(a-SL::Frame)/4;
		if(Range(a,0x6e000,8192))return 2057+(a-0x6e000)/4;
		if(Range(a,0x17ffc0,12))return 4105+(a-0x17ffc0)/4;
		assert(Range(a,SL::Output,256));return 4108+(a-SL::Output)/4;
	}
	bool CodeAddress(unsigned a)const override{return a==0x6000 || Range(a,0x70000,unsigned(code.size()));}
	bool InlineTensix()const override{return true;}
	unsigned StepLimit()const override{return 30000000;}
	unsigned ReadL1(unsigned a)const{return ::Word(native.l1,a);}
	void WriteL1(unsigned a,unsigned v) {
		assert(!(a&3) && a+3<native.l1.size());for(unsigned i=0;i<4;i++)native.l1[a+i]=uint8_t(v>>(i*8));
		if(CodeAddress(a) || Range(a,0x6e000,8192) || Range(a,SL::Frame,32) || Range(a,0x17ffc0,12) || Range(a,SL::Output,256))l1[Index(a)]=v;
	}
	void Progress() {
		if(bus.reader<2 && native.matrixOps==native.readerStart+128 && native.Clean())bus.reader=2;
		bus.Progress();
	}
	void Instruction(unsigned op)override {
		if(op==0xa2101816){assert(!gpr63 && !awaiting);awaiting=true;}
		else if(op==0x4500017e){assert(awaiting && !gpr63);acks++;if(acks!=dropAckAt){native.Drain();gpr63=1;awaiting=false;}}
		else{directIssues++;native.Op(op);}Progress();
	}
	void Store(unsigned a,unsigned value)override {
		assert(!(a&3));
		if(a==0xffe40000){portWrites++;Instruction(value);return;}
		if(a==0xffe000fc){assert(!value);gpr63=0;return;}
		if(Range(a,0xffef0000,224*4)){unsigned i=(a-0xffef0000)/4;if(i!=dropConfig)native.Config(i,value);Progress();return;}
		if(SL::HeaderWord(a) || Range(a,0xffb20000,0x2000)){bus.Write(a,value);return;}
		assert(Range(a,0x6e000,8192) || Range(a,0x17ffc0,12) || Range(a,SL::Output,256));
		if(Range(a,0x6e000,8192))minStore=std::min(minStore,a);
		if(Range(a,SL::Output,256)){assert(native.Clean() && !awaiting && bus.queued.empty() && bus.responses.empty());stores++;}
		WriteL1(a,value);Progress();
	}
	unsigned Load(unsigned a)override {
		assert(!(a&3));if(a==0xffe000fc)return gpr63;
		if(Range(a,0xffef0000,224*4)){assert(!native.Pending() && !native.mathPending);return native.config[(a-0xffef0000)/4];}
		if(SL::HeaderWord(a) || Range(a,0xffb20000,0x2000))return bus.Read(a);
		if(Range(a,0xffbd8800,64) || Range(a,0xffbd8a00,64)) {
			if(!native.Clean() || awaiting)throw Q4FpuPipelineMachine::Fault{};
			unsigned row=(a-0xffbd8000)/64,col=(a%64)/4;assert(!native.config[3] && (row==32 || row==40));native.outputReads++;return native.FloatDst(row,col);
		}
		assert(CodeAddress(a) || Range(a,0x6e000,8192) || Range(a,SL::Frame,32) || Range(a,0x17ffc0,12));return ReadL1(a);
	}
	bool Execute() {
		while(pc!=0x70038 && pc!=0x70058) {
			unsigned instruction=l1[Index(pc)];Step();if(instruction==0x0ff0000f)bus.Fence();Progress();
			if(regs[2])stackSeen=true;if(stackSeen){assert(regs[2]>=0x6e000 && regs[2]<=0x70000 && !(regs[2]&15));minSP=std::min(minSP,regs[2]);}
		}
		assert(l1[Index(pc)]==0x0000006f && minStore>=0x6e000);
		if(pc==0x70058){assert(ReadL1(0x17ffc8)==0x45534351);return false;}
		assert(ReadL1(0x17ffc0)==0x7000c && ReadL1(0x17ffc4)==(0x04000000|width) && ReadL1(0x17ffc8)==0x31534351);
		assert(native.Clean() && stores==64 && native.outputReads==64 && native.zeroLoads==10 && native.extraZeroLoads==14 && native.compactImports==4*(width/256) && native.metadataLoads==0 && native.matrixOps==2*(width/256)*128
			&& bus.jobs==2 && bus.issues==2*(width/256)*SL::Packets && bus.completed==bus.issues && bus.queued.empty() && bus.responses.empty() && bus.overlap);
		return true;
	}
	bool Matches(const std::array<uint32_t,32>& expected)const {
		for(unsigned i=0;i<64;i++)if(ReadL1(SL::Output+i*4)!=expected[i%32])return false;return true;
	}
};
#ifndef PIONEER_Q4_COMPACT_STREAM_IMAGE_ONLY
int main(int argc,char** argv) {
	assert(argc==3);unsigned k=unsigned(strtoul(argv[2],nullptr,10));assert(SL::Width(k));static_cast<void>(&NoMore);
	CompactStreamData data(k,0);auto image=std::make_unique<CompactStreamRV32>(argv[1],k);data.Stage(image->bus);
	assert(image->Execute() && image->Matches(data.expected));
	for(unsigned pass=0;pass<2;pass++)for(unsigned row=0;row<32;row++)printf("Q4_COMPACT_STREAM_REFERENCE width%u pass%u row%u word%08x\n",k,pass,row,image->ReadL1(SL::Output+pass*128+row*4));
	printf("PASS actual RV32 compact Q4 fullK%u stream:two jobs/all64 exact,unchanged Step/canonical L1/NoC replies/FENCE visibility,two buffers/overlap/no host K-loop,ONE clear/job,SP%u/touched%u;NO scalar payload/GDDR writes/native owner/hardware/model-quality/speed proof\n",k,0x70000-image->minSP,0x70000-image->minStore);
}
#endif

