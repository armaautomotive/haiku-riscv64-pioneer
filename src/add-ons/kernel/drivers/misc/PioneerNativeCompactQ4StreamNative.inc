/* SPDX-License-Identifier: MIT */
// Separate diagnostic104 full-K Q4 owner;NO Work/runtime mode or arbitrary operands.
#include "BlackholeDramSweepArcClock.h"
#include "BlackholeDramSweepRoutes.h"
#include "../../../../../tools/pioneer/PioneerNativeCompactQ4StreamLease.h"
#include "PioneerNativeCompactQ4StreamGeneratedImage.h"
#include "PioneerNativeCompactQ4StreamData.h" // PRIVATE generated weights,NEVER commit
namespace PioneerNativeCompactQ4Stream104 {
constexpr uint32 NativeResultMarker=PioneerNativeCompactQ4StreamVariant::Marker;
constexpr const char* MapName="P100A isolated full-K Q4 compact streaming104";
struct Cookie {PioneerProbeCookie* base=NULL;bool used=false;};
struct Owner;
static Owner* current=NULL;
struct IO:PioneerComputeNocIO {
	Owner& owner;
	area_id memoryArea=-1,controlArea=-1,pcArea=-1,writeArea=-1;
	volatile uint32* memory=NULL;volatile uint32* resetWrite=NULL;volatile uint32* niuWrite=NULL;
	const volatile uint32* pc=NULL;
	IO(Owner&,const tt_pioneer_profile&,tt_arc_scratch_result&,tt_arc_telemetry_result&,
		tt_tensix_reset_result&,tt_dram_samples_result&,tt_compute_noc_result&);
	bool Map();bool Unmap()=delete;bool AfterNodeId(uint32)=delete;
	bool Access();bool Active();bool Selected(bool=false,bool=true);
	uint32 CardRead(uint16,uint8);void CardWrite(uint16,uint8,uint32);
	uint32 RootRead(uint16,uint8);void RootWrite(uint16,uint8,uint32);
	bool Quiet();uint32 ReadNodeId();uint32 SourceRead(unsigned);uint32 DramIdRead(unsigned);
	void SweepQuarantineMemoryOnly(); // exact route-only prerequisite failure callback
	uint32 ComputeNocRead(uint32);uint32 DramTlbRead(unsigned,unsigned);void DramTlbWrite(unsigned,unsigned,uint32);
	uint32 ResetRead();void Pause();
	bool NativeScope(const PioneerNativeCompactQ4StreamLease*,const tt_dram_samples_result*,const uint32*,unsigned,const uint32*,unsigned);
	void NativeRetain();bool NativeSourceRead(unsigned,uint32&);
	bool NativeTlbRead(unsigned,unsigned,uint32&);bool NativeTlbWrite(unsigned,unsigned,uint32);
	bool NativeResetRead(uint32&);bool NativeResetWrite(uint32);
	bool NativeNiuRead(unsigned,uint32&);bool NativeNiuWrite(unsigned,uint32);
	bool NativeL1Read(unsigned,uint32&);bool NativeL1Write(unsigned,uint32);
	bool NativeDramRead(unsigned,uint32&);bool NativeDramWrite(unsigned,uint32);
	bool NativeControlRead(unsigned,uint32&);bool NativePcRead(uint32&);
	bool NativeClockBegin();bool NativeClockEnd();bool NativeNow(uint64&);bool NativePause();
};
struct ClockIO:PioneerArcQueryIO {
	Owner& owner;
	ClockIO(Owner& o,const tt_pioneer_profile& p,tt_arc_scratch_result& a,tt_arc_telemetry_result& t,tt_arc_queue_result& q)
		:PioneerArcQueryIO(p,a,t,q),owner(o){}
	bool Check();void MemoryOnlyRetain();bool Unmap()=delete;bool AfterNodeId(uint32)=delete;
	uint32 TlbRead(unsigned);void TlbWrite(unsigned,uint32);
	uint32 CsmRead(uint32);void SetQueueBounds(uint32,uint32,uint32);void QueueWrite(uint32,uint32);
	uint32 MsiStatusRead();void NotifyArc();void Pause();
};
struct Owner {
	Cookie* const cookie;PioneerProbeCookie* const fd;
	const thread_id thread;const team_id team;const sem_id lock;
	pci_module_info* const pci;pioneer_driver_reload_guard_info* const guard;
	const uint64 generation;const pci_info card;const unsigned sample;
	tt_pioneer_profile profile={};tt_pioneer_mmio_result mapping={};tt_pioneer_mmio_transaction transaction={};
	tt_arc_scratch_result arc={};tt_arc_telemetry_result telemetry={};tt_tensix_reset_result tensix={};
	tt_dram_samples_result dram={};tt_compute_noc_result noc={};tt_arc_queue_result query={};tt_arc_clock_guard_result clockState={};
	PioneerNativeCompactQ4StreamLease lease;
	bool reserved=false,retained=false,ready=false,closing=false,clockCall=false,clockEntered=false,viewsClosed=false,closeAttempted=false;
	unsigned resetStage=0,fifoStage=0;
	uint32 savedTlb[2][3]={},savedCommand[10]={};bool haveTlb[2][3]={},haveCommand[10]={};
	IO io;ClockIO clock;
	Owner(Cookie* c,const thread_info& t,unsigned s):cookie(c),fd(c->base),thread(t.thread),team(t.team),lock(sLock),
		pci(sPCI),guard(sReloadGuard),generation(sReloadGeneration),card(sCard),sample(s),
		io(*this,profile,arc,telemetry,tensix,dram,noc),clock(*this,profile,arc,telemetry,query){}
	Owner(const Owner&)=delete;Owner& operator=(const Owner&)=delete;
	bool Scope()const {
		if(current!=this || retained || !cookie || cookie->base!=fd || !cookie->used || !fd || fd->magic!=0x50313030
			|| fd->team!=team || fd->context || sRuntimeOwner!=fd || sBankStorageOnly || sOpenCookies!=1
			|| sReloadQuiesced || sColdRecoveryRequired || lock<0 || lock!=sLock || !pci || pci!=sPCI || !reserved
			|| !guard || guard!=sReloadGuard || guard->version!=1 || !generation || generation!=sReloadGeneration
			|| card.bus!=sCard.bus || card.device!=sCard.device || card.function!=sCard.function
			|| card.vendor_id!=0x1e52 || card.device_id!=0xb140 || card.u.h0.subsystem_vendor_id!=0x1e52 || card.u.h0.subsystem_id!=0x0043)return false;
		thread_info now={};int32 count=1;
		return find_thread(NULL)==thread && get_thread_info(thread,&now)==B_OK && now.team==team
			&& get_sem_count(lock,&count)==B_OK && count<=0;
	}
	bool View(area_id id,const volatile void* pointer,unsigned offset,size_t bytes,bool write)const {
		if(id<0 || !pointer)return false;
		area_info info={};const uint32 permissions=B_KERNEL_READ_AREA|(write?B_KERNEL_WRITE_AREA:0);
		return get_area_info(id,&info)==B_OK && info.area==id && info.size==bytes
			&& reinterpret_cast<uintptr_t>(info.address)+offset==reinterpret_cast<uintptr_t>(pointer)
			&& (info.protection&(B_KERNEL_READ_AREA|B_KERNEL_WRITE_AREA))==permissions
			&& !(info.protection&(B_READ_AREA|B_WRITE_AREA|B_EXECUTE_AREA|B_KERNEL_EXECUTE_AREA));
	}
	bool Views()const {
		return View(io.memoryArea,io.memory,0,0x200000,true) && View(io.controlArea,io.resetWrite,0x1b0,B_PAGE_SIZE,true)
			&& View(io.pcArea,io.pc,0x140,B_PAGE_SIZE,false) && View(io.writeArea,io.niuWrite,0,B_PAGE_SIZE,true)
			&& View(io.nocArea,io.nocRegisters,0,2*B_PAGE_SIZE,false) && io.dataArea<0 && !io.data
			&& View(io.dramArea,io.dramNiu,0,B_PAGE_SIZE,false) && View(io.resetArea,io.reset,0x1b0,B_PAGE_SIZE,false)
			&& View(io.csmArea,io.csm,0,0x20000,false) && View(io.scratchArea,io.scratch,0,B_PAGE_SIZE,false)
			&& View(io.tlbArea,io.tlb,0x96c,B_PAGE_SIZE,true) && View(io.area,io.nodeID,0x44,B_PAGE_SIZE,false)
			&& View(clock.queueWriteArea,clock.queueWrite,0,TT_ARC_QUEUE_CSM_BYTES,true) && View(clock.msiArea,clock.msi,0,B_PAGE_SIZE,true)
			&& View(clock.csmArea,clock.csm,0,0x20000,false) && View(clock.scratchArea,clock.scratch,0,B_PAGE_SIZE,false)
			&& View(clock.tlbArea,clock.tlb,0x96c,B_PAGE_SIZE,true) && View(clock.area,clock.nodeID,0x44,B_PAGE_SIZE,false);
	}
	bool Identity(bool enabled)const {
		// Raw PCI metadata,not recursive IO::Access. Mutations remain forbidden.
		return Scope() && pci->read_pci_config(card.bus,card.device,card.function,0,4)==0xb1401e52
			&& pci->read_pci_config(card.bus,card.device,card.function,0x2c,4)==0x00431e52
			&& pci->read_pci_config(profile.root_bus,0,0,0,4)==0x20421e30
			&& pci->read_pci_config(profile.root_bus,0,0,0x18,4)==0x003f0100
			&& (!enabled || (pci->read_pci_config(card.bus,card.device,card.function,4,2)==0x402
				&& pci->read_pci_config(profile.root_bus,0,0,4,2)==0x402));
	}
	void Retain(){retained=true;ready=false;clockCall=false;lease.Abandon();sColdRecoveryRequired=true;} // MEMORY ONLY
	bool Delete(area_id& id,const volatile void* pointer,unsigned offset,size_t bytes,bool write) {
		if(!Scope() || !closing || !View(id,pointer,offset,bytes,write))return false;
		if(delete_area(id)!=B_OK){Retain();return false;}id=-1;return true;
	}
	bool CloseViews() {
		if(!Scope() || !closing || !Views() || !Identity(true))return false;
		if(!Delete(io.memoryArea,io.memory,0,0x200000,true) || !Delete(io.controlArea,io.resetWrite,0x1b0,B_PAGE_SIZE,true)
			|| !Delete(io.pcArea,io.pc,0x140,B_PAGE_SIZE,false) || !Delete(io.writeArea,io.niuWrite,0,B_PAGE_SIZE,true)
			|| !Delete(clock.queueWriteArea,clock.queueWrite,0,TT_ARC_QUEUE_CSM_BYTES,true) || !Delete(clock.msiArea,clock.msi,0,B_PAGE_SIZE,true)
			|| !Delete(clock.csmArea,clock.csm,0,0x20000,false) || !Delete(clock.scratchArea,clock.scratch,0,B_PAGE_SIZE,false)
			|| !Delete(clock.tlbArea,clock.tlb,0x96c,B_PAGE_SIZE,true) || !Delete(clock.area,clock.nodeID,0x44,B_PAGE_SIZE,false)
			|| !Delete(io.nocArea,io.nocRegisters,0,2*B_PAGE_SIZE,false) || !Delete(io.dramArea,io.dramNiu,0,B_PAGE_SIZE,false)
			|| !Delete(io.resetArea,io.reset,0x1b0,B_PAGE_SIZE,false) || !Delete(io.csmArea,io.csm,0,0x20000,false)
			|| !Delete(io.scratchArea,io.scratch,0,B_PAGE_SIZE,false) || !Delete(io.tlbArea,io.tlb,0x96c,B_PAGE_SIZE,true)
			|| !Delete(io.area,io.nodeID,0x44,B_PAGE_SIZE,false))return false;
		io.memory=io.resetWrite=io.niuWrite=NULL;io.pc=NULL;io.nocRegisters=io.dramNiu=io.reset=io.csm=io.scratch=io.nodeID=NULL;
		io.data=io.tlb=NULL;clock.queueWrite=clock.msi=clock.tlb=NULL;clock.csm=clock.scratch=clock.nodeID=NULL;
		viewsClosed=true;return true;
	}
	bool ClosePci() {
		if(!Scope() || !closing || !viewsClosed || !transaction.config_attempted || !Identity(true))return false;
		for(unsigned p=0;p<2;p++) {
			if(!Identity(false))return false;
			const uint8 bus=p?profile.root_bus:card.bus,device=p?0:card.device,function=p?0:card.function;
			pci->write_pci_config(bus,device,function,4,2,0);
			if(pci->read_pci_config(bus,device,function,4,2)!=0)return false;
		}
		const unsigned order[]={5,4,1,0,3,2};
		for(unsigned i:order) {
			if(!Identity(false) || pci->read_pci_config(card.bus,card.device,card.function,4,2) || pci->read_pci_config(profile.root_bus,0,0,4,2))return false;
			pci->write_pci_config(card.bus,card.device,card.function,0x10+i*4,4,mapping.bars_before[i]);
			if(pci->read_pci_config(card.bus,card.device,card.function,0x10+i*4,4)!=mapping.bars_before[i])return false;
		}
		const unsigned bridge[]={1,2,0};
		for(unsigned i:bridge) {
			if(!Identity(false) || pci->read_pci_config(card.bus,card.device,card.function,4,2) || pci->read_pci_config(profile.root_bus,0,0,4,2))return false;
			pci->write_pci_config(profile.root_bus,0,0,0x24+i*4,4,mapping.bridge_before[i]);
			if(pci->read_pci_config(profile.root_bus,0,0,0x24+i*4,4)!=mapping.bridge_before[i])return false;
		}
		for(unsigned i=0;i<6;i++)if(!Identity(false) || pci->read_pci_config(card.bus,card.device,card.function,0x10+i*4,4)!=mapping.bars_before[i])return false;
		for(unsigned i=0;i<3;i++)if(!Identity(false) || pci->read_pci_config(profile.root_bus,0,0,0x24+i*4,4)!=mapping.bridge_before[i])return false;
		if(!Identity(false) || pci->read_pci_config(card.bus,card.device,card.function,4,2) || pci->read_pci_config(profile.root_bus,0,0,4,2))return false;
		mapping.restored=1;mapping.stage=5;transaction={};return true;
	}
};
IO::IO(Owner& o,const tt_pioneer_profile& p,tt_arc_scratch_result& a,tt_arc_telemetry_result& t,
	tt_tensix_reset_result& c,tt_dram_samples_result& d,tt_compute_noc_result& n):PioneerComputeNocIO(p,a,t,c,d,n,2),owner(o){}
bool IO::Access() {
	if(owner.Scope() && !owner.closing && (!owner.ready || (owner.Views() && owner.Identity(true))))return true;
	owner.Retain();return false;
}
bool IO::Map() {
	if(!Access() || !PioneerComputeNocIO::Map())return false;
	const uint32 offsets[]={0x19000000,0x19312000,0x19313000,0x19320000};
	area_id* ids[]={&memoryArea,&controlArea,&pcArea,&writeArea};void* pages[4]={};
	for(unsigned i=0;i<4;i++) {
		if(!Access())return false;
		*ids[i]=map_physical_memory(MapName,profile.bar0_host_start+offsets[i],i?B_PAGE_SIZE:0x200000,
			B_ANY_KERNEL_ADDRESS,B_KERNEL_READ_AREA|B_UNCACHED_MEMORY|(i==2?0:B_KERNEL_WRITE_AREA),&pages[i]);
		if(*ids[i]<B_OK)return false;
	}
	memory=(volatile uint32*)pages[0];resetWrite=(volatile uint32*)((uint8*)pages[1]+0x1b0);
	pc=(const volatile uint32*)((uint8*)pages[2]+0x140);niuWrite=(volatile uint32*)pages[3];
	return owner.clock.PioneerArcQueryIO::Map();
}
uint32 IO::CardRead(uint16 a,uint8 n){return Access()?PioneerMmioIO::CardRead(a,n):UINT32_MAX;}
void IO::CardWrite(uint16 a,uint8 n,uint32 v){if(Access())PioneerMmioIO::CardWrite(a,n,v);}
uint32 IO::RootRead(uint16 a,uint8 n){return Access()?PioneerMmioIO::RootRead(a,n):UINT32_MAX;}
void IO::RootWrite(uint16 a,uint8 n,uint32 v){if(Access())PioneerMmioIO::RootWrite(a,n,v);}
bool IO::Quiet(){return Access() && PioneerMmioIO::Quiet();}
uint32 IO::ReadNodeId(){return Access()?PioneerMmioIO::ReadNodeId():UINT32_MAX;}
uint32 IO::SourceRead(unsigned i){return i<16 && Access()?PioneerDramSamplesIO::SourceRead(i):UINT32_MAX;}
uint32 IO::DramIdRead(unsigned i){return i<4 && Access()?PioneerDramSamplesIO::DramIdRead(i):UINT32_MAX;}
uint32 IO::ComputeNocRead(uint32 a){return BlackholeComputeNocReadOffset(a) && Access()?PioneerComputeNocIO::ComputeNocRead(a):UINT32_MAX;}
uint32 IO::DramTlbRead(unsigned s,unsigned i){return s<2 && i<3 && Access()?PioneerDramSamplesIO::DramTlbRead(s,i):UINT32_MAX;}
void IO::DramTlbWrite(unsigned s,unsigned i,uint32 v){if(s<2 && i<3 && Access())PioneerDramSamplesIO::DramTlbWrite(s,i,v);}
uint32 IO::ResetRead(){return Access()?PioneerTensixIO::ResetRead():UINT32_MAX;}
void IO::Pause(){if(Access())spin(100);}
void IO::SweepQuarantineMemoryOnly(){owner.Retain();}
bool IO::Active() {
	const auto& r=owner.lease.Result();
	return owner.ready && !owner.clockCall && r.phase>=1 && r.phase<=9 && !r.compute.cold
		&& r.compute.state!=PIONEER_NATIVE_BFP_UNKNOWN && Access();
}
bool IO::Selected(bool gddr,bool l1) {
	if(!Active())return false;
	for(unsigned i=0;i<3;i++) {
		if((l1 && PioneerRead32(tlb-3+i)!=BlackholeDramFixedTlbWord(gddr?0x04000000:0,
			gddr?owner.dram.translated[0][0]:1,gddr?owner.dram.translated[0][1]:2,false,i))
			|| PioneerRead32(tlb+i)!=BlackholeDramFixedTlbWord(0xffa00000,1,2,false,i))return false;
	}
	return true;
}
bool IO::NativeScope(const PioneerNativeCompactQ4StreamLease* a,const tt_dram_samples_result* p,
	const uint32* l1,unsigned n,const uint32* d,unsigned m) {
	const unsigned phase=owner.lease.Result().phase,s=owner.resetStage,f=owner.fifoStage;
	const bool ledger=(phase>=1 && phase<=3 && s==0 && f==0) || (phase==4 && s==0 && f<=1)
		|| (phase==5 && s<=2 && f==1) || (phase==6 && s>=2 && s<=4 && f==1)
		|| ((phase==7 || phase==8) && s==4 && f==1) || (phase==9 && s==4 && f>=1 && f<=2);
	return a==&owner.lease && p==&owner.dram && owner.lease.BoundTo(p,l1,n,d,m)
		&& ledger && ((phase==1 || phase==9) || owner.clockEntered) && Active();
}
void IO::NativeRetain(){owner.Retain();}
bool IO::NativeSourceRead(unsigned i,uint32& v){if(i>=16 || !Active())return false;v=PioneerDramSamplesIO::SourceRead(i);return true;}
bool IO::NativeTlbRead(unsigned s,unsigned i,uint32& v) {
	if(s>=2 || i>=3 || !Active())return false;
	v=PioneerRead32(tlb-3+s*3+i);
	if(owner.lease.Result().phase==1 && !owner.clockEntered && !owner.haveTlb[s][i]){owner.savedTlb[s][i]=v;owner.haveTlb[s][i]=true;}
	return true;
}
bool IO::NativeTlbWrite(unsigned s,unsigned i,uint32 v) {
	if(s>=2 || i>=3 || !owner.ready || owner.clockCall)return false;
	const unsigned phase=owner.lease.Result().phase;bool allowed=false;
	if(phase==9)allowed=owner.haveTlb[s][i] && v==owner.savedTlb[s][i];
	else if(phase>=1 && phase<=8) {
		if(!s)allowed=v==BlackholeDramFixedTlbWord(0,1,2,false,i)
			|| v==BlackholeDramFixedTlbWord(0x04000000,owner.dram.translated[0][0],owner.dram.translated[0][1],false,i);
		else {
			allowed=v==BlackholeDramFixedTlbWord(0xffa00000,1,2,false,i);
			if((phase==1 && owner.resetStage==0) || (phase==6 && owner.resetStage==4)) {
				const unsigned xs[]={1,2,3,4,5,6,7,10,11,12,13,14};
				for(unsigned x:xs)for(unsigned y=2;y<=11;y++)allowed|=v==BlackholeDramFixedTlbWord(0xffa00000,x,y,true,i);
			}
		}
	}
	if(!allowed || !Active())return false;
	PioneerWrite32(tlb-3+s*3+i,v);return true;
}
bool IO::NativeResetRead(uint32& v) {
	if(!Active())return false;
	uint32 bound[3];for(unsigned i=0;i<3;i++)bound[i]=PioneerRead32(tlb+i);
	bool selected=true;for(unsigned i=0;i<3;i++)selected&=bound[i]==BlackholeDramFixedTlbWord(0xffa00000,1,2,false,i);
	const unsigned phase=owner.lease.Result().phase;
	if(!selected && ((phase==1 && !owner.resetStage) || (phase==6 && owner.resetStage==4))) {
		const unsigned xs[]={1,2,3,4,5,6,7,10,11,12,13,14};
		for(unsigned x:xs)for(unsigned y=2;y<=11;y++) {
			bool match=true;for(unsigned i=0;i<3;i++)match&=bound[i]==BlackholeDramFixedTlbWord(0xffa00000,x,y,true,i);
			selected|=match;
		}
	}
	if(!selected)return false;
	v=PioneerRead32(reset);return true;
}
bool IO::NativeResetWrite(uint32 v) {
	const auto& r=owner.lease.Result();const unsigned stage=owner.resetStage;
	const bool allowed=(stage==0 && v==0x87ffff && r.phase==5 && !r.compute.started)
		|| (stage==1 && v==0x46800 && r.phase==5 && r.compute.started && !r.compute.completed)
		|| (stage==2 && v==0x87ffff && r.phase==6 && r.compute.state==PIONEER_NATIVE_BFP_RESULT && r.compute.completed)
		|| (stage==3 && v==0x47800 && r.phase==6 && r.compute.state==PIONEER_NATIVE_BFP_RESULT && r.compute.completed);
	if(!allowed || !Selected())return false;
	const unsigned previous=stage==0?0x47800:stage==2?0x46800:0x87ffff;
	if(PioneerRead32(reset)!=previous || PioneerRead32(reset)!=previous)return false;
	owner.resetStage++;PioneerWrite32(resetWrite,v);return true; // attempt recorded BEFORE write
}
static bool NiuReadOffset(unsigned a) {
	if((a&3) || a>=0x2000)return false;
	if(BlackholeComputeNocReadOffset(a) || a<=0x24 || a==0x64 || a==0x68 || a==0x148 || a==0x178 || (a>=0x240 && a<0x2c0))return true;
	const unsigned indices[]={1,2,3,4,5,8,10,12,14};for(unsigned i:indices)if(a==0x200+i*4)return true;
	return false;
}
bool IO::NativeNiuRead(unsigned a,uint32& v) {
	if(!NiuReadOffset(a) || !Selected(false,false))return false;
	v=PioneerRead32(nocRegisters+a/4);
	if(a<=0x24 && owner.lease.Result().phase==1 && !owner.resetStage && !owner.haveCommand[a/4]) {
		owner.savedCommand[a/4]=v;owner.haveCommand[a/4]=true;
	}
	return true;
}
bool IO::NativeNiuWrite(unsigned a,uint32 v) {
	if((a&3) || !(a<=0x24 || a==0x100))return false; // NO host doorbell/counter reset
	const auto& r=owner.lease.Result();bool allowed=false;
	if(a==0x100)allowed=(r.phase==4 && !owner.resetStage && !owner.fifoStage && v==(owner.dram.source[0]|0x10000))
		|| (r.phase==9 && owner.resetStage==4 && owner.fifoStage==1 && v==owner.dram.source[0]);
	else allowed=r.phase==9 && owner.resetStage==4 && owner.haveCommand[a/4] && v==owner.savedCommand[a/4];
	if(!allowed || !Selected(false,false) || PioneerRead32(reset)!=0x47800 || PioneerRead32(reset)!=0x47800)return false;
	if(a==0x100)owner.fifoStage++;
	PioneerWrite32(niuWrite+a/4,v);return true;
}
static bool L1Address(unsigned a){return PioneerNativeCompactQ4StreamLayout::Contains(a);}

bool IO::NativeL1Read(unsigned a,uint32& v) {
	if(!L1Address(a))return false;
	if(owner.resetStage==2) {
		const auto& r=owner.lease.Result().compute;
		const bool status=a>=0x17ffc0 && a<0x17ffcc;
		const bool output=a>=PioneerNativeCompactQ4StreamLayout::Output && a<PioneerNativeCompactQ4StreamLayout::Output+256 && r.phase==0x7000c && r.count==PioneerNativeCompactQ4StreamVariant::Count(owner.sample) && r.marker==NativeResultMarker
			&& (r.pc[0]==0x70038 || r.pc[0]==0x7003c) && (r.pc[1]==0x70038 || r.pc[1]==0x7003c);
		if(owner.lease.Result().phase!=5 || (!status && !output))return false;

	}
	if(!Selected())return false;
	v=PioneerRead32(memory+a/4);return true;
}
bool IO::NativeL1Write(unsigned a,uint32 v) {
	if(!L1Address(a))return false;
	const unsigned phase=owner.lease.Result().phase;
	if(!((phase==4 && !owner.resetStage) || (phase==8 && owner.resetStage==4)) || !Selected()
		|| PioneerRead32(reset)!=0x47800 || PioneerRead32(reset)!=0x47800)return false;
	PioneerWrite32(memory+a/4,v);return true;
}
bool IO::NativeDramRead(unsigned a,uint32& v) {
	if(!PioneerNativeCompactQ4StreamLayout::DramContains(PioneerNativeCompactQ4StreamVariant::Width(owner.sample),a))return false;
	const unsigned phase=owner.lease.Result().phase;
	if(!(phase==2 || phase==3 || phase==7) || !(owner.resetStage==0 || owner.resetStage==4) || !Selected(true)
		|| PioneerRead32(reset)!=0x47800 || PioneerRead32(reset)!=0x47800)return false;
	v=PioneerRead32(memory+(a-0x04000000)/4);return true;
}
bool IO::NativeDramWrite(unsigned a,uint32 v) {
	if(!PioneerNativeCompactQ4StreamLayout::DramContains(PioneerNativeCompactQ4StreamVariant::Width(owner.sample),a))return false;
	const unsigned phase=owner.lease.Result().phase;
	if(!((phase==3 && !owner.resetStage) || (phase==7 && owner.resetStage==4)) || !Selected(true)
		|| PioneerRead32(reset)!=0x47800 || PioneerRead32(reset)!=0x47800)return false;
	PioneerWrite32(memory+(a-0x04000000)/4,v);return true;
}
bool IO::NativeControlRead(unsigned i,uint32& v) {
	if(i>=3 || !Selected(false,false))return false;
	const unsigned offsets[]={0x234,0xa0,0x90};
	const auto* base=reinterpret_cast<const volatile uint8*>(resetWrite)-0x1b0;
	v=PioneerRead32(reinterpret_cast<const volatile uint32*>(base+offsets[i]));return true;
}
bool IO::NativePcRead(uint32& v){if(owner.resetStage!=2 || !Selected())return false;v=PioneerRead32(pc);return true;}
bool IO::NativeNow(uint64& v){if(!Active())return false;const bigtime_t t=system_time();if(t<=0 || uint64(t)>UINT64_MAX/1000)return false;v=uint64(t)*1000;return true;}
bool IO::NativePause(){if(!Active())return false;spin(100);return true;}
bool IO::NativeClockBegin() {
	if(owner.clockEntered || owner.lease.Result().phase!=1 || !Active())return false;
	owner.clockCall=true;const bool okay=BlackholeDramSweepArc::Enter(owner.clock,owner.arc,owner.telemetry,owner.clockState);owner.clockCall=false;
	if(!okay || owner.retained){owner.Retain();return false;}owner.clockEntered=true;return Access();
}
bool IO::NativeClockEnd() {
	if(!owner.clockEntered || owner.lease.Result().phase!=9 || !Active())return false;
	owner.clockCall=true;const bool okay=BlackholeDramSweepArc::Leave(owner.clock,owner.arc,owner.telemetry,owner.clockState);owner.clockCall=false;
	if(!okay || owner.retained || !owner.clockState.restored || owner.clockState.state!=TT_ARC_CLOCK_RESTORED){owner.Retain();return false;}
	owner.clockEntered=false;return Access();
}
bool ClockIO::Check(){
	const auto& r=owner.lease.Result();
	// Reject an absent clock-call lease in memory,BEFORE VM/PCI observations.
	if(owner.clockCall && !owner.closing && owner.ready && !r.compute.cold && (r.phase==1 || r.phase==9)
		&& owner.Scope() && owner.Views() && owner.Identity(true))return true;
	owner.Retain();return false;
}
void ClockIO::MemoryOnlyRetain(){owner.Retain();}
uint32 ClockIO::TlbRead(unsigned i){return i<3 && Check()?PioneerArcQueryIO::TlbRead(i):UINT32_MAX;}
void ClockIO::TlbWrite(unsigned i,uint32 v){if(i<3 && Check())PioneerArcQueryIO::TlbWrite(i,v);}
uint32 ClockIO::CsmRead(uint32 a){return Check()?PioneerArcQueryIO::CsmRead(a):UINT32_MAX;}
void ClockIO::SetQueueBounds(uint32 b,uint32 r,uint32 s){if(Check())PioneerArcQueryIO::SetQueueBounds(b,r,s);}
void ClockIO::QueueWrite(uint32 a,uint32 v){if(Check())PioneerArcQueryIO::QueueWrite(a,v);}
uint32 ClockIO::MsiStatusRead(){return Check()?PioneerArcQueryIO::MsiStatusRead():UINT32_MAX;}
void ClockIO::NotifyArc(){if(Check())PioneerArcQueryIO::NotifyArc();}
void ClockIO::Pause(){if(Check())PioneerArcQueryIO::Pause();}
} // namespace PioneerNativeCompactQ4Stream104
