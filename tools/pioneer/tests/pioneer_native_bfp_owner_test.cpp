/* SPDX-License-Identifier: MIT */
#define B_DEVICE_OP_CODES_END 10000
#define PIONEER_NATIVE_BFP_IMAGE_ONLY 1
#include "pioneer_q38_native_bfp_image_test.cpp"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerNativeBfpLease.h"
#include "PioneerNativeBfpGeneratedImage.h"
#include <set>
using Lease=PioneerNativeBfpLease;
using Image=PioneerNativeBfpGeneratedImage;
using FixtureData=PioneerNativeBfpFixture;
static uint32_t Reference(unsigned bits,unsigned row) {
	long double sum=0;
	for(unsigned k=0;k<32;k++) {
		long double w=std::ldexp(static_cast<long double>(FixtureData::Magnitude(bits,row,k)),int(FixtureData::Exponent(row))-127-int(bits-2));
		if(FixtureData::Negative(row,k))w=-w;
		sum+=w*NativeBfpMachine::Float(uint32_t(FixtureData::InputBits(k))<<16);
	}
	return NativeBfpMachine::Bits(float(sum));
}
struct IO {
	std::vector<uint32_t> memory=std::vector<uint32_t>(0x180000/4),original;
	uint32_t source[16]={0x6005,0x0a418820,0x1ac4a166,0x127541ee,0x1e280440,0x1662b5c3,0x147,
		0x0a418820,0x16a4a0e6,0x06a12c20,0x0c72a089,0x0e548c20,2,0,3,0};
	uint32_t tlb[2][3]={{1,2,3},{4,5,6}},reset=0x47800;
	unsigned bits,reads=0,writes=0,resets=0,tlbWrites=0,pauses=0,dropL1=0,dropReset=0,dropTlb=0;
	bool timeout=false,badPC=false,faultMarker=false,sourceChanged=false,failRestore=false,executed=false,started=false;
	std::string directory;uint64_t now=1000000;
	IO(unsigned b,const char* p):bits(b),directory(p) {
		for(unsigned i=0;i<memory.size();i++)memory[i]=0x9e370000U^(i*71);original=memory;
	}
	uint32_t SourceRead(unsigned i) { reads++;assert(i<16);return source[i]^((sourceChanged && started)?1U:0U); }
	uint32_t DramTlbRead(unsigned s,unsigned w) { reads++;assert(s<2 && w<3);return tlb[s][w]; }
	void DramTlbWrite(unsigned s,unsigned w,uint32_t v) { assert(s<2 && w<3);if(++tlbWrites!=dropTlb)tlb[s][w]=v; }
	bool Match(unsigned s,unsigned x,unsigned y,bool ro) {
		for(unsigned w=0;w<3;w++)if(tlb[s][w]!=BlackholeDramFixedTlbWord(s?0xffa00000:0,x,y,ro,w))return false;
		return true;
	}
	uint32_t ResetRead() {
		reads++;
		if(Match(1,1,2,false) || Match(1,1,2,true))return reset;
		const unsigned xs[]={1,2,3,4,5,6,7,10,11,12,13,14};
		for(unsigned x:xs)for(unsigned y=2;y<=11;y++)if(Match(1,x,y,true))return 0x47800;
		assert(false && "reset read through wrong route");return 0;
	}
	void Window() { assert(Match(0,1,2,false) && Match(1,1,2,false)); }
	uint32_t BfpRead(uint32_t a) { Window();reads++;assert(a<memory.size()*4 && !(a&3));return memory[a/4]; }
	void BfpWrite(uint32_t a,uint32_t v) {
		Window();assert(reset==0x47800 && (!started || executed));
		assert(a<memory.size()*4 && !(a&3));
		if(++writes!=dropL1 && !(failRestore && started && v==original[a/4]))memory[a/4]=v;
	}
	uint32_t BfpControlRead(unsigned i) { Window();reads++;assert(i<3);return 0; }
	uint32_t BfpPcRead() { Window();reads++;return badPC?0x70040:pauses&1?0x70038:0x7003c; }
	void Execute() {
		NativeBfpRV32 image(directory.c_str());
		image.l1[0]=memory[0x6000/4];
		for(unsigned i=0;i<image.code.size()/4;i++)image.l1[1+i]=memory[(0x70000+i*4)/4];
		for(unsigned i=0;i<8192/4;i++)image.l1[image.Index(0x6e000+i*4)]=memory[(0x6e000+i*4)/4];
		std::array<uint8_t,1088> weights{};std::array<uint16_t,32> input{};
		for(unsigned i=0;i<FixtureData::WeightBytes(bits);i++)weights[i]=uint8_t(memory[(0x90000+i)/4]>>((i%4)*8));
		for(unsigned i=0;i<32;i++)input[i]=FixtureData::InputBits(i);
		for(unsigned i=0;i<128;i++)assert(memory[(0x91000+i*4)/4]==FixtureData::InputWord(i));
		image.native.Stage(weights.data(),FixtureData::WeightBytes(bits),input);
		assert(image.Execute());
		for(unsigned i=0;i<64;i++)memory[(0x92000+i*4)/4]=image.l1[image.Index(0x92000+i*4)];
		for(unsigned i=0;i<3;i++)memory[(0x17ffc0+i*4)/4]=image.l1[image.Index(0x17ffc0+i*4)];
		for(unsigned i=0;i<8192/4;i++)memory[(0x6e000+i*4)/4]=image.l1[image.Index(0x6e000+i*4)];
		executed=true;
	}
	void BfpResetWrite(uint32_t v) {
		Window();assert(v==0x87ffff || v==0x46800 || v==0x47800);
		if(++resets==dropReset)return;
		if(v==0x46800) { assert(reset==0x87ffff && !started);started=true;reset=v;if(!timeout)Execute();if(faultMarker)memory[0x17ffc8/4]=0x4642464e; }
		else { if(started)assert(executed && !timeout && !badPC && !sourceChanged && !faultMarker);reset=v; }
	}
	uint64_t BfpNow() { return now; }
	void Pause() { pauses++;now+=100000; }
	bool BfpHealthy() const { return true; }
	tt_dram_samples_result Baseline() {
		tt_dram_samples_result b={};b.version=1;b.state=TT_DRAM_SAMPLE_ROUTES_OK;b.tlb_restored=b.data_restored=1;b.routes_verified=7;
		memcpy(b.source,source,sizeof(source));assert(BlackholeDramBuildRoutes(b));return b;
	}
};
static void NoMoreIO(Lease& lease,IO& io,tt_dram_samples_result& b) {
	const auto reads=io.reads,writes=io.writes,resets=io.resets,tlb=io.tlbWrites;
	assert((!lease.End(io,b) && !lease.Run<IO,Image>(io,b,io.bits)));lease.Abandon();
	assert(io.reads==reads && io.writes==writes && io.resets==resets && io.tlbWrites==tlb);
}
int main(int argc,char** argv) {
	assert(argc==3);checked=0;
	std::set<uint32_t> addresses;
	for(unsigned i=0;i<Lease::SavedWords;i++)assert(addresses.insert(Lease::Address(i)).second);
	assert(addresses.size()==3548 && Lease::Address(3548)==UINT32_MAX);checked++;
	for(unsigned bits:{4U,8U}) {
		IO io(bits,argv[bits==4?1:2]);auto b=io.Baseline();Lease lease;
		assert((lease.Run<IO,Image>(io,b,bits)));const auto result=lease.Result();
		assert(result.state==PIONEER_NATIVE_BFP_RESULT && result.started && result.completed && !result.cold && io.resets==2);
		for(unsigned pass=0;pass<2;pass++)for(unsigned row=0;row<32;row++)assert(result.output[pass*32+row]==Reference(bits,row));
		assert(lease.End(io,b) && lease.Result().state==PIONEER_NATIVE_BFP_CLOSED && io.memory==io.original && io.resets==4);
		for(unsigned s=0;s<2;s++)for(unsigned w=0;w<3;w++)assert(io.tlb[s][w]==s*3+w+1);
		NoMoreIO(lease,io,b);checked++;
	}
	for(unsigned fault=0;fault<12;fault++) {
		IO io(4,argv[1]);auto b=io.Baseline();Lease lease;
		if(fault==0)io.timeout=true;
		if(fault==1)io.badPC=true;
		if(fault==2)io.faultMarker=true;
		if(fault==3)io.sourceChanged=true;
		if(fault==4)io.dropL1=1024;
		if(fault==5)io.dropReset=1;
		if(fault==6)io.dropReset=2;
		if(fault==7)io.dropTlb=2; // actual first-target coordinate change, NOT an idempotent same-base write
		bool ran=lease.Run<IO,Image>(io,b,4);
		if(fault>=8) {
			assert(ran);
			if(fault==8)io.dropReset=3;
			if(fault==9)io.dropReset=4;
			if(fault==10)io.failRestore=true;
			if(fault==11)io.dropTlb=io.tlbWrites+370; // after END's120-core scan+target selection
			assert(!lease.End(io,b));
		} else { if(ran)fprintf(stderr,"Fault %u was not injected effectively\n",fault);assert(!ran); }
		assert(lease.Result().state==PIONEER_NATIVE_BFP_UNKNOWN && lease.Result().cold);
		if(fault<8)assert(io.resets<3); // absolutely NO watchdog/PC/fault cleanup reset
		NoMoreIO(lease,io,b);checked++;
	}
	{
		IO io(8,argv[2]);auto b=io.Baseline();Lease lease;assert((lease.Run<IO,Image>(io,b,8)));
		const auto reads=io.reads,writes=io.writes,resets=io.resets,tlb=io.tlbWrites;lease.Abandon();
		assert(lease.Result().cold && io.reads==reads && io.writes==writes && io.resets==resets && io.tlbWrites==tlb);NoMoreIO(lease,io,b);checked++;
	}
	printf("PASS OFFBOARD actual native-BFP lease:%u cases;fresh real RV32 images/fixed dyadic exact64 results/3548-word L1+TLB restore;timeout,PC,fault,source,stage,start,STOP,restore,close failures retain with NO retry/reset/cleanup;NO physical/Work/model-speed proof\n",checked);
}
