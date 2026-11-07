/* SPDX-License-Identifier: MIT */
#define B_DEVICE_OP_CODES_END 10000
#define PIONEER_BFP4_DRAM_IMAGE_ONLY
#include "pioneer_bfp4_dram_band_image_test.cpp"
#include "../PioneerNativeBfp4DramModelLease.h"
#include "PioneerNativeBfp4DramModelMetadata.h" // PRIVATE rounded predictions
#include <memory>
#include <type_traits>
using Lease=PioneerNativeBfp4DramModelLease;
using B4L=PioneerNativeBfp4DramBandLayout;
struct Image {
	static constexpr unsigned Width=5120,Phases=2,LayoutRole=PioneerNativeBfp4DramVariant::Role;
	static inline unsigned Boot=0;static inline std::vector<uint8_t> code;
	static unsigned Word(unsigned i){return i<code.size()/4?::Word(code,i*4):0;}
};
struct LeaseIO {
	Lease& lease;const tt_dram_samples_result* proof=nullptr;
	const uint32_t* recordedL1=nullptr;const uint32_t* recordedDram=nullptr;
	std::vector<uint32_t> memory=std::vector<uint32_t>(0x180000/4),gddr=std::vector<uint32_t>(0x200000/4),original,originalDram;
	uint32_t source[16]={0x6005,0x0a418820,0x1ac4a166,0x127541ee,0x1e280440,0x1662b5c3,0x147,
		0x0a418820,0x16a4a0e6,0x06a12c20,0x0c72a089,0x0e548c20,2,0,3,0};
	uint32_t tlb[2][3]={{1,2,3},{4,5,6}},reset=0x47800,command[10]={},counters[16];
	unsigned operations=0,scopeCalls=0,retains=0,resets=0,tlbWrites=0,l1Writes=0,dramWrites=0,niuWrites=0,clockCalls=0;
	unsigned dropTlb=0,dropReset=0,dropL1=0,dropDram=0,dropNiu=0;
	bool retained=false,scopeValid=true,busyClock=false,started=false,executed=false,fifo=false;
	bool timeout=false,badMarker=false,badPC=false,badSource=false,badCounter=false,clockBeginFail=false,clockEndFail=false;
	bool failL1Restore=false,failDramRestore=false,damageEarlierDram=false;unsigned timingFault=0;
	std::string directory;uint64_t now=1000000;
	LeaseIO(Lease& owner,const char* path):lease(owner),directory(path) {
		for(unsigned i=0;i<memory.size();i++)memory[i]=0x9e370000U^(i*71);original=memory;
		for(unsigned i=0;i<gddr.size();i++)gddr[i]=0x78563412U^(i*101);originalDram=gddr;
		for(unsigned i=0;i<16;i++)counters[i]=UINT32_MAX-17+i;
	}
	void Check(){assert(!retained);operations++;}
	bool NativeScope(const Lease* subject,const tt_dram_samples_result* p,const uint32_t* a,unsigned n,const uint32_t* b,unsigned m) {
		assert(!retained && subject==&lease && p==proof && lease.BoundTo(p,a,n,b,m));recordedL1=a;recordedDram=b;scopeCalls++;return scopeValid;
	}
	void NativeRetain(){assert(!retained);retains++;retained=true;}
	bool NativeSourceRead(unsigned i,uint32_t& v){Check();assert(i<16);v=source[i]^((badSource && started)?1U:0U);return true;}
	bool NativeTlbRead(unsigned s,unsigned i,uint32_t& v){Check();assert(s<2 && i<3);v=tlb[s][i];return true;}
	bool NativeTlbWrite(unsigned s,unsigned i,uint32_t v){Check();assert(s<2 && i<3);if(++tlbWrites!=dropTlb)tlb[s][i]=v;return true;}
	bool Match(unsigned s,unsigned base,unsigned x,unsigned y,bool ro)const {
		for(unsigned i=0;i<3;i++)if(tlb[s][i]!=BlackholeDramFixedTlbWord(base,x,y,ro,i))return false;return true;
	}
	void Selected(bool dram=false)const {
		assert(proof && Match(1,0xffa00000,1,2,false));
		assert(Match(0,dram?B4L::DramWeight:0,dram?proof->translated[0][0]:1,dram?proof->translated[0][1]:2,false));
	}
	bool NativeResetRead(uint32_t& v) {
		Check();if(Match(1,0xffa00000,1,2,false) || Match(1,0xffa00000,1,2,true)){v=reset;return true;}
		const unsigned xs[]={1,2,3,4,5,6,7,10,11,12,13,14};
		for(unsigned x:xs)for(unsigned y=2;y<=11;y++)if(Match(1,0xffa00000,x,y,true)){v=0x47800;return true;}
		assert(false);return false;
	}
	bool NativeControlRead(unsigned i,uint32_t& v){Check();assert(i<3 && Match(1,0xffa00000,1,2,false));v=0;return true;}
	bool NativeNiuRead(unsigned a,uint32_t& v) {
		Check();assert(Match(1,0xffa00000,1,2,false));
		for(unsigned i=0;i<16;i++)if(a==BlackholeDramSourceOffset(i)){v=source[i]|(!i && fifo?0x10000U:0U);return true;}
		if(a==0x44){v=(17<<12)|(12<<19)|129;return true;}
		if(a==0x48){v=0x10000;return true;}if(a==0x148){v=129;return true;}
		if(a==0x178 || a==0x68 || a==0x40 || a==0x840 || a==0x1040 || a==0x1840 || (a>=0x240 && a<0x2c0)){v=0;return true;}
		if(a==0x64){v=fifo?0x10101010:0;return true;}
		if(a>=0x200 && a<0x240){v=counters[(a-0x200)/4]+(badCounter && started && a==0x208?1U:0U);return true;}
		assert(a<=0x24 && !(a&3));v=command[a/4];return true;
	}
	bool NativeNiuWrite(unsigned a,uint32_t v) {
		Check();assert(Match(1,0xffa00000,1,2,false) && reset==0x47800 && (a==0x100 || (a<=0x24 && !(a&3))));
		if(++niuWrites==dropNiu)return true;
		if(a==0x100){assert(v==source[0] || v==(source[0]|0x10000));fifo=bool(v&0x10000);}else command[a/4]=v;
		return true;
	}
	bool L1(unsigned a)const {
		return !(a&3) && (a==0x6000 || Dram4Image::Range(a,0x70000,8192) || Dram4Image::Range(a,0x6e000,8192)
			|| Dram4Image::Range(a,0x74000,32) || Dram4Image::Range(a,B4L::Weight-B4L::Header,160*B4L::WeightStride)
			|| Dram4Image::Range(a,B4L::Input-B4L::Header,160*B4L::InputStride) || Dram4Image::Range(a,B4L::Output,256) || Dram4Image::Range(a,0x17ffc0,12));
	}
	bool NativeL1Read(unsigned a,uint32_t& v){Check();Selected();assert(L1(a));v=memory[a/4];return true;}
	bool NativeL1Write(unsigned a,uint32_t v) {
		Check();Selected();assert(L1(a) && reset==0x47800 && (lease.Result().phase==4 || lease.Result().phase==8));
		if(++l1Writes!=dropL1 && !(failL1Restore && started && v==original[a/4]))memory[a/4]=v;return true;
	}
	bool Dram(unsigned a)const {
		return !(a&3) && (Dram4Image::Range(a,B4L::DramWeight,160*576) || Dram4Image::Range(a,B4L::DramInput,160*64));
	}
	bool NativeDramRead(unsigned a,uint32_t& v){Check();Selected(true);assert(Dram(a) && reset==0x47800);v=gddr[(a-B4L::DramWeight)/4];return true;}
	bool NativeDramWrite(unsigned a,uint32_t v) {
		Check();Selected(true);assert(Dram(a) && reset==0x47800 && (lease.Result().phase==3 || lease.Result().phase==7));
		const unsigned i=(a-B4L::DramWeight)/4;
		if(++dramWrites!=dropDram && !(failDramRestore && started && v==originalDram[i]))gddr[i]=v;
		if(damageEarlierDram && lease.Result().phase==3 && i==100)gddr[0]^=1;return true;
	}
	void Execute() {
		Dram4Image image(directory.c_str(),5120,proof->translated[0][0]|(proof->translated[0][1]<<6));
		assert(memory[0x6000/4]==Image::Boot);
		for(unsigned i=0;i<2048;i++)assert(memory[(0x70000+i*4)/4]==Image::Word(i));
		for(unsigned i=0;i<Lease::L1Words;i++) {
			const unsigned a=Lease::L1Address(i),v=memory[a/4];
			if(Dram4Image::Range(a,B4L::Weight-B4L::Header,160*B4L::WeightStride) || Dram4Image::Range(a,B4L::Input-B4L::Header,160*B4L::InputStride)) {
				for(unsigned b=0;b<4;b++)image.transfer.l1[a+b]=uint8_t(v>>(b*8));
			} else if(!Dram4Image::Range(a,0x70000+unsigned(image.code.size()),8192-unsigned(image.code.size())))image.l1[image.Index(a)]=v;
		}
		for(unsigned i=0;i<image.transfer.dram.size();i++)image.transfer.dram[i]=uint8_t(gddr[i/4]>>((i%4)*8));
		for(unsigned i=0;i<16;i++)image.transfer.counters[i]=counters[i];
		assert(image.Execute());
		for(unsigned i=0;i<Lease::L1Words;i++) {
			const unsigned a=Lease::L1Address(i);
			if(Dram4Image::Range(a,B4L::Weight-B4L::Header,160*B4L::WeightStride) || Dram4Image::Range(a,B4L::Input-B4L::Header,160*B4L::InputStride))memory[a/4]=::Word(image.transfer.l1,a);
			else if(!Dram4Image::Range(a,0x70000+unsigned(image.code.size()),8192-unsigned(image.code.size())))memory[a/4]=image.l1[image.Index(a)];
		}
		if(timingFault==1)memory[0x6e000/4]^=1;
		if(timingFault==2)memory[(0x6e000+4)/4]=PioneerNativeBfp4BandTiming::Unavailable;
		if(timingFault==3){memory[(0x6e000+48)/4]=0;memory[(0x6e000+52)/4]=0;}
		for(unsigned i=0;i<16;i++)counters[i]=image.transfer.counters[i];
		for(unsigned i=0;i<10;i++)command[i]=image.transfer.command[i];executed=true;
	}
	bool NativeResetWrite(unsigned v) {
		Check();Selected();assert(v==0x87ffff || v==0x46800 || v==0x47800);if(++resets==dropReset)return true;
		if(v==0x46800){assert(reset==0x87ffff && !started);started=true;reset=v;if(!timeout)Execute();if(badMarker)memory[0x17ffc8/4]=0x3442464e;}
		else {if(started)assert(executed && !timeout && !badMarker && !badPC && !badSource && !badCounter);reset=v;}return true;
	}
	bool NativePcRead(uint32_t& v){Check();Selected();v=badPC?0x70040:0x70038;return true;}
	bool NativeNow(uint64_t& v){Check();v=now;return true;}
	bool NativePause(){Check();now+=100000;return true;}
	bool NativeClockBegin(){Check();clockCalls++;assert(!busyClock);if(clockBeginFail)return false;busyClock=true;return true;}
	bool NativeClockEnd(){Check();clockCalls++;assert(busyClock);if(clockEndFail)return false;busyClock=false;return true;}
	tt_dram_samples_result Baseline() {
		tt_dram_samples_result r={};r.version=1;r.state=TT_DRAM_SAMPLE_ROUTES_OK;r.routes_verified=7;r.tlb_restored=r.data_restored=1;
		memcpy(r.source,source,sizeof(source));assert(BlackholeDramBuildRoutes(r));return r;
	}
};
static void NoMore(Lease& lease,LeaseIO& io) {
	const auto events=io.operations,scopes=io.scopeCalls;
	assert((!lease.Run<LeaseIO,Image>(io,*io.proof,0) && !lease.End<LeaseIO,Image>(io)));
	lease.Abandon();assert(events==io.operations && scopes==io.scopeCalls);
}
#ifndef PIONEER_NATIVE_BFP4_DRAM_LEASE_TEST_ONLY
int main(int argc,char** argv) {
	assert(argc==2);const std::string folder=argv[1];Image::code=Read((folder+"/text.bin").c_str());
	const auto boot=Read((folder+"/boot.bin").c_str());assert(boot.size()==4);Image::Boot=::Word(boot,0);
	static_assert(sizeof(Lease)>241968 && sizeof(Lease)<300000,"heap-only bounded journal owner");
	static_assert(!std::is_copy_constructible<Lease>::value && !std::is_copy_assignable<Lease>::value,"ownership journal cannot be copied");
	for(unsigned sample=0;sample<3;sample++) {
		auto lease=std::make_unique<Lease>();LeaseIO io(*lease,argv[1]);auto baseline=io.Baseline();io.proof=&baseline;
		assert((lease->Run<LeaseIO,Image>(io,baseline,sample)) && io.executed && io.resets==2);
		const auto& r=lease->Result();
		assert(PioneerNativeBfp4BandTiming::State(r.timing,5120,0)==1 && PioneerNativeBfp4BandTiming::State(r.timing+8,5120,1)==1);assert(r.compute.state==PIONEER_NATIVE_BFP_RESULT && r.compute.completed && !r.compute.cold
			&& r.requests==640 && r.flits==3200 && r.saved_l1==Lease::L1Words && r.saved_gddr==25600);
		for(unsigned i=0;i<64;i++)assert(r.compute.output[i]==PioneerNativeBfp4DramModelMetadata::Golden(sample)[i%32]);
		assert((lease->End<LeaseIO,Image>(io)) && r.compute.state==PIONEER_NATIVE_BFP_CLOSED && r.phase==10
			&& r.compute.held && r.compute.l1_restored && r.compute.tlb_restored && r.gddr_restored && r.niu_restored && r.clock_restored);
		assert(io.memory==io.original && io.gddr==io.originalDram && io.resets==4 && !io.retained && io.clockCalls==2 && !io.busyClock && !io.fifo);
		for(unsigned s=0;s<2;s++)for(unsigned i=0;i<3;i++)assert(io.tlb[s][i]==s*3+i+1);
		for(unsigned v:io.command)assert(!v);NoMore(*lease,io);
		printf("PASS OFFBOARD actual BFP4 GDDR learned-band lease sample%u:exact64 rounded words,640 device reads/two full BFP4 FPU jobs,complete%uB L1+102400B GDDR+NIU/FIFO/TLB/clock restoration;NO native PCI/VM/FD authority/hardware/full-model quality/speed proof\n",sample,Lease::L1Words*4);
	}
	unsigned failures=0;
	for(unsigned fault=0;fault<26;fault++) {
		auto lease=std::make_unique<Lease>();LeaseIO io(*lease,argv[1]);auto baseline=io.Baseline();io.proof=&baseline;
		if(fault==0)io.dropTlb=1;if(fault==1)io.dropL1=1;if(fault==2)io.dropDram=1;
		if(fault==3)io.dropNiu=1;if(fault==4)io.dropReset=1;if(fault==5)io.dropReset=2;
		if(fault==6)io.timeout=true;if(fault==7)io.badMarker=true;if(fault==8)io.badPC=true;
		if(fault==9)io.clockBeginFail=true;if(fault==10)io.badSource=true;if(fault==11)io.badCounter=true;
		if(fault==25)io.damageEarlierDram=true;
		const bool ran=lease->Run<LeaseIO,Image>(io,baseline,0);
		if(fault>=12 && fault<25) {
			assert(ran);
			if(fault==12)io.badSource=true;if(fault==13)io.scopeValid=false;if(fault==14)io.clockEndFail=true;
			if(fault==15)io.failDramRestore=true;if(fault==16)io.failL1Restore=true;
			if(fault==17)io.dropReset=3;if(fault==18)io.dropReset=4;
			if(fault==19)io.dropNiu=2;if(fault==20)io.dropNiu=12;
			if(fault==21)io.gddr[0]^=1;if(fault==22)io.memory[B4L::Weight/4]^=1;
			if(fault==23)io.memory[0x70000/4]^=1;if(fault==24)io.memory[0x74010/4]^=1;
			assert((!lease->End<LeaseIO,Image>(io)));
		} else assert(!ran);
		assert(io.retained && io.retains==1 && lease->Result().compute.state==PIONEER_NATIVE_BFP_UNKNOWN && lease->Result().compute.cold);
		if(fault==2 || fault==25)assert(!io.started && !io.resets);
		if(fault==12 || fault==13)assert(io.resets==2);
		if(fault>=21 && fault<=24)assert(io.dramWrites==25600 && !lease->Result().gddr_restored);
		NoMore(*lease,io);failures++;
	}
	for(unsigned bad:{3U,4U,UINT32_MAX}) {
		auto lease=std::make_unique<Lease>();LeaseIO io(*lease,argv[1]);auto baseline=io.Baseline();io.proof=&baseline;
		assert((!lease->Run<LeaseIO,Image>(io,baseline,bad)) && !io.operations && !io.scopeCalls);failures++;
	}
	for(unsigned fault=0;fault<5;fault++) {
		auto lease=std::make_unique<Lease>();LeaseIO io(*lease,argv[1]);auto baseline=io.Baseline();io.proof=&baseline;
		if(fault==0)baseline.version=0;if(fault==1)baseline.recovery_required=1;if(fault==2)baseline.routes_verified=6;
		if(fault==3)baseline.source[0]|=0x10000;if(fault==4)baseline.translated[0][0]^=1;
		assert((!lease->Run<LeaseIO,Image>(io,baseline,0)) && !io.operations && !io.scopeCalls);NoMore(*lease,io);failures++;
	}
	for(unsigned fault=1;fault<=4;fault++) {
		auto lease=std::make_unique<Lease>();LeaseIO io(*lease,argv[1]);auto baseline=io.Baseline();io.proof=&baseline;
		if(fault<=3)io.timingFault=fault;
		bool ran=lease->Run<LeaseIO,Image>(io,baseline,0);
		if(fault==4){assert(ran);io.memory[0x6e000/4]^=1;assert((!lease->End<LeaseIO,Image>(io)));}
		else assert(!ran);
		assert(io.retained && io.retains==1 && lease->Result().compute.state==PIONEER_NATIVE_BFP_UNKNOWN
			&& lease->Result().compute.cold && io.resets==2);
		NoMore(*lease,io);failures++;
	}
	static_assert(B4L::Weight==0x90000 && B4L::Input==0x12c000 && B4L::Output==0x140000
		&& B4L::WeightStride==640 && B4L::InputStride==128 && B4L::Header==64 && Lease::L1Words==34892,"independent BFP4 C64 geometry");
	for(unsigned operand=0;operand<2;operand++)for(unsigned tile:{0U,79U,159U})for(unsigned word=0;word<B4L::Header/4+2;word++) {
		auto lease=std::make_unique<Lease>();LeaseIO io(*lease,argv[1]);auto baseline=io.Baseline();io.proof=&baseline;
		assert((lease->Run<LeaseIO,Image>(io,baseline,0)) && io.executed);
		unsigned start=(operand?B4L::Input:B4L::Weight)+tile*(operand?B4L::InputStride:B4L::WeightStride);
		unsigned a=word<B4L::Header/4?start-B4L::Header+word*4:word==B4L::Header/4?start:start+(operand?64:576)-4;
		io.memory[a/4]^=1;
		assert((!lease->End<LeaseIO,Image>(io)) && io.retained && io.retains==1 && io.resets==4
			&& !lease->Result().gddr_restored && io.dramWrites==25600 && io.l1Writes==Lease::L1Words);
		NoMore(*lease,io);failures++;
	}

	{
		auto lease=std::make_unique<Lease>();LeaseIO io(*lease,argv[1]);auto baseline=io.Baseline();io.proof=&baseline;
		assert((lease->Run<LeaseIO,Image>(io,baseline,0)));const auto events=io.operations;
		lease->Abandon();assert(io.operations==events && lease->Result().compute.cold && io.resets==2);NoMore(*lease,io);failures++;
	}
	printf("PASS OFFBOARD GDDR learned-band lease:%u failure/admission/close cases;first uncertainty retains journals,NO subsequent IO/END/retry/reset/cleanup;PRIVATE weights not printed or published\n",failures);
}
#endif
