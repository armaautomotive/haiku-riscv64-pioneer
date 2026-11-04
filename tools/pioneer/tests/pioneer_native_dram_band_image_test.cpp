/* SPDX-License-Identifier: MIT */
#define PIONEER_TENSIX_MACHINE_ONLY
#include "pioneer_tensix_image_test.cpp"
#define PIONEER_NATIVE_DRAM_LOADER_TEST_ONLY
#include "pioneer_native_dram_band_loader_test.cpp"
#include "pioneer_q38_native_bfp_band_machine.h"
#include "pioneer_blackhole_arithmetic_observer.h"
#include "../PioneerNativeBfpBandGolden.h"
// Execute the ACTUAL compiled image with the UNCHANGED strict RV32 decoder.
// Only observer address normalization reuses the old independent unpack/math
// model:EVERY incoming descriptor is first checked against the NEW layout,
// then the exact bytes loaded by simulated NIU are copied into the observer.
// The image itself is forbidden scalar weight/input reads or writes. Existing
// rounded arithmetic+independent Fraction-qualified full-K predictions apply.
struct DramBandRV32:Machine {
	IO transfer;NativeBfpBandMachine native;std::vector<uint8_t> code;
	unsigned minSP=0x70000,minStore=0x70000,stores=0,acks=0,dropAckAt=0,weights=0,inputs=0;
	bool active=false,awaiting=false,stackSeen=false,changeFrame=false;
	DramBandRV32(const char* folder,unsigned width):transfer(width),native(width,4) {
		const std::string root=folder;code=Read((root+"/text.bin").c_str());const auto boot=Read((root+"/boot.bin").c_str());
		assert(boot.size()==4 && !code.empty() && code.size()<=8192 && !(code.size()&3));
		std::fill(std::begin(l1),std::end(l1),0xdeadbeef);l1[0]=Word(boot,0);
		for(unsigned i=0;i<code.size()/4;i++)l1[1+i]=Word(code,i*4);
		for(unsigned i=0;i<3;i++)l1[Index(0x17ffc0+i*4)]=0;
		const unsigned frame[8]={0x3142444e,1,width,transfer.route,1,0,0,0};
		for(unsigned i=0;i<8;i++)l1[Index(0x74000+i*4)]=frame[i];
		const auto data=NativeBfpBandMachine::Data(width);
		for(unsigned tile=0;tile<native.tiles;tile++) {
			memcpy(transfer.dram.data()+tile*1088,data.packed[tile].data(),1088);
			for(unsigned c=0;c<32;c++) {
				const unsigned at=L::DramInput-L::DramWeight+tile*64+c*2;
				transfer.dram[at]=uint8_t(data.input[tile][c]);transfer.dram[at+1]=uint8_t(data.input[tile][c]>>8);
			}
		}
	}
	static bool Range(uint32_t a,uint32_t first,unsigned bytes){return a>=first && a-first<bytes;}
	unsigned Index(uint32_t a)override {
		assert(!(a&3));if(a==0x6000)return 0;
		if(Range(a,0x70000,unsigned(code.size())))return 1+(a-0x70000)/4;
		if(Range(a,0x6e000,8192))return 2049+(a-0x6e000)/4;
		if(Range(a,0x74000,32))return 4097+(a-0x74000)/4;
		if(Range(a,0x17ffc0,12))return 4105+(a-0x17ffc0)/4;
		assert(Range(a,L::Output,256));return 4108+(a-L::Output)/4;
	}
	bool CodeAddress(uint32_t a)const override{return a==0x6000 || Range(a,0x70000,unsigned(code.size()));}
	// A real missing-response timeout polls9 master+32 outstanding counters
	// per iteration. Permit its full65536 bounded iterations in this observer;
	// do NOT shorten/alter the device timeout or the unchanged strict decoder.
	unsigned StepLimit()const override{return 10000000+PioneerDramQueuedReadSweep::PollLimit*2048;}
	bool Loaded()const{return transfer.completions==2*native.tiles && transfer.pending.empty();}
	void Descriptor(unsigned index,unsigned value) {
		assert(Loaded());
		if(index==112 && value==0x00200015)active=true;
		if(index==76) {
			const unsigned tile=active?weights++:0;
			assert(tile<native.tiles && value==L::Weight/16-1+tile*(L::WeightStride/16));
			value=PioneerQ38NativeBfpBand::Weight/16-1+tile*(PioneerQ38NativeBfpBand::WeightStride/16);
		}
		if(index==124) {
			const unsigned tile=active?inputs++:0;
			assert(tile<native.tiles && value==L::Input/16-1+tile*(L::InputStride/16));
			value=active?PioneerQ38NativeBfpBand::Input/16-1+tile*(PioneerQ38NativeBfpBand::InputStride/16):PioneerQ38NativeBfpMatvec::InputAddress/16-1;
		}
		native.Config(index,value);
	}
	void Operand(unsigned which) {
		assert(Loaded());const unsigned tile=native.unpacks[which]%native.tiles;
		const unsigned actual=(which?L::Input:L::Weight)+tile*(which?L::InputStride:L::WeightStride);
		const unsigned observed=(which?0x124000:0x90000)+tile*(which?80:1104),bytes=which?64:1088;
		for(unsigned i=0;i<32;i++)assert(!transfer.l1[actual-32+i]);
		memset(native.l1.data()+observed-16,0,16);memcpy(native.l1.data()+observed,transfer.l1.data()+actual,bytes);
	}
	void Instruction(uint32_t op)override {
		assert(Loaded());
		if(op==0xb2000000){active=false;weights=inputs=0;}
		if(op==0xa2101816){assert(!gpr63 && !awaiting);awaiting=true;}
		else if(op==0x4500017e){assert(awaiting && !gpr63);acks++;if(acks!=dropAckAt){native.Drain();gpr63=1;awaiting=false;}}
		else {
			if(op==0x420000c1 || op==0x428000c1)Operand((op>>23)&1);
			if((op>>24)==0x26)PioneerBlackholeArithmeticObserver::Matrix(native,op,true,[&]{native.Op(op);});
			else native.Op(op);
		}
	}
	void Store(uint32_t a,uint32_t value)override {
		assert(!(a&3));
		if(a==0xffe40000){Instruction(value);return;}
		if(a==0xffe000fc){assert(!value && Loaded());gpr63=0;return;}
		if(Range(a,0xffef0000,224*4)){Descriptor((a-0xffef0000)/4,value);return;}
		if(transfer.Header(a) || Range(a,0xffb20000,0x2000)){transfer.Write(a,value);return;}
		assert(Range(a,0x6e000,8192) || Range(a,0x17ffc0,12) || Range(a,L::Output,256));
		if(Range(a,0x6e000,8192))minStore=std::min(minStore,unsigned(a));
		if(Range(a,L::Output,256)){assert(native.Clean() && Loaded());stores++;}
		l1[Index(a)]=value;
	}
	uint32_t Load(uint32_t a)override {
		assert(!(a&3));if(a==0xffe000fc)return gpr63;
		if(Range(a,0xffef0000,224*4)) {
			assert(Loaded() && !native.Pending() && !native.mathPending);const unsigned index=(a-0xffef0000)/4;
			// Return the REAL written descriptor,not its observer normalization.
			if(index==76)return L::Weight/16-1+(active?weights-1:0)*(L::WeightStride/16);
			if(index==124)return L::Input/16-1+(active?inputs-1:0)*(L::InputStride/16);
			return native.config[index];
		}
		if(Range(a,0xffbd8800,64) || Range(a,0xffbd8a00,64)) {
			assert(native.Clean() && Loaded());const unsigned row=(a-0xffbd8000)/64,column=(a%64)/4;
			assert(native.valid[row] && (row==32 || row==40));native.outputReads++;return Bits(native.destination[row][column]);
		}
		if(transfer.Header(a) || Range(a,0xffb20000,0x2000)) {
			const unsigned result=transfer.Read(a);if(changeFrame && Loaded())l1[Index(0x74010)]=2;return result;
		}
		// NO scalar weights,inputs,GDDR,host or reference access permitted.
		assert(CodeAddress(a) || Range(a,0x6e000,8192) || Range(a,0x74000,32) || Range(a,0x17ffc0,12));return l1[Index(a)];
	}
	bool Execute() {
		while(pc!=0x70038 && pc!=0x70058){Step();if(regs[2])stackSeen=true;if(stackSeen){assert(regs[2]>=0x6e000 && regs[2]<=0x70000 && !(regs[2]&15));minSP=std::min(minSP,regs[2]);}}
		assert(l1[Index(pc)]==0x0000006f && minStore>=0x6e000);
		if(pc==0x70058){assert(l1[Index(0x17ffc8)]==0x4542444e && !stores && !native.outputReads);return false;}
		assert(l1[Index(0x17ffc0)]==0x7000c && l1[Index(0x17ffc4)]==(0x04000000|native.tiles*32) && l1[Index(0x17ffc8)]==0x3142444e);
		assert(Loaded() && native.Clean() && stores==64 && native.outputReads==64 && native.clears==2);
		assert(native.unpacks[0]==2*native.tiles && native.unpacks[1]==native.unpacks[0] && native.matrixOps==32*native.tiles);
		for(unsigned p=0;p<4;p++)assert(native.visits[p]==8*native.tiles);
		transfer.Exact();return true;
	}
};
int main(int argc,char** argv) {
	assert(argc==3);const unsigned width=unsigned(std::stoul(argv[2]));
	DramBandRV32 image(argv[1],width);assert(image.Execute());
	const auto* expected=PioneerNativeBfpBandGolden(width);assert(expected);
	for(unsigned pass=0;pass<2;pass++)for(unsigned row=0;row<32;row++)assert(image.l1[image.Index(L::Output+pass*128+row*4)]==expected[row]);
	printf("PASS OFFBOARD ACTUAL RV32 direct-GDDR K%u:one packed load%u requests/%u flits,two full4-phase FPU jobs/all64 exact vs prior independent Fraction-qualified rounded reference,C32 descriptors/wholeL1 guards;stack%u/touched%u,steps%u;NO scalar payload access/host roundtrip/owner/hardware/model quality/speed proof\n",width,image.transfer.issues,18*(width/32),0x70000-image.minSP,0x70000-image.minStore,image.steps);
	if(width==5120) {
		unsigned failures=0;
		for(unsigned word=0;word<8;word++) {
			DramBandRV32 bad(argv[1],width);bad.l1[bad.Index(0x74000+word*4)]^=word==4?1:0x80000000;
			assert(!bad.Execute() && !bad.transfer.issues && !bad.native.clears);failures++;
		}
		for(unsigned fault:{1U,4U,5U,6U,7U,8U,9U,10U,11U,12U}) {
			DramBandRV32 bad(argv[1],width);bad.transfer.fault=fault;assert(!bad.Execute() && !bad.native.clears && bad.transfer.issues<=16);failures++;
			if(fault==8)printf("PASS ACTUAL RV32 missing-response bounded fault park after%u steps;NO math/result/retry;device PollLimit65536 UNCHANGED\n",bad.steps);
		}
		{
			DramBandRV32 bad(argv[1],width);bad.changeFrame=true;assert(!bad.Execute() && bad.Loaded() && !bad.native.clears);failures++;
		}
		{
			DramBandRV32 bad(argv[1],width);bad.dropAckAt=1;assert(!bad.Execute() && bad.Loaded() && !bad.native.clears);failures++;
		}
		printf("PASS actual RV32 DRAM-band%u failure controls:bad immutable frame/NIU/counters/FIFO/header/job mutation/FPU completion fault park before output;NO retry or cleanup\n",failures);
	}
}
