/* SPDX-License-Identifier: MIT */
// ACTUAL extracted native CPU gate+physical119 audit. Initial private
// publication uses EXPLICIT COUNTER FABRIC,not native zero-data execution.
#define PIONEER_Q38_STATE_ZERO_CONTROLLER_MODEL_ONLY
#include "pioneer_q38_state_zero_controller_test.cpp"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38InitializeOwnedState.h"
#define PIONEER_Q38_NON_ROOT_HELD_MODEL_ONLY
#include "pioneer_q38_non_root_held_test.cpp"
#include "../../../src/add-ons/kernel/drivers/misc/BlackholeArcClockGuard.h"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38ResidentGdnRootController.h"
constexpr int B_OK=0;
static int currentThread=42;
static int find_thread(void*) { return currentThread; }
struct PioneerStoreContext;
struct PioneerMmioIO { int area=1; };
struct PioneerSessionIO:NonRootIO,PioneerMmioIO {
 PioneerStoreContext* q38MutableContext=nullptr;
 tt_dram_samples_result& dram;
 uint32_t dummy=0,*nodeID=&dummy,*tlb=&dummy,*reset=&dummy;
 int tlbArea=1,resetArea=1;
 unsigned runtimeCore=0;
 bool fleetEnabled=false,q38SpanAllowed=false;
 explicit PioneerSessionIO(tt_dram_samples_result& d):dram(d) { for (unsigned i=0;i<16;i++) expected[i]=d.source[i]; }
 bool StateZeroBeforeCohort() { return false; } // other role is separately tested,never selected here
 bool ResidentGdnBeforeRoot(); bool ResidentGdnOtherCoresHeld();
 bool NonRootHeldOwner(); bool NonRootHeldMappingStable();
};
struct ScopeLife { bool saved=false,dirty=false,ready=false,running=false,unsafe=false,awaiting=false; };
struct ScopeClock {
 tt_arc_clock_guard_result snapshot={}; bool requires=true,unsafe=false;
 ScopeClock() { snapshot.state=TT_ARC_CLOCK_BUSY_VERIFIED; snapshot.busy_requested=snapshot.busy_acknowledged=1; }
 const tt_arc_clock_guard_result& Snapshot() const { return snapshot; }
 bool RequiresExplicitEnd() const { return requires; }
};
struct PioneerStoreContext {
 PioneerSessionIO io;
 PioneerResidentBanks& banks;
 PioneerQ38GdnBanks* q38GdnBanks;
 PioneerQ38StateZeroLease* q38StateZero;
 PioneerQ38ResidentGdnRootLease* q38ResidentGdnRoot;
 PioneerQ38GdnIdentity q38GdnIdentity;
 PioneerQ38SpanLease* q38Span=nullptr;
 void* q38Dispatch=nullptr,*q38Cohort=nullptr,*fleet=nullptr;
 ScopeLife q38Delta,panelProbe,streamProbe;
 ScopeClock q38Clock;
 int runtimeCoreMode=1,q38GdnCallThread=42;
 uint64_t bytes=PioneerResidentBanks::Total;
 bool bankStorageOnly=true,otherUnsafe=false,retained=false;
 unsigned retainCalls=0;
 PioneerStoreContext(ZeroControlFixture& f,PioneerQ38ResidentGdnRootLease& root):io(f.owner.dram),banks(f.owner.banks),
  q38GdnBanks(f.state.get()),q38StateZero(&f.lease),q38ResidentGdnRoot(&root),q38GdnIdentity(f.identity) { io.q38MutableContext=this; }
 bool Unsafe(bool controlledRoot=false) const {
  return otherUnsafe || q38Clock.unsafe || (q38GdnBanks && q38GdnBanks->Unsafe())
   || (q38StateZero && (q38StateZero->unsafe || (!controlledRoot && q38StateZero->running)))
   || (q38ResidentGdnRoot && (q38ResidentGdnRoot->unsafe || (!controlledRoot && q38ResidentGdnRoot->running)));
 }
};
struct PioneerProbeCookie { PioneerStoreContext* context=nullptr; uint32_t magic=0x50313030; int team=17; };
static PioneerProbeCookie* sRuntimeOwner=nullptr;
static bool sColdRecoveryRequired=false,sReloadQuiesced=false,sBankStorageOnly=true;
static uint64_t sReloadGeneration=4;
static const void* sQ38LocalDeltaOwner=nullptr;
static void RetainQ38DispatchOwner(PioneerStoreContext& c,tt_resident_session_result&) {
 c.retained=true; c.retainCalls++; sColdRecoveryRequired=true; c.q38Clock.unsafe=true;
 if (c.q38StateZero) c.q38StateZero->unsafe=true;
 if (c.q38GdnBanks) c.q38GdnBanks->Fault();
 if (c.q38ResidentGdnRoot) c.q38ResidentGdnRoot->unsafe=true;
 c.banks.Quarantine(); // MEMORY-ONLY stand-in,actual full Retain tested separately
}
#include "root_scope.inc"
#include "owner_scope.inc"
#include "root_held.inc"
struct NativeScopeFixture {
 ZeroControlFixture zero;
 PioneerQ38ResidentGdnRootLease root;
 PioneerStoreContext c;
 PioneerProbeCookie cookie{&c};
 explicit NativeScopeFixture(const char* image):zero(image,true),c(zero,root) {
  zero.io.publicationCounterFabricOnly=true; PioneerQ38InitializeOwnedResult r;
  assert(PioneerQ38InitializeOwnedState(zero.io,zero.owner.dram,zero.owner.noc,*zero.state,zero.owner.banks,zero.identity,
   zero.owner.host,zero.owner.store,zero.bulk,zero.transfer,zero.wide,zero.lease,r) && r.published);
  sRuntimeOwner=&cookie; sColdRecoveryRequired=sReloadQuiesced=false; sBankStorageOnly=true;
  sReloadGeneration=zero.identity.driverGeneration; sQ38LocalDeltaOwner=nullptr; currentThread=42;
 }
 void Bound() { root.identity=zero.identity; root.owner=zero.state.get(); root.banks=&zero.owner.banks; root.proof=&zero.owner.dram; }
};
int main(int argc,char** argv) {
 assert(argc==2); (void)&Input; (void)&Prepare; (void)&Upload; (void)&Close; (void)&PreparePanel; (void)&Run; (void)&Job; (void)&ClosePanel; (void)&Finish;
 {
  NativeScopeFixture f(argv[1]); assert(f.c.io.ResidentGdnBeforeRoot() && !f.c.io.touches);
  f.Bound(); f.root.saved=f.root.ready=f.root.dirty=f.root.running=true;
  assert(f.c.Unsafe() && !f.c.Unsafe(true) && f.c.io.ResidentGdnBeforeRoot() && !f.c.io.touches);
  assert(f.c.io.ResidentGdnOtherCoresHeld() && f.c.io.observed.size()==119 && f.c.io.resets==238 && !f.c.retained);
 }
 for (unsigned bad=0;bad<45;bad++) {
  NativeScopeFixture f(argv[1]); f.Bound();
  if (!bad) sColdRecoveryRequired=true; if (bad==1) sReloadQuiesced=true; if (bad==2) sRuntimeOwner=nullptr;
  if (bad==3) f.cookie.magic=0; if (bad==4) f.cookie.context=nullptr; if (bad==5) sBankStorageOnly=false;
  if (bad==6) f.c.bankStorageOnly=false; if (bad==7) f.c.bytes--; if (bad==8) f.c.runtimeCoreMode=-1;
  if (bad==9) f.c.io.runtimeCore=1; if (bad==10) f.c.io.fleetEnabled=true; if (bad==11) f.c.io.q38SpanAllowed=true;
  if (bad==12) f.c.q38GdnCallThread=-1; if (bad==13) currentThread=43; if (bad==14) f.c.q38GdnBanks=nullptr;
  if (bad==15) f.c.q38StateZero=nullptr; if (bad==16) f.c.q38ResidentGdnRoot=nullptr;
  if (bad==17) f.c.q38GdnIdentity.session=0; if (bad==18) sReloadGeneration++;
  if (bad==19) f.c.q38Dispatch=&f; if (bad==20) f.c.q38Cohort=&f; if (bad==21) f.c.fleet=&f;
  if (bad==22) sQ38LocalDeltaOwner=&f; if (bad==23) f.c.q38Delta.awaiting=true;
  if (bad==24) f.c.panelProbe.ready=true; if (bad==25) f.c.streamProbe.saved=true;
  if (bad==26) f.zero.lease.closed=false; if (bad==27) f.zero.lease.running=true; if (bad==28) f.zero.lease.ready=true;
  if (bad==29) f.zero.lease.identity.sequence++; if (bad==30) f.root.identity.modelEpoch++;
  if (bad==31) f.root.owner=nullptr; if (bad==32) f.root.banks=nullptr; if (bad==33) f.root.proof=&f.c.io.dram+1;
  if (bad==34) f.root.running=true; if (bad==35) f.root.awaiting=true; if (bad==36) { f.root.closed=true; f.root.ready=true; }
  if (bad==37) f.root.unsafe=true; if (bad==38) f.c.q38Clock.requires=false;
  if (bad==39) f.c.q38Clock.snapshot.state=0; if (bad==40) f.c.q38Clock.snapshot.busy_requested=0;
  if (bad==41) f.c.q38Clock.snapshot.busy_acknowledged=0; if (bad==42) f.c.q38Clock.snapshot.restored=1;
  if (bad==43) f.c.otherUnsafe=true; if (bad==44) f.c.io.q38MutableContext=nullptr;
  assert(!f.c.io.ResidentGdnBeforeRoot() && !f.c.io.ResidentGdnOtherCoresHeld() && !f.c.io.touches && !f.c.retained);
 }
 for (unsigned where:{1U,119U,238U}) {
  NativeScopeFixture f(argv[1]); f.Bound(); f.root.saved=f.root.ready=f.root.dirty=f.root.running=true; f.c.io.failReset=where;
  assert(!f.c.io.ResidentGdnOtherCoresHeld() && f.c.retained && f.c.retainCalls==1 && f.root.unsafe && f.zero.state->Unsafe()
   && sColdRecoveryRequired && f.c.io.resets==where);
  unsigned touches=f.c.io.touches; assert(!f.c.io.ResidentGdnOtherCoresHeld() && f.c.io.touches==touches);
 }
 puts("PASS OFFLINE ACTUAL native resident-root scope:exact same initialized owner/proof/session/thread/generation,45 CPU refusals,controlled-running exemption ONLY with saved/dirty/ready root,ALL119 physical held audit,first/middle/last reset faults request memory-only retention with ZERO later IO;initial zero ledger EXPLICIT counter fabric,NO native/compute/model selection");
}
