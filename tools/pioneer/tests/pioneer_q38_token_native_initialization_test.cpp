/* SPDX-License-Identifier: MIT */
// Actual native factory/lock/phase callbacks/initializers and actual frozen
// zero RV32 instructions. OS/MMIO/GDDR endpoints are simulated. No synthetic
// completion fabric,no old embedding/prefix producer and no completed token.
#include "pioneer_q38_token_native_fixture_common.h"
#ifdef PIONEER_Q38_TOKEN_NATIVE_CLOCK_FIXTURE
using area_id=int;
#include "pioneer_q38_token_clock_raw.h"
#endif
#define PIONEER_Q38_STATE_ZERO_CONTROLLER_MODEL_ONLY
#include "pioneer_q38_state_zero_controller_test.cpp"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38NonRootHeld.h"
#ifdef PIONEER_Q38_TOKEN_NATIVE_HELD_FIXTURE
#define PIONEER_Q38_TOKEN_NATIVE_VIEW_ENDPOINT_TEST 1
#include "pioneer_q38_token_native_held_endpoints.h"
using TokenNativeRawEndpoints=TokenHeldEndpoints;
#else
using TokenNativeRawEndpoints=ZeroControlIO;
#endif
struct TokenNativeEndpoints:TokenNativeRawEndpoints {
 using TokenNativeRawEndpoints::TokenNativeRawEndpoints;
 using HostReadMock::quarantined; // same input-page flag;output has its own unrelated flag
 // Execute the actual production metadata-only base quarantine,including
 // after a successful OS release. Do NOT broaden the legacy HostReadMock
 // assertion used by older live-DMA tests or invent allocation ownership.
#include "token_native_base_host_quarantine.inc"
};
using PioneerHostPageIO=TokenNativeEndpoints;
#ifdef PIONEER_Q38_TOKEN_NATIVE_BOOTSTRAP_FIXTURE
#include "pioneer_q38_token_native_bootstrap_machine.h"
#endif
#ifdef PIONEER_Q38_TOKEN_NATIVE_EXECUTION_FIXTURE
#include "pioneer_q38_token_native_root_machine.h"
#endif
#ifdef PIONEER_Q38_TOKEN_NATIVE_FULL_FLOW_FIXTURE
#include "pioneer_q38_native_trace_test_bridge.h"
#endif
struct PioneerSessionIO:TokenNativeEndpoints {
 tt_dram_samples_result& dram;
 TokenPciRaw nativePci;
 tt_pioneer_mmio_result nativePciBegin={};tt_pioneer_mmio_transaction nativePciTransaction={};
#ifdef PIONEER_Q38_TOKEN_NATIVE_PCI_FIXTURE
 const PioneerQ38TokenPciResult* nativePciResult=nullptr;unsigned nativePciFault=0,nativePciFaultSeen=0,nativePciFaultEvents=0;
#endif
 uint32 CardRead(unsigned offset,unsigned width){
  const uint32 actual=nativePci.CardRead(offset,width);
#ifdef PIONEER_Q38_TOKEN_NATIVE_PCI_FIXTURE
  if(nativePciFault==2 && !nativePciFaultSeen && nativePciResult && nativePciResult->started
   && nativePci.writes==12 && offset==4 && width==2){nativePciFaultSeen++;nativePciFaultEvents=nativePci.events;return actual^1U;}
#endif
  return actual;
 }
 uint32 RootRead(unsigned offset,unsigned width){return nativePci.RootRead(offset,width);}
 void CardWrite(unsigned offset,unsigned width,uint32 value){nativePci.CardWrite(offset,width,value);}
 void RootWrite(unsigned offset,unsigned width,uint32 value){nativePci.RootWrite(offset,width,value);}
#ifdef PIONEER_Q38_TOKEN_NATIVE_HELD_FIXTURE
 unsigned preparationFault=0,preparationFaultSeen=0,preparationFaultTouches=0,preparationMapWrites=0,preparationPanelWrites=0;
 unsigned preparationAuditContracts=0;uint64 preparationAuditLastKey=0;
 bool NativePreparationAuditInactive()const{return !q38TokenPreparationAudit.key;}
 void NativePreparationAuditContract();
 std::vector<unsigned> preparationFifoEvents;
 bool NativePreparationObservation();
 uint32 NativePreparationFifoObserved(uint32,uint32);
 void DramTlbWrite(unsigned,unsigned,uint32);uint32 DramTlbRead(unsigned,unsigned);
 void ResidentNocWrite(uint32,uint32);void Q38PanelWrite(uint32,uint32);uint32 Q38PanelRead(uint32);
#ifdef PIONEER_Q38_TOKEN_NATIVE_RESTORE_SHARED_TLB_FIXTURE
 void NativeSharedAliasRefusals();
#endif
#endif
#ifdef PIONEER_Q38_TOKEN_NATIVE_FULL_FLOW_FIXTURE
 std::unique_ptr<Q38NativeTraceTest> nativeTrace=std::make_unique<Q38NativeTraceTest>();
 unsigned vocabularySequence[119]={};
#endif
#ifdef PIONEER_Q38_TOKEN_NATIVE_PEER_FIXTURE
#include "pioneer_q38_token_native_peer_bytes.inc"
#endif
 PioneerSessionIO(const char* image,tt_dram_samples_result& proof):TokenNativeEndpoints(image),dram(proof){
  assert(BlackholePioneerMmioBegin(nativePci,nativePciBegin,nativePciTransaction) && nativePci.writes==11);
#ifdef PIONEER_Q38_TOKEN_NATIVE_HELD_FIXTURE
  tokenHeldEndpoint=this;
  outputArea=302;wideArea=303;
#ifdef PIONEER_Q38_TOKEN_NATIVE_CLOCK_FIXTURE
  tokenModelViewDelete=[](area_id view){
   auto& io=*static_cast<PioneerSessionIO*>(tokenHeldEndpoint);io.Touch();io.nativeViewDeletes.push_back(view);
   assert(view<101 || view>106);
   for(unsigned i=0;i<3;i++)assert(view!=io.TokenHostView(i));
   if(io.nativeViewFault && io.nativeViewDeletes.size()==1){
    io.nativeViewFaultTouches=io.touches;
    if(io.nativeViewFault==2)sReloadGeneration++;
    if(io.nativeViewFault==1)return B_BAD_VALUE;
   }
   return B_OK;
  };
#endif
#endif
#ifdef PIONEER_Q38_TOKEN_NATIVE_BOOTSTRAP_FIXTURE
#ifdef PIONEER_Q38_TOKEN_NATIVE_PEER_FIXTURE
  PreparePeerBytes();
  for(unsigned w=1;w<120;w++)bootstrapPeer[w-1]=std::make_unique<TokenBootstrapPeer>(image,*this,w,peerBytes);
  for(unsigned w=0;w<96;w++)bootstrapPeer[w]->Link(*bootstrapPeer[w^1]);
#else
  for(unsigned w=1;w<120;w++)bootstrapPeer[w-1]=std::make_unique<TokenBootstrapPeer>(image,*this,w);
#endif
#endif
#ifdef PIONEER_Q38_TOKEN_NATIVE_EXECUTION_FIXTURE
  executionCpu=std::make_unique<TokenStartupRoot>(image,*this);
#ifdef PIONEER_Q38_TOKEN_NATIVE_LAYER_FIXTURE
  executionCpu->livePeerPC=[](TokenHeldEndpoints& endpoints,unsigned worker){
   auto& native=static_cast<PioneerSessionIO&>(endpoints);
   assert(worker && worker<120);return native.bootstrapPeer[worker-1]->pc;
  }; // live byte/PC endpoint attached BEFORE binding or the first instruction
#endif
#endif
 }
 PioneerStoreContext* q38MutableContext=nullptr;
#ifdef PIONEER_Q38_TOKEN_NATIVE_EXECUTION_FIXTURE
 std::unique_ptr<TokenStartupRoot> executionCpu;
 bool executionMode=false;unsigned executionFault=0,executionArms=0,executionStarts=0,executionObserved=0;
 unsigned terminalFault=0;
 unsigned executionPcReads=0,executionFaultPcObserved=0;
#if defined(PIONEER_Q38_TOKEN_NATIVE_HOLD_LAYER_FIXTURE) || defined(PIONEER_Q38_TOKEN_NATIVE_READOUT_FLOW_FIXTURE)
 std::vector<unsigned> terminalHoldOrder;
#endif
#ifdef PIONEER_Q38_TOKEN_NATIVE_EMBEDDING_FIXTURE
 unsigned embeddingFault=0;
#endif
 bool RootReset(uint32 value){
  if(!executionMode || runtimeCore)return false;
#if defined(PIONEER_Q38_TOKEN_NATIVE_HOLD_LAYER_FIXTURE) || defined(PIONEER_Q38_TOKEN_NATIVE_READOUT_FLOW_FIXTURE)
  if(value==0x47800){
   Touch();assert(C().coreReset==0x46800 && executionCpu->pc==0x60004 && !C().pending && terminalHoldOrder.empty());
   C().resets.push_back(value);C().coreReset=value;terminalHoldOrder.push_back(0);return true;
  } // actual requested HOLD endpoint only;no processor PC/history change
#endif
  Touch();assert((value==0x87ffff && C().coreReset==0x47800 && !executionArms && !executionCpu->steps)
   || (value==0x46800 && C().coreReset==0x87ffff && executionArms==1 && !executionStarts && !executionCpu->steps));
  C().resets.push_back(value);C().coreReset=value;
  if(value==0x87ffff)executionArms++;else executionStarts++;
  if(executionFault==1 && value==0x87ffff){sReloadGeneration++;executionObserved=touches;}
  if(executionFault==2 && value==0x46800){C().coreReset=0x87ffff;executionObserved=touches;}
  return true;
 }
#endif
#ifdef PIONEER_Q38_TOKEN_NATIVE_BOOTSTRAP_FIXTURE
 std::array<std::unique_ptr<TokenBootstrapPeer>,119> bootstrapPeer;
 bool bootstrapMode=false;unsigned bootstrapArms=0,bootstrapStarts=0,bootstrapFault=0,bootstrapObserved=0;
 void Pause(){
  if(!bootstrapMode){ZeroControlIO::Pause();return;}
  Touch();clock+=1000;
  for(unsigned w=1;w<120;w++)if(Core(w).coreReset==0x46800){
#ifdef PIONEER_Q38_TOKEN_NATIVE_LAYER_FIXTURE
   // Event scheduling,as in the full instruction fabric:pause the SAME
   // genuinely booted CPU at its actual WAIT while no doorbell is present.
   // Do not execute millions of irrelevant idle polls,fast-forward a PC,
   // copy processor state or invent a worker result. A delivered doorbell
   // makes this exact CPU runnable again. No wall-clock timing is modeled.
   if(executionMode && executionCpu->layerActive && bootstrapPeer[w-1]->Idle()
    && !Raw(w,PioneerQ38GlobalWorkerLayout::Doorbell))continue;
#ifdef PIONEER_Q38_TOKEN_NATIVE_FULL_LAYER_FIXTURE
   if(executionMode && executionCpu->layerActive && bootstrapPeer[w-1]->Idle()){
    const unsigned doorbell=Raw(w,PioneerQ38GlobalWorkerLayout::Doorbell),job=Raw(w,PioneerQ38GlobalWorkerLayout::Job+4);
    const auto observed=bootstrapPeer[w-1]->cpu->Stats();
    if(doorbell!=job || job!=observed.sequence+1)continue;
   } // actual WAIT/no newly delivered event:pause,NEVER advance PC/history or supply a completion
#endif
#endif
   bootstrapPeer[w-1]->Advance(256);
  }
#ifdef PIONEER_Q38_TOKEN_NATIVE_EXECUTION_FIXTURE
  if(executionMode && Core(0).coreReset==0x46800){
   executionCpu->Advance(256);
   if(executionFault==3 && executionCpu->markerWrites && !executionObserved){Core(0).source[11]^=1;executionObserved=touches;}
  }
#endif
 }
 uint32 RuntimePcRead(){
  if(!bootstrapMode)return ZeroControlIO::RuntimePcRead();
  Touch();assert(runtimeCore<120);
#ifdef PIONEER_Q38_TOKEN_NATIVE_EXECUTION_FIXTURE
  if(!runtimeCore){assert(executionMode);executionPcReads++;
   if(executionCpu->pc==0x60000 && !executionFaultPcObserved)executionFaultPcObserved=touches;
   return executionCpu->pc;}
#endif
  assert(runtimeCore);return bootstrapPeer[runtimeCore-1]->pc;
 }
 bool BootstrapReset(uint32 value){
  if(!bootstrapMode)return false;
#if defined(PIONEER_Q38_TOKEN_NATIVE_HOLD_LAYER_FIXTURE) || defined(PIONEER_Q38_TOKEN_NATIVE_READOUT_FLOW_FIXTURE)
  if(value==0x47800){
   Touch();assert(runtimeCore && runtimeCore<120 && Core(0).coreReset==0x47800
    && C().coreReset==0x46800 && terminalHoldOrder.size()==runtimeCore && !C().pending
    && bootstrapPeer[runtimeCore-1]->cpu->ObserveStopped());
   C().resets.push_back(value);C().coreReset=value;terminalHoldOrder.push_back(runtimeCore);return true;
  } // same actually stopped engine;HOLD never supplies a STOP receipt
#endif
  Touch();assert(runtimeCore && runtimeCore<120 && (value==0x87ffff || value==0x46800));
  auto& core=C();auto& cpu=*bootstrapPeer[runtimeCore-1];core.resets.push_back(value);
  if(value==0x87ffff){assert(core.coreReset==0x47800 && !cpu.steps && cpu.pc==0x6000);bootstrapArms++;}
  else{assert(bootstrapArms==119 && core.coreReset==0x87ffff && !cpu.steps);bootstrapStarts++;}
  core.coreReset=value;
  if(bootstrapFault==1 && bootstrapArms==1 && !bootstrapStarts){sReloadGeneration++;bootstrapObserved=touches;}
  if(bootstrapFault==2 && runtimeCore==119 && value==0x46800){core.accepted++;bootstrapObserved=touches;}
  return true;
 }
#endif
#ifndef PIONEER_Q38_TOKEN_NATIVE_HELD_FIXTURE
 unsigned runtimeCore=0;bool fleetEnabled=false;
 bool q38TokenMemoryMapAttempted=false;
 bool TokenInitializationFifoEmpty(unsigned);bool TokenInitializationFifoReadChecked(unsigned,bool,uint32&);
#endif
 unsigned initializationFifoFault=0,initializationFifoReads=0,initializationFifoObserved=0;
 bool pciCaptureFault=false;
 uint32 TokenFifoStatusWord(uint32 address){
  const uint32 value=TokenNativeEndpoints::TokenFifoStatusWord(address);
  if(initializationFifoFault){
   assert(!initializationFifoObserved);initializationFifoReads++;
   if((initializationFifoFault==1 && address==0x64) || (initializationFifoFault==2 && address==0x68)){
    initializationFifoObserved=touches;return address==0x64?0x1010100fU:1U;
   }
  } // bad OBSERVATION on actual scoped read,not caller-supplied native readiness or completion
#ifdef PIONEER_Q38_TOKEN_NATIVE_HELD_FIXTURE
  return NativePreparationFifoObserved(address,value);
#else
  return value;
#endif
 }
 bool runtimeMapped=true,q38SpanAllowed=true,nonrootFault=false;
 int outputArea=0,wideArea=0; // simulated owned VM handles;real quarantine method below
#ifdef PIONEER_Q38_TOKEN_NATIVE_HELD_FIXTURE
 std::array<area_id,25> nativeViewAreas=[](){std::array<area_id,25> values;for(unsigned i=0;i<25;i++)values[i]=int(200+i);return values;}();
 std::vector<area_id> nativeViewDeletes;unsigned nativeViewFault=0,nativeViewFaultTouches=0;
#endif
#include "token_native_host_quarantine.inc"
 unsigned failRun=0,loseTeamRun=0,loseGenerationRun=0,observedGdnPages=0,observedHistoryPages=0,observedProjectionPages=0;
 bool TokenCallScopeMatches();bool TokenModelSourceScope(bool);
 bool TokenInitializationScope(unsigned,bool);bool TokenInitializationOtherCoresHeld(unsigned);
 bool StateZeroBeforeCohort();bool StateZeroOtherCoresHeld();
 bool ConvHistoryZeroBeforeCohort();bool ConvHistoryZeroOtherCoresHeld();
 bool ProjectionZeroBeforeCohort();bool ProjectionZeroOtherCoresHeld();
 bool NonRootHeldOwner();
 bool NonRootHeldMappingStable(){return !coreMapLost && NonRootHeldOwner();} // simulated VM endpoint only
 PioneerQ38GdnRegion recurrent[672]={};PioneerQ38ConvHistoryExtent history[336]={};
 unsigned recurrentFirst[672]={};
 std::vector<uint32> recurrentWords=std::vector<uint32>(73728*1024,0x13579bdf);
 std::vector<unsigned> recurrentTouched=std::vector<unsigned>(73728);
 std::vector<uint32> historyWords=std::vector<uint32>(1680*1024,0x13579bdf);
 std::vector<unsigned> historyTouched=std::vector<unsigned>(1680);
 uint32 workspaceAddress=0;unsigned workspaceBank=7;
#ifdef PIONEER_Q38_TOKEN_NATIVE_RUNTIME_FIXTURE
 std::vector<uint32>& workspaceWords=runtimeDram; // SAME physical workspace bytes from construction through NoC staging
#else
 std::vector<uint32> workspaceWords=std::vector<uint32>(512*1024,0x13579bdf);
#endif
 std::vector<unsigned> workspaceTouched=std::vector<unsigned>(512);
#ifdef PIONEER_Q38_TOKEN_NATIVE_FULL_LAYER_FIXTURE
#include "pioneer_q38_token_native_mutable_bytes.inc"
#endif
#ifdef PIONEER_Q38_TOKEN_NATIVE_FULL_FLOW_FIXTURE
#include "pioneer_q38_token_native_kv_bytes.inc"
#endif
#ifdef PIONEER_Q38_TOKEN_NATIVE_HELD_FIXTURE
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38TokenNativeMapping.inc"
#else
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38TokenPreparationAuditMember.inc"
 const void* TokenPciTransport(){return nullptr;}uint64 TokenPciEndpoint(){return 0;}
 bool TokenPciLeaseIdentity(const PioneerQ38TokenPciLease&)const{return false;}
 // Initializer-only fixture exposes NO runtime/PCI-journal capability.
 void DramTlbWrite(unsigned slot,unsigned word,uint32 value){Touch();StoreMock::DramTlbWrite(slot,word,value);}
 uint32 DramTlbRead(unsigned slot,unsigned word){Touch();return StoreMock::DramTlbRead(slot,word);}
 uint32 SourceRead(unsigned word){Touch();return ZeroControlIO::SourceRead(word);}
 bool SelectRuntimeCore(unsigned core){Touch();if(core)return false;
  return BlackholeDramConfigure(*this,0,0,1,2,false) && BlackholeDramConfigure(*this,1,0xffa00000,1,2,false);
 }
 uint32 ResetRead(){
  Touch();bool root=false;
  for(bool readOnly:{false,true}){bool same=true;for(unsigned w=0;w<3;w++)
   same&=tlb[1][w]==BlackholeDramFixedTlbWord(0xffa00000,1,2,readOnly,w);root|=same;}
  return root?coreReset:nonrootFault?0x46800:0x47800;
 }
#endif
 void RuntimeResetWrite(uint32 value){
#ifdef PIONEER_Q38_TOKEN_NATIVE_EXECUTION_FIXTURE
  if(RootReset(value))return;
#endif
#ifdef PIONEER_Q38_TOKEN_NATIVE_BOOTSTRAP_FIXTURE
  if(BootstrapReset(value))return;
#endif
#ifdef PIONEER_Q38_TOKEN_NATIVE_HELD_FIXTURE
  assert(!runtimeCore); // root initialization ONLY;no held participant may launch
#endif
  assert(!publicationCounterFabricOnly && !authorityChanged);
  const bool executes=value==0x46800 && !noCompletion;const unsigned before=rootStarts;
  unsigned first=0,words=executes?l1[4]/4:0,kind=3,region=0;
  if(executes){
   for(unsigned i=0;i<672;i++)if(recurrent[i].bank==l1[7] && l1[5]>=recurrent[i].offset
    && l1[5]-recurrent[i].offset<recurrent[i].heads*65536){
    assert(kind==3);kind=0;region=i;first=(l1[5]-recurrent[i].offset)/4;
    assert(words && words<=recurrent[i].heads*16384-first);
   }
   for(unsigned i=0;i<336;i++)if(history[i].bank==l1[7] && l1[5]>=history[i].address && l1[5]-history[i].address<history[i].bytes){
    assert(kind==3);kind=1;region=i;first=(l1[5]-history[i].address)/4;assert(words && words<=5120-first);
   }
   if(l1[7]==workspaceBank && l1[5]>=workspaceAddress && l1[5]-workspaceAddress<2097152){
    assert(kind==3);kind=2;first=(l1[5]-workspaceAddress)/4;assert(words && words<=512*1024-first);
   }
   assert(kind<3);
   if(failRun==rootStarts+1)summaryFault=31; // corrupt real instruction-produced summary,not synthetic completion
  }
  ZeroControlIO::RuntimeResetWrite(value);
  if(executes && before!=rootStarts){
   assert(lastDeviceWords.size()==words);
   auto& out=kind==0?recurrentWords:kind==1?historyWords:workspaceWords;
   auto& touched=kind==0?recurrentTouched:kind==1?historyTouched:workspaceTouched;
   const unsigned base=kind==0?recurrentFirst[region]*1024:kind==1?region*5120:0;
   for(unsigned i=0;i<words;i++)out.at(base+first+i)=lastDeviceWords[i];
   for(unsigned p=first/1024;p<(first+words)/1024;p++)touched.at(base/1024+p)++;
   if(kind==0)observedGdnPages+=words/1024;else if(kind==1)observedHistoryPages+=words/1024;else observedProjectionPages+=words/1024;
   if(loseTeamRun==rootStarts)currentTeam++;
   if(loseGenerationRun==rootStarts)sReloadGeneration++;
   if(!(rootStarts%96))printf("OBSERVED actual frozen zero ROM starts%u/GDNpages%u/historyPages%u/projectionPages%u;NOT native initialization publication\n",
    rootStarts,observedGdnPages,observedHistoryPages,observedProjectionPages);
  }
 }
};
#ifdef PIONEER_Q38_TOKEN_NATIVE_HELD_FIXTURE
const void* PioneerSessionIO::TokenPciTransport(){return &nativePci;}
uint64 PioneerSessionIO::TokenPciEndpoint(){return (uint64(nativePci.profile.root_bus)<<24)|(uint64(nativePci.profile.card_bus)<<16);}
#endif
#ifdef PIONEER_Q38_TOKEN_NATIVE_HELD_FIXTURE
area_id* PioneerSessionIO::TokenViewHandle(unsigned index){
 return index<4?&q38TokenMemoryArea[index]:index<29?&nativeViewAreas[index-4]:nullptr;
} // simulated29 OS-handle endpoints present before any native owner exists
area_id PioneerSessionIO::TokenHostView(unsigned index){return index==0?301:index==1?outputArea:index==2?wideArea:-1;}
void PioneerSessionIO::TokenClearViewPointers(){
 for(auto& view:q38TokenMemoryView)view=nullptr;
 runtimeEntry=scratchData=nullptr;runtimePc=nullptr;runtimeMapped=false;fleetEnabled=false;
}
#endif
struct PioneerStoreContext {
 PioneerSessionIO& io;PioneerResidentBanks banks;
#ifdef PIONEER_Q38_TOKEN_NATIVE_CLOCK_FIXTURE
 struct ClockControl{ClockControl& control=*this;tt_arc_scratch_result arc=StrictQueue::Arc();tt_arc_telemetry_result telemetry=StrictQueue::Health();};
 struct D{tt_dram_samples_result& dram;ClockControl control={};};
#else
 struct D{tt_dram_samples_result& dram;};
#endif
 struct B1{D baseline;tt_compute_noc_result& noc;};
 struct B2{B1 baseline;MockLease path;};struct B3{B2 baseline;MockLease audit;};B3 baseline;
 tt_pioneer_profile profile=TokenPciRawModel::GoodProfile();tt_pioneer_mmio_result& pci;tt_pioneer_mmio_transaction& transaction;
 PioneerQ38TokenNativeState* q38TokenNative=nullptr;PioneerQ38CanonicalModelPlan* q38FfnModelPlan=nullptr;
 MockForeignOwner *q38GraphMutable=nullptr,*q38FfnZero=nullptr,*q38FfnGraph=nullptr,*q38FfnCode=nullptr,*q38GdnBanks=nullptr,*q38StateZero=nullptr;
 PioneerQ38DispatchLease* q38Dispatch=nullptr;MockCohort* q38Cohort=nullptr;MockFleet* fleet=nullptr;
 PioneerQ38GdnIdentity q38FfnModelIdentity={},q38GdnIdentity={};MockLease q38Delta,panelProbe,streamProbe,span,secondLease,secondNoc,idle;
 MockLease peerAttention[8],decoder,layer,coordinator,attention,compute,secondCompute;
 MockLease* q38Span=&span;
#ifdef PIONEER_Q38_TOKEN_NATIVE_CLOCK_FIXTURE
 tt_pioneer_profile tokenClockProfile;tt_arc_queue_result tokenClockMapping={};
 PioneerQ38ClockIO q38ClockIO;PioneerQ38ClockLease<PioneerQ38ClockIO> q38Clock;
 unsigned tokenClockFault=0;
#else
 MockClock q38Clock;
#endif
 PioneerResidentLease& lease;PioneerBulkUploadLease& bulk;PioneerOutputBufferLease& bulkHost;PioneerWideOutputLease& wideHost;tt_host_page_result& host;
 uint8* initialized=nullptr;uint32* hashes=nullptr;thread_id q38GdnCallThread=-1;uint64 bytes=PioneerResidentBanks::Total;
 bool bankStorageOnly=true,dispatchOwnerRetained=false,q38DispatchRoutes=false;unsigned q38EndPhase=0;int runtimeCoreMode=-1;
 PioneerStoreContext(PioneerSessionIO& native,PanelOwners& owner,PioneerBulkUploadLease& b,PioneerOutputBufferLease& transfer,
  PioneerWideOutputLease& wide):io(native),baseline{{{{native.dram},owner.noc},{ }},{ }},pci(native.nativePciBegin),transaction(native.nativePciTransaction)
#ifdef PIONEER_Q38_TOKEN_NATIVE_CLOCK_FIXTURE
  ,q38ClockIO(tokenClockProfile,baseline.baseline.baseline.baseline.control.control.control.arc,baseline.baseline.baseline.baseline.control.control.telemetry,tokenClockMapping),q38Clock(q38ClockIO)
#endif
  ,lease(owner.store),bulk(b),bulkHost(transfer),wideHost(wide),host(owner.host){}
 auto& Pci(){return pci;}
#include "token_native_unsafe.inc"
 ~PioneerStoreContext(){delete q38TokenNative;delete q38Dispatch;} // CPU-only fixture disposal;no physical resources exist
};
#include "token_native_lock.inc"
static void TestKernelLog(const char*,...){}
#define dprintf TestKernelLog
#include "token_native_retain.inc"
#undef dprintf
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38TokenNativeBind.inc"
#if defined(PIONEER_Q38_TOKEN_NATIVE_HELD_FIXTURE) && !defined(PIONEER_Q38_TOKEN_NATIVE_RUNTIME_FIXTURE)
bool PioneerSessionIO::TokenPciLeaseIdentity(const PioneerQ38TokenPciLease&)const{return false;}
// Held-only tests have NO runtime PCI lease. Keep that unreachable source
// branch explicitly refusing instead of leaving a missing linker endpoint.
#endif
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38TokenNativeInitialize.inc"
#ifdef PIONEER_Q38_TOKEN_NATIVE_HELD_FIXTURE
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38TokenNativeHeld.inc"
#ifdef PIONEER_Q38_TOKEN_NATIVE_RUNTIME_FIXTURE
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38TokenNativeRuntime.inc"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38TokenNativePciCapture.inc"
#ifdef PIONEER_Q38_TOKEN_NATIVE_EXECUTION_FIXTURE
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38TokenNativeExecution.inc"
#ifdef PIONEER_Q38_TOKEN_NATIVE_CLOCK_FIXTURE
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38TokenNativeClock.inc"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38TokenNativeHardware.inc"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38TokenNativeViews.inc"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38TokenNativePci.inc"
#endif
#endif
#endif
#endif
#include "token_native_zero_callbacks.inc"
#ifdef PIONEER_Q38_TOKEN_NATIVE_HELD_FIXTURE
#include "pioneer_q38_token_native_prepare_endpoints.inc"
#ifdef PIONEER_Q38_TOKEN_NATIVE_RESTORE_SHARED_TLB_FIXTURE
#include "pioneer_q38_token_native_shared_tlb_endpoints.inc"
#endif
#endif
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38TokenCallScope.inc"
static bool IncompleteEnd(PioneerStoreContext& c,tt_resident_session_result& result,bool& clean){
#include "token_native_end.inc"
 return true;
}
struct Fixture {
 tt_dram_samples_result proof={};PioneerSessionIO io;PanelOwners owner;
 PioneerBulkUploadLease bulk;PioneerOutputBufferLease transfer;PioneerWideOutputLease wide;
 std::unique_ptr<PioneerStoreContext> c;PioneerProbeCookie cookie;uint32 hash=0;
 std::vector<uint8> learned=std::vector<uint8>(PioneerResidentBanks::Pages);
 explicit Fixture(const char* image):io(image,proof),owner(io){
  assert(!lockHeld);sColdRecoveryRequired=false;sReloadQuiesced=false;sBankStorageOnly=true;
  currentThread=17;currentTeam=23;failThreadInfo=false;lockStatus=B_OK;sQ38LocalDeltaOwner=nullptr;
  proof=owner.dram;
  assert(PioneerPrepareOutputBuffer(io,owner.host,transfer) && PioneerBulkUploadPrepare(io,proof,owner.noc,owner.host,owner.store,transfer,bulk)
   && PioneerPrepareWideOutput(io,owner.host,transfer,wide));
  c.reset(new PioneerStoreContext(io,owner,bulk,transfer,wide));assert(c->banks.Bind(proof));
  c->span.closed=true;c->hashes=&hash;c->initialized=learned.data();io.q38MutableContext=c.get();cookie.context=c.get();sRuntimeOwner=&cookie;
#ifdef PIONEER_Q38_TOKEN_NATIVE_CLOCK_FIXTURE
  c->q38ClockIO.sharedWords=io.Core(0).tlb[1];
  assert(c->q38Clock.Begin(c->baseline.baseline.baseline.baseline.control.control.control.arc,c->baseline.baseline.baseline.baseline.control.control.telemetry)
   && c->q38ClockIO.notifications==3 && c->q38ClockIO.clock==1350);
#endif
  auto plan=std::make_unique<PioneerQ38CanonicalModelPlan>();assert(plan->Build());
  for(unsigned i=0;i<plan->Count();i++){const auto* p=plan->Part(i);if(p->kind!=plan->WeightKind)continue;
   unsigned first=p->bank*786432+p->offset/4096;for(unsigned j=0;j<p->allocated/4096;j++)learned[first+j]=1;
  }
  unsigned pages=0;assert(plan->RecurrentCount()==672);
  for(unsigned i=0;i<672;i++){io.recurrent[i]=plan->RecurrentRegions()[i];io.recurrent[i].offset+=plan->Base;
   io.recurrentFirst[i]=pages;pages+=io.recurrent[i].heads*16;}
  assert(pages==73728);
  unsigned n=0;
  for(unsigned i=0;i<plan->Count();i++){const auto* p=plan->Part(i);if(p->kind!=plan->ConvKind)continue;
   assert(n<336);io.history[n++]={p->bank,plan->Base+p->offset,PioneerQ38ProjectionRoute(p->bank),0,p->allocated/4096,p->allocated};
  }
  assert(n==336);io.workspaceAddress=plan->Base+plan->Workspace(0)->offset;io.workspaceBank=0;
#ifdef PIONEER_Q38_TOKEN_NATIVE_FULL_LAYER_FIXTURE
  io.BindMutableBytes();
#endif
#ifdef PIONEER_Q38_TOKEN_NATIVE_FULL_FLOW_FIXTURE
  io.BindKvBytes(*plan);
#endif
 }
 bool Bind(){PioneerQ38TokenCallScope scope(&cookie);return scope.Context() && BindQ38TokenNative(*scope.Context(),0);}
 bool Recurrent(PioneerQ38StateZeroResult& r){PioneerQ38TokenCallScope scope(&cookie);return scope.Context() && InitializeQ38TokenRecurrent(*scope.Context(),r);}
 bool History(PioneerQ38ConvHistoryZeroResult& r){PioneerQ38TokenCallScope scope(&cookie);return scope.Context() && InitializeQ38TokenHistory(*scope.Context(),r);}
 bool Projection(PioneerQ38ProjectionZeroResult& r){PioneerQ38TokenCallScope scope(&cookie);return scope.Context() && InitializeQ38TokenProjection(*scope.Context(),r);}
 void Unknown(){
  if(!io.wideQuarantined)fprintf(stderr,"TOKEN_INIT_RETAIN_DIAGNOSTIC unsafe%u retained%u banks%u clock%u cold%u bulk%u transferQ%u wideLeaseQ%u hostQ%u wideIOQ%u releases%u/%u/%u pciWrites%u;CPU metadata ONLY\n",
   unsigned(c->q38TokenNative->Unsafe()),unsigned(c->dispatchOwnerRetained),unsigned(c->banks.Quarantined()),unsigned(c->q38Clock.Unsafe()),
   unsigned(sColdRecoveryRequired),unsigned(c->bulk.unsafe),unsigned(c->bulkHost.quarantined),unsigned(c->wideHost.quarantined),
   unsigned(io.HostReadMock::quarantined),unsigned(io.wideQuarantined),unsigned(io.wideReleased),unsigned(io.PageMock::releases),unsigned(io.OutputBufferMock::releases),io.pciWrites);
  assert(c->q38TokenNative->Unsafe() && !c->q38TokenNative->Closed() && !c->q38TokenNative->InitializationsClosed(c->banks,proof)
   && c->dispatchOwnerRetained && c->banks.Quarantined() && c->q38Clock.Unsafe() && sColdRecoveryRequired
   && c->bulk.unsafe && c->bulkHost.quarantined && c->wideHost.quarantined && io.HostReadMock::quarantined
   && io.OutputBufferMock::quarantined && io.wideQuarantined
   && !io.wideReleased && !io.PageMock::releases && !io.OutputBufferMock::releases && !io.pciWrites);
  unsigned touches=io.touches;PioneerQ38StateZeroResult g;PioneerQ38ConvHistoryZeroResult h;PioneerQ38ProjectionZeroResult p;
  tt_resident_session_result end={};bool clean=true;
  assert(!Recurrent(g) && !History(h) && !Projection(p) && !IncompleteEnd(*c,end,clean) && io.touches==touches);
 }
};
#ifdef PIONEER_Q38_TOKEN_NATIVE_HELD_FIXTURE
#ifdef PIONEER_Q38_TOKEN_NATIVE_RUNTIME_FIXTURE
#include "pioneer_q38_token_native_runtime_checks.inc"
#ifdef PIONEER_Q38_TOKEN_NATIVE_BOOTSTRAP_FIXTURE
#include "pioneer_q38_token_native_bootstrap_checks.inc"
#ifdef PIONEER_Q38_TOKEN_NATIVE_EXECUTION_FIXTURE
#include "pioneer_q38_token_native_execution_checks.inc"
#endif
#endif
#endif
#include "pioneer_q38_token_native_held_checks.inc"
#include "pioneer_q38_token_native_prepare_checks.inc"
#endif
int main(int argc,char** argv){
 const bool clockContract=argc==3 && !strcmp(argv[2],"--clock-owner-contract");
 const bool peerBootstrap=argc==5 && !strcmp(argv[2],"--bootstrap-peer-engines");
 const bool fullFlow=argc==7 && (!strcmp(argv[2],"--execution-full-flow") || !strcmp(argv[2],"--execution-readout-full-flow"));
 const bool pciClose=argc==7 && (!strcmp(argv[2],"--execution-pci-close") || !strcmp(argv[2],"--execution-pci-generation-fault")
  || !strcmp(argv[2],"--execution-pci-readback-fault"));
 const bool viewsClose=pciClose || (argc==7 && (!strcmp(argv[2],"--execution-views-close") || !strcmp(argv[2],"--execution-views-delete-fault")
  || !strcmp(argv[2],"--execution-views-generation-fault")));
 const bool hardwareSeal=viewsClose || (argc==7 && (!strcmp(argv[2],"--execution-hardware-seal") || !strcmp(argv[2],"--execution-hardware-seal-tlb-fault")));
 const bool clockEnd=hardwareSeal || (argc==7 && (!strcmp(argv[2],"--execution-clock-end") || !strcmp(argv[2],"--execution-clock-end-generation-fault")
  || !strcmp(argv[2],"--execution-clock-end-response-fault") || !strcmp(argv[2],"--execution-clock-end-unmap-fault")));
 const bool restoreShared=clockEnd || (argc==7 && (!strcmp(argv[2],"--execution-restore-shared-tlb") || !strcmp(argv[2],"--execution-restore-shared-tlb-fault")
  || !strcmp(argv[2],"--execution-restore-shared-tlb-readback-fault") || !strcmp(argv[2],"--execution-restore-shared-tlb-verify-fault")));
 const bool restoreRoot=restoreShared || (argc==7 && (!strcmp(argv[2],"--execution-restore-root-store") || !strcmp(argv[2],"--execution-restore-root-store-fault")
  || !strcmp(argv[2],"--execution-restore-root-store-map-fault") || !strcmp(argv[2],"--execution-restore-root-store-config-fault")
  || !strcmp(argv[2],"--execution-restore-root-store-scratch-fault") || !strcmp(argv[2],"--execution-restore-root-store-fifo-fault")
  || !strcmp(argv[2],"--execution-restore-root-store-overflow-fault")));
 const bool restorePeers=restoreRoot || (argc==7 && (!strcmp(argv[2],"--execution-restore-peers") || !strcmp(argv[2],"--execution-restore-peers-fault")
  || !strcmp(argv[2],"--execution-restore-peers-map-fault") || !strcmp(argv[2],"--execution-restore-peers-config-fault")
  || !strcmp(argv[2],"--execution-restore-peers-scratch-fault") || !strcmp(argv[2],"--execution-restore-peers-fifo-fault")
  || !strcmp(argv[2],"--execution-restore-peers-overflow-fault")));
 const bool cancelLayer=restorePeers || (argc==7 && (!strcmp(argv[2],"--execution-cancel-first-layer") || !strcmp(argv[2],"--execution-hold-first-layer")
  || !strcmp(argv[2],"--execution-readout-empty") || !strcmp(argv[2],"--execution-restore-empty") || !strcmp(argv[2],"--execution-restore-empty-fault")
  || !strcmp(argv[2],"--execution-restore-dispatch") || !strcmp(argv[2],"--execution-restore-dispatch-fault")));
 const bool terminalFaults=argc==3 && !strcmp(argv[2],"--terminal-observation-faults");
 const bool initializationFifoFaults=argc==3 && !strcmp(argv[2],"--initializer-fifo-faults");
 const bool preparationFifoFaults=argc==3 && !strcmp(argv[2],"--preparation-fifo-faults");
 const bool preparationAccessFaults=argc==3 && !strcmp(argv[2],"--preparation-access-faults");
 const bool preparationFaults=preparationFifoFaults || preparationAccessFaults;
 const bool preparationEndpoints=argc==3 && !strcmp(argv[2],"--preparation-endpoint-contract");
 const bool fullLayer=fullFlow || cancelLayer || (argc==7 && !strcmp(argv[2],"--execution-first-layer"));
 (void)cancelLayer;
 const bool firstMatrix=fullLayer || (argc==6 && !strcmp(argv[2],"--execution-first-matrix"));
 const bool embedding=firstMatrix || (argc==4 && (!strcmp(argv[2],"--execution-embedding") || !strcmp(argv[2],"--execution-embedding-source-loss")
  || !strcmp(argv[2],"--execution-embedding-readback-loss") || !strcmp(argv[2],"--execution-layer-dispatch")));
 assert(clockContract || preparationEndpoints || preparationFaults || initializationFifoFaults || terminalFaults || peerBootstrap || embedding || argc==2 || (argc==3 && (!strcmp(argv[2],"--faults") || !strcmp(argv[2],"--late-faults") || !strcmp(argv[2],"--held") || !strcmp(argv[2],"--held-faults")
  || !strcmp(argv[2],"--bootstrap") || !strcmp(argv[2],"--bootstrap-faults")
  || !strcmp(argv[2],"--execution") || !strcmp(argv[2],"--execution-faults")
  || !strcmp(argv[2],"--execution-traffic") || !strcmp(argv[2],"--execution-traffic-faults")
  || !strcmp(argv[2],"--runtime") || !strcmp(argv[2],"--runtime-pci-capture-fault") || !strcmp(argv[2],"--runtime-faults"))));setvbuf(stdout,nullptr,_IOLBF,0);
 (void)&Input;(void)&Prepare;(void)&Upload;(void)&Close;(void)&PreparePanel;(void)&Run;(void)&Job;(void)&ClosePanel;(void)&Finish;(void)&AcquireQ38DeltaLock;
#ifdef PIONEER_Q38_TOKEN_NATIVE_HELD_FIXTURE
 assert(clockContract || preparationEndpoints || preparationFaults || terminalFaults || peerBootstrap || embedding || (argc==3 && (!strcmp(argv[2],"--held") || !strcmp(argv[2],"--held-faults") || !strcmp(argv[2],"--runtime") || !strcmp(argv[2],"--runtime-pci-capture-fault") || !strcmp(argv[2],"--runtime-faults")
  || !strcmp(argv[2],"--bootstrap") || !strcmp(argv[2],"--bootstrap-faults")
  || !strcmp(argv[2],"--execution") || !strcmp(argv[2],"--execution-faults")
  || !strcmp(argv[2],"--execution-traffic") || !strcmp(argv[2],"--execution-traffic-faults"))));
#else
 assert(argc==2 || strncmp(argv[2],"--held",6));
#endif
 const bool heldFaults=argc==3 && !strcmp(argv[2],"--held-faults");
 const bool bootstrap=peerBootstrap || (argc==3 && !strncmp(argv[2],"--bootstrap",11)),bootstrapFaults=bootstrap && !strcmp(argv[2],"--bootstrap-faults");
 const bool execution=terminalFaults || embedding || (argc==3 && !strncmp(argv[2],"--execution",11)),executionFaults=execution && !strcmp(argv[2],"--execution-faults");
 const bool trafficFaults=argc==3 && !strcmp(argv[2],"--execution-traffic-faults");
 const bool runtime=argc==3 && !strncmp(argv[2],"--runtime",9),runtimeFaults=runtime && !strcmp(argv[2],"--runtime-faults");
 const bool faults=!clockContract && !preparationEndpoints && !preparationFaults && !initializationFifoFaults && argc==3 && strncmp(argv[2],"--held",6) && !runtime && !bootstrap && !execution,late=faults && !strcmp(argv[2],"--late-faults");
#ifdef PIONEER_Q38_TOKEN_NATIVE_EMBEDDING_FIXTURE
 assert(embedding);Q38EmbeddingRawTestBridge original;
 puts("START authenticate SAME-open original27B GGUF RAW bytes BEFORE native fixture construction;NO CPU embedding/model products");
 assert(original.Open(argv[3]) && original.Ready());
#endif
#ifdef PIONEER_Q38_TOKEN_NATIVE_PEER_FIXTURE
 assert(peerBootstrap || firstMatrix);tokenPeerOriginalImage=argv[peerBootstrap?3:4];tokenPeerGlobalImage=argv[peerBootstrap?4:5];
#endif
#ifdef PIONEER_Q38_TOKEN_NATIVE_FULL_LAYER_FIXTURE
#ifdef PIONEER_Q38_TOKEN_NATIVE_FULL_FLOW_FIXTURE
 assert(fullFlow);tokenNativeTraceDirectory=argv[6];
#else
 assert(fullLayer);tokenNativePrefixFile=argv[6];
#endif
#endif
 for(unsigned fault=0;fault<(preparationFaults?4U:initializationFifoFaults?6U:terminalFaults?5U:trafficFaults?2U:executionFaults?4U:bootstrapFaults?2U:runtimeFaults?3U:heldFaults?3U:late?2U:faults?4U:1U);fault++){
  Fixture f(argv[1]);auto& c=*f.c;auto& io=f.io;assert(f.Bind());auto* state=c.q38TokenNative;
#ifdef PIONEER_Q38_TOKEN_NATIVE_HELD_FIXTURE
#ifdef PIONEER_Q38_TOKEN_NATIVE_CLOCK_FIXTURE
#ifdef PIONEER_Q38_TOKEN_NATIVE_PCI_FIXTURE
  io.nativePciFault=!strcmp(argv[2],"--execution-pci-generation-fault")?1:!strcmp(argv[2],"--execution-pci-readback-fault")?2:0;
#endif
  if(clockContract){
   PioneerQ38TokenClockResult result;const auto pristine=result;auto* words=reinterpret_cast<uint32*>(&result);
   const unsigned before=io.touches,raw=c.q38ClockIO.rawEvents;
   assert(c.q38ClockIO.tokenClockGuard.Pristine() && c.q38ClockIO.notifications==3 && c.q38ClockIO.TokenClockViewsRetained()
    && !EndQ38TokenNativeClock(c,result) && !Q38TokenNativeClockScope(&c));
   PioneerQ38TokenHardwareSealResult hardware;const auto pristineHardware=hardware;auto* hardwareWords=reinterpret_cast<uint32*>(&hardware);
   assert(!SealQ38TokenNativeHardware(c,hardware) && !io.TokenFinalOwnershipScope());
   PioneerQ38TokenViewsResult views;const auto pristineViews=views;auto* viewWords=reinterpret_cast<uint32*>(&views);
   assert(!CloseQ38TokenNativeViews(c,views) && !io.TokenViewsScope() && io.nativeViewDeletes.empty());
   const unsigned pciEvents=io.nativePci.events;
   PioneerQ38TokenPciResult pci;const auto pristinePci=pci;auto* pciWords=reinterpret_cast<uint32*>(&pci);
   assert(!CloseQ38TokenNativePci(c,pci) && !io.TokenPciScope());
   {
    PioneerQ38TokenCallScope scope(&f.cookie);assert(scope.Context()==&c);
    for(unsigned i=0;i<16;i++){words[i]^=1;assert(!EndQ38TokenNativeClock(c,result));result=pristine;}
    assert(!EndQ38TokenNativeClock(c,result) && !state->ClockRestored());
    for(unsigned i=0;i<16;i++){hardwareWords[i]^=1;assert(!SealQ38TokenNativeHardware(c,hardware));hardware=pristineHardware;}
    assert(!SealQ38TokenNativeHardware(c,hardware) && !io.TokenFinalOwnershipScope() && !io.TokenHardwareSealScope());
    for(unsigned i=0;i<16;i++){viewWords[i]^=1;assert(!CloseQ38TokenNativeViews(c,views));views=pristineViews;}
    assert(!CloseQ38TokenNativeViews(c,views) && !io.TokenViewsScope() && io.nativeViewDeletes.empty());
    for(unsigned i=0;i<16;i++){pciWords[i]^=1;assert(!CloseQ38TokenNativePci(c,pci));pci=pristinePci;}
    assert(!CloseQ38TokenNativePci(c,pci) && !io.TokenPciScope());
    uint32 value=0x3157a9c2;
    for(unsigned slot:{0U,1U,2U,UINT32_MAX})for(unsigned word:{0U,2U,3U,UINT32_MAX})
     assert(!io.TokenClockTlbReadChecked(slot,word,value) && !io.TokenHardwareSealTlbReadChecked(slot,word,value) && value==0x3157a9c2);
    for(bool root:{false,true})for(unsigned offset:{0U,4U,0x10U,0x34U,0x70U,0x100U,UINT32_MAX})for(unsigned width:{1U,2U,4U,8U})
     assert(!io.TokenPciReadChecked(root,offset,width,value) && value==0x3157a9c2 && !io.TokenPciWriteChecked(root,offset,width,0));
   }
   assert(io.touches==before && c.q38ClockIO.rawEvents==raw && io.nativePci.events==pciEvents && !c.q38ClockIO.deletes && !io.rootStarts
    && !memcmp(&result,&pristine,sizeof(result)) && c.q38Clock.RequiresExplicitEnd());
   tt_resident_session_result end={};bool clean=true;assert(!IncompleteEnd(c,end,clean) && !clean);f.Unknown();
   assert(io.touches==before && c.q38ClockIO.rawEvents==raw && !c.q38ClockIO.deletes);
   puts("PASS actual native PCI BEGIN+actual real clock BEGIN3 messages/17 each clock-END+hardware-seal+VM-close+PCI-close pending-or-non-pristine factories/32 mapping+112 PCI read-and-write refusals/output unchanged/ZERO following PCI-clock-model IO/instructions/VM deletes;NOT positive cleanup/compute/hardware/Work/TPS qualification");
   continue;
  }
#endif
  if(preparationEndpoints){
   unsigned checks=0;const auto starts=io.rootStarts,stops=io.rootStops,holds=io.rootHolds;
   assert(!io.maps && io.ExpectedMappedBytes(0)==(PioneerQ38TokenL1::Precise?131072U:8192U)
    && io.ExpectedMappedBytes(1)==131072 && io.ExpectedMappedBytes(2)==57344 && io.ExpectedMappedBytes(3)==868352
    && !io.ExpectedMappedBytes(4));
   for(unsigned address:{0x20000U,0x21ffcU,0x22000U,0x30000U,0x3fffcU,0x40000U,0xac000U,0x17fffcU})
    assert(!io.Mapped(address)); // no alias access is accepted before an actual successful TEST OS map
   for(unsigned worker=0;worker<120;worker++){
    auto& core=io.Core(worker);
    assert(core.nativePhysicalWindow && core.nativeWindowX==PioneerWorkerX(worker) && core.nativeWindowY==PioneerWorkerY(worker));
    assert(io.SelectRuntimeCore(worker));
    for(unsigned slot=0;slot<2;slot++){
     const uint32 base=slot?0xffa00000U:0U;
     for(unsigned word=0;word<3;word++)assert(io.DramTlbRead(slot,word)==BlackholeDramFixedTlbWord(base,PioneerWorkerX(worker),PioneerWorkerY(worker),false,word));
     core.Window(slot,base,1,2,false);checks+=3;
    }
    assert(io.TokenFifoStatusWord(0x64)==((core.config&0x10000U)?0x10101010U:0U) && !io.TokenFifoStatusWord(0x68));checks+=2;
   }
   assert(io.SelectRuntimeCore(0) && io.rootStarts==starts && !starts && io.rootStops==stops && io.rootHolds==holds);
   const unsigned rootBefore=io.touches;unsigned rootRefusals=0;
   auto refuseRoot=[&](){
    uint32 value=0xa5a55a5a;assert(!io.TokenRootStoreAccessScope());rootRefusals++;
    for(unsigned address:{0U,4U,36U,40U,0x40U,0x64U,0x100U,0x174U,0x204U,UINT32_MAX}){
     assert(!io.TokenRootNiuReadChecked(address,value) && value==0xa5a55a5a && !io.TokenRootNiuWrite(address,0));rootRefusals+=2;
    }
    for(unsigned word:{0U,2047U,2048U,UINT32_MAX}){
     assert(!io.TokenRootScratchReadChecked(word,value) && value==0xa5a55a5a && !io.TokenRootScratchWrite(word,0));rootRefusals+=2;
    }
    assert(io.touches==rootBefore);
   };
   refuseRoot();{PioneerQ38TokenCallScope scope(&f.cookie);assert(scope.Context()==&c);refuseRoot();}
   printf("PASS ROOT-only restoration endpoints/%u wrong-phase+outside-FD refusals/output unchanged/ZERO IO or zero-program/model instructions;NOT positive restoration/END/hardware/Work qualification\n",rootRefusals);
   unsigned sharedRefusals=0;
   auto refuseShared=[&](){
    uint32 value=0x59a731c2;assert(!io.TokenSharedTlbScope());sharedRefusals++;
    for(unsigned slot:{0U,1U,2U,UINT32_MAX})for(unsigned word:{0U,2U,3U,UINT32_MAX}){
     assert(!io.TokenSharedTlbReadChecked(slot,word,value) && value==0x59a731c2 && !io.TokenSharedTlbWrite(slot,word,0));sharedRefusals+=2;
    }
    for(unsigned word:{0U,9U,10U,UINT32_MAX}){
     assert(!io.TokenSharedRootCommandReadChecked(word,value) && value==0x59a731c2);sharedRefusals++;
    }
    assert(io.touches==rootBefore);
   };
   refuseShared();{PioneerQ38TokenCallScope scope(&f.cookie);assert(scope.Context()==&c);refuseShared();}
   printf("PASS shared200/201 restoration endpoints/%u wrong-phase+outside-FD refusals/output unchanged/ZERO IO or instructions;NOT positive mapping restoration/END/hardware/Work qualification\n",sharedRefusals);
   tt_resident_session_result end={};bool clean=true;const unsigned before=io.touches;
   assert(!IncompleteEnd(c,end,clean) && !clean && io.touches==before);f.Unknown();
   printf("PASS actual HELD TEST constructor/ALL120 physical endpoint expectations BEFORE any preparation/history/actual selected6 TLB words+FIFO observations/%u checks/ZERO zero-program or model instructions;NOT native preparation/model/STOP/END/hardware/Work/TPS qualification\n",checks);
   printf("PASS HELD TEST OS expected first-aperture%u bytes/4 lengths+invalid-index/8 unmapped address refusals;DATA endpoint preflight ONLY,NOT positive actual-map/native factory or runtime qualification\n",io.ExpectedMappedBytes(0));
   continue;
  }
#endif
#ifdef PIONEER_Q38_TOKEN_NATIVE_EMBEDDING_FIXTURE
  io.executionCpu->original=&original;io.embeddingFault=!strcmp(argv[2],"--execution-embedding-source-loss")?1U:
   !strcmp(argv[2],"--execution-embedding-readback-loss")?2U:0U;
#endif
  PioneerQ38StateZeroResult g;PioneerQ38ConvHistoryZeroResult h;PioneerQ38ProjectionZeroResult p;
  assert(!state->InitializationsClosed(c.banks,f.proof) && !io.StateZeroBeforeCohort() && !io.ConvHistoryZeroBeforeCohort() && !io.ProjectionZeroBeforeCohort());
  unsigned touches=io.touches;assert(!f.History(h) && !f.Projection(p) && io.touches==touches);
  for(unsigned kind:{0U,1U,2U,3U,UINT32_MAX})for(bool overflow:{false,true}){
   uint32 value=0x5a7183c9;
   assert(!io.TokenInitializationFifoEmpty(kind) && !io.TokenInitializationFifoReadChecked(kind,overflow,value)
    && value==0x5a7183c9 && io.touches==touches);
   PioneerQ38TokenCallScope scope(&f.cookie);assert(scope.Context()==&c);
   assert(!io.TokenInitializationFifoEmpty(kind) && !io.TokenInitializationFifoReadChecked(kind,overflow,value)
    && value==0x5a7183c9 && io.touches==touches);
  } // wrong FD or Bound(not initializer) phase never reads registers or changes output
  auto learned=f.learned;const auto panel=io.view;const std::vector<uint32> scratch(io.l1,io.l1+8192);const auto entry=io.entry;
  {PioneerQ38TokenCallScope scope(&f.cookie);assert(scope.Context()==&c);
   auto refuse=[&](auto& field,auto bad){auto saved=field;field=bad;
    assert(!InitializeQ38TokenRecurrent(c,g) && !InitializeQ38TokenHistory(c,h) && !InitializeQ38TokenProjection(c,p)
     && io.touches==touches && !state->Unsafe() && !state->InitializationsClosed(c.banks,f.proof));field=saved;
   };
   refuse(c.q38GdnCallThread,-1);refuse(currentThread,99);refuse(currentTeam,99);refuse(c.bulk.dirty,true);
   refuse(sReloadGeneration,sReloadGeneration+1);refuse(io.fleetEnabled,true);refuse(c.q38DispatchRoutes,true);
   refuse(c.hashes,nullptr);refuse(c.bulkHost.pending,true);
#ifndef PIONEER_Q38_TOKEN_NATIVE_CLOCK_FIXTURE
   refuse(c.q38Clock.value.busy_mhz,1401U);
#endif
  }
  if(initializationFifoFaults){
   const unsigned kind=fault/2;
   if(kind)assert(f.Recurrent(g) && state->RecurrentInitialized(c.banks,f.proof));
   if(kind==2)assert(f.History(h) && state->HistoryInitialized(c.banks,f.proof));
   const unsigned starts=io.rootStarts,stops=io.rootStops,holds=io.rootHolds;
   const auto originalPanel=io.view;const std::vector<uint32> originalScratch(io.l1,io.l1+8192);
   io.initializationFifoFault=1+fault%2;
   const bool okay=kind==0?f.Recurrent(g):kind==1?f.History(h):f.Projection(p);
   assert(!okay && io.initializationFifoObserved && io.touches==io.initializationFifoObserved
    && io.initializationFifoReads==1+fault%2 && io.rootStarts==starts && io.rootStops==stops && io.rootHolds==holds
    && io.view==originalPanel && std::equal(originalScratch.begin(),originalScratch.end(),io.l1));
   f.Unknown();assert(f.learned==learned);
   printf("PASS native initializer FIFO fault kind%u/%s/actual preceding initializers CLOSED/first bad ordered observation stops ALL later IO/counters/reset/retry/release;retained NO initialization/STOP/END/physical/Work/TPS grant\n",kind,fault%2?"overflow":"queued");
   continue;
  }
  if(late){
   assert(f.Recurrent(g) && state->RecurrentInitialized(c.banks,f.proof));
   if(!fault){io.failRun=io.rootStarts+1;assert(!f.History(h));}
   else{assert(f.History(h) && state->HistoryInitialized(c.banks,f.proof));io.failRun=io.rootStarts+1;assert(!f.Projection(p));}
   f.Unknown();assert(f.learned==learned);
   printf("PASS token native LATE init fault%u/actual prior controllers fully CLOSED before next-phase instruction-summary failure/ALLowners retained/no publication/retry/cleanup/following IO;SIMULATED endpoints\n",fault);continue;
  }
  if(faults){
   if(!fault)io.failRun=1;if(fault==1)io.nonrootFault=true;if(fault==2)io.loseTeamRun=1;if(fault==3)io.loseGenerationRun=1;
   assert(!f.Recurrent(g));f.Unknown();assert(f.learned==learned);
   if(fault==3)sReloadGeneration--;
   printf("PASS token native init fault%u/production retention/ALLowners retained/no publication/retry/cleanup/following IO;SIMULATED endpoints\n",fault);continue;
  }
  assert(f.Recurrent(g) && !state->Unsafe() && state->RecurrentInitialized(c.banks,f.proof)
   && !state->HistoryInitialized(c.banks,f.proof) && !state->InitializationsClosed(c.banks,f.proof)
   && io.observedGdnPages==73728 && io.rootStarts==672 && io.rootHolds==672 && io.rootStops==672);
  assert(io.view==panel && std::equal(scratch.begin(),scratch.end(),io.l1) && io.entry==entry);touches=io.touches;
  assert(!f.Recurrent(g) && io.touches==touches);
  for(auto value:io.recurrentWords)assert(!value);for(auto count:io.recurrentTouched)assert(count==1);
  assert(f.History(h) && state->HistoryInitialized(c.banks,f.proof) && !state->InitializationsClosed(c.banks,f.proof)
   && io.observedHistoryPages==1680 && io.rootStarts==1008 && io.rootHolds==1008 && io.rootStops==1008);
  assert(io.view==panel && std::equal(scratch.begin(),scratch.end(),io.l1) && io.entry==entry);touches=io.touches;
  assert(!f.History(h) && io.touches==touches);
  for(auto value:io.historyWords)assert(!value);for(auto count:io.historyTouched)assert(count==1);
  assert(f.Projection(p) && state->InitializationsClosed(c.banks,f.proof) && io.observedProjectionPages==512
   && io.rootStarts==1010 && io.rootHolds==1010 && io.rootStops==1010 && io.view==panel && std::equal(scratch.begin(),scratch.end(),io.l1) && io.entry==entry);
  for(unsigned i=0;i<512*1024;i++)assert(!io.workspaceWords[i]);for(auto count:io.workspaceTouched)assert(count==1);
  touches=io.touches;assert(!f.Recurrent(g) && !f.History(h) && !f.Projection(p) && io.touches==touches);
  assert(f.learned==learned && !c.bulk.dirty && !io.pciWrites && !io.HostReadMock::bmeWrites
   && !io.wideReleased && !io.PageMock::releases && !io.OutputBufferMock::releases && !state->Closed());
#ifdef PIONEER_Q38_TOKEN_NATIVE_HELD_FIXTURE
  if(preparationFaults){CheckNativePreparationFault(f,fault+(preparationAccessFaults?5U:1U));continue;}
#ifdef PIONEER_Q38_TOKEN_NATIVE_RUNTIME_FIXTURE
  io.runtimeWorkspace=io.workspaceAddress;io.runtimeFault=runtimeFaults?fault+1:0;
  io.pciCaptureFault=argc==3 && !strcmp(argv[2],"--runtime-pci-capture-fault");
#endif
#ifdef PIONEER_Q38_TOKEN_NATIVE_BOOTSTRAP_FIXTURE
  io.bootstrapFault=bootstrapFaults?fault+1:0;
#endif
#ifdef PIONEER_Q38_TOKEN_NATIVE_EXECUTION_FIXTURE
  io.executionFault=executionFaults?fault+1:0;
  io.terminalFault=terminalFaults?fault+1:0;
#ifdef PIONEER_Q38_TOKEN_NATIVE_RESTORE_EMPTY_FIXTURE
  io.restoreFault=!strcmp(argv[2],"--execution-restore-empty-fault");
#endif
#ifdef PIONEER_Q38_TOKEN_NATIVE_RESTORE_DISPATCH_FIXTURE
  io.dispatchRestoreFault=!strcmp(argv[2],"--execution-restore-dispatch-fault");
#endif
#ifdef PIONEER_Q38_TOKEN_NATIVE_RESTORE_PEERS_FIXTURE
  io.peerRestoreFault=!strcmp(argv[2],"--execution-restore-peers-fault")?1:
   !strcmp(argv[2],"--execution-restore-peers-map-fault")?2:!strcmp(argv[2],"--execution-restore-peers-config-fault")?3:
   !strcmp(argv[2],"--execution-restore-peers-scratch-fault")?4:!strcmp(argv[2],"--execution-restore-peers-fifo-fault")?5:
   !strcmp(argv[2],"--execution-restore-peers-overflow-fault")?6:0;
#endif
#ifdef PIONEER_Q38_TOKEN_NATIVE_RESTORE_ROOT_STORE_FIXTURE
  io.rootStoreFault=!strcmp(argv[2],"--execution-restore-root-store-fault")?1:
   !strcmp(argv[2],"--execution-restore-root-store-map-fault")?2:!strcmp(argv[2],"--execution-restore-root-store-config-fault")?3:
   !strcmp(argv[2],"--execution-restore-root-store-scratch-fault")?4:!strcmp(argv[2],"--execution-restore-root-store-fifo-fault")?5:
   !strcmp(argv[2],"--execution-restore-root-store-overflow-fault")?6:0;
#endif
#ifdef PIONEER_Q38_TOKEN_NATIVE_RESTORE_SHARED_TLB_FIXTURE
  io.sharedTlbFault=!strcmp(argv[2],"--execution-restore-shared-tlb-fault")?1:
   !strcmp(argv[2],"--execution-restore-shared-tlb-readback-fault")?2:!strcmp(argv[2],"--execution-restore-shared-tlb-verify-fault")?3:0;
#endif
#ifdef PIONEER_Q38_TOKEN_NATIVE_CLOCK_FIXTURE
  io.nativeViewFault=!strcmp(argv[2],"--execution-views-delete-fault")?1:!strcmp(argv[2],"--execution-views-generation-fault")?2:0;
  c.tokenClockFault=!strcmp(argv[2],"--execution-clock-end-generation-fault")?1:
   !strcmp(argv[2],"--execution-clock-end-response-fault")?2:!strcmp(argv[2],"--execution-clock-end-unmap-fault")?3:0;
#endif
#ifdef PIONEER_Q38_TOKEN_NATIVE_HARDWARE_SEAL_FIXTURE
  io.hardwareSealFault=!strcmp(argv[2],"--execution-hardware-seal-tlb-fault");
#endif
#endif
#ifdef PIONEER_Q38_TOKEN_NATIVE_TRAFFIC_FIXTURE
  io.trafficFault=trafficFaults?fault+1:0;
#endif
  CheckNativeHeld(f,heldFaults?fault+1:0);
#ifdef PIONEER_Q38_TOKEN_NATIVE_EXECUTION_FIXTURE
  if(!state->Unsafe())CheckNativeExecution(f);
#endif
  touches=io.touches;
#endif
  tt_resident_session_result end={};bool clean=true;
  assert(!IncompleteEnd(c,end,clean) && !clean && io.touches==touches);f.Unknown();
  puts("PASS SAME full851 native owner/actual zero RV32 672GDN regions73728pages BOTHslots+336history1680pages+2workspace512pages/1010actualRUN+STOP+HOLD/full coverage+private clean CLOSE publications/code+scratch restored/learned unchanged/no BME;SIMULATED endpoints,NOT KV/metadata/model execution/physical/TPS/END permission");
 }
 assert(!lockHeld && lockAcquires==lockReleases);
}
