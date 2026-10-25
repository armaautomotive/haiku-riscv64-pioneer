/* SPDX-License-Identifier: MIT */
// Compile actual new binding/lifetime fragments with REAL KernelExport.h
// thread/area ABI. Context shape endpoints below are NOT the full driver;
// unresolved base/retention functions intentionally prevent native linking.
#define PIONEER_Q38_RESIDENT_OUTPUT_OWNER 1
#define PIONEER_Q38_DECODER_RUNTIME 1
#define PIONEER_Q38_GDN_MUTABLE_STATE
#define PIONEER_Q38_PROJECTION_EXECUTION_OWNER 1
#define PIONEER_Q38_PROJECTION_RESULT_STASH 1
#define PIONEER_Q38_EMBEDDING_NATIVE 1
#define PIONEER_Q38_LEARNED_RMS_NATIVE 1
#include <KernelExport.h>
#include <new>
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38ResidentOutputNativeState.h"
#include "../../../src/add-ons/kernel/drivers/misc/BlackholeArcClockGuard.h"
struct PioneerHostPageIO {bool quarantined;};
struct PioneerSessionIO:PioneerHostPageIO {
 PioneerStoreContext* q38MutableContext;
 const tt_dram_samples_result& dram;
 unsigned runtimeCore;bool fleetEnabled,runtimeMapped,q38SpanAllowed,wideQuarantined;
 bool q38EmbeddingMemoryMapAttempted,q38ProjectionMemoryMapAttempted;
 bool ResidentOutputCallScopeMatches();bool ResidentOutputBindSourceScope();
};
struct NativeBindingOtherFlags {bool saved,dirty,ready,running,awaiting,closed,unsafe;};
struct NativeBindingClock {
 const tt_arc_clock_guard_result& Snapshot()const;
 bool RequiresExplicitEnd()const;
};
struct PioneerStoreContext {
 PioneerSessionIO& io;
 PioneerQ38EmbeddingNativeState* q38EmbeddingModel;
 PioneerQ38ResidentOutputNativeState* q38ResidentOutputNative;
 PioneerResidentBanks& banks;PioneerResidentLease& lease;PioneerBulkUploadLease& bulk;
 tt_host_page_result& host;PioneerOutputBufferLease& bulkHost;PioneerWideOutputLease& wideHost;
 struct Dram {tt_dram_samples_result& dram;};struct Level1 {Dram baseline;};
 struct Level2 {Level1 baseline;};struct Level3 {Level2 baseline;}baseline;
 thread_id q38GdnCallThread;uint64 bytes;int runtimeCoreMode;bool bankStorageOnly,dispatchOwnerRetained,q38DispatchRoutes;
 const uint8_t* initialized;const uint32_t* hashes;
 const void *q38FfnModelPlan,*q38GraphMutable,*q38FfnZero,*q38FfnGraph,*q38FfnCode,*q38GdnBanks,*q38StateZero;
 const void *q38Dispatch,*q38Cohort,*fleet;
 PioneerQ38GdnIdentity q38FfnModelIdentity,q38GdnIdentity;
 NativeBindingOtherFlags q38Delta,panelProbe,streamProbe;NativeBindingOtherFlags* q38Span;
 NativeBindingClock q38Clock;
 bool BaseUnsafe(bool,bool,bool); // intentionally unresolved;NOT a stand-in native permission
 bool Unsafe(bool zero=false,bool graph=false,bool controlledEmbedding=false){
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38ResidentOutputNativeUnsafe.inc"
  return BaseUnsafe(zero,graph,controlledEmbedding);
 }
};
struct PioneerProbeCookie {uint32 magic;PioneerStoreContext* context;team_id team;};
extern PioneerProbeCookie* sRuntimeOwner;
extern bool sColdRecoveryRequired,sReloadQuiesced,sBankStorageOnly;
extern uint64 sReloadGeneration;
extern const void* sQ38LocalDeltaOwner;
void RetainQ38DispatchOwner(PioneerStoreContext&,tt_resident_session_result&); // intentionally unresolved
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38ResidentOutputNativeBind.inc"
extern "C" bool pioneer_output_native_binding_compile(PioneerStoreContext& context){
 return BindQ38ResidentOutputNativeState(context);
}
extern "C" bool pioneer_output_native_end_gate_compile(PioneerStoreContext& c,tt_resident_session_result& result,bool& clean){
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38ResidentOutputNativeEnd.inc"
 return false; // no legacy END/cleanup is implemented or authorized by this compile endpoint
}
