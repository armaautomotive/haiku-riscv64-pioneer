/* SPDX-License-Identifier: MIT */
#define PIONEER_TENSIX_MACHINE_ONLY
#include "pioneer_tensix_image_test.cpp"
#define PIONEER_NATIVE_DRAM_LOADER_TEST_ONLY
#include "pioneer_native_dram_band_loader_test.cpp"
// No arithmetic observer,descriptor normalization or FPU model in this VM.
// NIU bytes are EXPLICIT SIMULATION,not physical transfer qualification.
struct DramLoadRV32:Machine {
	IO transfer;std::vector<uint8_t> code;
	unsigned minSP=0x70000,minStore=0x70000;bool stackSeen=false,changeFrame=false;
	explicit DramLoadRV32(const char* folder,unsigned route=17U|(12U<<6)):transfer(5120,route) {
		const std::string root=folder;code=Read((root+"/text.bin").c_str());const auto boot=Read((root+"/boot.bin").c_str());
		assert(boot.size()==4 && !code.empty() && code.size()<=8192 && !(code.size()&3));
		std::fill(std::begin(l1),std::end(l1),0x7fc00001);l1[0]=Word(boot,0);
		for(unsigned i=0;i<code.size()/4;i++)l1[1+i]=Word(code,i*4);
		for(unsigned i=0;i<3;i++)l1[Index(0x17ffc0+i*4)]=0;
		const unsigned frame[8]={0x314c444e,1,5120,transfer.route,1,0,0,0};
		for(unsigned i=0;i<8;i++)l1[Index(0x74000+i*4)]=frame[i];
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
	unsigned StepLimit()const override{return 10000000+PioneerDramQueuedReadSweep::PollLimit*2048;}
	bool Loaded()const{return transfer.completions==320 && transfer.pending.empty();}
	void Instruction(uint32_t)override{assert(false && "LOAD ONLY:ALL FPU instructions forbidden");}
	void Store(uint32_t a,uint32_t value)override {
		assert(!(a&3));
		if(transfer.Header(a) || Range(a,0xffb20000,0x2000)){transfer.Write(a,value);return;}
		// NO config/TTI/Dst/output/payload writes or scalar copies.
		assert(Range(a,0x6e000,8192) || Range(a,0x17ffc0,12));
		if(Range(a,0x6e000,8192))minStore=std::min(minStore,unsigned(a));
		l1[Index(a)]=value;
	}
	uint32_t Load(uint32_t a)override {
		assert(!(a&3));
		if(transfer.Header(a) || Range(a,0xffb20000,0x2000)) {
			const unsigned result=transfer.Read(a);if(changeFrame && Loaded())l1[Index(0x74010)]=2;return result;
		}
		// NO config,TTI,Dst,weights,inputs,GDDR or reference reads.
		assert(CodeAddress(a) || Range(a,0x6e000,8192) || Range(a,0x74000,32) || Range(a,0x17ffc0,12));return l1[Index(a)];
	}
	bool Execute() {
		while(pc!=0x70038 && pc!=0x70058) {
			Step();if(regs[2])stackSeen=true;
			if(stackSeen){assert(regs[2]>=0x6e000 && regs[2]<=0x70000 && !(regs[2]&15));minSP=std::min(minSP,regs[2]);}
		}
		assert(l1[Index(pc)]==0x0000006f && minStore>=0x6e000);
		if(pc==0x70058){assert(l1[Index(0x17ffc8)]==0x454c444e);return false;}
		assert(Loaded() && l1[Index(0x17ffc0)]==0x7000c && l1[Index(0x17ffc4)]==0x80001400 && l1[Index(0x17ffc8)]==0x314c444e);
		for(unsigned i=0;i<64;i++)assert(l1[Index(L::Output+i*4)]==0x7fc00001);
		transfer.Exact();return true;
	}
};
#ifndef PIONEER_NATIVE_DRAM_LOAD_IMAGE_ONLY
int main(int argc,char** argv) {
	assert(argc==2);unsigned valid=0,refused=0;
	for(unsigned x:{17U,18U})for(unsigned y:{12U,15U,18U,21U}) {
		DramLoadRV32 vm(argv[1],x|(y<<6));assert(vm.Execute());valid++;
		if(valid==1)printf("PASS actual RV32 LOAD ONLY:320 requests/2880 flits/NDL1 sentinel-only output;stack%u/touched%u/steps%u;NO FPU/config/Dst/scalar payload/model/speed authority\n",0x70000-vm.minSP,0x70000-vm.minStore,vm.steps);
	}
	for(unsigned word=0;word<8;word++) {
		DramLoadRV32 bad(argv[1]);bad.l1[bad.Index(0x74000+word*4)]^=word==4?1:0x80000000;
		assert(!bad.Execute() && !bad.transfer.issues);refused++;
	}
	for(unsigned fault=1;fault<=12;fault++) {
		DramLoadRV32 bad(argv[1]);bad.transfer.fault=fault;assert(!bad.Execute() && bad.transfer.issues<=16);refused++;
	}
	{
		DramLoadRV32 bad(argv[1]);bad.changeFrame=true;assert(!bad.Execute() && bad.Loaded());refused++;
	}
	printf("PASS actual RV32 LOAD ONLY:%u route-shape valid/%u frame/NIU/FIFO/counter/header/job faults;whole simulated L1 exact;NO FPU/normalization/host/device ownership or physical byte proof\n",valid,refused);
}
#endif
