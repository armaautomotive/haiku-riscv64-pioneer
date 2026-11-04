/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_NATIVE_Q4_FPU_LEASE_H
#define PIONEER_NATIVE_Q4_FPU_LEASE_H
#if defined(PIONEER_Q38_WORK_INFERENCE) || defined(TT_PIONEER_Q38_WORK_MODE) || defined(PIONEER_Q38_CPU_VALIDATION) || defined(PIONEER_Q38_FULL_MODEL_CPU_REFERENCE) || defined(PIONEER_NATIVE_MODEL_BAND_DIAGNOSTIC) || defined(PIONEER_NATIVE_DRAM_MODEL_BAND_DIAGNOSTIC) || defined(PIONEER_NATIVE_DRAM_LOAD_DIAGNOSTIC) || (defined(_KERNEL_MODE) && !defined(PIONEER_NATIVE_Q4_FPU_DIAGNOSTIC))
#error Q4 FPU lease requires its separate diagnostic85,never Work/reference/old roles
#endif
#if defined(PIONEER_NATIVE_Q4_FPU_DIAGNOSTIC) && (!defined(_KERNEL_MODE) || PIONEER_NATIVE_Q4_FPU_DIAGNOSTIC != 1 || !defined(TT_PIONEER_DRIVER_RELOAD) || PIONEER_DRIVER_BUILD_TAG != 0x2026110400010085ULL)
#error Exact kernel diagnostic85 reload identity required
#endif
#include "../../src/add-ons/kernel/drivers/misc/BlackholeComputeNocProbe.h"
#include "../../src/add-ons/kernel/drivers/misc/PioneerNativeQ4FpuProtocol.h"
#include "PioneerNativeQ4FpuLayout.h"
// Heap-only49488B snapshot. No destructor IO,implicit END,arbitrary source,
// GDDR/NIU write,reset retry or cleanup after uncertainty. Every primitive
// must bind THIS exact lease/proof/backing/FD/thread/team/generation/held
// semaphore/reservation/PCI identity/actual VM views. Native adapter required.
class PioneerNativeQ4FpuLease {
	using L=PioneerNativeQ4FpuLayout;using V=PioneerNativeQ4FpuVariant;
	uint32_t l1[L::L1Words]={},tlb[2][3]={},counters[9]={},command[10]={};
	tt_dram_samples_result routes={};const tt_dram_samples_result* proof=nullptr;
	unsigned selection=UINT32_MAX,reset=0x47800;bool attempted=false,owned=false,failed=false;
	PioneerNativeQ4FpuResult result={};
	template<class IO> bool Fail(IO& io) {
		result.compute.state=PIONEER_NATIVE_BFP_UNKNOWN;result.compute.cold=1;
		if(!failed){failed=true;io.NativeRetain();}return false; // MEMORY ONLY
	}
	template<class IO> bool Access(IO& io) {
		return attempted && owned && !failed && proof && io.NativeScope(this,proof,l1,L1Words);
	}
	template<class IO> bool Sources(IO& io) {
		if(!Access(io))return false;
		for(unsigned i=0;i<16;i++){uint32_t v=0;if(!io.NativeSourceRead(i,v) || v!=routes.source[i])return false;}
		return true;
	}
	template<class IO> bool Set(IO& io,unsigned slot,unsigned base,unsigned x,unsigned y,bool ro) {
		if(!Sources(io))return false;
		for(unsigned i=0;i<3;i++) {
			uint32_t v=0,expected=BlackholeDramFixedTlbWord(base,x,y,ro,i);
			if(!io.NativeTlbWrite(slot,i,expected) || !io.NativeTlbRead(slot,i,v) || v!=expected)return false;
		}
		return Sources(io);
	}
	template<class IO> bool Select(IO& io){return Set(io,0,0,1,2,false) && Set(io,1,0xffa00000,1,2,false);}
	template<class IO> bool AllHeld(IO& io) {
		const unsigned xs[]={1,2,3,4,5,6,7,10,11,12,13,14};
		for(unsigned x:xs)for(unsigned y=2;y<=11;y++) {
			if(!Set(io,1,0xffa00000,x,y,true))return false;
			for(unsigned i=0;i<2;i++){uint32_t v=0;if(!io.NativeResetRead(v) || v!=0x47800)return false;}
		}
		return Select(io);
	}
	static unsigned Counter(unsigned i){const unsigned n[]={1,2,3,4,5,8,10,12,14};return 0x200+n[i]*4;}
	template<class IO> bool Niu(IO& io,bool idle) {
		for(unsigned i=0;i<16;i++){uint32_t v=0;if(!io.NativeNiuRead(BlackholeDramSourceOffset(i),v) || v!=routes.source[i])return false;}
		uint32_t node=0,shape=0,logical=0,atomic=0;
		if(!io.NativeNiuRead(0x44,node) || !io.NativeNiuRead(0x48,shape) || !io.NativeNiuRead(0x148,logical)
			|| !io.NativeNiuRead(0x178,atomic) || (node&0xfff)!=(BlackholeDramSourceX(routes.source,1)|(BlackholeDramSourceY(routes.source,2)<<6))
			|| ((node>>12)&127)!=17 || ((node>>19)&127)!=12 || (shape&0xffffff00)!=0x10000 || (logical&0xfff)!=129 || (atomic&65535))return false;
		if(!idle)return true;
		for(unsigned i=0;i<4;i++){uint32_t v=0;if(!io.NativeNiuRead(0x40+i*0x800,v) || v)return false;}
		uint32_t available=0,error=0;
		if(!io.NativeNiuRead(0x64,available) || available || !io.NativeNiuRead(0x68,error) || (error&15))return false;
		for(unsigned i=0;i<32;i++){uint32_t v=0;if(!io.NativeNiuRead(0x240+i*4,v) || (v&255))return false;}
		return true;
	}
	template<class IO> bool Guard(IO& io,bool idle=true) {
		if(!Sources(io))return false;
		for(unsigned s=0;s<2;s++)for(unsigned i=0;i<3;i++){uint32_t v=0;if(!io.NativeTlbRead(s,i,v) || v!=BlackholeDramFixedTlbWord(s?0xffa00000:0,1,2,false,i))return false;}
		for(unsigned i=0;i<2;i++){uint32_t v=0;if(!io.NativeResetRead(v) || v!=reset)return false;}
		for(unsigned i=0;i<3;i++){uint32_t v=0;if(!io.NativeControlRead(i,v) || v!=result.compute.controls[i])return false;}
		return Niu(io,idle) && Sources(io);
	}
	template<class IO> bool Counters(IO& io) {
		for(unsigned i=0;i<9;i++){uint32_t v=0;if(!io.NativeNiuRead(Counter(i),v) || uint32_t(v-counters[i])!=0)return false;}
		return true; // no NoC commands in this image;NEVER rewind counters
	}
	template<class IO> bool Terminal(IO& io) {
		if(!Guard(io))return false;
		auto& r=result.compute;
		if(!io.NativeL1Read(0x17ffc0,r.phase) || !io.NativeL1Read(0x17ffc4,r.count) || !io.NativeL1Read(0x17ffc8,r.marker)
			|| r.phase!=0x7000c || r.count!=V::Count || r.marker!=V::Marker)return false;
		for(unsigned i=0;i<2;i++)if(!io.NativePause() || !io.NativePcRead(r.pc[i]) || (r.pc[i]!=0x70038 && r.pc[i]!=0x7003c))return false;
		return Guard(io) && Counters(io);
	}
public:
	PioneerNativeQ4FpuLease()=default;
	PioneerNativeQ4FpuLease(const PioneerNativeQ4FpuLease&)=delete;
	PioneerNativeQ4FpuLease& operator=(const PioneerNativeQ4FpuLease&)=delete;
	static constexpr unsigned L1Words=L::L1Words;
	static unsigned L1Address(unsigned i){return L::Address(i);}
	const PioneerNativeQ4FpuResult& Result()const{return result;}
	bool BoundTo(const tt_dram_samples_result* p,const uint32_t* a,unsigned n)const{return p && p==proof && a==l1 && n==L1Words;}
	void Abandon(){if(owned){failed=true;result.compute.state=PIONEER_NATIVE_BFP_UNKNOWN;result.compute.cold=1;}}
	template<class Image,class Data> static uint32_t Stage(unsigned sample,unsigned i) {
		unsigned a=L1Address(i);if(a==0x6000)return Image::Boot;
		if(a>=0x70000 && a<0x72000)return Image::Word((a-0x70000)/4);
		if(a>=L::Output && a<L::Output+256)return 0x7fc00001;
		return Data::Word(sample,a);
	}
	template<class IO,class Image,class Data> bool Run(IO& io,const tt_dram_samples_result& baseline,unsigned sample) {
		static_assert(Image::Width==256 && Image::Rows==32 && Image::Phases==4 && Image::LayoutRole==85 && Image::Bytes==7036,"exact QFP1 image only");
		static_assert(Data::Version==1 && Data::Width==256 && Data::Rows==32 && Data::Samples==3 && Data::Role==85,"exact compiled diagnostic data only");
		if(attempted || sample>=3)return false;
		attempted=true;result.compute.state=PIONEER_NATIVE_BFP_REFUSED;
		if(baseline.version!=1 || baseline.state!=TT_DRAM_SAMPLE_ROUTES_OK || baseline.routes_verified!=7
			|| !baseline.tlb_restored || !baseline.data_restored || baseline.recovery_required || (baseline.source[0]&0x10000))return false;
		routes=baseline;if(!BlackholeDramBuildRoutes(routes))return false;
		for(unsigned bank=0;bank<7;bank++)for(unsigned i=0;i<2;i++)if(routes.translated[bank][i]!=baseline.translated[bank][i])return false;
		proof=&baseline;selection=sample;owned=true;result.phase=1;
		if(!Sources(io))return Fail(io);
		for(unsigned s=0;s<2;s++)for(unsigned i=0;i<3;i++)if(!io.NativeTlbRead(s,i,tlb[s][i]) || (i==2 && (tlb[s][i]&0xffffc000)))return Fail(io);
		for(unsigned s=0;s<2;s++)for(unsigned i=0;i<3;i++){uint32_t v=0;if(!io.NativeTlbRead(s,i,v) || v!=tlb[s][i])return Fail(io);}
		if(!io.NativeClockBegin() || !AllHeld(io))return Fail(io);
		for(unsigned i=0;i<3;i++)if(!io.NativeControlRead(i,result.compute.controls[i]))return Fail(io);
		if((result.compute.controls[0]&1) || result.compute.controls[1] || result.compute.controls[2] || !Guard(io))return Fail(io);
		for(unsigned i=0;i<10;i++)if(!io.NativeNiuRead(i*4,command[i]) || command[i])return Fail(io);
		result.phase=2;
		for(unsigned i=0;i<L1Words;i++) {
			if((!(i&255) && !Guard(io)) || !io.NativeL1Read(L1Address(i),l1[i]))return Fail(io);
			result.saved_l1++;
		}
		for(unsigned i=0;i<L1Words;i++){uint32_t v=0;if(!io.NativeL1Read(L1Address(i),v) || v!=l1[i])return Fail(io);}
		result.phase=3;
		for(unsigned i=1;i<L1Words;i++)if(!io.NativeL1Write(L1Address(i),Stage<Image,Data>(sample,i)))return Fail(io);
		if(!Guard(io) || !io.NativeL1Write(0x6000,Image::Boot))return Fail(io); // BOOT LAST
		for(unsigned i=0;i<L1Words;i++){uint32_t v=0;if(!io.NativeL1Read(L1Address(i),v) || v!=Stage<Image,Data>(sample,i))return Fail(io);}
		if(!Guard(io))return Fail(io);
		result.phase=4;reset=0x87ffff;
		if(!io.NativeResetWrite(reset) || !io.NativePause() || !Guard(io))return Fail(io);
		for(unsigned i=0;i<9;i++)if(!io.NativeNiuRead(Counter(i),counters[i]))return Fail(io);
		uint64_t began=0,last=0;if(!io.NativeNow(began) || !began)return Fail(io);last=began;
		reset=0x46800;result.compute.started=1;if(!io.NativeResetWrite(reset))return Fail(io);
		bool complete=false;
		for(unsigned attempt=0;attempt<20000;attempt++) {
			result.compute.polls++;uint64_t now=0;uint32_t marker=0;
			if(!Guard(io,false) || !io.NativeNow(now) || now<last || now-began>UINT64_C(2000000000)
				|| !io.NativeL1Read(0x17ffc8,marker))return Fail(io);
			last=now;if(marker && marker!=V::Marker)return Fail(io);
			if(marker==V::Marker){complete=Terminal(io);break;}
			if(!io.NativePause())return Fail(io);
		}
		if(!complete)return Fail(io);
		for(unsigned i=0;i<64;i++) {
			if(!io.NativeL1Read(L::Output+i*4,result.compute.output[i]))return Fail(io);
			unsigned exponent=(result.compute.output[i]>>23)&255,mantissa=result.compute.output[i]&0x7fffff;
			if(exponent==255 || (!exponent && mantissa))return Fail(io);
		}
		if(!Terminal(io))return Fail(io);
		result.compute.completed=1;result.compute.state=PIONEER_NATIVE_BFP_RESULT;return true;
	}
	template<class IO,class Image,class Data> bool End(IO& io) {
		if(!owned || failed || result.phase!=4 || result.compute.state!=PIONEER_NATIVE_BFP_RESULT || !result.compute.completed)return false;
		result.phase=5;if(!Terminal(io))return Fail(io);
		reset=0x87ffff;if(!io.NativeResetWrite(reset) || !io.NativePause() || !Guard(io))return Fail(io);
		reset=0x47800;if(!io.NativeResetWrite(reset) || !io.NativePause() || !Guard(io))return Fail(io);
		result.compute.held=1;if(!AllHeld(io) || !Guard(io) || !Counters(io))return Fail(io);
		// Check code/payloads/headers/gaps/zero buffer BEFORE overwriting any
		// evidence. Only the declared stack/status/output spans may change.
		for(unsigned i=0;i<L1Words;i++)if(L::Immutable(L1Address(i))) {
			uint32_t v=0;if(!io.NativeL1Read(L1Address(i),v) || v!=Stage<Image,Data>(selection,i))return Fail(io);
		}
		for(unsigned i=0;i<64;i++){uint32_t v=0;if(!io.NativeL1Read(L::Output+i*4,v) || v!=result.compute.output[i])return Fail(io);}
		result.phase=6;
		for(unsigned i=1;i<L1Words;i++)if(!io.NativeL1Write(L1Address(i),l1[i]))return Fail(io);
		if(!Guard(io) || !io.NativeL1Write(0x6000,l1[0]))return Fail(io);
		for(unsigned i=0;i<L1Words;i++){uint32_t v=0;if(!io.NativeL1Read(L1Address(i),v) || v!=l1[i])return Fail(io);}
		if(!Guard(io))return Fail(io);
		result.compute.l1_restored=1;
		result.phase=7;
		for(unsigned i=0;i<10;i++){uint32_t v=0;if(!io.NativeNiuRead(i*4,v) || v!=command[i])return Fail(io);}
		if(!Guard(io) || !Counters(io))return Fail(io);
		result.niu_restored=1;
		for(unsigned s=0;s<2;s++)for(unsigned i=0;i<3;i++)if(!io.NativeTlbWrite(s,i,tlb[s][i]))return Fail(io);
		for(unsigned s=0;s<2;s++)for(unsigned i=0;i<3;i++){uint32_t v=0;if(!io.NativeTlbRead(s,i,v) || v!=tlb[s][i])return Fail(io);}
		if(!Sources(io))return Fail(io);
		result.phase=8;if(!io.NativeClockEnd() || !Sources(io))return Fail(io);
		for(unsigned s=0;s<2;s++)for(unsigned i=0;i<3;i++){uint32_t v=0;if(!io.NativeTlbRead(s,i,v) || v!=tlb[s][i])return Fail(io);}
		result.compute.tlb_restored=result.clock_restored=1;result.compute.state=PIONEER_NATIVE_BFP_CLOSED;owned=false;return true;
	}
};
#endif
