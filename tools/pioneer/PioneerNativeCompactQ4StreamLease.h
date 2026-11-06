/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_NATIVE_COMPACT_Q4_STREAM_LEASE_H
#define PIONEER_NATIVE_COMPACT_Q4_STREAM_LEASE_H
#if defined(PIONEER_Q38_WORK_INFERENCE) || defined(TT_PIONEER_Q38_WORK_MODE) || defined(PIONEER_Q38_CPU_VALIDATION) || defined(PIONEER_Q38_FULL_MODEL_CPU_REFERENCE) || defined(PIONEER_NATIVE_Q4_FPU_DIAGNOSTIC) || defined(PIONEER_NATIVE_DRAM_MODEL_BAND_DIAGNOSTIC) || defined(PIONEER_NATIVE_DRAM_LOAD_DIAGNOSTIC) || defined(__FAST_MATH__) || (defined(_KERNEL_MODE) && !defined(PIONEER_NATIVE_COMPACT_Q4_STREAM_DIAGNOSTIC))
#error Q4 stream lease requires separate diagnostic104,never Work/reference/old roles
#endif
#if defined(PIONEER_NATIVE_COMPACT_Q4_STREAM_TIMING)
#include "../../src/add-ons/kernel/drivers/misc/PioneerNativeCompactQ4TimingRole.h"
#elif defined(PIONEER_NATIVE_COMPACT_Q4_STREAM_DIAGNOSTIC) && (!defined(_KERNEL_MODE) || PIONEER_NATIVE_COMPACT_Q4_STREAM_DIAGNOSTIC != 1 || !defined(TT_PIONEER_DRIVER_RELOAD) || PIONEER_DRIVER_BUILD_TAG != 0x2026110500010104ULL)
#error Exact kernel diagnostic104 reload identity required
#endif
#if defined(PIONEER_NATIVE_Q4_FPU_STREAM_DIAGNOSTIC) || defined(PIONEER_NATIVE_MIXED_FPU_STREAM_DIAGNOSTIC) || defined(PIONEER_Q38_FULL_OFFLOAD_NATIVE) || defined(PIONEER_Q38_FULL_OFFLOAD_CLIENT)
#error Compact diagnostic104 cannot alias expanded/full-model roles
#endif
#include "../../src/add-ons/kernel/drivers/misc/BlackholeComputeNocProbe.h"
#include "../../src/add-ons/kernel/drivers/misc/PioneerNativeCompactQ4StreamProtocol.h"
#include "PioneerNativeCompactQ4StreamLayout.h"
#if defined(_KERNEL_MODE)
static_assert(PioneerNativeCompactQ4StreamVariant::Tag==PIONEER_DRIVER_BUILD_TAG,"exact cached math policy/driver binding");
#endif
// Full journal algorithm,not native authority by itself. Allocate on HEAP.
// Every checked primitive must bind THIS exact owner/backing/proof/FD/thread/
// generation/held semaphore/PCI reservation/actual VM views and fixed domains.
// No destructor IO or implicit END. First uncertainty retains EVERYTHING.
class PioneerNativeCompactQ4StreamLease {
	using L=PioneerNativeCompactQ4StreamLayout;
	using V=PioneerNativeCompactQ4StreamVariant;
	uint32_t l1[L::L1Words]={},dram[L::MaxDramWords]={},tlb[2][3]={},command[10]={},before[9]={},frame[8]={};
	tt_dram_samples_result routes={};const tt_dram_samples_result* proof=nullptr;
	unsigned selection=UINT32_MAX,reset=0x47800;bool attempted=false,owned=false,failed=false,fifo=false;
	PioneerNativeCompactQ4StreamResult result={};
	template<class IO> bool Fail(IO& io) {
		result.compute.state=PIONEER_NATIVE_BFP_UNKNOWN;result.compute.cold=1;
		if(!failed){failed=true;io.NativeRetain();}return false; // MEMORY ONLY
	}
	template<class IO> bool Access(IO& io) {
		return attempted && owned && !failed && proof
			&& io.NativeScope(this,proof,l1,L1Words,dram,MaxDramWords);
	}
	template<class IO> bool Sources(IO& io) {
		if(!Access(io))return false;
		for(unsigned i=0;i<16;i++){uint32_t v=0;if(!io.NativeSourceRead(i,v) || v!=routes.source[i])return false;}
		return true;
	}
	template<class IO> bool Set(IO& io,unsigned slot,unsigned base,unsigned x,unsigned y,bool ro) {
		if(!Sources(io))return false;
		for(unsigned i=0;i<3;i++) {
			const uint32_t expected=BlackholeDramFixedTlbWord(base,x,y,ro,i);uint32_t got=0;
			if(!io.NativeTlbWrite(slot,i,expected) || !io.NativeTlbRead(slot,i,got) || got!=expected)return false;
		}
		return Sources(io);
	}
	template<class IO> bool Select(IO& io,bool gddr=false) {
		return Set(io,0,gddr?L::DramWeight:0,gddr?routes.translated[0][0]:1,gddr?routes.translated[0][1]:2,false)
			&& Set(io,1,0xffa00000,1,2,false);
	}
	template<class IO> bool Niu(IO& io,bool idle) {
		for(unsigned i=0;i<16;i++) {
			uint32_t v=0;const uint32_t expected=routes.source[i]|(!i && fifo?0x10000U:0U);
			if(!io.NativeNiuRead(BlackholeDramSourceOffset(i),v) || v!=expected)return false;
		}
		uint32_t node=0,shape=0,logical=0,atomic=0;
		if(!io.NativeNiuRead(0x44,node) || !io.NativeNiuRead(0x48,shape) || !io.NativeNiuRead(0x148,logical)
			|| !io.NativeNiuRead(0x178,atomic) || (node&0xfff)!=(BlackholeDramSourceX(routes.source,1)|(BlackholeDramSourceY(routes.source,2)<<6))
			|| ((node>>12)&127)!=17 || ((node>>19)&127)!=12 || (shape&0xffffff00)!=0x10000 || (logical&0xfff)!=129 || (atomic&65535))return false;
		if(!idle)return true;
		for(unsigned i=0;i<4;i++){uint32_t v=0;if(!io.NativeNiuRead(0x40+i*0x800,v) || v)return false;}
		uint32_t available=0,error=0;
		if(!io.NativeNiuRead(0x64,available) || available!=(fifo?0x10101010U:0U) || !io.NativeNiuRead(0x68,error) || (error&15))return false;
		for(unsigned i=0;i<32;i++){uint32_t v=0;if(!io.NativeNiuRead(0x240+i*4,v) || (v&255))return false;}
		return true;
	}
	template<class IO> bool Guard(IO& io,bool gddr=false,bool idle=true) {
		if(!Sources(io))return false;
		for(unsigned s=0;s<2;s++)for(unsigned i=0;i<3;i++) {
			const unsigned base=s?0xffa00000:gddr?L::DramWeight:0;
			const unsigned x=!s && gddr?routes.translated[0][0]:1,y=!s && gddr?routes.translated[0][1]:2;
			uint32_t v=0;if(!io.NativeTlbRead(s,i,v) || v!=BlackholeDramFixedTlbWord(base,x,y,false,i))return false;
		}
		for(unsigned i=0;i<2;i++){uint32_t v=0;if(!io.NativeResetRead(v) || v!=reset)return false;}
		for(unsigned i=0;i<3;i++){uint32_t v=0;if(!io.NativeControlRead(i,v) || v!=result.compute.controls[i])return false;}
		return Niu(io,idle) && Sources(io);
	}
	template<class IO> bool AllHeld(IO& io) {
		const unsigned xs[]={1,2,3,4,5,6,7,10,11,12,13,14};
		for(unsigned x:xs)for(unsigned y=2;y<=11;y++) {
			if(!Set(io,1,0xffa00000,x,y,true))return false;
			for(unsigned i=0;i<2;i++){uint32_t v=0;if(!io.NativeResetRead(v) || v!=0x47800)return false;}
		}
		return Select(io);
	}
	static unsigned Counter(unsigned i){const unsigned indices[9]={1,2,3,4,5,8,10,12,14};return 0x200+indices[i]*4;}
	template<class IO> bool Counters(IO& io) {
		for(unsigned i=0;i<9;i++) {
			uint32_t v=0;const unsigned blocks=V::Width(selection)/256;
			const unsigned expected=i==2?2*blocks*L::Flits:(i==1 || i==3 || i==4 || i==8)?2*blocks*L::Packets:0;
			if(!io.NativeNiuRead(Counter(i),v) || uint32_t(v-before[i])!=expected)return false;
		}
		return true;
	}
	template<class IO> bool FinalCommands(IO& io) {
		const unsigned k=V::Width(selection),route=routes.translated[0][0]|(routes.translated[0][1]<<6);
		L::Read last={};if(!L::Packet(k,k/256-1,1,L::Packets-1,last))return false;
		const uint32_t expected[10]={last.source,0,route,last.destination,0,129,0,0,last.bytes,0};
		for(unsigned i=0;i<10;i++){uint32_t v=0;if(!io.NativeNiuRead(i*4,v) || v!=expected[i])return false;}
		return true;
	}
	template<class IO> bool Terminal(IO& io) {
		if(!Guard(io))return false;
		auto& r=result.compute;
		if(!io.NativeL1Read(0x17ffc0,r.phase) || !io.NativeL1Read(0x17ffc4,r.count) || !io.NativeL1Read(0x17ffc8,r.marker)
			|| r.phase!=0x7000c || r.count!=V::Count(selection) || r.marker!=V::Marker)return false;
		for(unsigned i=0;i<2;i++)if(!io.NativePause() || !io.NativePcRead(r.pc[i]) || (r.pc[i]!=0x70038 && r.pc[i]!=0x7003c))return false;
		return Guard(io) && Counters(io) && FinalCommands(io);
	}
public:
	PioneerNativeCompactQ4StreamLease()=default;
	PioneerNativeCompactQ4StreamLease(const PioneerNativeCompactQ4StreamLease&)=delete;
	PioneerNativeCompactQ4StreamLease& operator=(const PioneerNativeCompactQ4StreamLease&)=delete;
	static constexpr unsigned L1Words=L::L1Words,MaxDramWords=L::MaxDramWords;
	static unsigned L1Address(unsigned i){return L::Address(i);}
	unsigned DramWords()const{return L::DramWords(V::Width(selection));}
	unsigned DramAddress(unsigned i)const{return L::DramAddress(V::Width(selection),i);}
	template<class Data> static uint32_t Payload(unsigned sample,unsigned i){return Data::Word(sample,i);}

	template<class Image> uint32_t Stage(unsigned index)const {
		const unsigned a=L1Address(index);
		if(a==0x6000)return Image::Boot;
		if(a>=0x70000 && a<L::CodeEnd)return Image::Word(selection,(a-0x70000)/4);
		if(a>=0x74000 && a<0x74020)return frame[(a-0x74000)/4];
		return a>=L::Output && a<L::Output+256?0x7fc00001:0;
	}
	template<class Image,class Data> uint32_t FinalWord(unsigned index)const {
		const unsigned a=L1Address(index);
#if defined(PIONEER_NATIVE_FACTORED_Q4_COMPARE) || defined(PIONEER_NATIVE_FACTORED_Q4_TRACE)
		// Actual last group's packed unsigned Q4 codes,not an unaudited
		// mutable scratch exemption. Constants/header/exponents are exact too.
		for(unsigned slot=0;slot<2;slot++) {
			const unsigned tile=slot?0x88800:0x88000;
			if(a>=tile-16 && a<tile)return 0;
			if(a>=tile && a<tile+64)return 0x82828282;
			if(a>=tile+64 && a<tile+1088) {
				if(tile==0x88800)return 0x08080808;
				uint32_t word=0;const unsigned block=V::Width(selection)/256-1;
				for(unsigned b=0;b<4;b++) {
					const unsigned i=a+b-tile-64,face=i/256,k=i/16%16,c=i%16;
					const unsigned at=(face/2)*2048+(96+(face%2)*16+k)*16+c;
					const unsigned raw=Data::Word(selection,block*1152+at/4)>>((at%4)*8);
					word|=((raw>>4)&15)*8<<(b*8);
				}return word;
			}
		}
#endif
		const unsigned at=L::FinalPayloadIndex(V::Width(selection),a);
		if(at!=UINT32_MAX)return Payload<Data>(selection,at);
		return Stage<Image>(index);
	}
	const PioneerNativeCompactQ4StreamResult& Result()const{return result;}
	bool BoundTo(const tt_dram_samples_result* p,const uint32_t* a,unsigned n,const uint32_t* b,unsigned m)const {
		return p && p==proof && a==l1 && n==L1Words && b==dram && m==MaxDramWords;
	}
	void Abandon(){if(owned){failed=true;result.compute.state=PIONEER_NATIVE_BFP_UNKNOWN;result.compute.cold=1;}}
	template<class IO,class Image,class Data> bool Run(IO& io,const tt_dram_samples_result& baseline,unsigned sample) {
		static_assert(Image::Samples==V::Samples && Image::Rows==32 && Image::Phases==4 && Image::LayoutRole==V::Role && Image::Bytes==V::ImageBytes && Image::MetadataFirst==128 && Image::MetadataRows==128 && Image::DstLast==255,"exact compact QCS1 two-width image and Dst scope only");
#ifdef PIONEER_NATIVE_FACTORED_Q4_MANUAL_LAST
		static_assert(Image::ManualLast==1,"exact comparison113 manual-Last image only");
#endif
#ifdef PIONEER_NATIVE_FACTORED_Q4_TRACE
		static_assert(Image::FactoredTrace==1 && Image::TraceWords==64 && Image::TraceBlocks==20 && Image::NewPhases==2 && Image::Accumulator==80,"exact trace112 fixed observations and ownership");
#elif defined(PIONEER_NATIVE_FACTORED_Q4_COMPARE)
		static_assert(Image::FactoredComparison==1 && Image::NewPhases==2 && Image::Accumulator==80,"exact comparison111 changed arithmetic and ownership");
#endif
		static_assert(Data::Version==1 && Data::Rows==32 && Data::Samples==9 && Data::Role==104 && Data::PreparedBytes==4608,"exact compiled diagnostic104 data only");
		if(attempted || sample>=V::Samples)return false;
		attempted=true;result.compute.state=PIONEER_NATIVE_BFP_REFUSED;
		if(Image::Width(sample)!=V::Width(sample) || Data::Width(sample)!=V::Width(sample) || Data::Words(sample)!=L::DramWords(V::Width(sample)))return false;
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
		frame[0]=V::Marker;frame[1]=1;frame[2]=V::Width(selection);
		frame[3]=routes.translated[0][0]|(routes.translated[0][1]<<6);frame[4]=1;
		result.phase=2;
		for(unsigned i=0;i<L1Words;i++) {
			if((!(i&255) && !Guard(io)) || !io.NativeL1Read(L1Address(i),l1[i]))return Fail(io);
			result.saved_l1++;
		}
		for(unsigned i=0;i<L1Words;i++){uint32_t v=0;if(!io.NativeL1Read(L1Address(i),v) || v!=l1[i])return Fail(io);}
		if(!Select(io,true) || !Guard(io,true))return Fail(io);
		for(unsigned i=0;i<DramWords();i++) {
			if((!(i&255) && !Guard(io,true)) || !io.NativeDramRead(DramAddress(i),dram[i]))return Fail(io);
			result.saved_gddr++;
		}
		for(unsigned i=0;i<DramWords();i++){uint32_t v=0;if(!io.NativeDramRead(DramAddress(i),v) || v!=dram[i])return Fail(io);}
		result.phase=3;
		for(unsigned i=0;i<DramWords();i++) {
			const uint32_t expected=Payload<Data>(sample,i);uint32_t v=0;
			if((!(i&255) && !Guard(io,true)) || !io.NativeDramWrite(DramAddress(i),expected)
				|| !io.NativeDramRead(DramAddress(i),v) || v!=expected)return Fail(io);
		}
		for(unsigned i=0;i<DramWords();i++){uint32_t v=0;if(!io.NativeDramRead(DramAddress(i),v) || v!=Payload<Data>(sample,i))return Fail(io);}
		if(!Guard(io,true) || !Select(io))return Fail(io);
		result.phase=4;
		for(unsigned i=1;i<L1Words;i++)if(!io.NativeL1Write(L1Address(i),Stage<Image>(i)))return Fail(io);
		if(!Guard(io) || !io.NativeL1Write(0x6000,Image::Boot))return Fail(io); // BOOT LAST
		for(unsigned i=0;i<L1Words;i++){uint32_t v=0;if(!io.NativeL1Read(L1Address(i),v) || v!=Stage<Image>(i))return Fail(io);}
		if(!Guard(io))return Fail(io);
		fifo=true;if(!io.NativeNiuWrite(0x100,routes.source[0]|0x10000) || !Guard(io))return Fail(io);
		result.phase=5;reset=0x87ffff;
		if(!io.NativeResetWrite(reset) || !io.NativePause() || !Guard(io))return Fail(io);
		for(unsigned i=0;i<9;i++)if(!io.NativeNiuRead(Counter(i),before[i]))return Fail(io);
		uint64_t began=0,last=0;if(!io.NativeNow(began) || !began)return Fail(io);last=began;
		reset=0x46800;result.compute.started=1;if(!io.NativeResetWrite(reset))return Fail(io);
		bool complete=false;
		for(unsigned attempt=0;attempt<20000;attempt++) {
			result.compute.polls++;uint64_t now=0;uint32_t marker=0;
			if(!Guard(io,false,false) || !io.NativeNow(now) || now<last || now-began>UINT64_C(2000000000)
				|| !io.NativeL1Read(0x17ffc8,marker))return Fail(io);
			last=now;
			if(marker && marker!=V::Marker)return Fail(io); // wrong/old/fault marker:stop immediately
			if(marker==V::Marker){complete=Terminal(io);break;}
			if(!io.NativePause())return Fail(io);
		}
		if(!complete)return Fail(io);
		for(unsigned i=0;i<64;i++) {
			if(!io.NativeL1Read(L::Output+i*4,result.compute.output[i]))return Fail(io);
#ifndef PIONEER_NATIVE_FACTORED_Q4_TRACE
			unsigned e=(result.compute.output[i]>>23)&255,m=result.compute.output[i]&0x7fffff;
			if(e==255 || (!e && m))return Fail(io);
#endif
		}
#ifdef PIONEER_NATIVE_FACTORED_Q4_TRACE
		// Capture ALL returned DATA first. Final32 and six FP32 first-group
		// observations keep the finite/normal rule. Words32..57 are opaque
		// integer fingerprints/packed bytes,not floating-point numbers.
		for(unsigned i=0;i<64;i++)if(i<32 || i>=58){
			unsigned e=(result.compute.output[i]>>23)&255,m=result.compute.output[i]&0x7fffff;
			if(e==255 || (!e && m))return Fail(io);
		}
#endif
		if(!Terminal(io))return Fail(io);
	#ifdef PIONEER_NATIVE_COMPACT_Q4_STREAM_TIMING
		// Preserve raw DATA only after the SAME independently proven terminal.
		// Unstable/invalid clocks do not invalidate completed math or grant END.
		for(unsigned i=0;i<16;i++)if(!io.NativeL1Read(PioneerQ38Q4KernelTiming::Address+i*4,result.timing[i]))return Fail(io);
		if(!Terminal(io))return Fail(io);
	#endif
		result.requests=2*(V::Width(selection)/256)*L::Packets;result.flits=2*(V::Width(selection)/256)*L::Flits;result.compute.completed=1;result.compute.state=PIONEER_NATIVE_BFP_RESULT;return true;
	}
	template<class IO,class Image,class Data> bool End(IO& io) {
		if(!owned || failed || result.compute.state!=PIONEER_NATIVE_BFP_RESULT || !result.compute.completed)return false;
		if(!Terminal(io))return Fail(io);
		result.phase=6;reset=0x87ffff;
		if(!io.NativeResetWrite(reset) || !io.NativePause() || !Guard(io) || !Counters(io))return Fail(io);
		reset=0x47800;if(!io.NativeResetWrite(reset) || !io.NativePause() || !Guard(io) || !Counters(io) || !AllHeld(io))return Fail(io);
		result.compute.held=1;
		// BEFORE overwriting evidence,verify code/boot/frame plus BOTH final
		// producer slots,ALL headers/gaps/shared zero bytes and captured outputs.
		for(unsigned i=0;i<L1Words;i++)if(L::Immutable(L1Address(i))) {
			unsigned a=L1Address(i);
			uint32_t expected=FinalWord<Image,Data>(i),got=0;
			if(!io.NativeL1Read(a,got) || got!=expected)return Fail(io);
		}
		for(unsigned i=0;i<64;i++){uint32_t v=0;if(!io.NativeL1Read(L::Output+i*4,v) || v!=result.compute.output[i])return Fail(io);}
#ifdef PIONEER_NATIVE_COMPACT_Q4_STREAM_TIMING
		for(unsigned i=0;i<16;i++){
			uint32_t v=0;if(!io.NativeL1Read(PioneerQ38Q4KernelTiming::Address+i*4,v) || v!=result.timing[i])return Fail(io);
		}
#endif
		if(!FinalCommands(io))return Fail(io);

		result.phase=7;if(!Select(io,true) || !Guard(io,true))return Fail(io);
		for(unsigned i=0;i<DramWords();i++){uint32_t v=0;if(!io.NativeDramRead(DramAddress(i),v) || v!=Payload<Data>(selection,i))return Fail(io);}
		for(unsigned i=0;i<DramWords();i++) {
			uint32_t v=0;if((!(i&255) && !Guard(io,true)) || !io.NativeDramWrite(DramAddress(i),dram[i])
				|| !io.NativeDramRead(DramAddress(i),v) || v!=dram[i])return Fail(io);
		}
		for(unsigned i=0;i<DramWords();i++){uint32_t v=0;if(!io.NativeDramRead(DramAddress(i),v) || v!=dram[i])return Fail(io);}
		if(!Guard(io,true))return Fail(io);
		result.gddr_restored=1;
		result.phase=8;if(!Select(io) || !Guard(io))return Fail(io);
		for(unsigned i=1;i<L1Words;i++)if(!io.NativeL1Write(L1Address(i),l1[i]))return Fail(io);
		if(!Guard(io) || !io.NativeL1Write(0x6000,l1[0]))return Fail(io);
		for(unsigned i=0;i<L1Words;i++){uint32_t v=0;if(!io.NativeL1Read(L1Address(i),v) || v!=l1[i])return Fail(io);}
		if(!Guard(io))return Fail(io);
		result.compute.l1_restored=1;result.phase=9;
		for(unsigned i=0;i<10;i++){uint32_t v=0;if(!io.NativeNiuWrite(i*4,command[i]) || !io.NativeNiuRead(i*4,v) || v!=command[i])return Fail(io);}
		if(!Guard(io) || !Counters(io))return Fail(io);
		fifo=false;if(!io.NativeNiuWrite(0x100,routes.source[0]) || !Guard(io))return Fail(io);result.niu_restored=1;
		for(unsigned s=0;s<2;s++)for(unsigned i=0;i<3;i++){uint32_t v=0;if(!io.NativeTlbWrite(s,i,tlb[s][i]) || !io.NativeTlbRead(s,i,v) || v!=tlb[s][i])return Fail(io);}
		if(!Sources(io) || !io.NativeClockEnd() || !Sources(io))return Fail(io);
		for(unsigned s=0;s<2;s++)for(unsigned i=0;i<3;i++){uint32_t v=0;if(!io.NativeTlbRead(s,i,v) || v!=tlb[s][i])return Fail(io);}
		result.clock_restored=result.compute.tlb_restored=1;result.phase=10;result.compute.state=PIONEER_NATIVE_BFP_CLOSED;owned=false;return true;
	}
};
static_assert(PioneerNativeCompactQ4StreamLease::L1Words*4==
#if defined(PIONEER_NATIVE_FACTORED_Q4_COMPARE) || defined(PIONEER_NATIVE_FACTORED_Q4_TRACE)
 148544
#else
 107632
#endif
 &&
 PioneerNativeCompactQ4StreamLease::MaxDramWords*4==348160,"bounded separate heap journals");
#endif
