/* SPDX-License-Identifier: MIT */
#ifdef PIONEER_Q38_TOKEN_NATIVE_CONTEXT
#include "PioneerQ38TokenNativeRole.h"
#ifndef PIONEER_Q38_TOKEN_NATIVE_STATE_H
#define PIONEER_Q38_TOKEN_NATIVE_STATE_H
#include "PioneerQ38ProjectionMutableBanks.h"
#include "PioneerQ38TokenMemory.h"
#include "PioneerQ38TokenNativeOwner.h"
#include "PioneerQ38TokenViewLease.h"
#include "PioneerQ38TokenPciLease.h"
#include "PioneerQ38StateZeroController.h"
#include "PioneerQ38ConvHistoryZeroController.h"
#include "PioneerQ38ProjectionZeroController.h"
struct PioneerStoreContext;struct PioneerSessionIO;
[[maybe_unused]] static bool Q38TokenExecutionOutput(const PioneerStoreContext&,const void*,size_t);
[[maybe_unused]] static bool BindQ38TokenNative(PioneerStoreContext&,unsigned);
[[maybe_unused]] static bool InitializeQ38TokenRecurrent(PioneerStoreContext&,PioneerQ38StateZeroResult&);
[[maybe_unused]] static bool InitializeQ38TokenHistory(PioneerStoreContext&,PioneerQ38ConvHistoryZeroResult&);
[[maybe_unused]] static bool InitializeQ38TokenProjection(PioneerStoreContext&,PioneerQ38ProjectionZeroResult&);
[[maybe_unused]] static bool PrepareQ38TokenNativeHeld(PioneerStoreContext&,const uint32_t*,unsigned,unsigned,bool,unsigned,unsigned,PioneerQ38TokenMemoryResult&);
[[maybe_unused]] static bool CloseQ38TokenNativeHeld(PioneerStoreContext&,PioneerQ38TokenMemoryResult&);
[[maybe_unused]] static bool PrepareQ38TokenNativeRuntime(PioneerStoreContext&,PioneerQ38TokenRuntimeResult&);
[[maybe_unused]] static bool BootstrapQ38TokenNativeWorkers(PioneerStoreContext&,PioneerQ38TokenBootstrapResult&);
[[maybe_unused]] static bool StartQ38TokenNativeRoot(PioneerStoreContext&,PioneerQ38TokenExecutionResult&);
[[maybe_unused]] static bool CancelQ38TokenNativeRoot(PioneerStoreContext&,PioneerQ38TokenExecutionResult&);
[[maybe_unused]] static bool ObserveQ38TokenNativeRoot(PioneerStoreContext&,PioneerQ38TokenExecutionResult&);
[[maybe_unused]] static bool ObserveQ38TokenNativeTerminal(PioneerStoreContext&,PioneerQ38TokenTerminalResult&);
[[maybe_unused]] static bool HoldQ38TokenNativeStopped(PioneerStoreContext&,PioneerQ38TokenHoldResult&);
[[maybe_unused]] static bool ReadQ38TokenNativeRecords(PioneerStoreContext&,unsigned,uint32_t*,PioneerQ38TokenReadResult&);
[[maybe_unused]] static bool RestoreQ38TokenNativeL1(PioneerStoreContext&,PioneerQ38TokenRestoreResult&);
[[maybe_unused]] static bool RestoreQ38TokenNativeDispatchImages(PioneerStoreContext&,PioneerQ38TokenDispatchRestoreResult&);
[[maybe_unused]] static bool RestoreQ38TokenNativePeerStores(PioneerStoreContext&,PioneerQ38TokenPeerRestoreResult&);
[[maybe_unused]] static bool RestoreQ38TokenNativeRootStore(PioneerStoreContext&,PioneerQ38TokenRootStoreRestoreResult&);
[[maybe_unused]] static bool RestoreQ38TokenNativeSharedMappings(PioneerStoreContext&,PioneerQ38TokenSharedTlbRestoreResult&);
[[maybe_unused]] static bool EndQ38TokenNativeClock(PioneerStoreContext&,PioneerQ38TokenClockResult&);
[[maybe_unused]] static bool Q38TokenNativeClockScope(const void*);
[[maybe_unused]] static bool SealQ38TokenNativeHardware(PioneerStoreContext&,PioneerQ38TokenHardwareSealResult&);
struct PioneerQ38TokenViewsResult {
 uint32_t version=1,started=0,unsafe=0,ready=0,deleted=0,reserved[11]={};
};
[[maybe_unused]] static bool CloseQ38TokenNativeViews(PioneerStoreContext&,PioneerQ38TokenViewsResult&);
struct PioneerQ38TokenPciResult {
 uint32_t version=1,started=0,unsafe=0,ready=0,writes=0,reads=0,reserved[10]={};
};
[[maybe_unused]] static bool CloseQ38TokenNativePci(PioneerStoreContext&,PioneerQ38TokenPciResult&);
// Genuine retained full851 model/reservation object. It borrows NO old
// completed embedding/prefix owner and cannot make those old lifetimes safe.
// Private factory publication occurs under the real global FD call lock.
class PioneerQ38TokenNativeState {
 friend class PioneerQ38TokenPreparationAudit;
 friend struct PioneerStoreContext;friend struct PioneerSessionIO;
 friend bool Q38TokenExecutionOutput(const PioneerStoreContext&,const void*,size_t);
 friend bool BindQ38TokenNative(PioneerStoreContext&,unsigned);
 friend bool InitializeQ38TokenRecurrent(PioneerStoreContext&,PioneerQ38StateZeroResult&);
 friend bool InitializeQ38TokenHistory(PioneerStoreContext&,PioneerQ38ConvHistoryZeroResult&);
 friend bool InitializeQ38TokenProjection(PioneerStoreContext&,PioneerQ38ProjectionZeroResult&);
 friend bool PrepareQ38TokenNativeHeld(PioneerStoreContext&,const uint32_t*,unsigned,unsigned,bool,unsigned,unsigned,PioneerQ38TokenMemoryResult&);
 friend bool CloseQ38TokenNativeHeld(PioneerStoreContext&,PioneerQ38TokenMemoryResult&);
 friend bool PrepareQ38TokenNativeRuntime(PioneerStoreContext&,PioneerQ38TokenRuntimeResult&);
 friend bool BootstrapQ38TokenNativeWorkers(PioneerStoreContext&,PioneerQ38TokenBootstrapResult&);
 friend bool StartQ38TokenNativeRoot(PioneerStoreContext&,PioneerQ38TokenExecutionResult&);
 friend bool CancelQ38TokenNativeRoot(PioneerStoreContext&,PioneerQ38TokenExecutionResult&);
 friend bool ObserveQ38TokenNativeRoot(PioneerStoreContext&,PioneerQ38TokenExecutionResult&);
 friend bool ObserveQ38TokenNativeTerminal(PioneerStoreContext&,PioneerQ38TokenTerminalResult&);
 friend bool HoldQ38TokenNativeStopped(PioneerStoreContext&,PioneerQ38TokenHoldResult&);
 friend bool ReadQ38TokenNativeRecords(PioneerStoreContext&,unsigned,uint32_t*,PioneerQ38TokenReadResult&);
 friend bool RestoreQ38TokenNativeL1(PioneerStoreContext&,PioneerQ38TokenRestoreResult&);
 friend bool RestoreQ38TokenNativeDispatchImages(PioneerStoreContext&,PioneerQ38TokenDispatchRestoreResult&);
 friend bool RestoreQ38TokenNativePeerStores(PioneerStoreContext&,PioneerQ38TokenPeerRestoreResult&);
 friend bool RestoreQ38TokenNativeRootStore(PioneerStoreContext&,PioneerQ38TokenRootStoreRestoreResult&);
 friend bool RestoreQ38TokenNativeSharedMappings(PioneerStoreContext&,PioneerQ38TokenSharedTlbRestoreResult&);
 friend bool EndQ38TokenNativeClock(PioneerStoreContext&,PioneerQ38TokenClockResult&);
 friend bool Q38TokenNativeClockScope(const void*);
 friend bool SealQ38TokenNativeHardware(PioneerStoreContext&,PioneerQ38TokenHardwareSealResult&);
 friend bool CloseQ38TokenNativeViews(PioneerStoreContext&,PioneerQ38TokenViewsResult&);
 friend bool CloseQ38TokenNativePci(PioneerStoreContext&,PioneerQ38TokenPciResult&);
 enum Phase {Claiming,Bound,RecurrentZeroing,RecurrentClosing,HistoryZeroing,HistoryClosing,
  ProjectionZeroing,ProjectionClosing,Initialized,DispatchPreparing,MemoryPreparing,MemoryInstalled,RuntimePreparing,RuntimeReady,
  WorkersBootstrapping,WorkersReady,RootStarting,RootRunning,RootHolding,RootHeld,RootRestoring,RootRestored,
  DispatchRestoring,DispatchRestored,PeerStoresRestoring,PeerStoresRestored,RootStoreRestoring,RootStoreRestored,
  SharedTlbRestoring,SharedTlbRestored,ClockEnding,ClockEnded,HardwareSealing,HardwareSealed,ViewsClosing,ViewsClosed,PciClosing,PciClosed,MemoryClosing,MemoryClosed,Unknown};
 PioneerStoreContext* const context;const void* const cookie;const void* const hashes;
 const uint8_t* const ledger;const int64_t team;const unsigned bank;
 const PioneerQ38GdnIdentity identity;
 PioneerQ38CanonicalModelPlan canonical;
 PioneerQ38ProjectionMutableBanks graph;
 PioneerQ38StateZeroLease recurrentZero;
 PioneerQ38ConvHistoryZeroLease historyZero;
 PioneerQ38ProjectionZeroLease projectionZero;
 PioneerQ38TokenStartupPlan* startup=nullptr;
 PioneerQ38TokenMemory* memory=nullptr;
 PioneerQ38TokenNativeOwner* runtime=nullptr;
 PioneerQ38DispatchLease* participants=nullptr; // owned by SAME retained context;never a borrowed completed prefix
 uint32_t prompt[8192]={};unsigned prompts=0,limit=0,eos[2]={};bool stopEos=false;
 Phase phase=Claiming;bool unsafe=false;
 bool clockEndAttempted=false,clockEnded=false;
 uint64_t clockBegan=0,clockPrevious=0;
 bool hardwareSealAttempted=false,hardwareSealed=false;
 bool viewsCloseAttempted=false,viewsClosed=false;
 PioneerQ38TokenPciLease pci;
 bool pciCloseAttempted=false,pciClosed=false;
 uint64_t pciBegan=0,pciPrevious=0;
 bool SealedCohort()const{
  return hardwareSealAttempted && (phase==HardwareSealing || phase==HardwareSealed || phase==ViewsClosing || phase==ViewsClosed || phase==PciClosing || phase==PciClosed)
   && runtime && runtime->HardwareLeasesSealed();
 }
 PioneerQ38TokenNativeState(PioneerStoreContext& c,const void* fd,const void* h,const uint8_t* uploaded,int64_t owner,
  unsigned selected,const PioneerQ38GdnIdentity& id):context(&c),cookie(fd),hashes(h),ledger(uploaded),team(owner),bank(selected),identity(id){}
 ~PioneerQ38TokenNativeState(){delete runtime;delete memory;delete startup;} // known-clean native destruction ONLY
public:
 PioneerQ38TokenNativeState(const PioneerQ38TokenNativeState&)=delete;
 PioneerQ38TokenNativeState& operator=(const PioneerQ38TokenNativeState&)=delete;
 bool Unsafe()const{return unsafe || phase==Unknown || graph.Unsafe() || pci.Unsafe() || recurrentZero.unsafe || historyZero.unsafe
  || projectionZero.unsafe || (memory && memory->Unsafe()) || (runtime && runtime->Unsafe())
  || (hardwareSealed && !SealedCohort())
  || (participants && (participants->unsafe || (participants->closed && !SealedCohort()) || participants->bootAttempted || participants->booted
   || (participants->rootRunning && (!runtime || (phase!=RootStarting && phase!=RootRunning && phase!=RootHolding) || !runtime->OwnsRunningRoot(participants)))));}
 // Held-only cheap binding:whole-context and participant guards still audit
 // EVERY child lease before/after each pass,not120 peers again per L1 word.
 bool Closed()const{return false;} // NO full-token native END capability has been implemented yet
 bool TerminalObserved()const{return (phase==RootRunning || phase==RootHolding || phase==RootHeld) && !Unsafe() && runtime && runtime->TerminalObserved();}
 bool TerminalHeld()const{return phase==RootHeld && !Unsafe() && runtime && runtime->HeldAfterTerminal();}
 bool OriginalL1Restored()const{return phase==RootRestored && !Unsafe() && runtime && runtime->OriginalL1Restored();}
 bool DispatchImagesRestored()const{return phase==DispatchRestored && !Unsafe() && runtime && runtime->DispatchImagesRestored();}
 bool PeerStoreOriginalsRestored()const{return phase==PeerStoresRestored && !Unsafe() && runtime && runtime->PeerStoresRestored();}
 bool RootStoreOriginalsRestored()const{return phase==RootStoreRestored && !Unsafe() && runtime && runtime->RootStoreOriginalsRestored();}
 bool SharedMappingsRestored()const{return phase==SharedTlbRestored && !Unsafe() && runtime && runtime->SharedMappingsRestored();}
 bool ClockRestored()const{return phase==ClockEnded && clockEndAttempted && clockEnded && !Unsafe() && runtime && runtime->SharedMappingsRestored();}
 bool HardwareLeasesSealed()const{return phase==HardwareSealed && hardwareSealed && !Unsafe() && SealedCohort()
  && memory && memory->LaunchedJournalClosed();} // NOT full native END
 bool ViewsReleased()const{return phase==ViewsClosed && viewsCloseAttempted && viewsClosed && hardwareSealed && !Unsafe()
  && SealedCohort() && memory && memory->LaunchedJournalClosed();} // VM aliases ONLY;PCI/hosts/reservations still owned
 bool PciRestored()const{return phase==PciClosed && pciCloseAttempted && pciClosed && pci.Complete() && viewsCloseAttempted && viewsClosed
  && hardwareSealed && !Unsafe() && SealedCohort() && memory && memory->LaunchedJournalClosed();} // hosts/reservations/full END still owned
 bool PciSnapshotCaptured()const{return !Unsafe() && pci.Prepared();} // necessary CPU journal state,not PCI/launch/END permission
 // Necessary private STOP observation only;current sources/mappings and HOLD/restore/END remain separate.
 bool ControlsCohort(const PioneerQ38DispatchLease* p)const{
  if(!unsafe && hardwareSealAttempted && p && p==participants && runtime){
   if(phase==HardwareSealed || phase==ViewsClosing || phase==ViewsClosed || phase==PciClosing || phase==PciClosed)return hardwareSealed && runtime->HardwareLeasesSealed();
   if(phase==HardwareSealing)return runtime->HardwareLeasesSealed() || (runtime->ControlsCohort(p) && runtime->SharedMappingsRestored());
  }
  return !unsafe && (phase==WorkersBootstrapping || phase==WorkersReady || phase==RootStarting || phase==RootRunning || phase==RootHolding
    || phase==RootHeld || phase==RootRestoring || phase==RootRestored || phase==DispatchRestoring || phase==DispatchRestored
    || phase==PeerStoresRestoring || phase==PeerStoresRestored || phase==RootStoreRestoring || phase==RootStoreRestored
    || phase==SharedTlbRestoring || phase==SharedTlbRestored || phase==ClockEnding || phase==ClockEnded)
   && p && p==participants && runtime && runtime->ControlsCohort(p)
   && (phase!=WorkersReady || runtime->WorkersReady()) && (phase!=RootRunning || runtime->ExecutionStarted())
   && (phase!=RootHolding || runtime->TerminalObserved()) && (phase!=RootHeld || runtime->HeldAfterTerminal())
   && (phase!=RootRestoring || runtime->CommittedRecordsRead()) && (phase!=RootRestored || runtime->OriginalL1Restored())
   && (phase!=DispatchRestoring || runtime->OriginalL1Restored()) && (phase!=DispatchRestored || runtime->DispatchImagesRestored())
   && (phase!=PeerStoresRestoring || runtime->DispatchImagesRestored()) && (phase!=PeerStoresRestored || runtime->PeerStoresRestored())
   && (phase!=RootStoreRestoring || runtime->PeerStoresRestored()) && (phase!=RootStoreRestored || runtime->RootStoreOriginalsRestored())
   && (phase!=SharedTlbRestoring || runtime->RootStoreOriginalsRestored()) && (phase!=SharedTlbRestored || runtime->SharedMappingsRestored())
   && ((phase!=ClockEnding && phase!=ClockEnded) || (clockEndAttempted && runtime->SharedMappingsRestored()))
   && (phase!=ClockEnded || clockEnded);
 } // necessary SAME private bootstrap owner,not legacy boot flags or ROOT/RUN permission
 bool MatchesBinding(const PioneerStoreContext& c,const void* fd,const void* h,const uint8_t* uploaded,int64_t owner,uint64_t generation)const{
  return context==&c && cookie==fd && fd && hashes==h && h && ledger==uploaded && uploaded && team==owner && owner>=0
   && identity.Valid() && generation && identity.driverGeneration==generation && bank<7 && canonical.Ready() && !Unsafe();
 }
 bool MatchesReservation(const PioneerResidentBanks& banks,const tt_dram_samples_result& proof)const{
  return !Unsafe() && graph.CanonicalModel(identity)==&canonical && graph.MatchesInitializationOwner(banks,proof,identity);
 } // reservation binding ONLY,not initialized state,source authentication or RUN
 bool RecurrentInitialized(const PioneerResidentBanks& banks,const tt_dram_samples_result& proof)const{
  const auto* g=graph.Recurrent(identity);unsigned pages=0;
  return !Unsafe() && g && canonical.RecurrentCount()==672 && recurrentZero.saved && recurrentZero.closed && recurrentZero.nonce==672
   && !recurrentZero.dirty && !recurrentZero.ready && !recurrentZero.running && recurrentZero.identity==identity
   && g->InitializationProgress(identity,pages) && pages==73728 && g->InitializationPublished(identity)
   && g->MatchesInitializationOwner(banks,proof,identity);
 }
 bool HistoryInitialized(const PioneerResidentBanks& banks,const tt_dram_samples_result& proof)const{
  const auto* h=graph.ConvHistory(identity);unsigned pages=0;
  return !Unsafe() && RecurrentInitialized(banks,proof) && h && historyZero.saved && historyZero.closed && historyZero.nonce==336
   && !historyZero.dirty && !historyZero.ready && !historyZero.running && historyZero.identity==identity && historyZero.owner==h
   && historyZero.banks==&banks && historyZero.proof==&proof && !h->OwnsInitializationLease(&historyZero)
   && h->InitializationProgress(identity,pages) && pages==1680 && h->InitializationPublished(identity)
   && h->MatchesInitializationOwner(banks,proof,identity);
 }
 bool InitializationsClosed(const PioneerResidentBanks& banks,const tt_dram_samples_result& proof)const{
  const auto* p=graph.Projection(identity);unsigned pages=0;
  return !Unsafe() && HistoryInitialized(banks,proof) && p && projectionZero.saved && projectionZero.closed && projectionZero.nonce==2
   && !projectionZero.dirty && !projectionZero.ready && !projectionZero.running && projectionZero.identity==identity && projectionZero.owner==p
   && projectionZero.banks==&banks && projectionZero.proof==&proof && !p->OwnsInitializationLease(&projectionZero)
   && p->InitializationProgress(identity,pages) && pages==512 && p->InitializationPublished(identity)
   && p->MatchesInitializationOwner(banks,proof,identity);
 } // actual initialization-controller seals ONLY;not KV/metadata/input/RUN or END authority
 void Fault(){unsafe=true;phase=Unknown;recurrentZero.unsafe=historyZero.unsafe=projectionZero.unsafe=true;
  pci.Fault();graph.Fault();if(memory)memory->Fault();if(runtime)runtime->Fault();} // MEMORY ONLY;retain every borrowed resource
};
static_assert(sizeof(PioneerQ38TokenNativeState)>262144 && sizeof(PioneerQ38TokenNativeState)<1048576,
 "complete canonical+runtime reservation model is HEAP ONLY");
#endif
#endif
