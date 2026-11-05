/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_NATIVE_BFP_LEASE_H
#define PIONEER_NATIVE_BFP_LEASE_H
#include "BlackholeDramSamplesProbe.h"
#include "PioneerNativeBfpProtocol.h"
#include "PioneerNativeBfpFixture.h"
#ifdef PIONEER_NATIVE_EXACT_Q4_AFFINE_DIAGNOSTIC
#if PIONEER_NATIVE_EXACT_Q4_AFFINE_DIAGNOSTIC != 1 || defined(PIONEER_NATIVE_EXACT_Q4_DIAGNOSTIC)
#error Distinct fixed full-K affine lease role required
#endif
#endif
#if defined(PIONEER_NATIVE_EXACT_Q4_DIAGNOSTIC) || defined(PIONEER_NATIVE_EXACT_Q4_AFFINE_DIAGNOSTIC)
#ifdef PIONEER_NATIVE_EXACT_Q4_DIAGNOSTIC
#if PIONEER_NATIVE_EXACT_Q4_DIAGNOSTIC != 1
#error Distinct fixed exact-Q4 diagnostic mode required
#endif
#endif
#include "PioneerNativeExactQ4Fixture.h"
#endif

// Concrete sole-tile lifecycle used by the native adapter. Saved L1/TLBs
// remain in its retained HEAP owner. No destructor IO, guessed completion,
// retry, watchdog reset, numeric fallback or automatic cleanup on UNKNOWN.
class PioneerNativeBfpLease {
#ifdef PIONEER_NATIVE_EXACT_Q4_AFFINE_DIAGNOSTIC
	using Fixture=PioneerNativeExactQ4Fixture; // initial disposable DATA;shader replaces every consumed panel
	static constexpr uint32_t DoneMarker=0x31413451,FaultMarker=0x46413451;
	static constexpr unsigned RomWords=2048,ExtraWords=300;
#else
	static constexpr unsigned RomWords=1024,ExtraWords=0;
#ifdef PIONEER_NATIVE_EXACT_Q4_DIAGNOSTIC
	using Fixture=PioneerNativeExactQ4Fixture;
	static constexpr uint32_t DoneMarker=0x31433451,FaultMarker=0x46433451;
#else
	using Fixture=PioneerNativeBfpFixture;
	static constexpr uint32_t DoneMarker=0x3142464e,FaultMarker=0x4642464e;
#endif
#endif
	uint32_t saved[2524+RomWords+ExtraWords]={},tlb[2][3]={};
	unsigned bits=0;
	bool attempted=false,owned=false,modified=false;
	PioneerNativeBfpResult result={};
	bool Unknown() { result.state=PIONEER_NATIVE_BFP_UNKNOWN;result.cold=1;return false; }
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
		if(result.phase!=0x7000c || result.count!=2 || result.marker!=DoneMarker)return false;
		io.Pause();result.pc[0]=io.BfpPcRead();io.Pause();result.pc[1]=io.BfpPcRead();
		// PC snapshots are frontend/speculative. BOTH fixed park addresses
		// contain self-loop guards; neither snapshot substitutes for status
		// written AFTER two complete device-side FPU drains/output copies.
		for(unsigned pc:result.pc)if(pc!=0x70038 && pc!=0x7003c)return false;
		return Scope(io,baseline,0x46800)
			&& io.BfpRead(0x17ffc0)==result.phase && io.BfpRead(0x17ffc4)==2 && io.BfpRead(0x17ffc8)==result.marker;
	}
public:
	static constexpr unsigned SavedWords=2524+RomWords+ExtraWords;
	static uint32_t Address(unsigned index) {
		if(!index)return 0x6000;
		index--;if(index<RomWords)return 0x70000+index*4;
		index-=RomWords;if(index<2048)return 0x6e000+index*4;
		index-=2048;if(index<276)return 0x8fff0+index*4;
		index-=276;if(index<132)return 0x90ff0+index*4;
		index-=132;if(index<64)return 0x92000+index*4;
		index-=64;
#ifdef PIONEER_NATIVE_EXACT_Q4_AFFINE_DIAGNOSTIC
		if(index<20)return 0x93ff0+index*4;
		index-=20;if(index<20)return 0x94070+index*4;
		index-=20;if(index<260)return 0x95ff0+index*4;
		index-=260;
#endif
		return index<3?0x17ffc0+index*4:UINT32_MAX;
	}
	template<class Image> static uint32_t StagedWord(unsigned bits,unsigned index) {
		const uint32_t address=Address(index);
		if(address==0x6000)return Image::Boot;
		if(address>=0x70000 && address<0x70000+RomWords*4)return Image::Word(bits,(address-0x70000)/4);
		if(address>=0x90000 && address<0x90000+Fixture::WeightBytes(bits))
			return Fixture::WeightWord(bits,(address-0x90000)/4);
		if(address>=0x91000 && address<0x91200)return Fixture::InputWord((address-0x91000)/4);
		if(address>=0x92000 && address<0x92100)return 0x7fc00001; // invalid until device writes BOTH copies
		return 0; // private stack,headers,unused weight/code padding,status
	}
	const PioneerNativeBfpResult& Result() const { return result; }
	void Abandon() { if(owned)Unknown(); } // CPU metadata ONLY; no IO on close/failed reply
	template<class IO,class Image> bool Run(IO& io,tt_dram_samples_result& baseline,unsigned format) {
#if defined(PIONEER_NATIVE_EXACT_Q4_DIAGNOSTIC) || defined(PIONEER_NATIVE_EXACT_Q4_AFFINE_DIAGNOSTIC)
		if(format!=4)return false; // no old general-BFP8 route in this family
#endif
		if(attempted || (format!=4 && format!=8))return false;
		attempted=true;result.state=PIONEER_NATIVE_BFP_REFUSED;bits=format;
		if(baseline.version!=1 || baseline.state!=TT_DRAM_SAMPLE_ROUTES_OK || baseline.routes_verified!=7
			|| !baseline.tlb_restored || !baseline.data_restored || baseline.recovery_required || !BlackholeDramBuildRoutes(baseline))return false;
		for(unsigned s=0;s<2;s++)for(unsigned w=0;w<3;w++) {
			tlb[s][w]=io.DramTlbRead(s,w);if(w==2 && (tlb[s][w]&0xffffc000))return false;
		}
		owned=true;
		if(!BlackholeDramAllCoresHeld(io,baseline) || !Select(io) || !Scope(io,baseline,0x47800))return Unknown();
		for(unsigned i=0;i<3;i++)result.controls[i]=io.BfpControlRead(i);
		if((result.controls[0]&1) || result.controls[1] || result.controls[2])return Unknown();
		for(unsigned i=0;i<SavedWords;i++)saved[i]=io.BfpRead(Address(i));
		if(!Scope(io,baseline,0x47800))return Unknown();
		modified=true;result.l1_restored=0;
		// Boot is published LAST, after every other owned span is initialized.
		for(unsigned i=1;i<SavedWords;i++)io.BfpWrite(Address(i),StagedWord<Image>(bits,i));
		if(!Scope(io,baseline,0x47800))return Unknown();
		io.BfpWrite(0x6000,StagedWord<Image>(bits,0));
		for(unsigned i=0;i<SavedWords;i++)if(io.BfpRead(Address(i))!=StagedWord<Image>(bits,i))return Unknown();
		if(!Scope(io,baseline,0x47800))return Unknown();
		// Private DISPOSABLE reset/idle context; never a model/other live tile.
		io.BfpResetWrite(0x87ffff);io.Pause();
		if(!Scope(io,baseline,0x87ffff))return Unknown();
		result.started=1; // set BEFORE possibly dropped start
		io.BfpResetWrite(0x46800);
		const uint64_t began=io.BfpNow();
		bool complete=false;
		for(unsigned attempt=0;attempt<20000;attempt++) {
			result.polls++;
			if(!Scope(io,baseline,0x46800))return Unknown();
			const uint64_t now=io.BfpNow();if(now<began || now-began>UINT64_C(2000000000))return Unknown();
			const uint32_t marker=io.BfpRead(0x17ffc8);
			if(marker==FaultMarker)return Unknown();
			if(marker==DoneMarker) { complete=Terminal(io,baseline);break; }
			io.Pause();
		}
		if(!complete)return Unknown();
		for(unsigned i=0;i<64;i++)result.output[i]=io.BfpRead(0x92000+i*4);
		if(!Terminal(io,baseline))return Unknown();
		result.completed=1;result.state=PIONEER_NATIVE_BFP_RESULT;
		return true; // parked STILL OWNED; only explicit validated END can stop/restore
	}
	template<class IO> bool End(IO& io,tt_dram_samples_result& baseline) {
		if(!owned || !modified || result.state!=PIONEER_NATIVE_BFP_RESULT || !result.completed)return false;
		if(!Terminal(io,baseline))return Unknown();
		io.BfpResetWrite(0x87ffff);io.Pause();if(!Scope(io,baseline,0x87ffff))return Unknown();
		io.BfpResetWrite(0x47800);io.Pause();if(!Scope(io,baseline,0x47800))return Unknown();
		result.held=1;
		if(!BlackholeDramAllCoresHeld(io,baseline) || !Select(io) || !Scope(io,baseline,0x47800))return Unknown();
		for(unsigned i=1;i<SavedWords;i++)io.BfpWrite(Address(i),saved[i]);
		if(!Scope(io,baseline,0x47800))return Unknown();
		io.BfpWrite(0x6000,saved[0]);
		for(unsigned i=0;i<SavedWords;i++)if(io.BfpRead(Address(i))!=saved[i])return Unknown();
		if(!Scope(io,baseline,0x47800))return Unknown();
		result.l1_restored=1;
		for(unsigned s=0;s<2;s++)for(unsigned w=0;w<3;w++)io.DramTlbWrite(s,w,tlb[s][w]);
		for(unsigned s=0;s<2;s++)for(unsigned w=0;w<3;w++)if(io.DramTlbRead(s,w)!=tlb[s][w])return Unknown();
		if(!BlackholeDramSourceStable(io,baseline))return Unknown();
		result.tlb_restored=1;result.state=PIONEER_NATIVE_BFP_CLOSED;owned=false;return true;
	}
};
#endif
