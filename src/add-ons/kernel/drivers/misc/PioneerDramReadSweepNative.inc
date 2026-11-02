/* SPDX-License-Identifier: MIT */
// Included in the actual driver TU after its kernel/PCI/FD/IO definitions.
// PRIVATE DRSNODRV body only. No public opcode, install role, or Work fallback.
#include "PioneerDramReadSweepOwner.h"
#include "BlackholeDramSweepArcClock.h"
#include "BlackholeDramSweepRoutes.h"

struct PioneerDramSweepNativeReply {
 uint32 version=1,phase=0,retained=0,complete=0;
 uint32 busyMHz=0,idleMHz=0,viewsClosed=0,pciClosed=0,reservationClosed=0;
 PioneerDramReadSweepResult sweep;
 uint32 preflight=0;
};

struct PioneerDramSweepNativeIO : PioneerComputeNocIO {
 PioneerDramSweepNativeContext& owner;
 area_id sweepArea=-1,controlArea=-1,pcArea=-1,nocWriteArea=-1;
 volatile uint32* sweepMemory=NULL;
 volatile uint32* resetWrite=NULL;
 const volatile uint32* sweepPc=NULL;
 volatile uint32* nocWrite=NULL;
 PioneerDramSweepNativeIO(PioneerDramSweepNativeContext&,const tt_pioneer_profile&,
  tt_arc_scratch_result&,tt_arc_telemetry_result&,tt_tensix_reset_result&,
  tt_dram_samples_result&,tt_compute_noc_result&);
 // A retained object is deliberately NEVER deleted. Clean closure explicitly
 // consumes all view IDs before the inherited virtual-only destructors run.
 ~PioneerDramSweepNativeIO()=default;
 bool Map();
 bool Unmap()=delete; // NEVER inherit the legacy automatic rollback/retry path
 bool AfterNodeId(uint32)=delete;
 bool Access();
 uint32 CardRead(uint16,uint8);void CardWrite(uint16,uint8,uint32);
 uint32 RootRead(uint16,uint8);void RootWrite(uint16,uint8,uint32);
 bool Quiet();uint32 ReadNodeId();
 uint32 SourceRead(unsigned);uint32 HostPciRead(unsigned,uint16,uint8);
 uint32 DramIdRead(unsigned);
 uint32 ComputeNocRead(uint32);
 uint32 DramTlbRead(unsigned,unsigned);void DramTlbWrite(unsigned,unsigned,uint32);
 uint32 ResetRead();uint32 SweepEntryOverrideRead();
 uint32 ResidentNocRead(uint32);void ResidentNocWrite(uint32,uint32);
 bool SweepReadChecked(uint32,uint32&);bool SweepWrite(uint32,uint32);
 bool SweepWindowReadChecked(uint32,uint32&);
 void SweepResetWrite(uint32);uint32 SweepPcRead();void Pause();
 unsigned SweepAllocationWords();uint32* SweepSavedWords();uint32* SweepScratchWords();
 bool SweepNativeScope(const PioneerDramReadSweepOwner*,const tt_dram_samples_result*,
  const tt_compute_noc_result*,uint32*,uint32*,unsigned,unsigned);
 void SweepQuarantineMemoryOnly();bool SweepClockBegin();bool SweepClockEnd();
};

struct PioneerDramSweepClockIO : PioneerArcQueryIO {
 PioneerDramSweepNativeContext& owner;
 PioneerDramSweepClockIO(PioneerDramSweepNativeContext& o,const tt_pioneer_profile& p,
  tt_arc_scratch_result& a,tt_arc_telemetry_result& t,tt_arc_queue_result& q)
  :PioneerArcQueryIO(p,a,t,q),owner(o) {}
 bool Check();
 void MemoryOnlyRetain();
 bool AfterNodeId(uint32)=delete;bool Unmap()=delete;
 uint32 TlbRead(unsigned);void TlbWrite(unsigned,uint32);
 uint32 CsmRead(uint32);void SetQueueBounds(uint32,uint32,uint32);
 void QueueWrite(uint32,uint32);uint32 MsiStatusRead();void NotifyArc();void Pause();
};

struct PioneerDramSweepNativeContext {
 using Image=PioneerDramReadSweepOwner::SnapshotImage;
 static constexpr unsigned Allocation=Image::SavedWords+Image::RawWords;
 PioneerProbeCookie* fd;
 const thread_id thread;
 const team_id team;
 const sem_id lock;
 pci_module_info* const pci;
 pioneer_driver_reload_guard_info* const guard;
 const uint64 generation;
 const pci_info card;
 const unsigned queueDepth;
 tt_pioneer_profile profile={};
 tt_pioneer_mmio_result mapping={};tt_pioneer_mmio_transaction transaction={};
 tt_arc_scratch_result arc={};tt_arc_telemetry_result telemetry={};
 tt_tensix_reset_result tensix={};tt_dram_samples_result dram={};
 tt_compute_noc_result noc={};tt_arc_queue_result query={};
 tt_arc_clock_guard_result clockResult={};
 PioneerDramReadSweepOwner algorithm;
 PioneerDramSweepNativeReply reply;
 uint32* backing=NULL;
 bool lockHeld=true,reserved=false,quarantined=false,closing=false,clockCall=false;
 bool clockEntered=false,ready=false;
 unsigned resetStage=0;
 PioneerDramSweepNativeIO io;
 PioneerDramSweepClockIO clock;
 PioneerDramSweepNativeContext(PioneerProbeCookie* f,const thread_info& t,unsigned depth=1)
  :fd(f),thread(t.thread),team(t.team),lock(sLock),pci(sPCI),guard(sReloadGuard),
   generation(sReloadGeneration),card(sCard),queueDepth(depth),
   io(*this,profile,arc,telemetry,tensix,dram,noc),clock(*this,profile,arc,telemetry,query) {}
 ~PioneerDramSweepNativeContext(){free(backing);}
 bool View(area_id id,const volatile void* pointer,unsigned offset,size_t bytes,bool write) const {
  if(id<0 || !pointer)return false;
  area_info info={};
  const uint32 permissions=B_KERNEL_READ_AREA|(write?B_KERNEL_WRITE_AREA:0);
  return get_area_info(id,&info)==B_OK && info.area==id && info.size==bytes
   && reinterpret_cast<uintptr_t>(info.address)+offset==reinterpret_cast<uintptr_t>(pointer)
   && (info.protection&(B_KERNEL_READ_AREA|B_KERNEL_WRITE_AREA))==permissions
   && !(info.protection&(B_READ_AREA|B_WRITE_AREA|B_EXECUTE_AREA|B_KERNEL_EXECUTE_AREA));
 }
 bool ViewsPresent() const {
  return View(io.sweepArea,io.sweepMemory,0,0x200000,true)
   && View(io.controlArea,io.resetWrite,0x1b0,B_PAGE_SIZE,true)
   && View(io.pcArea,io.sweepPc,0x140,B_PAGE_SIZE,false)
   && View(io.nocWriteArea,io.nocWrite,0,B_PAGE_SIZE,true)
   && View(io.nocArea,io.nocRegisters,0,2*B_PAGE_SIZE,false)
   && io.dataArea<0 && !io.data
   && View(io.dramArea,io.dramNiu,0,B_PAGE_SIZE,false)
   && View(io.resetArea,io.reset,0x1b0,B_PAGE_SIZE,false)
   && View(io.csmArea,io.csm,0,0x20000,false)
   && View(io.scratchArea,io.scratch,0,B_PAGE_SIZE,false)
   && View(io.tlbArea,io.tlb,0x96c,B_PAGE_SIZE,true)
   && View(io.area,io.nodeID,0x44,B_PAGE_SIZE,false)
   && View(clock.queueWriteArea,clock.queueWrite,0,TT_ARC_QUEUE_CSM_BYTES,true)
   && View(clock.msiArea,clock.msi,0,B_PAGE_SIZE,true)
   && View(clock.csmArea,clock.csm,0,0x20000,false)
   && View(clock.scratchArea,clock.scratch,0,B_PAGE_SIZE,false)
   && View(clock.tlbArea,clock.tlb,0x96c,B_PAGE_SIZE,true)
   && View(clock.area,clock.nodeID,0x44,B_PAGE_SIZE,false);
 }
 bool Scope() const {
  if(sDramSweepNativeContext!=this || !lockHeld || quarantined || !fd
   || fd->magic!=0x50313030 || fd->team!=team || fd->context || sRuntimeOwner
   || sBankStorageOnly || !sOpenCookies || sReloadQuiesced || sColdRecoveryRequired
   || lock<0 || lock!=sLock || !pci || pci!=sPCI || !reserved
   || !guard || guard!=sReloadGuard || guard->version!=1
   || !generation || generation!=sReloadGeneration
   || !backing || card.bus!=sCard.bus || card.device!=sCard.device
   || card.function!=sCard.function || card.vendor_id!=0x1e52 || card.device_id!=0xb140
   || card.u.h0.subsystem_vendor_id!=0x1e52 || card.u.h0.subsystem_id!=0x0043)return false;
  thread_info current={};int32 count=1;
  return find_thread(NULL)==thread && get_thread_info(thread,&current)==B_OK
   && current.team==team && get_sem_count(lock,&count)==B_OK && count<=0;
 }
 void Retain() {
  // CPU metadata only, including the shared KEEP_LOADED cold-recovery latch.
  // No destructor/VM delete/MMIO/ARC/reset/PCI release after this point.
  quarantined=true;ready=false;clockCall=false;reply.retained=1;
  fd=NULL;sColdRecoveryRequired=true;
 }
 bool DeleteView(area_id& view) {
  if(!Scope() || !closing)return false;
  if(view<0)return true;
  if(delete_area(view)!=B_OK){Retain();return false;}
  view=-1;return true;
 }
 bool CloseViews() {
  if(!Scope() || !closing)return false;
  area_id* views[]={&io.sweepArea,&io.controlArea,&io.pcArea,&io.nocWriteArea,
   &clock.queueWriteArea,&clock.msiArea,&clock.csmArea,&clock.scratchArea,&clock.tlbArea,&clock.area,
   &io.nocArea,&io.dataArea,&io.dramArea,&io.resetArea,&io.csmArea,&io.scratchArea,&io.tlbArea,&io.area};
  for(auto* view:views)if(!DeleteView(*view))return false;
  io.sweepMemory=io.resetWrite=io.nocWrite=NULL;io.sweepPc=NULL;
  io.nocRegisters=io.dramNiu=io.reset=io.csm=io.scratch=io.nodeID=NULL;io.data=io.tlb=NULL;
  clock.queueWrite=clock.msi=clock.tlb=NULL;clock.csm=clock.scratch=clock.nodeID=NULL;
  return true;
 }
 bool ClosePci() {
  // No generic MmioEnd: it continues cleanup after uncertainty. Every
  // changed field here is consumed ONCE and independently read back.
  if(!Scope() || !closing || !reply.viewsClosed || !transaction.config_attempted)return false;
  for(unsigned peer=0;peer<2;peer++){
   if(!Scope())return false;
   const uint8 bus=peer?profile.root_bus:card.bus;
   pci->write_pci_config(bus,peer?0:card.device,peer?0:card.function,4,2,0);
   if(pci->read_pci_config(bus,peer?0:card.device,peer?0:card.function,4,2)!=0){Retain();return false;}
  }
  const unsigned order[]={5,4,1,0,3,2};
  for(unsigned i:order){
   if(!Scope())return false;
   pci->write_pci_config(card.bus,card.device,card.function,0x10+4*i,4,mapping.bars_before[i]);
   if(pci->read_pci_config(card.bus,card.device,card.function,0x10+4*i,4)!=mapping.bars_before[i]){Retain();return false;}
  }
  const unsigned bridgeOrder[]={1,2,0};
  for(unsigned i:bridgeOrder){
   if(!Scope())return false;
   pci->write_pci_config(profile.root_bus,0,0,0x24+4*i,4,mapping.bridge_before[i]);
   if(pci->read_pci_config(profile.root_bus,0,0,0x24+4*i,4)!=mapping.bridge_before[i]){Retain();return false;}
  }
  // Final reread detects a later restore damaging an earlier field.
  for(unsigned i=0;i<6;i++)if(!Scope()
   || pci->read_pci_config(card.bus,card.device,card.function,0x10+4*i,4)!=mapping.bars_before[i]){Retain();return false;}
  for(unsigned i=0;i<3;i++)if(!Scope()
   || pci->read_pci_config(profile.root_bus,0,0,0x24+4*i,4)!=mapping.bridge_before[i]){Retain();return false;}
  if(!Scope() || pci->read_pci_config(card.bus,card.device,card.function,4,2)!=0
   || pci->read_pci_config(profile.root_bus,0,0,4,2)!=0){Retain();return false;}
  mapping.restored=1;mapping.stage=5;transaction={};return true;
 }
};

PioneerDramSweepNativeIO::PioneerDramSweepNativeIO(PioneerDramSweepNativeContext& o,
 const tt_pioneer_profile& p,tt_arc_scratch_result& a,tt_arc_telemetry_result& t,
 tt_tensix_reset_result& c,tt_dram_samples_result& d,tt_compute_noc_result& n)
 :PioneerComputeNocIO(p,a,t,c,d,n,2),owner(o) {}

bool PioneerDramSweepNativeIO::Access(){
 if(owner.Scope() && !owner.closing && (!owner.ready || owner.ViewsPresent()))return true;
 owner.Retain();return false;
}
bool PioneerDramSweepNativeIO::Map(){
 if(!Access() || !PioneerComputeNocIO::Map())return false;
 const uint32 offsets[]={0x19000000,0x19312000,0x19313000,0x19320000};
 area_id* ids[]={&sweepArea,&controlArea,&pcArea,&nocWriteArea};
 void* pages[4]={};
 for(unsigned i=0;i<4;i++){
  if(!Access())return false;
  *ids[i]=map_physical_memory("P100A private DRAM READ sweep",profile.bar0_host_start+offsets[i],
   i?B_PAGE_SIZE:0x200000,B_ANY_KERNEL_ADDRESS,
   B_KERNEL_READ_AREA|B_UNCACHED_MEMORY|(i==2?0:B_KERNEL_WRITE_AREA),&pages[i]);
  if(*ids[i]<B_OK)return false;
 }
 sweepMemory=(volatile uint32*)pages[0];resetWrite=(volatile uint32*)((uint8*)pages[1]+0x1b0);
 sweepPc=(const volatile uint32*)((uint8*)pages[2]+0x140);nocWrite=(volatile uint32*)pages[3];
 return owner.clock.PioneerArcQueryIO::Map(); // virtual aliases only;no ARC request
}
uint32 PioneerDramSweepNativeIO::CardRead(uint16 a,uint8 n){return Access()?PioneerMmioIO::CardRead(a,n):UINT32_MAX;}
void PioneerDramSweepNativeIO::CardWrite(uint16 a,uint8 n,uint32 v){if(Access())PioneerMmioIO::CardWrite(a,n,v);}
uint32 PioneerDramSweepNativeIO::RootRead(uint16 a,uint8 n){return Access()?PioneerMmioIO::RootRead(a,n):UINT32_MAX;}
void PioneerDramSweepNativeIO::RootWrite(uint16 a,uint8 n,uint32 v){if(Access())PioneerMmioIO::RootWrite(a,n,v);}
bool PioneerDramSweepNativeIO::Quiet(){return Access() && PioneerMmioIO::Quiet();}
uint32 PioneerDramSweepNativeIO::ReadNodeId(){return Access()?PioneerMmioIO::ReadNodeId():UINT32_MAX;}
uint32 PioneerDramSweepNativeIO::SourceRead(unsigned i){return Access()?PioneerDramSamplesIO::SourceRead(i):UINT32_MAX;}
uint32 PioneerDramSweepNativeIO::DramIdRead(unsigned i){return Access()?PioneerDramSamplesIO::DramIdRead(i):UINT32_MAX;}
uint32 PioneerDramSweepNativeIO::ComputeNocRead(uint32 a){return Access()?PioneerComputeNocIO::ComputeNocRead(a):UINT32_MAX;}
uint32 PioneerDramSweepNativeIO::HostPciRead(unsigned p,uint16 a,uint8 n){
 return p<2?(p?RootRead(a,n):CardRead(a,n)):UINT32_MAX;
}
uint32 PioneerDramSweepNativeIO::DramTlbRead(unsigned s,unsigned i){return Access()?PioneerDramSamplesIO::DramTlbRead(s,i):UINT32_MAX;}
void PioneerDramSweepNativeIO::DramTlbWrite(unsigned s,unsigned i,uint32 v){if(Access())PioneerDramSamplesIO::DramTlbWrite(s,i,v);}
uint32 PioneerDramSweepNativeIO::ResetRead(){return Access()?PioneerTensixIO::ResetRead():UINT32_MAX;}
uint32 PioneerDramSweepNativeIO::SweepEntryOverrideRead(){
 // Already-owned reset/control page: CSR0xffb12234, never a write.
 return Access() && owner.ready?PioneerRead32(resetWrite+(0x234-0x1b0)/4):UINT32_MAX;
}
uint32 PioneerDramSweepNativeIO::ResidentNocRead(uint32 a){
 if(!Access())return UINT32_MAX;
 const bool extra=(a<=0x24 && !(a&3)) || a==0x64 || a==0x68 || a==0x178
  || (a>=0x208 && a<=0x214 && !(a&3)) || (a>=0x240 && a<0x2c0 && !(a&3));
 return extra || BlackholeComputeNocReadOffset(a)?PioneerRead32(nocRegisters+a/4):UINT32_MAX;
}
void PioneerDramSweepNativeIO::ResidentNocWrite(uint32 a,uint32 v){
 if(!Access())return;
 if((a<=0x24 && !(a&3)) || a==0x100)PioneerWrite32(nocWrite+a/4,v);
 else owner.Retain(); // especially forbidden:host doorbell0x40
}
static bool PioneerDramSweepL1Address(uint32 a){
 using I=PioneerDramSweepNativeContext::Image;
 return !(a&3) && (a==0x6000 || (a>=I::Entry && a<I::Entry+I::TextWords*4)
  || (a>=0x6e000 && a<0x70000) || (a>=0x74000 && a<0x74040)
  || (a>=0x74100 && a<0x74140) || (a>=0x90000 && a<0x94000));
}
bool PioneerDramSweepNativeIO::SweepReadChecked(uint32 a,uint32& v){
 if(!Access() || !owner.ready || !PioneerDramSweepL1Address(a))return false;
 for(unsigned i=0;i<3;i++)if(DramTlbRead(0,i)!=BlackholeDramFixedTlbWord(0,1,2,false,i))return false;
 v=PioneerRead32(sweepMemory+a/4);return true;
}
bool PioneerDramSweepNativeIO::SweepWrite(uint32 a,uint32 v){
 if(!Access() || !owner.ready || !PioneerDramSweepL1Address(a))return false;
 for(unsigned i=0;i<3;i++)if(DramTlbRead(0,i)!=BlackholeDramFixedTlbWord(0,1,2,false,i))return false;
 PioneerWrite32(sweepMemory+a/4,v);return true;
}
bool PioneerDramSweepNativeIO::SweepWindowReadChecked(uint32 a,uint32& v){
 const unsigned chunk=owner.reply.sweep.report[4],depth=owner.queueDepth;
 if((chunk!=64 && chunk!=1024 && chunk!=4096 && chunk!=16384)
  || !depth || depth>16 || (depth&(depth-1)) || chunk*depth>16384)return false;
 const unsigned bytes=chunk*depth;
 if(!Access() || !owner.ready || owner.reply.sweep.phase!=7 || !owner.reply.sweep.held
  || (a&3) || a<0x200000-bytes || a>=0x200000)return false;
 const unsigned bank=owner.reply.sweep.bank;
 if(bank>=7)return false;
 for(unsigned i=0;i<3;i++)if(DramTlbRead(0,i)!=BlackholeDramFixedTlbWord(0x04600000,
  owner.dram.translated[bank][0],owner.dram.translated[bank][1],true,i))return false;
 v=PioneerRead32(sweepMemory+a/4);return true;
}
void PioneerDramSweepNativeIO::SweepResetWrite(uint32 v){
 if(!Access() || !owner.ready)return;
 const auto& r=owner.reply.sweep;
 const bool allowed=(v==0x87ffff && !owner.resetStage && r.phase==4 && !r.launched)
  || (v==0x46800 && owner.resetStage==1 && r.phase==5 && r.launched)
  || (v==0x47800 && owner.resetStage==2 && r.phase==6 && r.observed);
 if(!allowed){owner.Retain();return;}
 owner.resetStage++; // BEFORE potentially effective write;no second attempt
 PioneerWrite32(resetWrite,v);
}
uint32 PioneerDramSweepNativeIO::SweepPcRead(){return Access() && owner.ready?PioneerRead32(sweepPc):UINT32_MAX;}
void PioneerDramSweepNativeIO::Pause(){if(Access())spin(100);}
unsigned PioneerDramSweepNativeIO::SweepAllocationWords(){return owner.Scope()?owner.Allocation:0;}
uint32* PioneerDramSweepNativeIO::SweepSavedWords(){return owner.Scope()?owner.backing:NULL;}
uint32* PioneerDramSweepNativeIO::SweepScratchWords(){return owner.Scope()?owner.backing+PioneerDramSweepNativeContext::Image::SavedWords:NULL;}
bool PioneerDramSweepNativeIO::SweepNativeScope(const PioneerDramReadSweepOwner* a,
 const tt_dram_samples_result* d,const tt_compute_noc_result* n,uint32* saved,uint32* scratch,unsigned count,unsigned raw){
 return owner.Scope() && owner.ready && !owner.closing && !owner.clockCall
  && a==&owner.algorithm && d==&owner.dram && n==&owner.noc
  && saved==owner.backing && scratch==owner.backing+PioneerDramSweepNativeContext::Image::SavedWords
  && count==PioneerDramSweepNativeContext::Image::SavedWords && raw==PioneerDramSweepNativeContext::Image::RawWords
  && owner.ViewsPresent() && sweepArea>=0 && controlArea>=0 && pcArea>=0 && nocWriteArea>=0
  && sweepMemory && resetWrite && sweepPc && nocWrite && nocRegisters && reset && tlb && nodeID;
}
void PioneerDramSweepNativeIO::SweepQuarantineMemoryOnly(){owner.Retain();}
bool PioneerDramSweepNativeIO::SweepClockBegin(){
 if(!Access() || !owner.ready || owner.clockEntered || owner.clockCall)return false;
 owner.clockCall=true;
 const bool okay=BlackholeDramSweepArc::Enter(owner.clock,owner.arc,owner.telemetry,owner.clockResult);
 owner.clockCall=false;
 if(!okay || owner.quarantined){owner.Retain();return false;}
 owner.clockEntered=true;owner.reply.busyMHz=owner.clockResult.busy_mhz;return true;
}
bool PioneerDramSweepNativeIO::SweepClockEnd(){
 if(!Access() || !owner.ready || !owner.clockEntered || owner.clockCall)return false;
 owner.clockCall=true;
 const bool okay=BlackholeDramSweepArc::Leave(owner.clock,owner.arc,owner.telemetry,owner.clockResult);
 owner.clockCall=false;
 if(!okay || owner.quarantined || !owner.clockResult.restored
  || owner.clockResult.state!=TT_ARC_CLOCK_RESTORED){owner.Retain();return false;}
 owner.reply.idleMHz=owner.clockResult.after_mhz;owner.clockEntered=false;return true;
}
bool PioneerDramSweepClockIO::Check(){
 if(owner.Scope() && owner.ready && owner.ViewsPresent() && owner.clockCall && !owner.closing
  && tlbArea>=0 && queueWriteArea>=0 && msiArea>=0 && tlb && queueWrite && msi)return true;
 owner.Retain();return false;
}
void PioneerDramSweepClockIO::MemoryOnlyRetain(){owner.Retain();}
uint32 PioneerDramSweepClockIO::TlbRead(unsigned i){return Check()?PioneerArcQueryIO::TlbRead(i):UINT32_MAX;}
void PioneerDramSweepClockIO::TlbWrite(unsigned i,uint32 v){if(Check())PioneerArcQueryIO::TlbWrite(i,v);}
uint32 PioneerDramSweepClockIO::CsmRead(uint32 a){return Check()?PioneerArcQueryIO::CsmRead(a):UINT32_MAX;}
void PioneerDramSweepClockIO::SetQueueBounds(uint32 b,uint32 r,uint32 s){if(Check())PioneerArcQueryIO::SetQueueBounds(b,r,s);}
void PioneerDramSweepClockIO::QueueWrite(uint32 a,uint32 v){if(Check())PioneerArcQueryIO::QueueWrite(a,v);}
uint32 PioneerDramSweepClockIO::MsiStatusRead(){return Check()?PioneerArcQueryIO::MsiStatusRead():UINT32_MAX;}
void PioneerDramSweepClockIO::NotifyArc(){if(Check())PioneerArcQueryIO::NotifyArc();}
void PioneerDramSweepClockIO::Pause(){if(Check())PioneerArcQueryIO::Pause();}

[[maybe_unused]] static status_t RunPioneerDramReadSweepNative(PioneerProbeCookie* fd,
 unsigned bank,unsigned shift,unsigned sweeps,uint32 nonce,PioneerDramSweepNativeReply& output,
 bool preflightOnly=false,uint64 expectedGeneration=0,unsigned depth=1){
 // Kernel-private buffers only. A future public adapter must copy/validate
 // user DATA independently;this function is not exposed by ProbeControl.
 if(geteuid()!=0 || !sPCI || !sReloadGuard || sLock<0 || !fd || fd->magic!=0x50313030 || fd->context
  || bank>=7 || !nonce || !sweeps || sweeps>256 || !depth || depth>16 || (depth&(depth-1))
  || (!PioneerDramReadSweepOwner::Queued && depth!=1)
  || (shift!=6 && shift!=10 && shift!=12 && shift!=14))return B_BAD_VALUE;
 if((1U<<shift)*depth>16384)return B_BAD_VALUE;
 thread_info thread={};status_t status=get_thread_info(find_thread(NULL),&thread);
 if(status!=B_OK || thread.team!=fd->team)return B_NOT_ALLOWED;
 status=AcquireProbeLock();if(status!=B_OK)return status;
 if(expectedGeneration && expectedGeneration!=sReloadGeneration){release_sem(sLock);return B_BAD_VALUE;}
 if(sDramSweepNativeContext){release_sem(sLock);return B_BUSY;}
 auto* c=new(std::nothrow) PioneerDramSweepNativeContext(fd,thread,depth);
 if(!c){release_sem(sLock);return B_NO_MEMORY;}
 c->backing=(uint32*)malloc(c->Allocation*sizeof(uint32));
 if(!c->backing){delete c;release_sem(sLock);return B_NO_MEMORY;}
 sDramSweepNativeContext=c;
 status=sPCI->reserve_device(sCard.bus,sCard.device,sCard.function,TT_PCI_PROBE_DEVICE,&sCookie);
 if(status!=B_OK){sDramSweepNativeContext=NULL;delete c;release_sem(sLock);return status;}
 c->reserved=true;c->reply.phase=1;
 if(!sDeviceManager)status=get_module(B_DEVICE_MANAGER_MODULE_NAME,(module_info**)&sDeviceManager);
 const bool profile=status==B_OK && c->Scope() && GetPioneerPciProfile(sDeviceManager,sCard,c->profile)==B_OK
  && PioneerMinimumBarLayoutMatches(c->profile);
 const bool began=profile && BlackholePioneerMmioBegin(c->io,c->mapping,c->transaction);
 c->reply.phase=2;
 // Do not instantiate the old bool-selected GDDR SAMPLE-WRITE path or its
 // 2240-byte stack buffer for a route-only diagnostic. ARC/telemetry/Tensix
 // retain their existing bounded read-only prerequisite policies.
 const bool proved=began && c->ViewsPresent()
  && c->io.PioneerTensixIO::AfterNodeId(c->mapping.node_id[0])
  && BlackholeDramSweepRoutes(c->io,c->telemetry,c->tensix,c->dram)
  && BlackholeComputeNocProbe(c->io,c->dram,c->noc,2);
 c->ready=proved && c->Scope() && c->ViewsPresent();
 bool okay=false;
 if(c->ready && preflightOnly){c->reply.preflight=1;okay=true;}
 else if(c->ready)okay=c->algorithm.Execute(c->io,c->dram,c->noc,bank,shift,sweeps,nonce,c->reply.sweep,depth);
 if(okay && (c->reply.sweep.closed || c->reply.preflight) && !c->quarantined && !c->clockEntered){
  c->reply.phase=3;c->closing=true;c->ready=false;
  okay=c->CloseViews();c->reply.viewsClosed=okay;
  if(okay){okay=c->ClosePci();c->reply.pciClosed=okay;}
 }else if(!c->transaction.config_attempted && !c->quarantined){
  // Refusal BEFORE the first physical mutation: release only reservation.
  c->closing=true;okay=true;
 }
 if(okay && c->Scope()){
  status=c->pci->unreserve_device(c->card.bus,c->card.device,c->card.function,TT_PCI_PROBE_DEVICE,&sCookie);
  if(status==B_OK){c->reserved=false;c->reply.reservationClosed=1;}
  else okay=false;
 }
 if(!okay || !c->reply.reservationClosed){
  c->Retain();output=c->reply;c->lockHeld=false;release_sem(sLock);return B_DEV_NOT_READY;
 }
 c->reply.complete=!preflightOnly && c->reply.sweep.closed && c->reply.viewsClosed && c->reply.pciClosed;
 output=c->reply;c->lockHeld=false;sDramSweepNativeContext=NULL;delete c;release_sem(sLock);
 return output.complete || (preflightOnly && output.preflight && output.viewsClosed
  && output.pciClosed && output.reservationClosed)?B_OK:B_NOT_ALLOWED;
}
