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
#define TT_PIONEER_EXPERIMENTAL_Q38_AFFINE
#define TT_PIONEER_EXPERIMENTAL_Q38_BLOCK
#define TT_PIONEER_EXPERIMENTAL_Q38_STREAM
#include <KernelExport.h>
#include <new>
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38ResidentOutputNativeState.h"
#include "../../../src/add-ons/kernel/drivers/misc/BlackholeArcClockGuard.h"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38NonRootHeld.h"
uint32 PioneerRead32(const volatile uint32*);void PioneerWrite32(volatile uint32*,uint32);
struct PioneerHostPageIO {bool quarantined;};
struct PioneerSessionIO:PioneerHostPageIO {
 PioneerStoreContext* q38MutableContext;
 const tt_dram_samples_result& dram;
 const tt_compute_noc_result& noc;
 struct {uint64 bar0_host_start;}profile;unsigned payloadBytes;volatile uint32* runtimeEntry;volatile uint32* scratchData;
 unsigned runtimeCore;bool fleetEnabled,runtimeMapped,q38SpanAllowed,wideQuarantined;
 bool q38EmbeddingMemoryMapAttempted,q38ProjectionMemoryMapAttempted;
 bool ResidentOutputCallScopeMatches();bool ResidentOutputBindSourceScope();
 bool ResidentOutputInitializationBaseScope();bool ResidentOutputInitializationSourceScope(bool,bool);
 bool ResidentOutputInitializationOtherCoresHeld(bool);
 bool ResidentOutputPrefixInitializationScope();
 bool ResidentOutputSourceScope(bool);bool ResidentOutputMappingSourceScope();void ResidentOutputRetainHostOwners();
 bool ResidentOutputMemoryScope(const PioneerQ38ResidentOutputMemory&,const PioneerQ38CanonicalModelPlan&,const PioneerQ38GdnIdentity&,const uint8_t*,unsigned);
 bool ResidentOutputExecutionScope(const PioneerQ38ResidentOutputOwner&,const PioneerQ38ProjectionGraphOwner&,const PioneerQ38ResidentOutputMemory&,
  const PioneerQ38ProjectionMutableBanks&,const PioneerQ38CanonicalModelPlan&,const PioneerQ38GdnIdentity&,const uint8_t*,unsigned);
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38ResidentOutputNativeMapping.inc"
 bool ConvHistoryZeroBeforeCohort();bool ConvHistoryZeroOtherCoresHeld();
 bool StateZeroBeforeCohort(){return ResidentOutputInitializationSourceScope(false,false);}
 bool StateZeroOtherCoresHeld(){return ResidentOutputInitializationOtherCoresHeld(false);}
 bool NonRootHeldOwner();bool NonRootHeldMappingStable();
 bool SelectRuntimeCore(unsigned);bool Q38PanelMappingStable();bool BulkMappingStable();bool SpanViewCapacity(unsigned);
 uint32* Q38PanelSavedWords();uint32* BulkSavedWords();
 uint32 RuntimeCodeRead(unsigned);void RuntimeCodeWrite(unsigned,uint32);
 uint32 RuntimeStatusRead(unsigned);void RuntimeStatusWrite(unsigned,uint32);
 uint32 RuntimePcRead();void RuntimeResetWrite(uint32);uint32 ResetRead();uint32 SourceRead(unsigned);
 uint32 Q38PanelRead(uint32);void Q38PanelWrite(uint32,uint32);uint32 BulkL1Read(unsigned);void BulkL1Write(unsigned,uint32);
 uint32 ResidentNocRead(uint32);uint32 HostPciRead(unsigned,uint16,uint8);
 void ResidentNocWrite(uint32,uint32);uint32 ResidentL1Read(unsigned);void ResidentL1Write(unsigned,uint32);
 uint32 DramTlbRead(unsigned,unsigned);void DramTlbWrite(unsigned,unsigned,uint32);
 int HostPageMap(uint64_t&,uint64_t&);int OutputBufferMap(uint64_t&,uint64_t&);int OutputBufferPageMap(unsigned,uint64_t&,uint64_t&);
 int WideOutputMap(uint64_t&,uint64_t&);int WideOutputPageMap(unsigned,uint64_t&,uint64_t&);
 uint32 WideHostLoad(unsigned);void WideHostStore(unsigned,uint32);void WideHostFence();
 void QuarantineHostPage();void QuarantineOutputBuffer();void QuarantineWideOutput();void Pause();uint64_t Q38CohortNow();
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
 struct Dram {tt_dram_samples_result& dram;};struct Level1 {Dram baseline;tt_compute_noc_result& noc;};
 struct Level2 {Level1 baseline;};struct Level3 {Level2 baseline;}baseline;
 thread_id q38GdnCallThread;uint64 bytes;int runtimeCoreMode;bool bankStorageOnly,dispatchOwnerRetained,q38DispatchRoutes;
 const uint8_t* initialized;const uint32_t* hashes;
 const void *q38FfnModelPlan,*q38GraphMutable,*q38FfnZero,*q38FfnGraph,*q38FfnCode,*q38GdnBanks,*q38StateZero;
 PioneerQ38DispatchLease* q38Dispatch;const void *q38Cohort,*fleet;
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
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38ResidentOutputNativeInitialize.inc"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38ResidentOutputNativeExecution.inc"
extern "C" bool pioneer_output_native_binding_compile(PioneerStoreContext& context){
 return BindQ38ResidentOutputNativeState(context);
}
extern "C" bool pioneer_output_native_initialization_compile(PioneerStoreContext& context,PioneerQ38StateZeroResult& gdn,PioneerQ38ConvHistoryZeroResult& history){
 return InitializeQ38ResidentOutputRecurrent(context,gdn) && InitializeQ38ResidentOutputHistory(context,history);
}
extern "C" bool pioneer_output_native_execution_compile(PioneerStoreContext& context,PioneerQ38ResidentOutputResult& result){
 return PrepareQ38ResidentOutputNative(context,result) && RunQ38ResidentOutputNative(context,result) && CloseQ38ResidentOutputNative(context,result);
}
extern "C" bool pioneer_output_native_end_gate_compile(PioneerStoreContext& c,tt_resident_session_result& result,bool& clean){
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38ResidentOutputNativeEnd.inc"
 return false; // no legacy END/cleanup is implemented or authorized by this compile endpoint
}
