/* SPDX-License-Identifier: MIT */
#define B_DEVICE_OP_CODES_END 10000
#define PIONEER_Q4_COMPACT_STREAM_IMAGE_ONLY
#ifdef PIONEER_NATIVE_COMPACT_Q4_STREAM_TIMING
#define PIONEER_Q4_KERNEL_TIMING_MACHINE_ONLY
#include "pioneer_q38_q4_kernel_timing_image_test.cpp"
#ifdef PIONEER_NATIVE_COMPACT_Q4_PAIR_ISSUE_TIMING
struct NMachine:Q4TimingRV32 {
 NMachine(const char* directory,unsigned width):Q4TimingRV32(directory,width,true){}
};
#else
using NMachine=Q4TimingRV32;
#endif
#else
#include "pioneer_q38_q4_fpu_compact_stream_image_test.cpp"
using NMachine=CompactStreamRV32;
#endif
#include "../PioneerNativeCompactQ4StreamLease.h"
#include <type_traits>
using NLease=PioneerNativeCompactQ4StreamLease;using NL=PioneerNativeCompactQ4StreamLayout;
using NV=PioneerNativeCompactQ4StreamVariant;
struct NImage {
	static constexpr unsigned Samples=9,Rows=32,Phases=4,LayoutRole=NV::Role,Bytes=NV::ImageBytes,MetadataFirst=128,MetadataRows=128,DstLast=255;
	static inline unsigned Boot=0;static inline std::vector<uint8_t> code[2];
	static unsigned Width(unsigned s){return NV::Width(s);}
	static unsigned Word(unsigned s,unsigned i){const auto& c=code[s<6?0:1];return i<c.size()/4?::Word(c,i*4):0;}
};
struct NData {
	static constexpr unsigned Version=1,Rows=32,Samples=9,Role=104,PreparedBytes=4608;
	static inline std::vector<CompactStreamData> samples;
	static unsigned Width(unsigned s){return NV::Width(s);}
	static unsigned Words(unsigned s){return NL::DramWords(Width(s));}
	static unsigned Word(unsigned s,unsigned i) {
		assert(s<9 && i<Words(s));const auto& data=samples[s];unsigned k=Width(s);
		if(i<NL::WeightBytes(k)/4){
			const auto& raw=data.blocks[i/1152].original;unsigned value=0;
			for(unsigned b=0;b<4;b++){
				unsigned byte=(i%1152)*4+b,row=0,at=0;
				if(byte<4096){row=(byte/2048)*16+byte%16;at=16+(byte%2048)/16;}
				else{byte-=4096;row=(byte/256)*16+byte%16;at=(byte%256)/16;}
				value|=unsigned(raw[row*144+at])<<(b*8);
			}return value;
		}
		i-=NL::WeightBytes(k)/4;const auto& b=data.blocks[i/128];unsigned c=(i%128)*2;return unsigned(b.input[c])|(unsigned(b.input[c+1])<<16);
	}
};
// The lease stages into the SAME actual RV32/NIU/UNPACK/FPU canonical L1
// and GDDR vectors. No completed-result injection,post-BOOT payload copying,
// normalized descriptor,host K loop or reference in the native driver.
struct NLeaseIO {
	NLease& lease;NMachine vm;std::vector<uint8_t> original,originalDram;
	const tt_dram_samples_result* proof=nullptr;unsigned sample;const uint32_t* recordedL1=nullptr;const uint32_t* recordedDram=nullptr;
	uint32_t source[16]={0x6005,0x0a418820,0x1ac4a166,0x127541ee,0x1e280440,0x1662b5c3,0x147,
		0x0a418820,0x16a4a0e6,0x06a12c20,0x0c72a089,0x0e548c20,2,0,3,0};
	uint32_t tlb[2][3]={{1,2,3},{4,5,6}},reset=0x47800;
	unsigned operations=0,scopes=0,retains=0,resets=0,l1Writes=0,dramWrites=0,niuWrites=0,tlbWrites=0,clockCalls=0;
	unsigned dropTlb=0,dropReset=0,dropL1=0,dropDram=0,dropNiu=0;
#ifdef PIONEER_NATIVE_COMPACT_Q4_STREAM_TIMING
	unsigned timingReads=0,dropTimingRead=0;
#endif
	bool retained=false,scopeValid=true,clock=false,started=false,executed=false,fifo=false;
	bool timeout=false,badMarker=false,badPc=false,badSource=false,badCounter=false,beginFail=false,endFail=false;
	bool restoreL1Fail=false,restoreDramFail=false,clockTlbDamage=false,earlierDramDamage=false;
	uint64_t now=1000000;
	NLeaseIO(NLease& l,const char* small,const char* large,unsigned s):lease(l),vm(s<6?small:large,NV::Width(s)),sample(s) {
		for(unsigned i=0;i<vm.native.l1.size();i++)vm.native.l1[i]=uint8_t((i*71)^(i>>7)^0x9e);original=vm.native.l1;
		originalDram=vm.bus.dram;vm.bus.command.fill(0);
	}
	void Check(){assert(!retained);operations++;}
	bool NativeScope(const NLease* l,const tt_dram_samples_result* p,const uint32_t* a,unsigned n,const uint32_t* b,unsigned m) {
		assert(!retained && l==&lease && p==proof && lease.BoundTo(p,a,n,b,m) && n==26908 && m==87040);recordedL1=a;recordedDram=b;scopes++;return scopeValid;
	}
	void NativeRetain(){assert(!retained);retained=true;retains++;}
	bool NativeSourceRead(unsigned i,uint32_t& v){Check();assert(i<16);v=source[i]^((badSource && started)?1U:0U);return true;}
	bool NativeTlbRead(unsigned s,unsigned i,uint32_t& v){Check();assert(s<2 && i<3);v=tlb[s][i];return true;}
	bool NativeTlbWrite(unsigned s,unsigned i,uint32_t v){Check();assert(s<2 && i<3);if(++tlbWrites!=dropTlb)tlb[s][i]=v;return true;}
	bool Match(unsigned s,unsigned base,unsigned x,unsigned y,bool ro)const {
		for(unsigned i=0;i<3;i++)if(tlb[s][i]!=BlackholeDramFixedTlbWord(base,x,y,ro,i))return false;return true;
	}
	void Selected(bool dram=false)const{assert(proof && Match(1,0xffa00000,1,2,false) && Match(0,dram?NL::DramWeight:0,dram?proof->translated[0][0]:1,dram?proof->translated[0][1]:2,false));}
	bool NativeResetRead(uint32_t& v) {
		Check();if(Match(1,0xffa00000,1,2,false) || Match(1,0xffa00000,1,2,true)){v=reset;return true;}
		for(unsigned x:{1U,2U,3U,4U,5U,6U,7U,10U,11U,12U,13U,14U})for(unsigned y=2;y<=11;y++)if(Match(1,0xffa00000,x,y,true)){v=0x47800;return true;}
		assert(false);return false;
	}
	bool NativeControlRead(unsigned i,uint32_t& v){Check();assert(i<3 && Match(1,0xffa00000,1,2,false));v=0;return true;}
	bool NativeNiuRead(unsigned a,uint32_t& v) {
		Check();assert(Match(1,0xffa00000,1,2,false));
		for(unsigned i=0;i<16;i++)if(a==BlackholeDramSourceOffset(i)){v=source[i]|(!i && fifo?0x10000U:0U);return true;}
		if(a==0x44){v=(17<<12)|(12<<19)|129;return true;}if(a==0x48){v=0x10000;return true;}if(a==0x148){v=129;return true;}
		if(a==0x178 || a==0x68 || a==0x40 || a==0x840 || a==0x1040 || a==0x1840 || (a>=0x240 && a<0x2c0)){v=0;return true;}
		if(a==0x64){v=fifo?0x10101010:0;return true;}
		if(a>=0x200 && a<0x240){v=vm.bus.counters[(a-0x200)/4]+(badCounter && started && a==0x208?1U:0U);return true;}
		assert(a<=0x24 && !(a&3));v=vm.bus.command[a/4];return true;
	}
	bool NativeNiuWrite(unsigned a,uint32_t v) {
		Check();Selected();assert(reset==0x47800 && (a==0x100 || (a<=0x24 && !(a&3))));if(++niuWrites==dropNiu)return true;
		if(a==0x100){assert(v==source[0] || v==(source[0]|0x10000));fifo=bool(v&0x10000);}else vm.bus.command[a/4]=v;return true;
	}
	bool NativeL1Read(unsigned a,uint32_t& v){Check();Selected();assert(NL::Contains(a));
#ifdef PIONEER_NATIVE_COMPACT_Q4_STREAM_TIMING
		if(started && (lease.Result().phase==5 || lease.Result().phase==6)
			&& a>=PioneerQ38Q4KernelTiming::Address && a<PioneerQ38Q4KernelTiming::Address+64
			&& ++timingReads==dropTimingRead)return false;
#endif
		v=vm.ReadL1(a);return true;}
	bool NativeL1Write(unsigned a,uint32_t v) {
		Check();Selected();assert(NL::Contains(a) && reset==0x47800 && (lease.Result().phase==4 || lease.Result().phase==8));
		if(++l1Writes!=dropL1 && !(restoreL1Fail && started && v==::Word(original,a)))vm.WriteL1(a,v);return true;
	}
	bool NativeDramRead(unsigned a,uint32_t& v){Check();Selected(true);assert(NL::DramContains(NV::Width(sample),a) && reset==0x47800);v=::Word(vm.bus.dram,a-NL::DramWeight);return true;}
	bool NativeDramWrite(unsigned a,uint32_t v) {
		Check();Selected(true);assert(NL::DramContains(NV::Width(sample),a) && reset==0x47800 && (lease.Result().phase==3 || lease.Result().phase==7));
		unsigned at=a-NL::DramWeight;
		if(++dramWrites!=dropDram && !(restoreDramFail && started && v==::Word(originalDram,at)))for(unsigned b=0;b<4;b++)vm.bus.dram[at+b]=uint8_t(v>>(b*8));
		if(earlierDramDamage && lease.Result().phase==3 && at==400)vm.bus.dram[0]^=1;return true;
	}
	bool NativeResetWrite(unsigned v) {
		Check();Selected();assert(v==0x87ffff || v==0x46800 || v==0x47800);if(++resets==dropReset)return true;
		if(v==0x46800){assert(reset==0x87ffff && !started && fifo);started=true;reset=v;if(!timeout){assert(vm.Execute());executed=true;}if(badMarker)vm.WriteL1(0x17ffc8,0x31504651);}
		else{if(started)assert(executed && !timeout && !badMarker && !badPc && !badSource && !badCounter);reset=v;}return true;
	}
	bool NativePcRead(uint32_t& v){Check();Selected();v=badPc?0x70040:0x70038;return true;}
	bool NativeNow(uint64_t& v){Check();v=now;return true;}
	bool NativePause(){Check();now+=100000;return true;}
	bool NativeClockBegin(){Check();clockCalls++;assert(!clock);if(beginFail)return false;clock=true;return true;}
	bool NativeClockEnd(){Check();clockCalls++;assert(clock);if(endFail)return false;clock=false;if(clockTlbDamage)tlb[1][2]^=1;return true;}
	tt_dram_samples_result Baseline(){tt_dram_samples_result r={};r.version=1;r.state=TT_DRAM_SAMPLE_ROUTES_OK;r.routes_verified=7;r.tlb_restored=r.data_restored=1;memcpy(r.source,source,sizeof(source));assert(BlackholeDramBuildRoutes(r));vm.bus.route=r.translated[0][0]|(r.translated[0][1]<<6);return r;}
};
static void NoMoreLease(NLease& l,NLeaseIO& io) {
	unsigned n=io.operations,s=io.scopes;assert((!l.Run<NLeaseIO,NImage,NData>(io,*io.proof,0) && !l.End<NLeaseIO,NImage,NData>(io)));
	l.Abandon();assert(n==io.operations && s==io.scopes);
}
#ifndef PIONEER_NATIVE_COMPACT_Q4_STREAM_LEASE_ONLY
int main(int argc,char** argv) {
	assert(argc==3);static_cast<void>(&NoMore);
	for(unsigned w=0;w<2;w++){std::string d=argv[w+1];NImage::code[w]=Read((d+"/text.bin").c_str());assert(NImage::code[w].size()==NV::ImageBytes);NImage::Boot=::Word(Read((d+"/boot.bin").c_str()),0);}
	for(unsigned s=0;s<9;s++)NData::samples.emplace_back(NV::Width(s),s);
	static_assert(!std::is_copy_constructible<NLease>::value && !std::is_copy_assignable<NLease>::value,"unique heap journals");
	for(unsigned s=0;s<9;s++) {
		auto l=std::make_unique<NLease>();auto io=std::make_unique<NLeaseIO>(*l,argv[1],argv[2],s);auto p=io->Baseline();io->proof=&p;
		assert((l->Run<NLeaseIO,NImage,NData>(*io,p,s)) && io->executed && io->resets==2);
		const auto& r=l->Result();assert(r.compute.state==PIONEER_NATIVE_BFP_RESULT && r.compute.completed && !r.compute.cold && r.saved_l1==26908
			&& r.saved_gddr==NL::DramWords(NV::Width(s)) && r.requests==2*(NV::Width(s)/256)*NL::Packets && r.flits==2*(NV::Width(s)/256)*NL::Flits);
		for(unsigned i=0;i<64;i++)assert(r.compute.output[i]==NData::samples[s].expected[i%32]);
		assert((l->End<NLeaseIO,NImage,NData>(*io)) && r.compute.state==PIONEER_NATIVE_BFP_CLOSED && r.phase==10 && r.compute.held && r.compute.l1_restored
			&& r.compute.tlb_restored && r.gddr_restored && r.niu_restored && r.clock_restored);
		assert(io->vm.native.l1==io->original && io->vm.bus.dram==io->originalDram && io->resets==4 && io->clockCalls==2 && !io->clock && !io->fifo && !io->retained);
		for(unsigned a=0;a<2;a++)for(unsigned i=0;i<3;i++)assert(io->tlb[a][i]==a*3+i+1);for(unsigned v:io->vm.bus.command)assert(!v);NoMoreLease(*l,*io);
		printf("PASS OFFBOARD compact%u full-K Q4 lease sample%u K%u:actual strict RV32/NIU/shared L1/FPU all64 exact;whole107632B L1/%uB GDDR/NIU/FIFO/TLB/clock restored;NO native PCI/VM/FD/hardware/quality/speed authority\n",NV::Role,s,NV::Width(s),NL::DramWords(NV::Width(s))*4);fflush(stdout);
	}
	unsigned failed=0;
	for(unsigned f=0;f<45;f++) {
		auto l=std::make_unique<NLease>();auto io=std::make_unique<NLeaseIO>(*l,argv[1],argv[2],0);auto p=io->Baseline();io->proof=&p;
		if(f==0)io->dropTlb=1;if(f==1)io->dropL1=1;if(f==2)io->dropDram=1;if(f==3)io->dropNiu=1;if(f==4)io->dropReset=1;if(f==5)io->dropReset=2;
		if(f==6)io->timeout=true;if(f==7)io->badMarker=true;if(f==8)io->badPc=true;if(f==9)io->beginFail=true;if(f==10)io->badSource=true;if(f==11)io->badCounter=true;
		if(f==12)io->earlierDramDamage=true;
		bool ran=l->Run<NLeaseIO,NImage,NData>(*io,p,0);
		if(f>=13) {
			assert(ran);if(f==13)io->scopeValid=false;if(f==14)io->endFail=true;if(f==15)io->restoreDramFail=true;if(f==16)io->restoreL1Fail=true;
			if(f==17)io->dropReset=3;if(f==18)io->dropReset=4;if(f==19)io->dropNiu=2;if(f==20)io->dropNiu=12;if(f==21)io->clockTlbDamage=true;
			if(f==22)io->vm.bus.dram[0]^=1;
			const unsigned damage[]={0x6000,0x70000,0x74010,0x8ffc0,0x90000,0x913bc,0x93fc0,0x94000,0x94140,0x94fbc,0x96000,0x97000,0x98000,0x9ffc0,0xa0000,0xa13bc,0xa4000,0xa4140,0xa6000,0xa63fc};
			if(f>=23 && f<43){unsigned a=damage[f-23];io->vm.WriteL1(a,io->vm.ReadL1(a)^1);}
			if(f==43)io->vm.bus.command[8]^=1;if(f==44)io->vm.WriteL1(0x17ffc4,0x04000100);
			assert((!l->End<NLeaseIO,NImage,NData>(*io)));
		} else assert(!ran);
		assert(io->retained && io->retains==1 && l->Result().compute.cold && l->Result().compute.state==PIONEER_NATIVE_BFP_UNKNOWN);NoMoreLease(*l,*io);failed++;
	}
	for(unsigned bad:{9U,10U,UINT32_MAX}) {
		auto l=std::make_unique<NLease>();auto io=std::make_unique<NLeaseIO>(*l,argv[1],argv[2],0);auto p=io->Baseline();io->proof=&p;
		assert((!l->Run<NLeaseIO,NImage,NData>(*io,p,bad)) && !io->operations && !io->scopes);failed++;
	}
	for(unsigned f=0;f<6;f++) {
		auto l=std::make_unique<NLease>();auto io=std::make_unique<NLeaseIO>(*l,argv[1],argv[2],0);auto p=io->Baseline();io->proof=&p;
		if(f==0)p.version=0;if(f==1)p.recovery_required=1;if(f==2)p.routes_verified=6;if(f==3)p.source[0]|=0x10000;if(f==4)p.translated[0][0]^=1;if(f==5)p.data_restored=0;
		assert((!l->Run<NLeaseIO,NImage,NData>(*io,p,0)) && !io->operations && !io->scopes);NoMoreLease(*l,*io);failed++;
	}
	{
		struct BadImage:NImage{static unsigned Width(unsigned){return 256;}};
		struct BadData:NData{static unsigned Width(unsigned){return 5121;}};
		struct BadLength:NData{static unsigned Words(unsigned){return 33279;}};
		for(unsigned f=0;f<3;f++) {
			auto l=std::make_unique<NLease>();auto io=std::make_unique<NLeaseIO>(*l,argv[1],argv[2],0);auto p=io->Baseline();io->proof=&p;
			if(f==0)assert((!l->Run<NLeaseIO,BadImage,NData>(*io,p,0)));
			if(f==1)assert((!l->Run<NLeaseIO,NImage,BadData>(*io,p,0)));
			if(f==2)assert((!l->Run<NLeaseIO,NImage,BadLength>(*io,p,0)));
			assert(!io->operations && !io->scopes);NoMoreLease(*l,*io);failed++;
		}
	}
	printf("PASS OFFBOARD compact104 full-K Q4 lease:%u failure/refusal cases,unknown retains ALL journals,no next IO/END/reset/retry;NOT native hardware/quality/speed proof\n",failed);
}
#endif
