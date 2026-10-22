/* SPDX-License-Identifier: MIT */
// ACTUAL extracted native lock/scope/bind/initialize/retain/end gate,plus
// ACTUAL frozen zero RV32 machine. Endpoints/clock/cookie are simulated;
// neither native MMIO qualification nor model inference is claimed.
#define PIONEER_Q38_FFN_ZERO_CONTROLLER_MODEL_ONLY
#include "pioneer_q38_ffn_zero_controller_test.cpp"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38GraphMutableBanks.h"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38NonRootHeld.h"
#include "../../../src/add-ons/kernel/drivers/misc/BlackholeArcClockGuard.h"
#define B_DEVICE_OP_CODES_END 1000
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38FfnModelProtocol.h"
#include <functional>
#include <new>
static unsigned failAllocation=0;
template<class Base,unsigned N>struct NativeAllocation:Base {
 static void* operator new(size_t n,const std::nothrow_t& t)noexcept{return failAllocation==N?nullptr: ::operator new(n,t);}
 static void operator delete(void* p)noexcept{::operator delete(p);}
 static void operator delete(void* p,const std::nothrow_t&)noexcept{::operator delete(p);}
};
using NativePlan=NativeAllocation<PioneerQ38CanonicalModelPlan,1>;
using NativeGraph=NativeAllocation<PioneerQ38GraphMutableBanks,2>;
using NativeZero=NativeAllocation<PioneerQ38FfnZeroLease,3>;
// Only heap failure injection is substituted. SAME base implementation,
// actual native bodies and frozen zero instructions still run below.
#define PioneerQ38CanonicalModelPlan NativePlan
#define PioneerQ38GraphMutableBanks NativeGraph
#define PioneerQ38FfnZeroLease NativeZero
using uint8=uint8_t;using uint32=uint32_t;using status_t=int;using thread_id=int;
constexpr int B_OK=0,B_BAD_VALUE=-4,B_NOT_ALLOWED=-3,B_BUSY=-2,B_DEV_NOT_READY=-1;
static int currentThread=42,currentTeam=17,threadStatus=0,sLock=1,acquired=0,released=0;
static int find_thread(void*){return currentThread;}
struct thread_info{int team=0;};
static int get_thread_info(int,thread_info* i){i->team=currentTeam;return threadStatus;}
static int acquire_sem(int){acquired++;return B_OK;}
static void release_sem(int){released++;}
static unsigned copyCalls=0,failCopy=0;
static status_t user_memcpy(void* to,const void* from,size_t n){if(++copyCalls==failCopy)return -99;memcpy(to,from,n);return B_OK;}
struct PioneerStoreContext;
struct PioneerMmioIO{int area=1;};
struct PioneerSessionIO:FfnZeroControlIO,PioneerMmioIO {
 using FfnZeroControlIO::FfnZeroControlIO;
 PioneerStoreContext* q38MutableContext=nullptr;
 uint32 dummy=0,*nodeID=&dummy,*tlb=&dummy,*reset=&dummy;
 int tlbArea=1,resetArea=1;
 unsigned runtimeCore=0;bool fleetEnabled=false,q38SpanAllowed=true;
 tt_dram_samples_result dram={};
 uint32 peerTlb[3]={};unsigned peerReads=0,failPeer=0;
 bool StateZeroBeforeCohort();bool StateZeroOtherCoresHeld();bool NonRootHeldOwner();bool NonRootHeldMappingStable();
 void RuntimeResetWrite(uint32 value);
 bool peerAudit=false;
 void DramTlbWrite(unsigned slot,unsigned word,uint32 v){
  if(slot!=1){FfnZeroControlIO::DramTlbWrite(slot,word,v);return;}
  Touch();assert(word<3);peerTlb[word]=v;peerAudit=true;
 }
 uint32 DramTlbRead(unsigned slot,unsigned word){
  if(slot!=1 || !peerAudit)return FfnZeroControlIO::DramTlbRead(slot,word);
  Touch();assert(word<3);return peerTlb[word];
 }
 uint32 ResetRead(){
  bool root=true;for(unsigned w=0;w<3;w++)root &= peerTlb[w]==BlackholeDramFixedTlbWord(0xffa00000,1,2,true,w);
  if(root || !peerAudit)return FfnZeroControlIO::ResetRead();
  Touch();return ++peerReads==failPeer?0x46800:0x47800;
 }
};
struct NativeClock {
 tt_arc_clock_guard_result value={};bool required=true,quarantined=false;
 NativeClock(){value.state=TT_ARC_CLOCK_BUSY_VERIFIED;value.busy_requested=value.busy_acknowledged=1;}
 const auto& Snapshot()const{return value;}
 bool RequiresExplicitEnd()const{return required;}
 void Quarantine(){quarantined=true;value.recovery_required=1;}
};
struct FakeLifecycle{bool saved=false,dirty=false,ready=false,running=false,unsafe=false,awaiting=false;};
struct NativeOwners {
 tt_dram_samples_result& dram;tt_compute_noc_result noc={};tt_host_page_result host={};
 PioneerResidentLease store;PioneerResidentBanks banks;
 explicit NativeOwners(PioneerSessionIO& io):dram(io.dram){dram=BankProof(io);noc=io.Compute();assert(banks.Bind(dram) && Prepared(io,host,store));}
};
struct PioneerStoreContext {
 PioneerSessionIO& io;PioneerResidentBanks& banks;
 struct DramRef{tt_dram_samples_result& dram;};struct Base1{DramRef baseline;tt_compute_noc_result& noc;};
 struct Base2{Base1 baseline;};struct Base3{Base2 baseline;};
 Base3 baseline;
 PioneerResidentLease& lease;PioneerBulkUploadLease& bulk;PioneerOutputBufferLease& bulkHost;PioneerWideOutputLease& wideHost;
 tt_host_page_result& host;
 NativeClock q38Clock;
 PioneerQ38CanonicalModelPlan* q38FfnModelPlan=nullptr;PioneerQ38GraphMutableBanks* q38GraphMutable=nullptr;
 PioneerQ38FfnZeroLease* q38FfnZero=nullptr;PioneerQ38GdnIdentity q38FfnModelIdentity={};
 const uint8* q38FfnModelLedger=nullptr;unsigned q38FfnModelLayer=64;
 PioneerQ38GdnBanks* q38GdnBanks=nullptr;PioneerQ38StateZeroLease* q38StateZero=nullptr;
 PioneerQ38GdnIdentity q38GdnIdentity={};PioneerQ38SpanLease* q38Span=nullptr;
 int* q38Dispatch=nullptr;void* q38Cohort=nullptr;void* fleet=nullptr;
 FakeLifecycle q38Delta,panelProbe,streamProbe;
 uint8* initialized=nullptr;uint32* hashes=nullptr;
 int q38GdnCallThread=-1,runtimeCoreMode=-1;
 bool bankStorageOnly=true,dispatchOwnerRetained=false,otherUnsafe=false;
 uint64_t bytes=PioneerResidentBanks::Total;
 PioneerStoreContext(PioneerSessionIO& i,NativeOwners& o,PioneerBulkUploadLease& b,PioneerOutputBufferLease& t,PioneerWideOutputLease& w)
  :io(i),banks(o.banks),baseline{{{{i.dram},o.noc}}},lease(o.store),bulk(b),bulkHost(t),wideHost(w),host(o.host){}
 ~PioneerStoreContext(){delete q38FfnZero;delete q38GraphMutable;delete q38FfnModelPlan;}// TEST ONLY reclaims after retention assertions
 bool Unsafe(bool controlled=false)const{return otherUnsafe || q38Clock.quarantined || banks.Quarantined()
  || (q38GraphMutable && q38GraphMutable->Unsafe()) || (q38FfnZero && (q38FfnZero->unsafe || (!controlled && q38FfnZero->running)));}
};
struct PioneerProbeCookie{PioneerStoreContext* context=nullptr;uint32 magic=0x50313030;int team=17;};
void PioneerSessionIO::RuntimeResetWrite(uint32 value){
 FfnZeroControlIO::RuntimeResetWrite(value);
 if(activeCohort){assert(q38MutableContext);q38MutableContext->q38Cohort=this;}
}
static PioneerProbeCookie* sRuntimeOwner=nullptr;
static bool sColdRecoveryRequired=false,sReloadQuiesced=false,sBankStorageOnly=true;
static uint64_t sReloadGeneration=7;
static const void* sQ38LocalDeltaOwner=nullptr;
// No competing dispatch exists in this role. Stub ONLY this unreachable
// legacy branch;the full production Retain body below is otherwise exact.
static void PioneerQ38DispatchQuarantine(int&,PioneerResidentBanks&){assert(false);}
#include "native_lock.inc"
#include "native_retain.inc"
#include "native_scope.inc"
#include "call_scope.inc"
#include "model_bind.inc"
#include "model_initialize.inc"
#include "model_control.inc"
static bool EndGate(PioneerStoreContext& c){tt_resident_session_result result={};bool clean=true;
#include "model_end.inc"
 return clean;
}
struct NativeFixture {
 PioneerSessionIO io;NativeOwners owner;PioneerOutputBufferLease transfer;PioneerBulkUploadLease bulk;PioneerWideOutputLease wide;
 PioneerStoreContext c;PioneerProbeCookie cookie{&c};PioneerQ38SpanLease span;
 std::vector<uint8> learned=std::vector<uint8>(PioneerResidentBanks::Pages,0);uint32 hash=0;
 NativeFixture(const char* path,unsigned layer=0):io(path),owner(io),c(io,owner,bulk,transfer,wide) {
  sRuntimeOwner=&cookie;sColdRecoveryRequired=sReloadQuiesced=false;sBankStorageOnly=true;sReloadGeneration=7;
  sQ38LocalDeltaOwner=nullptr;currentThread=42;currentTeam=17;threadStatus=0;acquired=released=0;copyCalls=failCopy=0;failAllocation=0;
  io.dram=owner.dram;io.q38MutableContext=&c;c.initialized=learned.data();c.hashes=&hash;span.closed=true;c.q38Span=&span;
  for(unsigned w=0;w<3;w++)io.peerTlb[w]=BlackholeDramFixedTlbWord(0xffa00000,1,2,true,w);
  assert(PioneerPrepareOutputBuffer(io,owner.host,transfer));
  assert(PioneerBulkUploadPrepare(io,owner.dram,owner.noc,owner.host,owner.store,transfer,bulk));
  assert(PioneerPrepareWideOutput(io,owner.host,transfer,wide));
  auto plan=std::unique_ptr<PioneerQ38CanonicalModelPlan>(new(std::nothrow) PioneerQ38CanonicalModelPlan);assert(plan && plan->Build());
  for(unsigned role=0;role<3;role++)for(unsigned s=0;s<7;s++){
   const auto& p=*plan->FfnStripe(layer,role,s);unsigned at=p.bank*unsigned(PioneerResidentBanks::Capacity/4096)+p.offset/4096;
   for(unsigned page=0;page<p.allocated/4096;page++)learned[at+page]=1;
  }
 }
 bool Bind(unsigned layer=0){PioneerQ38GdnCallScope scope(&cookie);return scope.Context() && BindQ38FfnModel(c,layer);}
 int Control(tt_q38_ffn_model_initialize& r){return Q38FfnModelControl(&cookie,TT_PCI_PROBE_Q38_FFN_MODEL_INITIALIZE,&r,sizeof(r));}
 void Retained(){assert(c.dispatchOwnerRetained && sColdRecoveryRequired && c.q38Clock.quarantined
  && c.q38GraphMutable && c.q38GraphMutable->Unsafe() && c.q38FfnZero && c.q38FfnZero->unsafe
  && c.lease.recovery_required && transfer.quarantined && wide.quarantined && io.HostReadMock::quarantined
  && !io.wideReleased && !io.PageMock::releases && !io.OutputBufferMock::releases && !io.pciWrites);
  unsigned before=io.touches;assert(!EndGate(c) && !io.StateZeroBeforeCohort() && !io.StateZeroOtherCoresHeld()
   && !Bind() && before==io.touches);
 }
};
int main(int argc,char** argv){
 assert(argc==2);setvbuf(stdout,nullptr,_IOLBF,0);
 (void)&Input;(void)&Prepare;(void)&Upload;(void)&Close;(void)&PreparePanel;(void)&Run;(void)&Job;(void)&ClosePanel;(void)&Finish;
 unsigned positive=0,refusals=0,faults=0;
 for(unsigned layer:{0U,31U,63U}){
  NativeFixture f(argv[1],layer);auto before=f.learned;tt_q38_ffn_model_initialize r={};r.version=1;r.layer=layer;
  int status=f.Control(r);
  if(status!=B_OK || r.state!=TT_RUNTIME_PASSED)printf("DIAGNOSTIC layer%u status%d state%u graph%u zero saved%u ready%u running%u closed%u starts%u peerReads%u\n",layer,status,r.state,unsigned(f.c.q38GraphMutable!=nullptr),unsigned(f.c.q38FfnZero && f.c.q38FfnZero->saved),unsigned(f.c.q38FfnZero && f.c.q38FfnZero->ready),unsigned(f.c.q38FfnZero && f.c.q38FfnZero->running),unsigned(f.c.q38FfnZero && f.c.q38FfnZero->closed),f.io.rootStarts,f.io.peerReads);
  assert(status==B_OK && r.state==TT_RUNTIME_PASSED && r.pages==74 && r.closed && r.initialization_published
   && !r.recovery_required && r.session && r.driver_generation==7 && EndGate(f.c));
  assert(f.c.q38GdnBanks==nullptr && f.c.q38StateZero==nullptr && !f.c.q38GdnIdentity.Valid()
   && f.c.q38FfnModelLedger==f.learned.data() && f.learned==before && f.io.rootStarts==1 && f.io.rootStops==1
   && f.io.rootHolds==1 && f.io.peerReads && f.io.lastDeviceWords.size()==303104/4 && acquired==released);
  for(auto word:f.io.lastDeviceWords)assert(!word);
  auto* recurrent=f.c.q38GraphMutable->Recurrent(f.c.q38FfnModelIdentity);assert(recurrent && !recurrent->InitializationPublished(f.c.q38FfnModelIdentity));
  unsigned touches=f.io.touches;r={};r.version=1;r.layer=layer;
  assert(f.Control(r)==B_NOT_ALLOWED && AcquireProbeLock(&f.cookie,true)==B_NOT_ALLOWED && !f.c.dispatchOwnerRetained && touches==f.io.touches);
  positive++;
 }
 for(unsigned fault=0;fault<44;fault++){
  NativeFixture f(argv[1]);unsigned before=f.io.touches;tt_q38_ffn_model_initialize r={};r.version=1;
  if(fault==0)f.span.closed=false;if(fault==1)f.span.dirty=true;if(fault==2)f.span.ready=true;if(fault==3)f.span.running=true;
  if(fault==4)f.io.q38SpanAllowed=false;if(fault==5)f.c.runtimeCoreMode=1;if(fault==6)f.c.hashes=nullptr;if(fault==7)f.c.initialized=nullptr;
  if(fault==8)f.c.bytes--;if(fault==9)f.c.bankStorageOnly=false;if(fault==10)f.io.runtimeCore=1;if(fault==11)f.io.fleetEnabled=true;
  if(fault==12)f.c.q38Dispatch=&sLock;if(fault==13)f.c.q38Cohort=&f;if(fault==14)f.c.fleet=&f;if(fault==15)sQ38LocalDeltaOwner=&f;
  if(fault==16)f.c.q38Delta.saved=true;if(fault==17)f.c.panelProbe.ready=true;if(fault==18)f.c.streamProbe.dirty=true;
  if(fault==19)f.c.q38Clock.required=false;if(fault==20)f.c.q38Clock.value.busy_acknowledged=0;
  if(fault==21)f.c.q38Clock.value.restored=1;if(fault==22)f.c.q38Clock.value.state=0;
  if(fault==23)f.c.lease.ready=false;if(fault==24)f.c.bulk.ready=false;if(fault==25)f.c.bulk.dirty=true;
  if(fault==26)f.transfer.ready=false;if(fault==27)f.transfer.released=true;if(fault==28)f.wide.memory_type_set=false;
  if(fault==29)f.wide.released=true;if(fault==30)r.layer=64;if(fault==31)r.reserved[0]=1;if(fault==32)r.reserved[1]=1;
  if(fault==33)r.state=1;if(fault==34)r.pages=74;if(fault==35)r.closed=1;if(fault==36)r.initialization_published=1;
  if(fault==37)r.recovery_required=1;if(fault==38)r.reserved_reply=1;if(fault==39)r.session=1;if(fault==40)r.driver_generation=7;
  if(fault==41)currentTeam=18;if(fault==42)threadStatus=-1;
  if(fault==43){for(auto& page:f.learned)page=0;}
  assert(f.Control(r)!=B_OK && !f.c.q38GraphMutable && !f.c.q38FfnZero && !f.c.q38FfnModelPlan
   && !f.c.dispatchOwnerRetained && f.io.touches==before && acquired==released);refusals++;
 }
 for(unsigned fault=0;fault<12;fault++){
  NativeFixture f(argv[1]);assert(f.Bind());PioneerQ38GdnCallScope scope(&f.cookie);
  assert(scope.Context()==&f.c && f.io.StateZeroBeforeCohort());unsigned before=f.io.touches;
  if(fault==0)f.c.q38FfnModelLedger++;if(fault==1)f.c.q38FfnModelLayer=64;if(fault==2)f.c.q38FfnModelIdentity.driverGeneration++;
  if(fault==3)f.c.q38GdnCallThread=43;if(fault==4)f.io.q38MutableContext=nullptr;if(fault==5)sRuntimeOwner=nullptr;
  if(fault==6)f.c.q38FfnZero->identity={1,1,1,7};if(fault==7)f.c.q38FfnZero->closed=true;
  if(fault==8)f.io.nodeID=nullptr;if(fault==9)f.io.tlb=nullptr;if(fault==10)f.io.resetArea=-1;if(fault==11)f.io.area=-1;
  assert(!f.io.StateZeroOtherCoresHeld() && f.io.touches==before);refusals++;
 }
 for(unsigned fault=0;fault<9;fault++){
  NativeFixture f(argv[1]);tt_q38_ffn_model_initialize r={};r.version=1;
  if(fault==0)f.io.failPeer=1;if(fault==1)f.io.failPeer=238;if(fault==2)f.io.noCompletion=true;
  if(fault==3)f.io.deviceDropPacket=0;if(fault==4)f.io.corruptCounter=true;if(fault==5)f.io.lateCohort=true;
  if(fault==6)f.io.lateMapLoss=true;if(fault==7)f.io.summaryFault=7;if(fault==8)failCopy=2;
  assert(f.Control(r)==(fault==8?-99:B_OK));f.Retained();faults++;
 }
 // A bound but never initialized context cannot use END as implicit zero.
 {NativeFixture f(argv[1]);assert(f.Bind());unsigned before=f.io.touches;assert(!EndGate(f.c) && before==f.io.touches);f.Retained();faults++;}
 for(unsigned allocation=1;allocation<=3;allocation++){
  NativeFixture f(argv[1]);failAllocation=allocation;unsigned before=f.io.touches;
  tt_q38_ffn_model_initialize r={};r.version=1;assert(f.Control(r)==B_NOT_ALLOWED && !f.c.q38FfnModelPlan
   && !f.c.q38GraphMutable && !f.c.q38FfnZero && !f.c.dispatchOwnerRetained && before==f.io.touches);refusals++;
 }
 {NativeFixture f(argv[1]);auto p=std::unique_ptr<PioneerQ38CanonicalModelPlan>(new(std::nothrow) PioneerQ38CanonicalModelPlan);
  assert(p && p->Build());const auto& workspace=*p->Workspace(0);f.learned[workspace.offset/4096]=1;
  unsigned before=f.io.touches;tt_q38_ffn_model_initialize r={};r.version=1;
  assert(f.Control(r)==B_NOT_ALLOWED && f.io.touches==before);f.Retained();faults++;
 }
 printf("PASS ACTUAL native canonical FFN model context + frozen zero RV32: %u positive/%u inert refusals/%u fault-retentions;SAME ledger/joint owner,74page known-clean zero CLOSE,no legacy GDN publication,NO native/model compute or speed claim\n",positive,refusals,faults);
}
