/* SPDX-License-Identifier: MIT */
#define PIONEER_TENSIX_MACHINE_ONLY
#include "pioneer_tensix_image_test.cpp"
#include "pioneer_q38_native_bfp_band_machine.h"
#include <string>
struct BandRV32:Machine {
	NativeBfpBandMachine native;std::vector<uint8_t> code;bool direct;
	unsigned minSP=0x70000,minStore=0x70000,stores=0,acks=0,dropAckAt=0,configWrites=0,portWrites=0,directIssues=0;
	unsigned dropConfig=~0U;bool awaiting=false,stackSeen=false;
	BandRV32(const char* folder,unsigned width,unsigned phases,bool inlineMode,unsigned bands=2)
		:native(width,phases,0x90000,0x124000,1104,80,16,0x130000,bands),direct(inlineMode) {
		const std::string root=folder;code=Read((root+"/text.bin").c_str());const auto boot=Read((root+"/boot.bin").c_str());
		assert(boot.size()==4 && !code.empty() && code.size()<=4096 && !(code.size()&3));
		std::fill(std::begin(l1),std::end(l1),0xdeadbeef);l1[0]=Word(boot,0);
		for(unsigned i=0;i<code.size()/4;i++)l1[1+i]=Word(code,i*4);
		for(unsigned i=0;i<3;i++)l1[Index(0x17ffc0+i*4)]=0;
	}
	static bool Range(uint32_t a,uint32_t first,unsigned size){return a>=first && a-first<size;}
	unsigned Index(uint32_t a) override {
		assert(!(a&3));if(a==0x6000)return 0;
		if(Range(a,0x70000,unsigned(code.size())))return 1+(a-0x70000)/4;
		if(Range(a,0x6e000,8192))return 1025+(a-0x6e000)/4;
		if(Range(a,0x17ffc0,12))return 3073+(a-0x17ffc0)/4;
		assert(Range(a,0x130000,256));return 3076+(a-0x130000)/4;
	}
	bool CodeAddress(uint32_t a) const override{return a==0x6000 || Range(a,0x70000,unsigned(code.size()));}
	bool InlineTensix() const override{return direct;}
	unsigned StepLimit() const override{return 10000000;}
	void Instruction(uint32_t op) override {
		if(op==0xa2101816){assert(!gpr63 && !awaiting);awaiting=true;}
		else if(op==0x4500017e){assert(awaiting && !gpr63);acks++;if(acks!=dropAckAt){native.Drain();gpr63=1;awaiting=false;}}
		else native.Op(op);
	}
	void Store(uint32_t a,uint32_t value) override {
		assert(!(a&3));
		if(a==0xffe40000){portWrites++;Instruction(value);return;}
		if(a==0xffe000fc){assert(!value);gpr63=0;return;}
		if(Range(a,0xffef0000,224*4)){configWrites++;const unsigned w=(a-0xffef0000)/4;if(w!=dropConfig)native.Config(w,value);return;}
		assert(Range(a,0x6e000,8192) || Range(a,0x17ffc0,12) || Range(a,0x130000,256));
		if(Range(a,0x6e000,8192))minStore=std::min(minStore,unsigned(a));
		if(Range(a,0x130000,256)){assert(native.Clean());stores++;}l1[Index(a)]=value;
	}
	uint32_t Load(uint32_t a) override {
		assert(!(a&3));if(a==0xffe000fc)return gpr63;
		if(Range(a,0xffef0000,224*4)){assert(!native.Pending() && !native.mathPending);return native.config[(a-0xffef0000)/4];}
		if(Range(a,0xffbd8800,64) || Range(a,0xffbd8a00,64)) {
			if(!native.Clean())throw NativeBfpMachine::Unknown;
			assert(!native.config[3]);const unsigned row=(a-0xffbd8000)/64,column=(a%64)/4;
			assert(native.valid[row] && (row==32 || row==40));native.outputReads++;return Bits(native.destination[row][column]);
		}
		// No scalar weight/input/NoC/DRAM/reference reads can be accepted.
		assert(CodeAddress(a) || Range(a,0x6e000,8192) || Range(a,0x17ffc0,12));return l1[Index(a)];
	}
	bool Execute() {
		while(pc!=0x70038 && pc!=0x70058){Step();if(regs[2])stackSeen=true;if(stackSeen){assert(regs[2]>=0x6e000 && regs[2]<=0x70000 && !(regs[2]&15));minSP=std::min(minSP,regs[2]);}}
		assert(l1[Index(pc)]==0x0000006f && minStore>=0x6e000);
		if(pc==0x70058){assert(l1[Index(0x17ffc8)]==0x4542464e && !stores && !native.outputReads);return false;}
		assert(l1[Index(0x17ffc0)]==0x7000c && l1[Index(0x17ffc4)]==((native.phases<<24)|(native.tiles*32)) && l1[Index(0x17ffc8)]==0x3342464e);
		assert(native.Clean() && stores==64 && native.outputReads==64 && native.clears==native.maxBands);
		assert(native.unpacks[0]==native.maxBands*native.tiles && native.unpacks[1]==native.unpacks[0]
			&& native.matrixOps==native.maxBands*native.tiles*4*native.phases);
		for(unsigned p=0;p<4;p++)assert(native.visits[p]==(native.phases==4 || !p?4*native.maxBands*native.tiles:0));return true;
	}
	void Exact(const std::array<float,32>& expected) {
		for(unsigned pass=0;pass<2;pass++)for(unsigned row=0;row<32;row++)assert(l1[Index(0x130000+pass*128+row*4)]==Bits(expected[row]));
	}
	bool Matches(const std::array<float,32>& expected) {
		for(unsigned pass=0;pass<2;pass++)for(unsigned row=0;row<32;row++)if(l1[Index(0x130000+pass*128+row*4)]!=Bits(expected[row]))return false;
		return true;
	}
};
#ifndef PIONEER_NATIVE_BFP_BAND_IMAGE_ONLY
int main(int argc,char** argv) {
	assert(argc==5);const unsigned width=unsigned(std::stoul(argv[2])),phases=unsigned(std::stoul(argv[3]));
	assert(!strcmp(argv[4],"mmio") || !strcmp(argv[4],"direct"));const bool direct=!strcmp(argv[4],"direct");
	const auto data=NativeBfpBandMachine::Data(width);
	bool different=false;for(unsigned i=0;i<32;i++)different|=Bits(data.full[i])!=Bits(data.fast[i]);assert(different);
	BandRV32 image(argv[1],width,phases,direct);image.native.Stage(data);image.native.delay[0]=100003;image.native.delay[1]=7;
	assert(image.Execute());image.Exact(phases==4?data.full:data.fast);
	if(direct)assert(image.portWrites<256);else assert(image.portWrites>width/32*8);
	printf("PASS OFFBOARD actual full-K BFP8/broadcast RV32I:K%u phases%u %s,two complete launches,exact32 outputs each vs independent IDEAL phasewise reference,one Dst clear/launch,two source-bank streams%u tiles,only valid broadcast rows32/40,private8KiB SP%u/touched%u,RISCsteps%u portWrites%u;NO scalar weight/activation access,NoC/GDDR/owner/driver/silicon rounding/model quality/speed proof\n",width,phases,direct?"direct":"mmio",width/32,0x70000-image.minSP,0x70000-image.minStore,image.steps,image.portWrites);
	if(width==5120 && phases==4 && !direct) {
		unsigned failures=0;
		for(unsigned fault=0;fault<3;fault++) {
			BandRV32 bad(argv[1],width,phases,false);bad.native.Stage(data);
			if(fault==0)bad.dropConfig=72;
			if(fault==1)bad.dropAckAt=1;
			if(fault==2)bad.dropAckAt=4+2*bad.native.tiles;
			assert(!bad.Execute() && !bad.stores && !bad.native.outputReads);
			if(fault==2)assert(bad.native.matrixOps==4*phases*bad.native.tiles && bad.native.mathPending);
			else assert(!bad.native.matrixOps);
			failures++;
		}
		for(unsigned missing:{1U,2U}) {
			BandRV32 bad(argv[1],width,phases,false);bad.native.Stage(data);bad.native.dropProducer=missing;bool refused=false;
			try{bad.Execute();}catch(uint32_t marker){refused=marker==NativeBfpMachine::Unknown;}
			assert(refused && !bad.stores && !bad.native.outputReads && !bad.native.matrixOps);failures++;
		}
		{
			BandRV32 bad(argv[1],width,phases,false);bad.native.Stage(data);bad.native.omitFlip=true;bool refused=false;
			try{bad.Execute();}catch(uint32_t marker){refused=marker==NativeBfpMachine::Unknown;}
			assert(refused && !bad.stores && !bad.native.outputReads);failures++;
		}
		for(unsigned phase:{1U,2U,3U}) {
			BandRV32 bad(argv[1],width,phases,false);bad.native.Stage(data);bad.native.dropPhase=phase;
			assert(bad.Execute() && !bad.Matches(data.full));failures++;
		}
		struct NoIO{unsigned events=0;void Op(unsigned){events++;}void Config(unsigned,unsigned){events++;}void Drain(){events++;}} none;
		for(unsigned invalid:{0U,32U,5119U,5121U,17409U,~0U}){assert(!PioneerQ38NativeBfpBand::Run<4>(none,invalid) && !none.events);failures++;}
		printf("PASS actual full-K failure controls:%u cases;config/initial/final completion fault park before result,missing producers/unreleased banks refuse before output,all3 omitted fidelity terms differ from full reference,invalid widths NO command/config IO;NO recovery/reset/physical proof\n",failures);
	}
}
#endif
