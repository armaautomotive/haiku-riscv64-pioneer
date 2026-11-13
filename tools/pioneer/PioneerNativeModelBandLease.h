/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_NATIVE_MODEL_BAND_LEASE_H
#define PIONEER_NATIVE_MODEL_BAND_LEASE_H
#if defined(PIONEER_NATIVE_RECURRENT_OUT_BAND_OWNER_INSPECTION) && (PIONEER_NATIVE_RECURRENT_OUT_BAND_OWNER_INSPECTION != 1 \
 || defined(_KERNEL_MODE) || defined(PIONEER_NATIVE_MODEL_BAND_DIAGNOSTIC) || defined(PIONEER_DRIVER_BUILD_TAG) \
 || defined(PIONEER_Q38_TOKEN_NATIVE_CONTEXT) || defined(TT_PIONEER_Q38_WORK_MODE) || defined(PIONEER_Q38_FULL_MODEL_CPU_REFERENCE) || defined(__FAST_MATH__))
#error Recurrent output band owner is offline inspection only
#endif
#if defined(PIONEER_Q38_WORK_INFERENCE) || defined(PIONEER_Q38_CPU_VALIDATION) || (defined(_KERNEL_MODE) && !defined(PIONEER_NATIVE_MODEL_BAND_DIAGNOSTIC))
#error Learned-band lease requires its distinct diagnostic80 role,never Work/validation
#endif
#if defined(PIONEER_NATIVE_QGWD_DENSE_BAND_DIAGNOSTIC) && (PIONEER_NATIVE_QGWD_DENSE_BAND_DIAGNOSTIC != 1 \
 || !defined(PIONEER_NATIVE_MODEL_BAND_DIAGNOSTIC) || !defined(_KERNEL_MODE) || !defined(TT_PIONEER_DRIVER_RELOAD) \
 || defined(TT_PIONEER_Q38_WORK_MODE) || defined(PIONEER_Q38_FULL_MODEL_CPU_REFERENCE) || defined(__FAST_MATH__) \
 || PIONEER_DRIVER_BUILD_TAG != 0x2026111000010140ULL)
#error QGWD dense-band owner requires its separate fixed kernel diagnostic role
#endif
#if defined(PIONEER_NATIVE_RECURRENT_BAND_DIAGNOSTIC) && (PIONEER_NATIVE_RECURRENT_BAND_DIAGNOSTIC != 1 \
 || defined(PIONEER_NATIVE_QGWD_DENSE_BAND_DIAGNOSTIC) || !defined(PIONEER_NATIVE_MODEL_BAND_DIAGNOSTIC) \
 || !defined(_KERNEL_MODE) || !defined(TT_PIONEER_DRIVER_RELOAD) || defined(TT_PIONEER_Q38_WORK_MODE) \
 || defined(PIONEER_Q38_FULL_MODEL_CPU_REFERENCE) || defined(__FAST_MATH__) || PIONEER_DRIVER_BUILD_TAG != 0x2026111200010147ULL)
#error Recurrent-band owner requires its separate fixed kernel diagnostic role
#endif
#if defined(PIONEER_NATIVE_RECURRENT_OUT_BAND_DIAGNOSTIC) && (PIONEER_NATIVE_RECURRENT_OUT_BAND_DIAGNOSTIC != 1 \
 || defined(PIONEER_NATIVE_RECURRENT_OUT_BAND_OWNER_INSPECTION) || defined(PIONEER_NATIVE_RECURRENT_BAND_DIAGNOSTIC) \
 || defined(PIONEER_NATIVE_QGWD_DENSE_BAND_DIAGNOSTIC) || !defined(PIONEER_NATIVE_MODEL_BAND_DIAGNOSTIC) \
 || !defined(_KERNEL_MODE) || !defined(TT_PIONEER_DRIVER_RELOAD) || defined(TT_PIONEER_Q38_WORK_MODE) \
 || defined(PIONEER_Q38_FULL_MODEL_CPU_REFERENCE) || defined(__FAST_MATH__) || PIONEER_DRIVER_BUILD_TAG != 0x2026111200010148ULL)
#error Recurrent output band owner requires its separate fixed kernel diagnostic role
#endif
#if defined(PIONEER_NATIVE_MODEL_BAND_DIAGNOSTIC) && (!defined(_KERNEL_MODE) || PIONEER_NATIVE_MODEL_BAND_DIAGNOSTIC != 1 || !defined(TT_PIONEER_DRIVER_RELOAD) \
 || (PIONEER_DRIVER_BUILD_TAG != 0x2026110400010080ULL && (!defined(PIONEER_NATIVE_QGWD_DENSE_BAND_DIAGNOSTIC) || PIONEER_DRIVER_BUILD_TAG != 0x2026111000010140ULL) \
 && (!defined(PIONEER_NATIVE_RECURRENT_BAND_DIAGNOSTIC) || PIONEER_DRIVER_BUILD_TAG != 0x2026111200010147ULL) \
 && (!defined(PIONEER_NATIVE_RECURRENT_OUT_BAND_DIAGNOSTIC) || PIONEER_DRIVER_BUILD_TAG != 0x2026111200010148ULL)))
#error Full-K kernel owner requires its explicit distinct diagnostic80 role
#endif
#include "../../src/add-ons/kernel/drivers/misc/BlackholeDramSamplesProbe.h"
#include "../../src/add-ons/kernel/drivers/misc/PioneerNativeBfpProtocol.h"
#include "PioneerNativeBfpBandLayout.h"
#include "PioneerNativeBfpModelBandData.h" // PRIVATE generated learned bytes,not committed
// Separate FIXED learned-weight full-K/broadcast/four-fidelity lifecycle. Never accepts old
// NFB1 completion or a runtime payload/code/address. Kernel compilation is
// reserved for its distinct diagnostic80 adapter;never installed Work.
// Allocate this0.20MB saved-state owner on the HEAP,not a kernel stack.
// No destructor IO. UNKNOWN retains every saved span/TLB and never retries.
class PioneerNativeModelBandLease {
	using B=PioneerNativeBfpBandLayout;
#if defined(PIONEER_NATIVE_RECURRENT_OUT_BAND_OWNER_INSPECTION) || defined(PIONEER_NATIVE_RECURRENT_OUT_BAND_DIAGNOSTIC)
	enum {FixedWidth=6144,FixedSavedWords=59972};
#else
	enum {FixedWidth=5120,FixedSavedWords=50500};
#endif
	static_assert(FixedSavedWords==1+1024+2048+B::Tiles(FixedWidth)*(B::WeightStride+B::InputStride)/4+64+3,
		"snapshot must cover boot, code, stack, exact input/weight extents, output and status");
	uint32_t saved[FixedSavedWords]={},tlb[2][3]={};
	unsigned width=0,sample=UINT32_MAX;
	bool attempted=false,owned=false,modified=false;
	PioneerNativeBfpResult result={};
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
		if(result.phase!=0x7000c || result.count!=(0x04000000|width) || result.marker!=0x3342464e)return false;
		io.Pause();result.pc[0]=io.BfpPcRead();io.Pause();result.pc[1]=io.BfpPcRead();
		for(unsigned pc:result.pc)if(pc!=0x70038 && pc!=0x7003c)return false;
		return Scope(io,baseline,0x46800) && io.BfpRead(0x17ffc0)==result.phase
			&& io.BfpRead(0x17ffc4)==result.count && io.BfpRead(0x17ffc8)==result.marker;
	}
public:
	static constexpr unsigned MaxSavedWords=FixedSavedWords;
	static constexpr unsigned SavedWords(unsigned k) {
		return k==FixedWidth?FixedSavedWords:0;
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
	template<class Image> static uint32_t StagedWord(unsigned selection,unsigned k,unsigned index) {
		const uint32_t a=Address(k,index);
		if(a==0x6000)return Image::Boot;
		if(a>=0x70000 && a<0x71000)return Image::Word((a-0x70000)/4);
		if(a>=B::Output && a<B::Output+256)return 0x7fc00001;
		return PioneerNativeBfpModelBandData::Word(selection,k,a); // headers/stack/status zero
	}
	const PioneerNativeBfpResult& Result()const{return result;}
	void Abandon(){if(owned)Unknown();} // CPU metadata only
	template<class IO,class Image> bool Run(IO& io,tt_dram_samples_result& baseline,unsigned selection) {
		static_assert(Image::Width==FixedWidth && Image::Phases==4,"fixed full-K ALL-four-fidelity image only");
		if(attempted || selection>=3)return false;
		const unsigned k=FixedWidth;attempted=true;width=k;sample=selection;result.state=PIONEER_NATIVE_BFP_REFUSED;
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
		for(unsigned i=1;i<words;i++)io.BfpWrite(Address(k,i),StagedWord<Image>(sample,k,i));
		if(!Scope(io,baseline,0x47800))return Unknown();
		io.BfpWrite(0x6000,StagedWord<Image>(sample,k,0)); // publish boot LAST
		for(unsigned i=0;i<words;i++)if(io.BfpRead(Address(k,i))!=StagedWord<Image>(sample,k,i))return Unknown();
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
			if(marker==0x3342464e){complete=Terminal(io,baseline);break;}
			io.Pause();
		}
		if(!complete)return Unknown();
		for(unsigned i=0;i<64;i++)result.output[i]=io.BfpRead(B::Output+i*4);
		if(!Terminal(io,baseline))return Unknown();
		result.completed=1;result.state=PIONEER_NATIVE_BFP_RESULT;return true;
	}
	template<class IO> bool End(IO& io,tt_dram_samples_result& baseline) {
		if(!owned || !modified || result.state!=PIONEER_NATIVE_BFP_RESULT || !result.completed)return false;
		if(!Terminal(io,baseline))return Unknown();
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
#if defined(PIONEER_NATIVE_RECURRENT_OUT_BAND_OWNER_INSPECTION) || defined(PIONEER_NATIVE_RECURRENT_OUT_BAND_DIAGNOSTIC)
static_assert(PioneerNativeModelBandLease::SavedWords(6144)==PioneerNativeModelBandLease::MaxSavedWords,"exact bounded heap snapshot");
#else
static_assert(PioneerNativeModelBandLease::SavedWords(5120)==PioneerNativeModelBandLease::MaxSavedWords,"exact bounded heap snapshot");
#endif
#endif
