/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_NATIVE_BFP_BAND_LEASE_H
#define PIONEER_NATIVE_BFP_BAND_LEASE_H
#if defined(PIONEER_Q38_WORK_INFERENCE) || defined(PIONEER_Q38_CPU_VALIDATION) || (defined(_KERNEL_MODE) && !defined(PIONEER_NATIVE_BFP_BAND_DIAGNOSTIC) && !defined(PIONEER_NATIVE_BFP4_BAND_DIAGNOSTIC) && !defined(PIONEER_NATIVE_BFP4_TIMED_DIAGNOSTIC))
#error Full-K band lease is an offboard owner prototype,not an admitted driver/Work/validation path
#endif
#if defined(PIONEER_NATIVE_BFP_BAND_DIAGNOSTIC) && (!defined(_KERNEL_MODE) || PIONEER_NATIVE_BFP_BAND_DIAGNOSTIC != 1 || !defined(TT_PIONEER_DRIVER_RELOAD) || PIONEER_DRIVER_BUILD_TAG != 0x2026110400010079ULL || defined(PIONEER_NATIVE_BFP4_TIMED_DIAGNOSTIC))
#error Full-K kernel owner requires its explicit distinct diagnostic79 role
#endif
#if defined(PIONEER_NATIVE_BFP4_BAND_DIAGNOSTIC) && (!defined(_KERNEL_MODE) || PIONEER_NATIVE_BFP4_BAND_DIAGNOSTIC != 1 || !defined(TT_PIONEER_DRIVER_RELOAD) || PIONEER_DRIVER_BUILD_TAG != 0x2026110600010118ULL || defined(PIONEER_NATIVE_BFP_BAND_DIAGNOSTIC) || defined(PIONEER_NATIVE_BFP4_TIMED_DIAGNOSTIC))
#error Full-K kernel owner requires its explicit distinct BFP4 diagnostic118 role
#endif
#if defined(PIONEER_NATIVE_BFP4_TIMED_DIAGNOSTIC) && (!defined(_KERNEL_MODE) || PIONEER_NATIVE_BFP4_TIMED_DIAGNOSTIC != 1 || !defined(TT_PIONEER_DRIVER_RELOAD) || PIONEER_DRIVER_BUILD_TAG != 0x2026110600010119ULL || defined(PIONEER_NATIVE_BFP_BAND_DIAGNOSTIC) || defined(PIONEER_NATIVE_BFP4_BAND_DIAGNOSTIC))
#error Full-K kernel owner requires its explicit distinct timed BFP4 diagnostic119 role
#endif
#include "../../src/add-ons/kernel/drivers/misc/BlackholeDramSamplesProbe.h"
#include "../../src/add-ons/kernel/drivers/misc/PioneerNativeBfpProtocol.h"
#include "PioneerNativeBfpBandFixture.h"
#include "PioneerNativeBfp4BandFixture.h"
#include "PioneerNativeBfp4BandTiming.h"
// Separate fixed full-K/broadcast lifecycle. Each role accepts only its own
// fidelity and marker, never old NFB1 or a runtime payload/code/address.
// Kernel compilation is reserved for diagnostic79 or diagnostic118; neither
// is installed Work or grants authority to execute an untested build.
// Allocate this0.66MB saved-state owner on the HEAP,not a kernel stack.
// No destructor IO. UNKNOWN retains every saved span/TLB and never retries.
template<unsigned Bits> struct PioneerNativeBfpBandPolicy;
template<> struct PioneerNativeBfpBandPolicy<8> {
	static constexpr unsigned Phases=4,Marker=0x3342464e,Timed=0;
	static uint32_t Word(unsigned k,unsigned address){return PioneerNativeBfpBandFixture::Word(k,address);}
};
template<> struct PioneerNativeBfpBandPolicy<4> {
	static constexpr unsigned Phases=2,Marker=0x3442464e,Timed=0;
	static uint32_t Word(unsigned k,unsigned address){return PioneerNativeBfp4BandFixture::Word(k,address);}
};
template<> struct PioneerNativeBfpBandPolicy<44> {
	static constexpr unsigned Phases=2,Marker=0x5442464e,Timed=1;
	static uint32_t Word(unsigned k,unsigned address){return PioneerNativeBfp4BandFixture::Word(k,address);}
};
template<unsigned Bits> class PioneerNativeBfpBandLeaseFor {
	static_assert(Bits==4 || Bits==8 || Bits==44,"only distinct fixed synthetic BFP4/BFP8 leases");
#if defined(PIONEER_NATIVE_BFP_BAND_DIAGNOSTIC)
	static_assert(Bits==8,"diagnostic79 must never admit BFP4");
#elif defined(PIONEER_NATIVE_BFP4_BAND_DIAGNOSTIC)
	static_assert(Bits==4,"diagnostic118 must never admit BFP8");
#elif defined(PIONEER_NATIVE_BFP4_TIMED_DIAGNOSTIC)
	static_assert(Bits==44,"diagnostic119 must never admit an untimed image");
#endif
	using Policy=PioneerNativeBfpBandPolicy<Bits>;
	using B=PioneerNativeBfpBandLayout;
	uint32_t saved[164164]={},tlb[2][3]={};
	unsigned width=0;
	bool attempted=false,owned=false,modified=false;
	PioneerNativeBfpResult result={};
	uint32_t timing[16]={};
	bool Unknown(){result.state=PIONEER_NATIVE_BFP_UNKNOWN;result.cold=1;return false;}
	template<class IO> bool Scope(IO& io,const tt_dram_samples_result& baseline,uint32_t reset) {
		if(!io.BfpHealthy() || !BlackholeDramSourceStable(io,baseline))return false;
		for(unsigned slot=0;slot<2;slot++)for(unsigned word=0;word<3;word++)
			if(io.DramTlbRead(slot,word)!=BlackholeDramFixedTlbWord(slot?0xffa00000:0,1,2,false,word))return false;
		return io.ResetRead()==reset && io.ResetRead()==reset;
	}
	template<class IO> bool Select(IO& io) {
		return BlackholeDramConfigure(io,0,0,1,2,false) && BlackholeDramConfigure(io,1,0xffa00000,1,2,false);
	}
	template<class IO> bool Terminal(IO& io,const tt_dram_samples_result& baseline) {
		if(!Scope(io,baseline,0x46800))return false;
		result.phase=io.BfpRead(0x17ffc0);result.count=io.BfpRead(0x17ffc4);result.marker=io.BfpRead(0x17ffc8);
		if(result.phase!=0x7000c || result.count!=((Policy::Phases<<24)|width) || result.marker!=Policy::Marker)return false;
		io.Pause();result.pc[0]=io.BfpPcRead();io.Pause();result.pc[1]=io.BfpPcRead();
		for(unsigned pc:result.pc)if(pc!=0x70038 && pc!=0x7003c)return false;
		return Scope(io,baseline,0x46800) && io.BfpRead(0x17ffc0)==result.phase
			&& io.BfpRead(0x17ffc4)==result.count && io.BfpRead(0x17ffc8)==result.marker;
	}
public:
	static constexpr unsigned MaxSavedWords=164164;
	static constexpr unsigned SavedWords(unsigned k) {
		return B::Width(k)?1+1024+2048+B::Tiles(k)*(B::WeightStride+B::InputStride)/4+64+3:0;
	}
	static uint32_t Address(unsigned k,unsigned index) {
		if(index>=SavedWords(k))return UINT32_MAX;
		if(!index)return 0x6000;
		index--;if(index<1024)return 0x70000+index*4;
		index-=1024;if(index<2048)return 0x6e000+index*4;
		index-=2048;unsigned words=B::Tiles(k)*B::WeightStride/4;
		if(index<words)return B::Weight-16+index*4;
		index-=words;words=B::Tiles(k)*B::InputStride/4;
		if(index<words)return B::Input-16+index*4;
		index-=words;if(index<64)return B::Output+index*4;
		return 0x17ffc0+(index-64)*4;
	}
	template<class Image> static uint32_t StagedWord(unsigned k,unsigned index) {
		const uint32_t a=Address(k,index);
		if(a==0x6000)return Image::Boot;
		if(a>=0x70000 && a<0x71000)return Image::Word((a-0x70000)/4);
		if(a>=B::Output && a<B::Output+256)return 0x7fc00001;
		return Policy::Word(k,a); // fixed synthetic data,headers/stack/status zero
	}
	const PioneerNativeBfpResult& Result()const{return result;}
	const uint32_t* Timing()const{return timing;}
	void Abandon(){if(owned)Unknown();} // CPU metadata only
	template<class IO,class Image> bool Run(IO& io,tt_dram_samples_result& baseline,unsigned k) {
		static_assert(B::Width(Image::Width) && Image::Phases==Policy::Phases,"fixed full-K and exact format fidelity only");
		if(attempted || !B::Width(k) || k!=Image::Width)return false;
		attempted=true;width=k;result.state=PIONEER_NATIVE_BFP_REFUSED;
		if(baseline.version!=1 || baseline.state!=TT_DRAM_SAMPLE_ROUTES_OK || baseline.routes_verified!=7
			|| !baseline.tlb_restored || !baseline.data_restored || baseline.recovery_required || !BlackholeDramBuildRoutes(baseline))return false;
		for(unsigned s=0;s<2;s++)for(unsigned w=0;w<3;w++) {
			tlb[s][w]=io.DramTlbRead(s,w);if(w==2 && (tlb[s][w]&0xffffc000))return false;
		}
		owned=true;
		if(!BlackholeDramAllCoresHeld(io,baseline) || !Select(io) || !Scope(io,baseline,0x47800))return Unknown();
		for(unsigned i=0;i<3;i++)result.controls[i]=io.BfpControlRead(i);
		if((result.controls[0]&1) || result.controls[1] || result.controls[2])return Unknown();
		const unsigned words=SavedWords(k);
		for(unsigned i=0;i<words;i++)saved[i]=io.BfpRead(Address(k,i));
		if(!Scope(io,baseline,0x47800))return Unknown();
		modified=true;
		for(unsigned i=1;i<words;i++)io.BfpWrite(Address(k,i),StagedWord<Image>(k,i));
		if(!Scope(io,baseline,0x47800))return Unknown();
		io.BfpWrite(0x6000,StagedWord<Image>(k,0)); // publish boot LAST
		for(unsigned i=0;i<words;i++)if(io.BfpRead(Address(k,i))!=StagedWord<Image>(k,i))return Unknown();
		if(!Scope(io,baseline,0x47800))return Unknown();
		io.BfpResetWrite(0x87ffff);io.Pause();if(!Scope(io,baseline,0x87ffff))return Unknown();
		result.started=1;io.BfpResetWrite(0x46800);
		const uint64_t began=io.BfpNow();bool complete=false;
		for(unsigned attempt=0;attempt<20000;attempt++) {
			result.polls++;
			if(!Scope(io,baseline,0x46800))return Unknown();
			const uint64_t now=io.BfpNow();if(now<began || now-began>UINT64_C(2000000000))return Unknown();
			const uint32_t marker=io.BfpRead(0x17ffc8);
			if(marker==0x4542464e)return Unknown();
			if(marker==Policy::Marker){complete=Terminal(io,baseline);break;}
			io.Pause();
		}
		if(!complete)return Unknown();
		for(unsigned i=0;i<64;i++)result.output[i]=io.BfpRead(B::Output+i*4);
		if(Policy::Timed){
			for(unsigned i=0;i<16;i++)timing[i]=io.BfpRead(PioneerNativeBfp4BandTiming::Address+i*4);
			if(PioneerNativeBfp4BandTiming::State(timing,k,0)!=1 || PioneerNativeBfp4BandTiming::State(timing+8,k,1)!=1
				|| !PioneerNativeBfp4BandTiming::Later(timing+12,timing+6))return Unknown();
		}
		if(!Terminal(io,baseline))return Unknown();
		result.completed=1;result.state=PIONEER_NATIVE_BFP_RESULT;return true;
	}
	template<class IO> bool End(IO& io,tt_dram_samples_result& baseline) {
		if(!owned || !modified || result.state!=PIONEER_NATIVE_BFP_RESULT || !result.completed)return false;
		if(!Terminal(io,baseline))return Unknown();
		if(Policy::Timed)for(unsigned i=0;i<16;i++)if(io.BfpRead(PioneerNativeBfp4BandTiming::Address+i*4)!=timing[i])return Unknown();
		io.BfpResetWrite(0x87ffff);io.Pause();if(!Scope(io,baseline,0x87ffff))return Unknown();
		io.BfpResetWrite(0x47800);io.Pause();if(!Scope(io,baseline,0x47800))return Unknown();
		result.held=1;
		if(!BlackholeDramAllCoresHeld(io,baseline) || !Select(io) || !Scope(io,baseline,0x47800))return Unknown();
		const unsigned words=SavedWords(width);
		for(unsigned i=1;i<words;i++)io.BfpWrite(Address(width,i),saved[i]);
		if(!Scope(io,baseline,0x47800))return Unknown();
		io.BfpWrite(0x6000,saved[0]);
		for(unsigned i=0;i<words;i++)if(io.BfpRead(Address(width,i))!=saved[i])return Unknown();
		if(!Scope(io,baseline,0x47800))return Unknown();
		result.l1_restored=1;
		for(unsigned s=0;s<2;s++)for(unsigned w=0;w<3;w++)io.DramTlbWrite(s,w,tlb[s][w]);
		for(unsigned s=0;s<2;s++)for(unsigned w=0;w<3;w++)if(io.DramTlbRead(s,w)!=tlb[s][w])return Unknown();
		if(!BlackholeDramSourceStable(io,baseline))return Unknown();
		result.tlb_restored=1;result.state=PIONEER_NATIVE_BFP_CLOSED;owned=false;return true;
	}
};
using PioneerNativeBfpBandLease=PioneerNativeBfpBandLeaseFor<8>;
using PioneerNativeBfp4BandLease=PioneerNativeBfpBandLeaseFor<4>;
using PioneerNativeBfp4TimedLease=PioneerNativeBfpBandLeaseFor<44>;
#if defined(PIONEER_NATIVE_BFP4_TIMED_DIAGNOSTIC)
static_assert(PioneerNativeBfp4TimedLease::SavedWords(17408)==PioneerNativeBfp4TimedLease::MaxSavedWords,"exact bounded heap snapshot");
#elif defined(PIONEER_NATIVE_BFP4_BAND_DIAGNOSTIC)
static_assert(PioneerNativeBfp4BandLease::SavedWords(17408)==PioneerNativeBfp4BandLease::MaxSavedWords,"exact bounded heap snapshot");
#else
static_assert(PioneerNativeBfpBandLease::SavedWords(17408)==PioneerNativeBfpBandLease::MaxSavedWords,"exact bounded heap snapshot");
#endif
#endif
