/* SPDX-License-Identifier: MIT */
#define B_DEVICE_OP_CODES_END 10000
#define PIONEER_Q4_FPU_BLOCK_IMAGE_ONLY
#include "pioneer_q38_q4_fpu_block_image_test.cpp"
#include "../PioneerNativeQ4FpuLease.h"
#include <memory>
#include <type_traits>
using Lease=PioneerNativeQ4FpuLease;using Layout=PioneerNativeQ4FpuLayout;
struct Image {
	static constexpr unsigned Width=256,Rows=32,Phases=4,LayoutRole=85,Bytes=7036;
	static inline unsigned Boot=0;static inline std::vector<uint8_t> code;
	static unsigned Word(unsigned i){return i<code.size()/4?::Word(code,i*4):0;}
};
struct Data {
	static constexpr unsigned Version=1,Width=256,Rows=32,Role=85,Samples=3;
	static inline std::vector<Q4PipelineFixture> samples;
	static uint32_t Word(unsigned s,unsigned a) {
		assert(s<3);unsigned at=Layout::PreparedIndex(a);uint32_t result=0;
		if(at!=UINT32_MAX){for(unsigned i=0;i<4;i++)result|=uint32_t(samples[s].prepared[at+i])<<(i*8);return result;}
		at=Layout::InputIndex(a);if(at!=UINT32_MAX)return uint32_t(samples[s].input[at/2])|(uint32_t(samples[s].input[at/2+1])<<16);
		return 0;
	}
};
// Canonical physical L1 contains code/stack/status/results AND all operands.
// Strict Step's immutable instruction-fetch cache mirrors ONLY staged ROM;
// scalar Load/Store and UNPACK share native.l1,never normalized data copies.
struct LeaseVM:Q4FpuBlockRV32 {
	using Q4FpuBlockRV32::Q4FpuBlockRV32;
	unsigned ReadL1(unsigned a)const{return ::Word(native.l1,a);}
	void WriteL1(unsigned a,unsigned v) {
		assert(!(a&3) && a+3<native.l1.size());for(unsigned i=0;i<4;i++)native.l1[a+i]=uint8_t(v>>(i*8));
		if(CodeAddress(a))l1[Index(a)]=v;
	}
	void Store(uint32_t a,uint32_t v)override {
		Q4FpuBlockRV32::Store(a,v);if(Layout::Contains(a))WriteL1(a,v);
	}
	uint32_t Load(uint32_t a)override {
		if(CodeAddress(a) || Range(a,0x6e000,8192) || Range(a,0x17ffc0,12))return ReadL1(a);
		return Q4FpuBlockRV32::Load(a);
	}
};
struct LeaseIO {
	Lease& lease;LeaseVM vm;std::vector<uint8_t> original;
	const tt_dram_samples_result* proof=nullptr;const uint32_t* backing=nullptr;
	uint32_t source[16]={0x6005,0x0a418820,0x1ac4a166,0x127541ee,0x1e280440,0x1662b5c3,0x147,
		0x0a418820,0x16a4a0e6,0x06a12c20,0x0c72a089,0x0e548c20,2,0,3,0};
	uint32_t tlb[2][3]={{1,2,3},{4,5,6}},reset=0x47800,counters[16];
	unsigned operations=0,scopes=0,retains=0,resets=0,writes=0,tlbWrites=0,clockCalls=0;
	unsigned dropWrite=0,dropTlb=0,dropReset=0,failAt=0;
	bool retained=false,scopeValid=true,clock=false,started=false,executed=false,timeout=false;
	bool badMarker=false,badPc=false,badSource=false,badCounter=false,beginFail=false,endFail=false,restoreFail=false;
	bool clockTlbDamage=false;
	uint64_t now=1000000;
	LeaseIO(Lease& l,const char* path):lease(l),vm(path) {
		for(unsigned i=0;i<vm.native.l1.size();i++)vm.native.l1[i]=uint8_t((i*71)^(i>>7)^0x9e);original=vm.native.l1;
		for(unsigned i=0;i<16;i++)counters[i]=UINT32_MAX-17+i;
	}
	bool Check(){assert(!retained);return ++operations!=failAt;}
	bool NativeScope(const Lease* l,const tt_dram_samples_result* p,const uint32_t* b,unsigned n) {
		assert(!retained && l==&lease && p==proof && lease.BoundTo(p,b,n) && n==12372);backing=b;scopes++;return scopeValid;
	}
	void NativeRetain(){assert(!retained);retained=true;retains++;}
	bool NativeSourceRead(unsigned i,uint32_t& v){if(!Check())return false;assert(i<16);v=source[i]^((badSource && started)?1U:0U);return true;}
	bool NativeTlbRead(unsigned s,unsigned i,uint32_t& v){if(!Check())return false;assert(s<2 && i<3);v=tlb[s][i];return true;}
	bool NativeTlbWrite(unsigned s,unsigned i,uint32_t v){if(!Check())return false;assert(s<2 && i<3);if(++tlbWrites!=dropTlb)tlb[s][i]=v;return true;}
	bool Match(unsigned s,unsigned base,unsigned x,unsigned y,bool ro)const {
		for(unsigned i=0;i<3;i++)if(tlb[s][i]!=BlackholeDramFixedTlbWord(base,x,y,ro,i))return false;return true;
	}
	void Selected()const{assert(Match(0,0,1,2,false) && Match(1,0xffa00000,1,2,false));}
	bool NativeResetRead(uint32_t& v) {
		if(!Check())return false;
		if(Match(1,0xffa00000,1,2,false) || Match(1,0xffa00000,1,2,true)){v=reset;return true;}
		for(unsigned x:{1U,2U,3U,4U,5U,6U,7U,10U,11U,12U,13U,14U})for(unsigned y=2;y<=11;y++)
			if(Match(1,0xffa00000,x,y,true)){v=0x47800;return true;}
		assert(false);return false;
	}
	bool NativeControlRead(unsigned i,uint32_t& v){if(!Check())return false;assert(i<3);v=0;return true;}
	bool NativeNiuRead(unsigned a,uint32_t& v) {
		if(!Check())return false;assert(Match(1,0xffa00000,1,2,false));
		for(unsigned i=0;i<16;i++)if(a==BlackholeDramSourceOffset(i)){v=source[i];return true;}
		if(a==0x44){v=(17<<12)|(12<<19)|129;return true;}
		if(a==0x48){v=0x10000;return true;}if(a==0x148){v=129;return true;}
		if(a>=0x200 && a<0x240){v=counters[(a-0x200)/4]+(badCounter && started && a==0x208?1U:0U);return true;}
		assert(a<=0x24 || a==0x178 || a==0x68 || a==0x40 || a==0x840 || a==0x1040 || a==0x1840 || a==0x64 || (a>=0x240 && a<0x2c0));v=0;return true;
	}
	bool NativeL1Read(unsigned a,uint32_t& v){if(!Check())return false;Selected();assert(Layout::Contains(a));v=vm.ReadL1(a);return true;}
	bool NativeL1Write(unsigned a,uint32_t v) {
		if(!Check())return false;Selected();assert(Layout::Contains(a) && reset==0x47800 && (lease.Result().phase==3 || lease.Result().phase==6));
		if(++writes!=dropWrite && !(restoreFail && started && v==::Word(original,a)))vm.WriteL1(a,v);return true;
	}
	bool NativeResetWrite(uint32_t v) {
		if(!Check())return false;Selected();assert(v==0x87ffff || v==0x46800 || v==0x47800);if(++resets==dropReset)return true;
		if(v==0x46800){assert(reset==0x87ffff && !started);started=true;reset=v;if(!timeout){assert(vm.Execute());executed=true;}if(badMarker)vm.WriteL1(0x17ffc8,0x3436424e);}
		else{if(started)assert(executed && !timeout && !badMarker && !badPc && !badSource && !badCounter);reset=v;}return true;
	}
	bool NativePcRead(uint32_t& v){if(!Check())return false;Selected();v=badPc?0x70040:0x70038;return true;}
	bool NativeNow(uint64_t& v){if(!Check())return false;v=now;return true;}
	bool NativePause(){if(!Check())return false;now+=100000;return true;}
	bool NativeClockBegin(){if(!Check())return false;clockCalls++;assert(!clock);if(beginFail)return false;clock=true;return true;}
	bool NativeClockEnd(){if(!Check())return false;clockCalls++;assert(clock);if(endFail)return false;clock=false;if(clockTlbDamage)tlb[1][2]^=1;return true;}
	tt_dram_samples_result Baseline(){tt_dram_samples_result r={};r.version=1;r.state=TT_DRAM_SAMPLE_ROUTES_OK;r.routes_verified=7;r.tlb_restored=r.data_restored=1;memcpy(r.source,source,sizeof(source));assert(BlackholeDramBuildRoutes(r));return r;}
};
static void NoMore(Lease& lease,LeaseIO& io) {
	const unsigned operations=io.operations,scopes=io.scopes;
	assert((!lease.Run<LeaseIO,Image,Data>(io,*io.proof,0) && !lease.End<LeaseIO,Image,Data>(io)));
	lease.Abandon();assert(operations==io.operations && scopes==io.scopes);
}
#ifndef PIONEER_NATIVE_Q4_FPU_LEASE_TEST_ONLY
int main(int argc,char** argv)
{
	assert(argc==2);Image::code=Read((std::string(argv[1])+"/text.bin").c_str());assert(Image::code.size()==7036);
	Image::Boot=::Word(Read((std::string(argv[1])+"/boot.bin").c_str()),0);
	for(unsigned i=0;i<3;i++)Data::samples.emplace_back(i);
	static_assert(!std::is_copy_constructible<Lease>::value && !std::is_copy_assignable<Lease>::value,"unique journal");
	for(unsigned sample=0;sample<3;sample++) {
		auto lease=std::make_unique<Lease>();auto io=std::make_unique<LeaseIO>(*lease,argv[1]);auto baseline=io->Baseline();io->proof=&baseline;
		assert((lease->Run<LeaseIO,Image,Data>(*io,baseline,sample)) && io->executed && io->resets==2);
		const auto& r=lease->Result();assert(r.compute.state==PIONEER_NATIVE_BFP_RESULT && r.compute.completed && !r.compute.cold && r.saved_l1==12372);
		for(unsigned i=0;i<64;i++)assert(r.compute.output[i]==Data::samples[sample].expected[i%32]);
		assert((lease->End<LeaseIO,Image,Data>(*io)) && r.compute.state==PIONEER_NATIVE_BFP_CLOSED && r.compute.held
			&& r.compute.l1_restored && r.compute.tlb_restored && r.niu_restored && r.clock_restored);
		assert(io->vm.native.l1==io->original && io->resets==4 && io->clockCalls==2 && !io->clock && !io->retained);
		for(unsigned s=0;s<2;s++)for(unsigned i=0;i<3;i++)assert(io->tlb[s][i]==s*3+i+1);
		NoMore(*lease,*io);
		printf("PASS OFFBOARD Q4 lease sample%u:actual RV32/all64 exact,49488B whole L1/TLB/unchanged NIU/clock closure;NO native PCI/VM/FD or hardware/quality/speed proof\n",sample);
	}
	unsigned failures=0;
	for(unsigned fault=0;fault<24;fault++) {
		auto lease=std::make_unique<Lease>();auto io=std::make_unique<LeaseIO>(*lease,argv[1]);auto baseline=io->Baseline();io->proof=&baseline;
		if(fault==0)io->dropTlb=1;if(fault==1)io->dropWrite=1;if(fault==2)io->dropReset=1;if(fault==3)io->dropReset=2;
		if(fault==4)io->timeout=true;if(fault==5)io->badMarker=true;if(fault==6)io->badPc=true;if(fault==7)io->beginFail=true;
		if(fault==8)io->badSource=true;if(fault==9)io->badCounter=true;if(fault==10)io->scopeValid=false;
		const bool ran=lease->Run<LeaseIO,Image,Data>(*io,baseline,0);
		if(fault>=11) {
			assert(ran);
			if(fault==11)io->restoreFail=true;if(fault==12)io->endFail=true;if(fault==13)io->dropReset=3;if(fault==14)io->dropReset=4;
			if(fault==15)io->scopeValid=false;
			const unsigned damage[]={0x70000,0x8ffc0,0x94000,0x97000,0x92000,0x98000};
			if(fault>=16 && fault<=21){unsigned a=damage[fault-16];io->vm.WriteL1(a,io->vm.ReadL1(a)^1);}
			if(fault==22)io->vm.WriteL1(0x17ffc4,0x04001400);
			if(fault==23)io->clockTlbDamage=true;
			assert((!lease->End<LeaseIO,Image,Data>(*io)));
		} else assert(!ran);
		assert(io->retained && io->retains==1 && lease->Result().compute.cold && lease->Result().compute.state==PIONEER_NATIVE_BFP_UNKNOWN);
		NoMore(*lease,*io);failures++;
	}
	printf("PASS OFFBOARD Q4 lease:%u failure/no-retry/no-END/retained-journal cases;NO GDDR/NIU write methods,all actual scalar/unpack operand accesses share canonical L1;NOT native authorization\n",failures);
}
#endif
