/* SPDX-License-Identifier: MIT */
#include "PioneerDramCohortNativeRole.h"
#include "PioneerDramCohortReadSweepOwner.h"
// Reuse ONLY the actual scoped PCI/VM/ARC/FD lease,not its old sweep factory,
// algorithm,allocation geometry,L1 allowlist,reset policy or public controls.
// Separate exact derived object/algorithm/backing/role ledger below. No ioctl
// or conventional driver entry selects this compile-only private context.
struct PioneerDramCohortNativeContext;
static PioneerDramCohortNativeContext* sDramCohortNativeContext=NULL;
struct PioneerDramCohortNativeReply {
 uint32 version=5,phase=0,retained=0,complete=0,busyMHz=0,idleMHz=0;
 uint32 viewsClosed=0,pciClosed=0,reservationClosed=0;
 PioneerDramCohortReadSweepResult sweep;
};
struct PioneerDramCohortNativeIO {
 PioneerDramCohortNativeContext& owner;
 explicit PioneerDramCohortNativeIO(PioneerDramCohortNativeContext& o):owner(o){}
 bool Access();bool Selected(unsigned,bool=true);
 uint32 SourceRead(unsigned);uint32 HostPciRead(unsigned,uint16,uint8);
 uint32 DramTlbRead(unsigned,unsigned);void DramTlbWrite(unsigned,unsigned,uint32);
 unsigned CohortAllocationWords();uint32* CohortSavedWords();uint32* CohortScratchWords();
 bool CohortNativeScope(const PioneerDramCohortReadSweepOwner*,const tt_dram_samples_result*,
  uint32*,uint32*,unsigned,unsigned);
 void CohortQuarantineMemoryOnly();bool CohortClockBegin();bool CohortClockEnd();
 uint32 CohortHeldRead(unsigned,unsigned);uint32 CohortResetRead(unsigned);uint32 CohortEntryRead(unsigned);
 uint32 CohortNocRead(unsigned,unsigned);void CohortNocWrite(unsigned,unsigned,uint32);
 bool CohortRead(unsigned,uint32,uint32&);bool CohortWrite(unsigned,uint32,uint32);
 bool CohortWindowRead(unsigned,uint32,uint32&);
 void CohortResetWrite(unsigned,uint32);uint32 CohortPcRead(unsigned);
 bool CohortHostTime(uint64&);void CohortPause();
};
struct PioneerDramCohortNativeContext final:PioneerDramSweepNativeContext {
 static constexpr unsigned AllocationWords=PioneerDramCohortReadSweepOwner::Allocation;
 const unsigned depth;
 uint32* exactBacking=NULL;
 bool cohortReady=false;
 unsigned resetStages[8]={};
 PioneerDramCohortReadSweepOwner cohort;
 PioneerDramCohortNativeReply result;
 PioneerDramCohortNativeIO cohortIO;
 PioneerDramCohortNativeContext(PioneerProbeCookie* f,const thread_info& t,unsigned d)
  :PioneerDramSweepNativeContext(f,t,1),depth(d),cohortIO(*this){}
 bool CohortScope() const {
  return sDramCohortNativeContext==this && sDramSweepNativeContext==static_cast<const PioneerDramSweepNativeContext*>(this)
   && PioneerDramSweepNativeContext::Scope() && cohortReady && ready && !closing && !clockCall
   && exactBacking && backing==exactBacking && depth && depth<=16 && !(depth&(depth-1)) && queueDepth==1
   && !reply.sweep.started && !resetStage && ViewsPresent();
 }
 bool CloseCohortResources(){
  if(!CohortScope() || !result.sweep.closed || quarantined || clockEntered)return false;
  result.phase=3;cohortReady=false;ready=false;closing=true;
  const bool views=CloseViews();result.viewsClosed=reply.viewsClosed=views;
  if(!views)return false;
  const bool pciClosed=ClosePci();result.pciClosed=reply.pciClosed=pciClosed;
  return pciClosed;
 }
};
static unsigned PioneerDramCohortX(unsigned r){return r?(PioneerDramCohortReadSweep::Worker(r)&63):1;}
static bool PioneerDramCohortL1(unsigned r,uint32 a){
 using I=BlackholeDramCohortReadSweepImage;
 if(r>=8 || (a&3))return false;
 return a==0x6000 || (a>=I::Entry && a<I::Entry+I::TextWords*4) || (a>=0x6e000 && a<0x70000)
  || (a>=0x74000 && a<0x74080) || (a>=0x74100 && a<0x74180)
  || a==0x74200 || a==0x74220 || a==0x74240
  || (r?(a>=0x90000 && a<0xd0000):(a==0x74300 || (a>=0x74400 && a<0x74480) || (a>=0x74500 && a<0x74880)));
}
bool PioneerDramCohortNativeIO::Access(){
 if(owner.CohortScope())return true;
 owner.Retain();return false;
}
bool PioneerDramCohortNativeIO::Selected(unsigned r,bool l1){
 if(r>=8 || !Access())return false;
 const unsigned x=PioneerDramCohortX(r);
 for(unsigned i=0;i<3;i++)if((l1 && DramTlbRead(0,i)!=BlackholeDramFixedTlbWord(0,x,2,false,i))
  || DramTlbRead(1,i)!=BlackholeDramFixedTlbWord(0xffa00000,x,2,false,i))return false;
 return true;
}
uint32 PioneerDramCohortNativeIO::SourceRead(unsigned i){return Access()?owner.io.SourceRead(i):UINT32_MAX;}
uint32 PioneerDramCohortNativeIO::HostPciRead(unsigned p,uint16 a,uint8 n){return Access()?owner.io.HostPciRead(p,a,n):UINT32_MAX;}
uint32 PioneerDramCohortNativeIO::DramTlbRead(unsigned s,unsigned i){return Access()?owner.io.DramTlbRead(s,i):UINT32_MAX;}
void PioneerDramCohortNativeIO::DramTlbWrite(unsigned s,unsigned i,uint32 v){if(Access())owner.io.DramTlbWrite(s,i,v);}
unsigned PioneerDramCohortNativeIO::CohortAllocationWords(){return Access()?owner.AllocationWords:0;}
uint32* PioneerDramCohortNativeIO::CohortSavedWords(){return Access()?owner.exactBacking:NULL;}
uint32* PioneerDramCohortNativeIO::CohortScratchWords(){return Access()?owner.exactBacking+BlackholeDramCohortReadSweepImage::CohortSavedWords:NULL;}
bool PioneerDramCohortNativeIO::CohortNativeScope(const PioneerDramCohortReadSweepOwner* a,
 const tt_dram_samples_result* d,uint32* saved,uint32* scratch,unsigned count,unsigned raw){
 return owner.CohortScope() && a==&owner.cohort && d==&owner.dram && saved==owner.exactBacking
  && scratch==owner.exactBacking+BlackholeDramCohortReadSweepImage::CohortSavedWords
  && count==BlackholeDramCohortReadSweepImage::CohortSavedWords && raw==BlackholeDramCohortReadSweepImage::RawWords;
}
void PioneerDramCohortNativeIO::CohortQuarantineMemoryOnly(){owner.cohortReady=false;owner.Retain();owner.result.retained=1;}
bool PioneerDramCohortNativeIO::CohortClockBegin(){
 if(!Access() || !owner.io.SweepClockBegin())return false;
 owner.result.busyMHz=owner.reply.busyMHz;return owner.CohortScope();
}
bool PioneerDramCohortNativeIO::CohortClockEnd(){
 if(!Access() || !owner.io.SweepClockEnd())return false;
 owner.result.idleMHz=owner.reply.idleMHz;return owner.CohortScope();
}
uint32 PioneerDramCohortNativeIO::CohortHeldRead(unsigned x,unsigned y){
 if(!Access() || y<2 || y>11 || !((x>=1 && x<=7) || (x>=10 && x<=14)))return UINT32_MAX;
 for(unsigned i=0;i<3;i++)if(DramTlbRead(1,i)!=BlackholeDramFixedTlbWord(0xffa00000,x,y,true,i))return UINT32_MAX;
 return owner.io.ResetRead();
}
uint32 PioneerDramCohortNativeIO::CohortResetRead(unsigned r){return Selected(r,false)?owner.io.ResetRead():UINT32_MAX;}
uint32 PioneerDramCohortNativeIO::CohortEntryRead(unsigned r){return Selected(r,false)?owner.io.SweepEntryOverrideRead():UINT32_MAX;}
uint32 PioneerDramCohortNativeIO::CohortNocRead(unsigned r,unsigned a){
 if(!Selected(r,false))return UINT32_MAX;
 bool permit=BlackholeComputeNocReadOffset(a) || (a<=0x24 && !(a&3)) || a==0x64 || a==0x68
  || a==0x148 || a==0x178 || (a>=0x240 && a<0x2c0 && !(a&3));
 for(unsigned i=0;i<9;i++)permit|=a==PioneerDramCohortReadSweep::CoordinatorLink::Offset(i);
 return permit?PioneerRead32(owner.io.nocRegisters+a/4):UINT32_MAX;
}
void PioneerDramCohortNativeIO::CohortNocWrite(unsigned r,unsigned a,uint32 v){
 if(!Selected(r,false))return;
 if(owner.resetStages[r]==2 || !((a<=0x24 && !(a&3)) || a==0x100)){
  CohortQuarantineMemoryOnly();return;
 }
 PioneerWrite32(owner.io.nocWrite+a/4,v);
}
bool PioneerDramCohortNativeIO::CohortRead(unsigned r,uint32 a,uint32& v){
 if(!PioneerDramCohortL1(r,a) || !Selected(r,true))return false;
 v=PioneerRead32(owner.io.sweepMemory+a/4);return true;
}
bool PioneerDramCohortNativeIO::CohortWrite(unsigned r,uint32 a,uint32 v){
 if(!PioneerDramCohortL1(r,a) || !Selected(r,true) || owner.resetStages[r]==2)return false;
 PioneerWrite32(owner.io.sweepMemory+a/4,v);return true;
}
bool PioneerDramCohortNativeIO::CohortWindowRead(unsigned r,uint32 a,uint32& v){
 if(r<1 || r>7 || !Selected(r,false) || owner.result.sweep.phase!=7 || owner.result.sweep.held!=255
  || owner.resetStages[r]!=3 || (a&3) || a<0x200000-owner.depth*16384 || a>=0x200000)return false;
 for(unsigned i=0;i<3;i++)if(DramTlbRead(0,i)!=BlackholeDramFixedTlbWord(0x04600000,
  owner.dram.translated[r-1][0],owner.dram.translated[r-1][1],true,i))return false;
 v=PioneerRead32(owner.io.sweepMemory+a/4);return true;
}
void PioneerDramCohortNativeIO::CohortResetWrite(unsigned r,uint32 v){
 if(!Selected(r,false))return;
 const auto& s=owner.result.sweep;auto& stage=owner.resetStages[r];
 const bool valid=(v==0x87ffff && stage==0 && s.phase==4 && !s.launched)
  || (v==0x46800 && stage==1 && s.phase==5 && (s.launched&(1U<<r)))
  || (v==0x47800 && stage==2 && s.phase==6 && s.observed==255);
 if(!valid){CohortQuarantineMemoryOnly();return;}
 stage++; // record attempt BEFORE the potentially effective physical write
 PioneerWrite32(owner.io.resetWrite,v);
}
uint32 PioneerDramCohortNativeIO::CohortPcRead(unsigned r){return Selected(r,false)?PioneerRead32(owner.io.sweepPc):UINT32_MAX;}
bool PioneerDramCohortNativeIO::CohortHostTime(uint64& value){
 if(!Access())return false;
 const bigtime_t now=system_time();if(now<=0)return false;
 value=uint64(now);return true;
}
void PioneerDramCohortNativeIO::CohortPause(){if(Access())spin(100);}

[[maybe_unused]] static status_t RunPioneerDramCohortNative(PioneerProbeCookie* fd,unsigned sweeps,
 unsigned depth,uint32 nonce,PioneerDramCohortNativeReply& output){
 if(geteuid()!=0 || !sPCI || !sReloadGuard || sLock<0 || !fd || fd->magic!=0x50313030 || fd->context
  || !nonce || !sweeps || sweeps>256 || !depth || depth>16 || (depth&(depth-1)))return B_BAD_VALUE;
 thread_info t={};status_t status=get_thread_info(find_thread(NULL),&t);
 if(status!=B_OK || t.team!=fd->team)return B_NOT_ALLOWED;
 status=AcquireProbeLock();if(status!=B_OK)return status;
 if(sDramSweepNativeContext || sDramCohortNativeContext){release_sem(sLock);return B_BUSY;}
 auto* c=new(std::nothrow) PioneerDramCohortNativeContext(fd,t,depth);
 if(!c){release_sem(sLock);return B_NO_MEMORY;}
 c->backing=(uint32*)malloc(c->AllocationWords*sizeof(uint32));c->exactBacking=c->backing;
 if(!c->backing){delete c;release_sem(sLock);return B_NO_MEMORY;}
 sDramCohortNativeContext=c;sDramSweepNativeContext=c;
 status=sPCI->reserve_device(sCard.bus,sCard.device,sCard.function,TT_PCI_PROBE_DEVICE,&sCookie);
 if(status!=B_OK){sDramCohortNativeContext=NULL;sDramSweepNativeContext=NULL;delete c;release_sem(sLock);return status;}
 c->reserved=true;c->result.phase=1;
 if(!sDeviceManager)status=get_module(B_DEVICE_MANAGER_MODULE_NAME,(module_info**)&sDeviceManager);
 const bool profile=status==B_OK && c->Scope() && GetPioneerPciProfile(sDeviceManager,sCard,c->profile)==B_OK
  && PioneerMinimumBarLayoutMatches(c->profile);
 const bool begun=profile && BlackholePioneerMmioBegin(c->io,c->mapping,c->transaction);
 c->result.phase=2;
 const bool proved=begun && c->ViewsPresent() && c->io.PioneerTensixIO::AfterNodeId(c->mapping.node_id[0])
  && BlackholeDramSweepRoutes(c->io,c->telemetry,c->tensix,c->dram)
  && BlackholeComputeNocProbe(c->io,c->dram,c->noc,2);
 c->ready=proved && c->Scope() && c->ViewsPresent();c->cohortReady=c->ready;
 bool okay=c->cohortReady && c->cohort.Execute(c->cohortIO,c->dram,sweeps,depth,nonce,c->result.sweep);
 if(okay && c->result.sweep.closed && !c->quarantined && !c->clockEntered){
  okay=c->CloseCohortResources();
 }else if(!c->transaction.config_attempted && !c->quarantined){c->closing=true;okay=true;}
 if(okay && c->Scope()){
  status=c->pci->unreserve_device(c->card.bus,c->card.device,c->card.function,TT_PCI_PROBE_DEVICE,&sCookie);
  if(status==B_OK){c->reserved=false;c->result.reservationClosed=1;}else okay=false;
 }
 if(!okay || !c->result.reservationClosed){
  c->cohortIO.CohortQuarantineMemoryOnly();output=c->result;c->lockHeld=false;release_sem(sLock);return B_DEV_NOT_READY;
 }
 c->result.complete=c->result.sweep.closed && c->result.viewsClosed && c->result.pciClosed;
 output=c->result;c->lockHeld=false;sDramCohortNativeContext=NULL;sDramSweepNativeContext=NULL;
 delete c;release_sem(sLock);return output.complete?B_OK:B_NOT_ALLOWED;
}
