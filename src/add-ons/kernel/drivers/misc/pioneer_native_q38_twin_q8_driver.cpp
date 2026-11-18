/* SPDX-License-Identifier: MIT */
// Private one-shard comparison only. Do not attach to existing Work routes.
#if !defined(_KERNEL_MODE) || !defined(PIONEER_Q38_TWIN_Q8_NATIVE_APERTURE) \
 || PIONEER_Q38_TWIN_Q8_NATIVE_APERTURE != 1 || PIONEER_DRIVER_BUILD_TAG != 0x2026111700010223ULL \
 || !defined(TT_PIONEER_DRIVER_RELOAD) || !defined(TT_PIONEER_EXPERIMENTAL_Q38_DISPATCH)
#error Exact tag223 comparison driver role required
#endif
#if defined(PIONEER_Q38_TOKEN_NATIVE_CONTEXT) || defined(PIONEER_Q38_FULL_OFFLOAD_NATIVE) \
 || defined(TT_PIONEER_Q38_WORK_MODE) || defined(PIONEER_Q38_WORK_INFERENCE) \
 || defined(PIONEER_Q38_CPU_VALIDATION) || defined(PIONEER_Q38_GDN_MUTABLE_STATE) \
 || defined(PIONEER_NATIVE_BFP_DIAGNOSTIC) || defined(PIONEER_Q38_SPAN_PRELOAD) \
 || defined(__FAST_MATH__)
#error Twin Q8 diagnostic cannot share a model or legacy route
#endif
#include <Drivers.h> // declare the real C ABI before renaming the legacy definition
#define PIONEER_NATIVE_BFP_DIAGNOSTIC 1 // existing legacy find is inert
#define find_device PioneerTwinQ8InertLegacyFind
#include "tenstorrent_pci_probe.cpp"
#undef find_device
#undef PIONEER_NATIVE_BFP_DIAGNOSTIC
#include "PioneerQ38TwinQ8NativeAperture.h"
#include "PioneerQ38TwinQ8DiagnosticProtocol.h"
#include "PioneerQ38TwinQ8Payload.h"
#include "../../../../../tools/pioneer/PioneerQ38PushedWorkerLayout.h"

namespace PioneerQ38TwinQ8Diagnostic {
namespace A=PioneerQ38TwinQ8NativeAperture;
namespace L=PioneerQ38TwinQ8L1Lease;
namespace G=PioneerQ38PushedWorkerLayout;
using ROM=PioneerQ38TwinQ8Payload::ROM;
using L::Tile;
static_assert(Tag==PIONEER_DRIVER_BUILD_TAG && S::Worker==69,"exact private program binding");

class Owner;
static Owner* sRetainedTwinQ8=nullptr;
class Owner {
 PioneerStoreContext& c;
 PioneerProbeCookie& cookie;
 PioneerSessionIO& io;
 const uint64_t generation;
 const S::Arm arm;
 const unsigned variant;
 Request r;
 uint32_t scratch[4096/4]={}; // resident DMA staging, never kernel stack
 uint32_t saved[L::SavedWords]={};
 PioneerQ38CohortWorker peer;
 A::Access<Owner> aperture;
 L::Lease journal;
 S::Binding binding={};
 const ROM rom={};
 bool retained=false,started=false,provedStopped=false;
 decltype(c.baseline.baseline.baseline.baseline.dram)& Dram(){return c.baseline.baseline.baseline.baseline.dram;}
 decltype(c.baseline.baseline.baseline.noc)& RootNoc(){return c.baseline.baseline.baseline.noc;}

 bool Participant(Tile tile){
  if(!Scope() || io.runtimeCore!=(tile==Tile::Root?0U:unsigned(S::Worker)))return false;
  if(tile==Tile::Root)return PioneerQ38PanelSources(io,Dram(),RootNoc(),c.lease,c.banks);
  return PioneerQ38PanelSources(io,Dram(),peer.noc,peer.Store(),c.banks);
 }
 bool Upload(){
  const uint32_t* payload=rom.WeightWords(arm);
  if(!payload || !Scope() || c.uploadedPages || !io.SelectRuntimeCore(0))return false;
  if(!c.lease.ready && (c.lease.saved || !PioneerResidentLeaseBegin(io,RootNoc(),c.lease)))return false;
  for(unsigned page=0;page<S::WeightPages;page++){
   r.item=page;
   uint32_t address=0,route=0,index=0;
   if(!Scope() || !c.banks.Page(Dram(),0,4096*page,address,route,index)
    || route!=S::Route || index!=page || address!=PioneerResidentBanks::Base+4096*page
    || c.initialized[index])return false;
   if(!page)binding.address=address;
   for(unsigned w=0;w<1024;w++)scratch[w]=payload[page*1024+w];
   tt_resident_store_result result={};
   const bool uploaded=PioneerResidentStorePage(io,Dram(),RootNoc(),c.host,c.lease,
    PioneerResidentBanks::Capacity,4096*page,true,scratch,result,&c.banks,0);
   r.uploadState=result.state;r.uploadCompleted=result.completed;
   r.uploadMismatches=result.mismatches;r.uploadRecovery=result.recovery_required;
   if(!uploaded || result.version!=1 || result.state!=TT_STORE_PASSED || !result.wrote_device
    || result.requests!=3 || result.completed!=3 || result.mismatches || result.recovery_required)return false;
   c.initialized[index]=1;c.hashes[index]=PioneerResidentPageHash(scratch);c.uploadedPages++;r.uploaded++;
  }
  return Scope() && r.uploaded==S::WeightPages && c.uploadedPages==S::WeightPages;
 }
 bool BindBoot(){
  if(!Participant(Tile::Worker))return false;
  auto* b=binding.boot;
  const uint32_t h[]={S::Magic,1,64,S::Worker,S::Peer,peer.Store().config|0x10000U,8192,64};
  for(unsigned i=0;i<8;i++){b[i]=h[i];b[8+i]=(i&1)?uint32_t(generation>>32):uint32_t(generation)|1U;}
  for(unsigned i=0;i<16;i++){
   if(!Scope())return false;
   b[16+i]=io.ResidentNocRead(BlackholeDramSourceOffset(i));
   if(b[16+i]!=(i?peer.noc.source[0][i]:h[5]))return false;
  }
  for(unsigned i=40;i<55;i++)b[i]=G::BootWord(i);
  b[S::W::BootIndex]=arm==S::Arm::Native?unsigned(S::W::BootTag):0U;
  return S::Shape(arm,binding) && Participant(Tile::Worker);
 }
 bool Started(){
  if(!Scope() || !Participant(Tile::Worker) || !aperture.Held(Tile::Worker)
   || (io.RuntimeControlRead(0)&1) || io.RuntimeControlRead(1) || io.RuntimeControlRead(2))return false;
  // QGW3 checks NIU0 command word five before publishing READY.
  // The peer lease admitted all ten original words as zero and its
  // verified END restores that baseline. Arm must not ring a doorbell.
  r.step=Step::SelfRoute;
  auto read=[&](unsigned offset,uint32_t& value){
   if(!Scope())return false;
   value=io.ResidentNocRead(offset);return true;
  };
  auto write=[&](unsigned offset,uint32_t value){
   if(offset!=S::NiuSelfRouteOffset || value!=S::Peer || !aperture.Held(Tile::Worker))return false;
   io.ResidentNocWrite(offset,value);return true;
  };
  if(!S::StageSelfRoute(read,write) || !Participant(Tile::Worker) || !aperture.Held(Tile::Worker))return false;
  r.step=Step::Start;
  // Bind the attempted lifetime before the very first reset write.
  if(!journal.NoteRelease() || !aperture.NoteRelease() || !Scope())return false;
  io.RuntimeResetWrite(0x87ffff);io.Pause();
  if(!Scope() || io.ResetRead()!=0x87ffff || io.ResetRead()!=0x87ffff || !Participant(Tile::Worker))return false;
  if(!Scope())return false;
  io.RuntimeResetWrite(0x46800);io.Pause();
  const uint64_t start=Now();
  for(unsigned poll=0;poll<10000;poll++){
   const uint64_t now=Now();if(now<start || now-start>=UINT64_C(1000000))return false;
   if(!aperture.Read(Tile::Worker,S::Status+8,r.bootFault) || r.bootFault)return false;
   if(!aperture.Read(Tile::Worker,S::Status+12,r.bootStatus) || (r.bootStatus && r.bootStatus!=S::Magic))return false;
   if(r.bootStatus==S::Magic){
    if(!Scope())return false;
    r.bootPC=io.RuntimePcRead();
    if(r.bootPC==ROM::FaultPC)return false;
    if(r.bootPC==ROM::WaitPC || r.bootPC==ROM::WaitPC+4){started=true;return Participant(Tile::Worker);}
   }
   Poll();
  }
  return false;
 }
 void SaveJournalReport(){
  const auto& d=journal.Detail();r.journalOp=d.op;r.journalWord=d.ordinal;r.journalAddress=d.address;
  r.journalSeen=d.seen;r.journalExpected=d.expected;
 }
public:
 Owner(PioneerStoreContext& ctx,PioneerProbeCookie& fd,const Request& result)
  :c(ctx),cookie(fd),io(ctx.io),generation(sReloadGeneration),arm(S::Arm(result.arm)),
   variant(result.variant),r(result),aperture(*this,io),journal(saved,L::SavedWords){}
 const Request& Report()const{return r;}
 bool Scope(){
  return !retained && sRuntimeOwner==&cookie && cookie.context==&c && cookie.magic==0x50313030
   && sOpenCookies==1 && !sColdRecoveryRequired && !sReloadQuiesced && generation && generation==sReloadGeneration
   && sBankStorageOnly && c.bankStorageOnly && !c.Unsafe() && c.bytes==PioneerResidentBanks::Total
   && c.host.state==TT_HOST_PAGE_READY && !c.host.released && c.initialized && c.hashes;
 }
 bool WorkerStopProved(){return provedStopped && Scope();}
 uint64_t Now(){return uint64_t(system_time());}
 void Poll(){io.Pause();}
 bool Read(Tile t,unsigned at,uint32_t& v){return aperture.Read(t,at,v);}
 bool Write(Tile t,unsigned at,uint32_t v){return aperture.Write(t,at,v);}
 bool Count(Tile t,unsigned at,uint32_t& v){return aperture.Count(t,at,v);}
 bool Begin(S::Arm use,const S::Binding& b){
  return Scope() && started && use==arm && &b==&binding && r.uploaded==S::WeightPages;
 }
 bool PrepareInput(unsigned which){
  if(which!=variant || which>1 || !Scope())return false;
  for(unsigned i=0;i<S::K/2;i++)if(!Write(Tile::Root,L::RootInput+i*4,S::InputWord(which,i)))return false;
  for(unsigned i=0;i<S::Blocks;i++)if(!Write(Tile::Root,L::RootScale+i*4,S::ScaleWord(which,i)))return false;
  for(unsigned i=0;i<S::K/2;i++){uint32_t v;if(!Read(Tile::Root,L::RootInput+i*4,v) || v!=S::InputWord(which,i))return false;}
  for(unsigned i=0;i<S::Blocks;i++){uint32_t v;if(!Read(Tile::Root,L::RootScale+i*4,v) || v!=S::ScaleWord(which,i))return false;}
  return Participant(Tile::Root);
 }
 bool FinishStopped(){
  uint32_t fault=0;
  if(!Read(Tile::Worker,S::Status+8,fault) || fault || !Participant(Tile::Worker))return false;
  auto stop=[](uint32_t pc){return pc==ROM::StopPC || pc==ROM::StopPC+4;};
  r.stopPC=io.RuntimePcRead();io.Pause();
  if(!stop(r.stopPC) || !Scope() || !stop(io.RuntimePcRead()) || !Participant(Tile::Worker))return false;
  io.RuntimeResetWrite(0x87ffff);io.Pause();
  if(!Scope() || io.ResetRead()!=0x87ffff || io.ResetRead()!=0x87ffff || !Participant(Tile::Worker))return false;
  io.RuntimeResetWrite(0x47800);io.Pause();
  if(!Scope() || io.ResetRead()!=0x47800 || io.ResetRead()!=0x47800 || !Participant(Tile::Worker))return false;
  provedStopped=true;
  const bool restored=journal.Restore(aperture);SaveJournalReport();
  return restored && aperture.ReleaseRestoredViews(journal);
 }
 void Retain(){
  if(retained)return;
  retained=true;aperture.Retain();journal.Retain();c.banks.Quarantine();
  c.lease.recovery_required=true;peer.Store().recovery_required=true;
  c.io.QuarantineHostPage();sColdRecoveryRequired=true;sRetainedTwinQ8=this;r.cold=1;
 } // MEMORY ONLY, no further device I/O or alias disposal
 bool Run(){
  r.step=Step::MapRoot;
  if(!Scope() || !io.SelectRuntimeCore(0) || !io.MapRuntime())return false;
  r.step=Step::Upload;if(!Upload())return false;
  r.step=Step::FleetHeld;
  io.fleetEnabled=true;
  if(!Scope() || !BlackholeDramAllCoresHeld(io,Dram()) || !PioneerWorkerNocProbe(io,Dram(),S::Worker,peer.noc))return false;
  r.step=Step::PeerStore;
  if(!Scope() || !io.SelectRuntimeCore(S::Worker) || !PioneerQ38CohortPeerLeaseBegin(io,Dram(),S::Worker,peer))return false;
  r.step=Step::Aperture;
  if(!aperture.MapHeld() || !BindBoot())return false;
  r.step=Step::Journal;
  if(!journal.Capture(aperture)){SaveJournalReport();return false;}
  r.step=Step::Image;
  if(!journal.Install(aperture,rom,arm,binding)){SaveJournalReport();return false;}
  r.step=Step::Start;if(!Started())return false;
  r.step=Step::Sequence;
  if(!S::Run(*this,arm,variant,binding,r.result))return false;
  r.step=Step::RootClose;
  return Scope() && io.SelectRuntimeCore(S::Worker) && Participant(Tile::Worker)
   && PioneerQ38DispatchPeerLeaseEnd(io,peer.noc,peer.Store()) && io.SelectRuntimeCore(0)
   && Participant(Tile::Root);
 }
};

static void Suppress(Request& r){
 r.result.matrixMicros=0;r.result.closed=r.result.stopped=0;
 for(auto& v:r.result.output)v=0;
}
static status_t Trial(PioneerProbeCookie* fd,void* data,size_t length){
 if(!data || length!=sizeof(Request))return B_BAD_VALUE;
 Request request={};
 status_t status=user_memcpy(&request,data,sizeof(request));
 const S::Arm arm=S::Arm(request.arm);
 if(status!=B_OK || request.version!=Version || request.tag!=Tag || !request.generation
  || !S::Valid(arm) || request.variant>1 || request.reserved)return status!=B_OK?status:B_BAD_VALUE;
 status=AcquireProbeLock(fd,true);if(status!=B_OK)return status;
 if(fd->context || sRuntimeOwner || sOpenCookies!=1 || request.generation!=sReloadGeneration || sRetainedTwinQ8){
  release_sem(sLock);return B_NOT_ALLOWED;
 }
 const uint64_t generation=request.generation;const unsigned variant=request.variant;
 request={};request.generation=generation;request.version=Version;request.tag=Tag;request.arm=unsigned(arm);request.variant=variant;
 request.step=Step::Begin;
 status=BeginResidentSession(fd,PioneerResidentBanks::Total,request.closed,true);
 if(status!=B_OK){release_sem(sLock);return status;}
 if(fd->context && request.closed.state==TT_RESIDENT_SESSION_READY && request.closed.active && !request.closed.cold){
  request.step=Step::Memory;
  auto* owner=new(std::nothrow) Owner(*fd->context,*fd,request);
  if(!owner)EndResidentSession(fd,request.closed,true);
  else if(!owner->Run()){owner->Retain();request=owner->Report();}
  else{
   request=owner->Report();
   request.step=Step::ResidentClose;
   EndResidentSession(fd,request.closed,true);
   if(fd->context){owner->Retain();}
   else{
    delete owner;
    if(sRuntimeOwner)sColdRecoveryRequired=true;
    else if(Closed(request.closed))request.step=Step::Complete;
   }
  }
 }else if(fd->context){
  fd->context->banks.Quarantine();fd->context->lease.recovery_required=true;
  fd->context->io.QuarantineHostPage();sColdRecoveryRequired=true;
 }
 request.cold=sColdRecoveryRequired;
 if(request.step!=Step::Complete || request.cold)Suppress(request);
 status=user_memcpy(data,&request,sizeof(request));
 release_sem(sLock);return status;
}
static status_t Free(void* value){
 auto* fd=static_cast<PioneerProbeCookie*>(value);
 if(!fd || fd->magic!=0x50313030)return B_BAD_VALUE;
 status_t held=acquire_sem(sLock);if(held!=B_OK)return held;
 if(fd->context){
  fd->context->banks.Quarantine();fd->context->lease.recovery_required=true;
  fd->context->io.QuarantineHostPage();sColdRecoveryRequired=true;
  if(sOpenCookies)sOpenCookies--;
  release_sem(sLock);return B_OK;
 }
 release_sem(sLock);return ProbeFree(fd);
}
static status_t Control(void* value,uint32 op,void* data,size_t length){
 auto* fd=static_cast<PioneerProbeCookie*>(value);
 if(geteuid()!=0 || !fd || fd->magic!=0x50313030)return B_NOT_ALLOWED;
 thread_info self={};const status_t info=get_thread_info(find_thread(NULL),&self);
 if(info!=B_OK || self.team!=fd->team)return info!=B_OK?info:B_NOT_ALLOWED;
 if(op==TT_PCI_PROBE_RELOAD_STATUS || op==TT_PCI_PROBE_RELOAD_QUIESCE)return DriverReloadControl(fd,op,data,length);
 if(op==RunOperation)return Trial(fd,data,length);
 return B_NOT_ALLOWED;
}
}
device_hooks* find_device(const char* name){
 using namespace PioneerQ38TwinQ8Diagnostic;
 static device_hooks hooks={ProbeOpen,ProbeClose,Free,Control,NULL,NULL,NULL,NULL,NULL,NULL};
 return strcmp(name,TT_PCI_PROBE_DEVICE)==0?&hooks:NULL;
}
