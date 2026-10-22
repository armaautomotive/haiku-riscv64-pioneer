/* SPDX-License-Identifier: MIT */
// Actual EXTRACTED native scope/Unsafe/END,ACTUAL zero/QPE1 controller.
// Native context/PCI/TLB/reset/host endpoints simulated;not a card run.
#define PIONEER_Q38_EMBEDDING_CONTROLLER_FIXTURE_ONLY
#include "pioneer_q38_embedding_controller_test.cpp"
#include "../../../src/add-ons/kernel/drivers/misc/BlackholeArcClockGuard.h"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38NonRootHeld.h"
using uint8=uint8_t;using uint32=uint32_t;
// Borrow REAL retained owners for the actual scope body,not copied ready
// flags. Factory allocation/layout separately compiles with ACTUAL context.
struct PioneerQ38EmbeddingNativeState {
 enum Phase{Bound,Zeroing,Embedding,Complete};Phase phase=Embedding;
 PioneerQ38CanonicalModelPlan& plan;PioneerQ38ProjectionMutableBanks& graph;
 PioneerQ38ProjectionZeroLease& zero;PioneerQ38EmbeddingController& embedding;
 PioneerQ38ProjectionIdentity identity;const uint8_t* ledger;
 PioneerQ38EmbeddingNativeState(EmbeddingControllerFixture& f):plan(*f.f.plan),graph(*f.f.graph),zero(f.f.lease),embedding(*f.controller),identity(f.f.id),ledger(f.f.learned.data()){}
 bool Unsafe()const{return graph.Unsafe() || zero.unsafe || embedding.Unsafe();}
 bool Closed()const{return phase==Complete && !Unsafe() && zero.closed && embedding.Completed();}
 void Fault(){graph.Fault();zero.unsafe=true;embedding.Fault();}
};
struct PioneerStoreContext;
struct PioneerHostPageIO{bool quarantined=false;};
struct PioneerSessionIO:EmbeddingControllerIO,PioneerHostPageIO {
 PioneerStoreContext* q38MutableContext=nullptr;
 unsigned runtimeCore=0;bool fleetEnabled=false,runtimeMapped=true,q38SpanAllowed=false,wideQuarantined=false,q38EmbeddingMemoryMapAttempted=true;
 bool resetView=true;uint32_t tlb[2][3]={};
 struct DramAlias{const tt_dram_samples_result* target;const tt_dram_samples_result* operator&()const{return target;}
  operator const tt_dram_samples_result&()const{return *target;}} dram;
 const tt_compute_noc_result& noc;
 PioneerSessionIO(EmbeddingControllerFixture& f,const char* image):EmbeddingControllerIO(f.f,image),dram{&f.f.owner.dram},noc(f.f.owner.noc){result=&f.result;}
 bool EmbeddingSourceScope(bool);
 bool ProjectionZeroBeforeCohort();bool ProjectionZeroOtherCoresHeld();
 bool EmbeddingNativeScope(const PioneerQ38EmbeddingController&,const PioneerQ38ProjectionMutableBanks&,const PioneerQ38CanonicalModelPlan&,
  const PioneerQ38ProjectionIdentity&,const uint8_t*,unsigned);
 bool EmbeddingOtherCoresHeld();void EmbeddingRetainHostOwners();
 bool NonRootHeldOwner(){return EmbeddingSourceScope(false);}
 bool NonRootHeldMappingStable(){return resetView && NonRootHeldOwner();}
 void DramTlbWrite(unsigned s,unsigned w,uint32_t value){Touch();assert(s==1 && w<3);tlb[s][w]=value;}
 uint32_t DramTlbRead(unsigned s,unsigned w){Touch();return tlb[s][w];}
 uint32_t ResetRead(){
  bool root=true;for(unsigned i=0;i<3;i++)root&=tlb[1][i]==BlackholeDramFixedTlbWord(0xffa00000,1,2,true,i);
  if(root)return EmbeddingControllerIO::ResetRead();Touch();return 0x47800;
 }
 void QuarantineHostPage(){PioneerHostPageIO::quarantined=true;EmbeddingControllerIO::QuarantineHostPage();}
};
struct Flags{bool saved=false,dirty=false,ready=false,running=false,unsafe=false,awaiting=false,closed=false;};
struct Clock{
 tt_arc_clock_guard_result value={};bool required=true,quarantined=false;
 Clock(){value.state=TT_ARC_CLOCK_BUSY_VERIFIED;value.busy_requested=value.busy_acknowledged=1;}
 const auto& Snapshot()const{return value;}bool RequiresExplicitEnd()const{return required;}
 void Quarantine(){quarantined=true;}
};
struct PioneerStoreContext{
 PioneerSessionIO& io;PioneerQ38EmbeddingNativeState* q38EmbeddingModel;
 PioneerResidentBanks& banks;PioneerResidentLease& lease;PioneerBulkUploadLease& bulk;
 PioneerOutputBufferLease& bulkHost;PioneerWideOutputLease& wideHost;tt_host_page_result& host;
 struct D{tt_dram_samples_result& dram;};struct B1{D baseline;tt_compute_noc_result& noc;};struct B2{B1 baseline;};struct B3{B2 baseline;};B3 baseline;
 uint8* initialized;uint32 hash=0,*hashes=&hash;
 void *q38FfnModelPlan=nullptr,*q38GraphMutable=nullptr,*q38FfnZero=nullptr,*q38FfnGraph=nullptr,*q38FfnCode=nullptr;
 void *q38GdnBanks=nullptr,*q38StateZero=nullptr,*q38Dispatch=nullptr,*q38Cohort=nullptr,*fleet=nullptr;
 PioneerQ38ProjectionIdentity q38FfnModelIdentity={},q38GdnIdentity={};
 Flags span,q38Delta,panelProbe,streamProbe;Flags* q38Span=&span;Clock q38Clock;
 int runtimeCoreMode=1,q38GdnCallThread=42;uint64_t bytes=PioneerResidentBanks::Total;
 bool bankStorageOnly=true,dispatchOwnerRetained=false,otherUnsafe=false;
 PioneerStoreContext(EmbeddingControllerFixture& f,PioneerSessionIO& native,PioneerQ38EmbeddingNativeState& model)
  :io(native),q38EmbeddingModel(&model),banks(f.f.owner.banks),lease(f.f.owner.store),bulk(f.f.bulk),bulkHost(f.f.transfer),wideHost(f.f.wide),host(f.f.owner.host),
   baseline{{{{f.f.owner.dram},f.f.owner.noc}}},initialized(f.f.learned.data()){span.closed=true;io.q38MutableContext=this;}
 bool Unsafe(bool zero=false,bool graph=false,bool controlledEmbedding=false){
  (void)zero;(void)graph;
#include "embedding_native_unsafe.inc"
  return otherUnsafe || banks.Quarantined() || q38Clock.quarantined;
 }
};
struct PioneerProbeCookie{PioneerStoreContext* context;uint32_t magic=0x50313030;};
static PioneerProbeCookie* sRuntimeOwner=nullptr;
static bool sColdRecoveryRequired=false,sReloadQuiesced=false,sBankStorageOnly=true;
static uint64_t sReloadGeneration=4;static const void* sQ38LocalDeltaOwner=nullptr;
static int currentThread=42;static int find_thread(void*){return currentThread;}
static void RetainQ38DispatchOwner(PioneerStoreContext& c,tt_resident_session_result& result){
 c.dispatchOwnerRetained=true;if(c.q38EmbeddingModel)c.q38EmbeddingModel->Fault();
 c.banks.Quarantine();c.lease.recovery_required=true;c.q38Clock.Quarantine();c.io.QuarantineHostPage();
 c.bulkHost.quarantined=c.wideHost.quarantined=true;c.io.retained=true;sColdRecoveryRequired=true;result.cold=1;
} // Simulated MEMORY retention endpoint;actual full driver body compiles separately.
#include "embedding_native_scope.inc"
static bool End(PioneerStoreContext& c){bool clean=true;tt_resident_session_result result={};
#include "embedding_native_end.inc"
 return clean;
}
int main(int argc,char** argv){
 assert(argc==3);setvbuf(stdout,nullptr,_IOLBF,0);unsigned refusals=0;
 (void)&Input;(void)&Prepare;(void)&Upload;(void)&Close;(void)&PreparePanel;(void)&Run;(void)&Job;(void)&ClosePanel;(void)&Finish;
 EmbeddingControllerFixture f(argv[1],argv[2]);PioneerSessionIO io(f,argv[2]);PioneerQ38EmbeddingNativeState model(f);
 PioneerStoreContext c(f,io,model);PioneerProbeCookie cookie{&c};sRuntimeOwner=&cookie;
 assert(io.EmbeddingSourceScope(false) && c.Unsafe() && !c.Unsafe(true,false,true));unsigned touches=io.touches;
 // During ACTUAL zero execution bulk scratch is dirty by design;ONLY
 // that exact phase may admit it. Embedding must require restored bulk.
 model.phase=PioneerQ38EmbeddingNativeState::Zeroing;c.bulk.dirty=true;
 assert(io.EmbeddingSourceScope(false) && !io.EmbeddingSourceScope(true) && io.touches==touches);
 model.phase=PioneerQ38EmbeddingNativeState::Embedding;assert(!io.EmbeddingSourceScope(false) && io.touches==touches);
 c.bulk.dirty=false;refusals+=2;
 for(unsigned which=0;which<49;which++){
  auto test=[&](auto& field,auto bad){auto saved=field;field=bad;assert(!io.EmbeddingSourceScope(false) && io.touches==touches);field=saved;};
  switch(which){
   case 0:test(sColdRecoveryRequired,true);break;case 1:test(sReloadQuiesced,true);break;case 2:test(sReloadGeneration,uint64_t(5));break;
   case 3:test(cookie.magic,0U);break;case 4:test(cookie.context,static_cast<PioneerStoreContext*>(nullptr));break;
   case 5:test(sRuntimeOwner,static_cast<PioneerProbeCookie*>(nullptr));break;case 6:test(sBankStorageOnly,false);break;
   case 7:test(c.bankStorageOnly,false);break;case 8:test(c.bytes,uint64_t(0));break;case 9:test(c.runtimeCoreMode,0);break;
   case 10:test(io.runtimeCore,1U);break;case 11:test(io.fleetEnabled,true);break;case 12:test(io.runtimeMapped,false);break;case 13:test(io.q38SpanAllowed,true);break;
   case 14:test(c.q38GdnCallThread,-1);break;case 15:test(currentThread,41);break;case 16:test(c.dispatchOwnerRetained,true);break;
   case 17:test(c.q38EmbeddingModel,static_cast<PioneerQ38EmbeddingNativeState*>(nullptr));break;
   case 18:test(model.ledger,model.ledger+1);break;case 19:test(c.initialized,static_cast<uint8*>(nullptr));break;
   case 20:test(c.hashes,static_cast<uint32*>(nullptr));break;case 21:test(c.q38FfnModelPlan,static_cast<void*>(&c));break;
   case 22:test(c.q38GraphMutable,static_cast<void*>(&c));break;case 23:test(c.q38FfnZero,static_cast<void*>(&c));break;
   case 24:test(c.q38FfnGraph,static_cast<void*>(&c));break;case 25:test(c.q38FfnCode,static_cast<void*>(&c));break;
   case 26:test(c.q38Dispatch,static_cast<void*>(&c));break;case 27:test(c.q38Cohort,static_cast<void*>(&c));break;case 28:test(c.fleet,static_cast<void*>(&c));break;
   case 29:test(sQ38LocalDeltaOwner,static_cast<const void*>(&c));break;case 30:test(c.q38Delta.ready,true);break;case 31:test(c.panelProbe.saved,true);break;
   case 32:test(c.streamProbe.dirty,true);break;case 33:test(c.span.closed,false);break;case 34:test(c.lease.ready,false);break;case 35:test(c.bulk.ready,false);break;
   case 36:test(c.host.released,1U);break;case 37:test(c.bulkHost.pending,true);break;case 38:test(c.wideHost.memory_type_set,false);break;
   case 39:test(io.PioneerHostPageIO::quarantined,true);break;case 40:test(io.wideQuarantined,true);break;case 41:test(model.zero.closed,false);break;
   case 42:test(model.zero.nonce,1U);break;case 43:test(model.zero.dirty,true);break;case 44:test(c.q38Clock.required,false);break;
   case 45:test(c.q38Clock.value.busy_acknowledged,0U);break;case 46:test(c.q38Clock.value.restored,1U);break;case 47:test(c.otherUnsafe,true);break;
   case 48:test(model.phase,PioneerQ38EmbeddingNativeState::Complete);break;
  }
  assert(io.EmbeddingSourceScope(false) && io.touches==touches);refusals++;
 }
 assert(f.controller->RunOnce(io,f.f.owner.dram,f.f.owner.noc,f.f.owner.banks,*f.f.graph,*f.f.plan,f.f.id,
  f.f.learned.data(),unsigned(f.f.learned.size()),f.f.owner.store,f.token,f.position,f.bank,f.result));
 model.phase=PioneerQ38EmbeddingNativeState::Complete;assert(model.Closed() && !c.Unsafe() && End(c));
 // Lost call / incomplete END is CPU-only rejection BEFORE device cleanup.
 touches=io.touches;model.phase=PioneerQ38EmbeddingNativeState::Embedding;
 assert(!End(c) && io.touches==touches && io.retained && model.Unsafe() && c.q38Clock.quarantined);
 assert(!io.EmbeddingSourceScope(false) && !io.EmbeddingSourceScope(true) && io.touches==touches);
 printf("PASS ACTUAL extracted embedding native scope/Unsafe/END refusals%u inert/ACTUAL QPE1+zero under retained real-owner references/nonroot physical-reset TLB proof/clean close/END failure retainsALL ZERO IO;context/transport/retention endpoints simulated,NO_NATIVE_FACTORY_RUNTIME_OR_CARD_SPEED_EVIDENCE\n",refusals);
}
