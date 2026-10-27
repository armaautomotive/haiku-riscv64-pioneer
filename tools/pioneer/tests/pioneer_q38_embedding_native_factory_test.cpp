/* SPDX-License-Identifier: MIT */
// ACTUAL native factory/scope/mapping/retention/END + REAL owned state.
// Both zero and embedding execute their ACTUAL emitted RISC instructions.
// ONLY kernel/PCI/TLB/GDDR endpoints are simulated. No ready-state facade.
#define PIONEER_Q38_PROJECTION_ZERO_FIXTURE_ONLY
#include "pioneer_q38_projection_zero_controller_test.cpp"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38EmbeddingNativeState.h"
#ifdef PIONEER_Q38_MIXED_LIVE_NATIVE_CONTEXT
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38MixedLiveNativeState.h"
#endif
#ifdef PIONEER_Q38_RECURRENT_BLOCK_NATIVE_CONTEXT
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38RecurrentBlockNativeState.h"
#endif
#ifdef PIONEER_Q38_RESIDENT_OUTPUT_OWNER
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38ResidentOutputNativeState.h"
#endif
#ifdef PIONEER_Q38_RESIDENT_LAYER_NATIVE_CONTEXT
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38ResidentLayerNativeState.h"
#endif
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38NonRootHeld.h"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38DispatchBootstrap.h"
#if defined(PIONEER_Q38_RESIDENT_OUTPUT_VALIDATION_NATIVE) || defined(PIONEER_Q38_RESIDENT_LAYER_VALIDATION_NATIVE) || defined(PIONEER_Q38_MIXED_LIVE_NATIVE_CONTEXT) || defined(PIONEER_Q38_RECURRENT_BLOCK_NATIVE_CONTEXT)
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38ResidentOutputLearned.h"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38GraphMutableBanks.h"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38FfnZeroController.h"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38FfnPersistentGraphOwner.h"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38FfnGraphCodeOwner.h"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38DeltaProbe.h"
#endif
#include "../../../src/add-ons/kernel/drivers/misc/BlackholeArcClockGuard.h"
#include "pioneer_q38_embedding_execution_bridge.h"
#ifdef PIONEER_Q38_MIXED_LIVE_NATIVE_FACTORY_FIXTURE
#include "pioneer_q38_mixed_live_native_transport.h"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38DispatchClose.h"
#endif
#ifdef PIONEER_Q38_RESIDENT_OUTPUT_OWNER
#include "pioneer_q38_projection_native_transport.h"
#endif
#ifdef PIONEER_Q38_RESIDENT_DECODER_NATIVE
#include "pioneer_q38_decoder_conv_bridge.h"
#endif
#ifdef PIONEER_Q38_LEARNED_RMS_NATIVE
#include "pioneer_q38_learned_rms_execution_bridge.h"
#endif
#include <array>
#include <new>
#ifdef PIONEER_Q38_MIXED_LIVE_END_FIXTURE
#include "pioneer_q38_mixed_live_native_clock.h"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38MixedLiveFinalPciState.h" // pure necessary CPU predicate,NOT final release authority
#endif
#ifdef PIONEER_Q38_PROJECTION_NATIVE_FACTORY_FIXTURE
#include "pioneer_q38_projection_native_transport.h"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38DispatchClose.h"
#endif
using uint8=uint8_t;using uint32=uint32_t;using uint64=uint64_t;using area_id=int;
static constexpr int B_OK=0,B_ANY_KERNEL_ADDRESS=1,B_KERNEL_READ_AREA=2,B_KERNEL_WRITE_AREA=4,B_UNCACHED_MEMORY=8;
static constexpr uint64 Bar=UINT64_C(0x900000000);
struct PioneerStoreContext;struct PioneerSessionIO;
static PioneerSessionIO* activeIO=nullptr;
static uint32 PioneerRead32(const volatile uint32*);
static void PioneerWrite32(volatile uint32*,uint32);
static area_id map_physical_memory(const char*,uint64,unsigned,int,unsigned,void**);
#ifdef PIONEER_Q38_MIXED_LIVE_FINAL_RELEASE
#include "PioneerQ38MixedFinalNativeLayout.h"
using int32=int32_t;using status_t=int;
static constexpr int B_BAD_VALUE=-2;
#ifdef PIONEER_Q38_MIXED_LIVE_FRESH_FINAL_TEST
using thread_id=int;
static constexpr int B_BUSY=-3,B_DEV_NOT_READY=-4,B_NOT_ALLOWED=-5;
#endif
#endif
struct PioneerHostPageIO{bool quarantined=false;
#ifdef PIONEER_Q38_MIXED_LIVE_FINAL_RELEASE
 area_id area=9001;volatile uint32* page=&finalAlias;bool releaseAttempted=false;
#include "native_release_host.inc"
#endif
};
struct PioneerSessionIO:ProjectionZeroIO,PioneerHostPageIO
#ifdef PIONEER_Q38_MIXED_LIVE_FINAL_RELEASE
 ,MixedFinalNativeLayout
#endif
#ifdef PIONEER_Q38_RESIDENT_LAYER_NATIVE_CONTEXT
 ,ResidentLayerNativeStateBytes
#elif defined(PIONEER_Q38_RESIDENT_OUTPUT_OWNER)
 ,ResidentOutputNativeStateBytes
#endif
#ifdef PIONEER_Q38_RESIDENT_DECODER_NATIVE
 ,DecoderHistoryBytes
#endif
 {
#ifdef PIONEER_Q38_RESIDENT_DECODER_NATIVE
 bool DecoderCallScopeMatches();bool ConvHistoryZeroBeforeCohort();bool ConvHistoryZeroOtherCoresHeld();
 bool ResidentDecoderScope(const PioneerQ38ResidentDecoderOwner&,const PioneerQ38ProjectionGraphOwner&,const PioneerQ38ConvHistoryBanks&);
#ifdef PIONEER_Q38_GDN_STATE_INIT_NATIVE
 bool RecurrentZeroSourceScope();bool StateZeroBeforeCohort();bool StateZeroOtherCoresHeld();
#ifdef PIONEER_Q38_GDN_RECURRENT_NATIVE
 bool ResidentGdnRecurrentScope(const PioneerQ38ResidentDecoderOwner&,const PioneerQ38ProjectionGraphOwner&,const PioneerQ38GdnBanks&);
 PioneerQ38GdnBankExtent recurrentExtents[672]={};unsigned recurrentFirst[672]={};
 bool GdnBytes(unsigned bank,uint32 at,uint32* out,const uint32* input,unsigned words){
  if(!words || (at&3) || (!out && !input))return false;
  for(unsigned region=0;region<672;region++){const auto& e=recurrentExtents[region];
   if(e.bank!=bank || !e.pages || at<e.address || at-e.address>=e.bytes || words>(e.bytes-(at-e.address))/4)continue;
   unsigned first=recurrentFirst[region]*1024+(at-e.address)/4;
   for(unsigned i=0;i<words;i++){if(out)out[i]=recurrentWords[first+i];else recurrentWords[first+i]=input[i];}return true;
  }
  return false;
 }
 bool ReadGdnState(unsigned bank,uint32 at,uint32* out,unsigned words)override{return GdnBytes(bank,at,out,nullptr,words);}
 bool WriteGdnState(unsigned bank,uint32 at,const uint32* input,unsigned words)override{return GdnBytes(bank,at,nullptr,input,words);}
#endif
 std::vector<uint32> recurrentWords=std::vector<uint32>(73728*1024,0xa55aa55a);
 std::array<unsigned,73728> recurrentTouched={};unsigned recurrentPages=0,recurrentRuns=0,recurrentFailRun=0;
 bool recurrentCloseFault=false;
#endif
 std::vector<uint32> historyWords=std::vector<uint32>(1680*1024,0xa55aa55a);PioneerQ38ConvHistoryExtent historyExtents[336]={};
 bool ReadHistory(unsigned bank,uint32_t address,uint32_t* out,unsigned words)override{
  if(!out || !words || (address&3))return false;
  for(unsigned i=0;i<336;i++){const auto& e=historyExtents[i];
   if(e.bank==bank && address>=e.address && address-e.address<e.bytes && words<=(e.bytes-(address-e.address))/4){
    for(unsigned w=0;w<words;w++)out[w]=historyWords[i*5120+(address-e.address)/4+w];return true;
   }
  }
  return false;
 }
#endif
 PioneerStoreContext* q38MutableContext=nullptr;
#ifdef PIONEER_Q38_RECURRENT_BLOCK_NATIVE_CONTEXT
 // Binding-only endpoint fixture: NO recurrent mapping or execution.
 bool q38RecurrentBlockMemoryMapAttempted=false;
 bool RecurrentBlockCallScopeMatches();bool RecurrentBlockBaseScope();
 bool RecurrentBlockMemoryMappingStable(unsigned){assert(false && "binding-only fixture cannot map/close execution");return false;}
#ifdef PIONEER_Q38_RECURRENT_BLOCK_INITIALIZATION_FIXTURE
 bool RecurrentBlockInitializationScope(bool,bool);bool RecurrentBlockInitializationOtherCoresHeld(bool);
 bool StateZeroBeforeCohort();bool StateZeroOtherCoresHeld();bool ConvHistoryZeroBeforeCohort();bool ConvHistoryZeroOtherCoresHeld();
 std::vector<uint32> recurrentBlockGdnWords,recurrentBlockHistoryWords;
 std::vector<unsigned> recurrentBlockGdnTouched,recurrentBlockHistoryTouched;
 PioneerQ38GdnBankExtent recurrentBlockGdnExtents[672]={};PioneerQ38ConvHistoryExtent recurrentBlockHistoryExtents[336]={};
 unsigned recurrentBlockGdnFirst[672]={},recurrentBlockGdnPages=0,recurrentBlockGdnRuns=0,recurrentBlockHistoryRuns=0;
 unsigned recurrentBlockFailRun=0,recurrentBlockHistoryFailRun=0,recurrentBlockLoseTeamRun=0;
 bool recurrentBlockNonrootFault=false;
#endif
#endif
#ifdef PIONEER_Q38_MIXED_LIVE_NATIVE_CONTEXT
#ifdef PIONEER_Q38_MIXED_LIVE_NATIVE_FACTORY_FIXTURE
 MixedLiveNativeTransport* mixedLiveTransport=nullptr;
 bool UseMixedLiveIO()const;
 void ConfigureMixedLiveTestTransport();
 std::array<std::vector<uint32>,4> mixedLiveAliases;
 int mixedLiveFailMap=-1,mixedLiveNullMap=-1;unsigned mixedLiveMapFaultFence=0;
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38MixedLiveNativeMapping.inc"
 bool MapRuntime(){return runtimeMapped;}
 bool MapQ38Panel(){return UseMixedLiveIO()?mixedLiveTransport->MapPanel():ProjectionZeroIO::MapQ38Panel();}
 bool Q38PanelMappingStable(){return UseMixedLiveIO()?mixedLiveTransport->PanelStable():ProjectionZeroIO::Q38PanelMappingStable();}
 uint32* Q38PanelSavedWords(){return UseMixedLiveIO()?mixedLiveTransport->PanelSaved():ProjectionZeroIO::Q38PanelSavedWords();}
 uint32* HostReadBackup(unsigned n){return UseMixedLiveIO()?mixedLiveTransport->Backup(n):ProjectionZeroIO::HostReadBackup(n);}
 uint32 ComputeNocRead(uint32 a){assert(UseMixedLiveIO());return mixedLiveTransport->Read32(MixedLiveNativeTransport::ComputeNoc,a);}
 uint32 HostPciRead(unsigned p,uint16_t a,uint8 n){
#ifdef PIONEER_Q38_MIXED_LIVE_FINAL_RELEASE
  if(q38MixedLiveFinalUnmapped)return p?RootRead(a,n):CardRead(a,n);
#endif
  uint32 value=UseMixedLiveIO()?mixedLiveTransport->Read32(MixedLiveNativeTransport::Pci,p,a,n):ProjectionZeroIO::HostPciRead(p,a,n);
#ifdef PIONEER_Q38_MIXED_LIVE_FRESH_FINAL_TEST
  if(finalAuditReturned){finalQuietReads++;if(finalQuietFailure && a==4)return 0x406;}
#endif
  return value;}
 uint32 Q38PanelRead(uint32 a){return UseMixedLiveIO()?mixedLiveTransport->Read32(MixedLiveNativeTransport::Panel,a):ProjectionZeroIO::Q38PanelRead(a);}
 void Q38PanelWrite(uint32 a,uint32 v){if(UseMixedLiveIO())mixedLiveTransport->Write32(MixedLiveNativeTransport::SetPanel,a,v);else ProjectionZeroIO::Q38PanelWrite(a,v);}
 uint32 RuntimeCodeRead(unsigned i){return UseMixedLiveIO()?mixedLiveTransport->Read32(MixedLiveNativeTransport::Code,i):ProjectionZeroIO::RuntimeCodeRead(i);}
 void RuntimeCodeWrite(unsigned i,uint32 v){if(UseMixedLiveIO())mixedLiveTransport->Write32(MixedLiveNativeTransport::SetCode,i,v);else ProjectionZeroIO::RuntimeCodeWrite(i,v);}
 uint32 RuntimeControlRead(unsigned i){return UseMixedLiveIO()?mixedLiveTransport->Read32(MixedLiveNativeTransport::Control,i):ProjectionZeroIO::RuntimeControlRead(i);}
 uint32 RuntimeStatusRead(unsigned i){return UseMixedLiveIO()?mixedLiveTransport->Read32(MixedLiveNativeTransport::Status,i):ProjectionZeroIO::RuntimeStatusRead(i);}
 void RuntimeStatusWrite(unsigned i,uint32 v){if(UseMixedLiveIO())mixedLiveTransport->Write32(MixedLiveNativeTransport::SetStatus,i,v);else ProjectionZeroIO::RuntimeStatusWrite(i,v);}
 void Pause(){if(UseMixedLiveIO())mixedLiveTransport->Pause();else ProjectionZeroIO::Pause();}
 uint64 Q38CohortNow(){return UseMixedLiveIO()?mixedLiveTransport->Now():ProjectionZeroIO::Q38CohortNow();}
 uint32 ResidentNocRead(uint32 a){
  uint32 value=UseMixedLiveIO()?mixedLiveTransport->Read32(MixedLiveNativeTransport::Noc,a):ProjectionZeroIO::ResidentNocRead(a);
#ifdef PIONEER_Q38_MIXED_LIVE_FRESH_FINAL_TEST
  if(finalAuditReturned && finalIdleFailure && a==0x178)return value|1;
#endif
  return value;}
 void ResidentNocWrite(uint32 a,uint32 v){if(UseMixedLiveIO())mixedLiveTransport->Write32(MixedLiveNativeTransport::SetNoc,a,v);else ProjectionZeroIO::ResidentNocWrite(a,v);}
#else
 bool q38MixedLiveMemoryMapAttempted=false;
 bool MixedLiveCallScopeMatches();bool MixedLiveBaseScope();
 bool MixedLiveMemoryMappingStable(unsigned){assert(false && "binding-only fixture never admits mapping/closed execution");return false;}
#endif
#endif
#ifdef PIONEER_Q38_RESIDENT_LAYER_NATIVE_CONTEXT
 bool ResidentLayerCallScopeMatches();bool ResidentLayerBindSourceScope();
 bool ResidentLayerInitializationBaseScope();bool ResidentLayerInitializationSourceScope(bool,bool);
 bool ResidentLayerInitializationOtherCoresHeld(bool);bool ResidentLayerPrefixInitializationScope();
 bool ResidentLayerSourceScope(bool);bool ResidentLayerMappingSourceScope();void ResidentLayerRetainHostOwners();
 bool ResidentLayerMemoryScope(const PioneerQ38ResidentLayerMemory&,const PioneerQ38CanonicalModelPlan&,const PioneerQ38GdnIdentity&,const uint8_t*,unsigned);
 bool ResidentLayerExecutionScope(const PioneerQ38ResidentLayerOwner&,const PioneerQ38ProjectionGraphOwner&,const PioneerQ38ResidentLayerMemory&,
  const PioneerQ38ProjectionMutableBanks&,const PioneerQ38CanonicalModelPlan&,const PioneerQ38GdnIdentity&,const uint8_t*,unsigned);
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38ResidentLayerNativeMapping.inc"
 std::array<uint32,10240> layerPrefixAlias={};std::vector<uint32_t> layerSuffixAlias;
 int layerFailMap=-1,layerNullMap=-1;
#endif
#ifdef PIONEER_Q38_RESIDENT_OUTPUT_OWNER
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
 bool StateZeroBeforeCohort(){
#ifdef PIONEER_Q38_RESIDENT_LAYER_NATIVE_CONTEXT
  if(q38MutableContext && ResidentLayerInitializationSourceScope(false,false))return true;
#endif
  return ResidentOutputInitializationSourceScope(false,false);
 }
 bool StateZeroOtherCoresHeld(){
#ifdef PIONEER_Q38_RESIDENT_LAYER_NATIVE_CONTEXT
  if(ResidentLayerInitializationSourceScope(false,false))return ResidentLayerInitializationOtherCoresHeld(false);
#endif
  return ResidentOutputInitializationOtherCoresHeld(false);
 }
 std::vector<uint32_t> outputGdnWords,outputHistoryWords;
 std::vector<unsigned> outputGdnTouched,outputHistoryTouched;
 PioneerQ38GdnBankExtent outputGdnExtents[672]={};PioneerQ38ConvHistoryExtent outputHistoryExtents[336]={};
 unsigned outputGdnFirst[672]={},outputGdnPages=0,outputGdnRuns=0,outputHistoryRuns=0;
 unsigned outputGdnFailRun=0,outputHistoryFailRun=0;
 bool outputGdnCloseFault=false,outputHistoryCloseFault=false;
 bool outputNonrootFault=false;unsigned outputLoseTeamRun=0;
 bool ReadState(unsigned,uint32_t,uint32_t*,unsigned)override;
 bool WriteState(unsigned,uint32_t,const uint32_t*,unsigned)override;
 std::array<uint32,10240> outputPrefixAlias={};std::vector<uint32_t> outputSuffixAlias;
 int outputFailMap=-1,outputNullMap=-1;
 unsigned outputBackwardsAfterStarts=UINT32_MAX;
#ifndef PIONEER_Q38_PROJECTION_NATIVE_FACTORY_FIXTURE
 void RuntimeCodeWrite(unsigned,uint32);
 uint64_t Q38CohortNow(){
  return outputBackwardsAfterStarts!=UINT32_MAX && rootStarts>outputBackwardsAfterStarts?0:ProjectionZeroIO::Q38CohortNow();
 }
#endif
#endif
 unsigned runtimeCore=0;bool fleetEnabled=false,runtimeMapped=true,q38SpanAllowed=true;
#if defined(PIONEER_Q38_MIXED_LIVE_END_FIXTURE) && !defined(PIONEER_Q38_MIXED_LIVE_FINAL_RELEASE)
 bool q38MixedLiveFinalUnmapAttempted=false; // simulated VM transaction endpoint DATA,not a closed-owner setter
#endif
 std::array<uint32,10240> prefix;
 volatile uint32* runtimeEntry=&entry;volatile uint32* scratchData=destination;unsigned payloadBytes=8192;
 struct{uint64 bar0_host_start=Bar;}profile;
 area_id q38EmbeddingMemoryArea=-1;volatile uint32* q38EmbeddingMemoryView=nullptr;
 bool q38EmbeddingMemoryMapAttempted=false;uint64 q38EmbeddingMemoryBar=0;
#ifdef PIONEER_Q38_PROJECTION_EXECUTION_OWNER
 area_id q38ProjectionMemoryArea[3]={-1,-1,-1};volatile uint32* q38ProjectionMemoryView[3]={nullptr,nullptr,nullptr};
 bool q38ProjectionMemoryMapAttempted=false;uint64 q38ProjectionMemoryBar=0;
#endif
#ifdef PIONEER_Q38_PROJECTION_NATIVE_FACTORY_FIXTURE
 ProjectionNativeTransport* projectionTransport=nullptr;
#if defined(PIONEER_Q38_PROJECTION_VALIDATION_NATIVE) || defined(PIONEER_Q38_RESIDENT_DECODER_VALIDATION_NATIVE) || defined(PIONEER_Q38_GDN_INPUT_VALIDATION_NATIVE) || defined(PIONEER_Q38_GDN_RECURRENT_VALIDATION_NATIVE) || defined(PIONEER_Q38_GDN_RENDEZVOUS_VALIDATION_NATIVE) || defined(PIONEER_Q38_GDN_OUTPUT_VALIDATION_NATIVE) || defined(PIONEER_Q38_RESIDENT_OUTPUT_VALIDATION_NATIVE) || defined(PIONEER_Q38_RESIDENT_LAYER_VALIDATION_NATIVE)
 mutable bool projectionSeedRequired=false; // TEST delayed physical byte handoff after genuine QRN1
#endif
 std::array<uint32,10240> projectionPrefix={};std::array<uint32,9216> projectionPrimary={},projectionShadow={};
 int projectionFailMap=-1,projectionNullMap=-1;
 bool UseProjectionIO()const;
 bool ProjectionExecutionSourceScope(bool);
 bool ProjectionGraphMemoryScope(const PioneerQ38ProjectionGraphMemory&,const PioneerQ38CanonicalModelPlan&,
  const PioneerQ38GdnIdentity&,const uint8*,unsigned);
 bool ProjectionGraphExecutionScope(const PioneerQ38ProjectionGraphOwner&,const PioneerQ38ProjectionGraphMemory&,
  const PioneerQ38ProjectionMutableBanks&,const PioneerQ38CanonicalModelPlan&,const PioneerQ38LearnedRmsController&,
  const PioneerQ38ProjectionIdentity&,const uint8*,unsigned);
 void ProjectionGraphRetainHostOwners();
 bool MapRuntime(){return runtimeMapped;}
 bool MapQ38Panel(){return UseProjectionIO()?projectionTransport->MapPanel():ProjectionZeroIO::MapQ38Panel();}
 bool Q38PanelMappingStable(){return UseProjectionIO()?projectionTransport->PanelStable():ProjectionZeroIO::Q38PanelMappingStable();}
 uint32* Q38PanelSavedWords(){return UseProjectionIO()?projectionTransport->PanelSaved():ProjectionZeroIO::Q38PanelSavedWords();}
 uint32* HostReadBackup(unsigned n){return UseProjectionIO()?projectionTransport->Backup(n):ProjectionZeroIO::HostReadBackup(n);}
 uint32 ComputeNocRead(uint32 a){assert(UseProjectionIO());return projectionTransport->Read32(ProjectionNativeTransport::ComputeNoc,a);}
 uint32 HostPciRead(unsigned p,uint16_t a,uint8 n){if(UseProjectionIO())return projectionTransport->Read32(ProjectionNativeTransport::Pci,p,a,n);return ProjectionZeroIO::HostPciRead(p,a,n);}
 uint32 Q38PanelRead(uint32 a){if(UseProjectionIO())return projectionTransport->Read32(ProjectionNativeTransport::Panel,a);return ProjectionZeroIO::Q38PanelRead(a);}
 void Q38PanelWrite(uint32 a,uint32 v){if(UseProjectionIO()){projectionTransport->Write32(ProjectionNativeTransport::SetPanel,a,v);return;}ProjectionZeroIO::Q38PanelWrite(a,v);}
 uint32 RuntimeCodeRead(unsigned i){if(UseProjectionIO())return projectionTransport->Read32(ProjectionNativeTransport::Code,i);return ProjectionZeroIO::RuntimeCodeRead(i);}
 void RuntimeCodeWrite(unsigned i,uint32 v){if(UseProjectionIO()){projectionTransport->Write32(ProjectionNativeTransport::SetCode,i,v);return;}ProjectionZeroIO::RuntimeCodeWrite(i,v);}
 uint32 RuntimeControlRead(unsigned i){if(UseProjectionIO())return projectionTransport->Read32(ProjectionNativeTransport::Control,i);return ProjectionZeroIO::RuntimeControlRead(i);}
 uint32 RuntimeStatusRead(unsigned i){if(UseProjectionIO())return projectionTransport->Read32(ProjectionNativeTransport::Status,i);return ProjectionZeroIO::RuntimeStatusRead(i);}
 void RuntimeStatusWrite(unsigned i,uint32 v){if(UseProjectionIO()){projectionTransport->Write32(ProjectionNativeTransport::SetStatus,i,v);return;}ProjectionZeroIO::RuntimeStatusWrite(i,v);}
 void Pause(){if(UseProjectionIO()){projectionTransport->Pause();return;}ProjectionZeroIO::Pause();}
 uint64 Q38CohortNow(){return UseProjectionIO()?projectionTransport->Now():ProjectionZeroIO::Q38CohortNow();}
 uint32 ResidentNocRead(uint32 a){if(UseProjectionIO())return projectionTransport->Read32(ProjectionNativeTransport::Noc,a);return ProjectionZeroIO::ResidentNocRead(a);}
 void ResidentNocWrite(uint32 a,uint32 v){if(UseProjectionIO()){projectionTransport->Write32(ProjectionNativeTransport::SetNoc,a,v);return;}ProjectionZeroIO::ResidentNocWrite(a,v);}
#include "projection_factory_mapping.inc"
#endif
 bool failMap=false,nullMap=false,retained=false;
 unsigned mapCalls=0,nativeReads=0,nativeWrites=0,embeddingStarts=0,embeddingStops=0,embeddingPc=0;
 unsigned embeddingPacketFault=0,zeroFailRun=0,embeddingSummaryFault=UINT32_MAX;
 const char* embeddingImage;
 const uint8_t* originalEmbeddingWeights=nullptr; // TEST transport bytes ONLY
#ifdef PIONEER_Q38_LEARNED_RMS_NATIVE
 const char* rmsImage=nullptr;
 const uint32_t* originalRmsWeights=nullptr; // TEST transport bytes ONLY
 unsigned rmsStarts=0,rmsStops=0,rmsPc=0,rmsPacketFault=0,rmsBadCounter=0,rmsSourceFault=0,rmsDescriptorFault=0;
 unsigned rmsSummaryFault=UINT32_MAX,rmsPostFault=0,rmsReadFault=0,rmsReads=0;
 bool LearnedRmsSourceScope(bool);
 bool LearnedRmsNativeScope(const PioneerQ38LearnedRmsController&,const PioneerQ38ProjectionMutableBanks&,
  const PioneerQ38CanonicalModelPlan&,const PioneerQ38EmbeddingController&,const PioneerQ38ProjectionIdentity&,const uint8*,unsigned);
 bool LearnedRmsOtherCoresHeld();bool RmsMode()const;
 void RmsResetWrite(uint32);
#endif
#if defined(PIONEER_Q38_EMBEDDING_VALIDATION_NATIVE) || defined(PIONEER_Q38_LEARNED_RMS_VALIDATION_NATIVE)
 unsigned validationReads=0,validationFailRead=0,validationPoisonRead=0,validationBadCounter=0,validationSourceFault=0;
 bool validationPending=false;
 std::array<uint32,5120> computedEmbedding={};
 std::vector<uint32> embeddingOriginal=std::vector<uint32>(73737);
 bool ValidationTrafficMode()const;
 uint32 ResidentNocRead(uint32 offset){if(offset==0x40 && validationPending){Touch();return 1;}return ProjectionZeroIO::ResidentNocRead(offset);}
 void ResidentNocWrite(uint32 offset,uint32 value);
#endif
 struct DramAlias{const tt_dram_samples_result* target=nullptr;
#ifdef PIONEER_Q38_MIXED_LIVE_FINAL_RELEASE
#ifdef PIONEER_Q38_MIXED_LIVE_FRESH_FINAL_TEST
  FinalAuditField<tt_dram_samples_result> tlb_restored{&target,&tt_dram_samples_result::tlb_restored},
   data_restored{&target,&tt_dram_samples_result::data_restored},recovery_required{&target,&tt_dram_samples_result::recovery_required};
#else
  uint32 tlb_restored=1,data_restored=1,recovery_required=0;
#endif
#endif
  const tt_dram_samples_result* operator&()const{return target;}
  operator const tt_dram_samples_result&()const{assert(target);return *target;}}dram;
 struct NocAlias{const tt_compute_noc_result* target=nullptr;
#ifdef PIONEER_Q38_MIXED_LIVE_FINAL_RELEASE
#ifdef PIONEER_Q38_MIXED_LIVE_FRESH_FINAL_TEST
  FinalAuditField<tt_compute_noc_result> tlb_restored{&target,&tt_compute_noc_result::tlb_restored},
   recovery_required{&target,&tt_compute_noc_result::recovery_required};
#else
  uint32 tlb_restored=1,recovery_required=0;
#endif
#endif
  operator const tt_compute_noc_result&()const{assert(target);return *target;}}noc;
 explicit PioneerSessionIO(const char* zero,const char* embedding):ProjectionZeroIO(zero),embeddingImage(embedding){
  for(unsigned i=0;i<prefix.size();i++)prefix[i]=i%17?0x9e3779b9U*(i+1):UINT32_MAX;
 }
 bool EmbeddingSourceScope(bool);
 bool ProjectionZeroBeforeCohort();bool ProjectionZeroOtherCoresHeld();
 bool EmbeddingNativeScope(const PioneerQ38EmbeddingController&,const PioneerQ38ProjectionMutableBanks&,
  const PioneerQ38CanonicalModelPlan&,const PioneerQ38ProjectionIdentity&,const uint8_t*,unsigned);
 bool EmbeddingOtherCoresHeld();void EmbeddingRetainHostOwners();
 bool EmbeddingMode()const;
#ifdef PIONEER_Q38_RECURRENT_BLOCK_INITIALIZATION_FIXTURE
 bool NonRootHeldOwner();
#else
 bool NonRootHeldOwner(){return EmbeddingSourceScope(false)
#ifdef PIONEER_Q38_RESIDENT_LAYER_NATIVE_CONTEXT
  || ResidentLayerInitializationSourceScope(false,false) || ResidentLayerInitializationSourceScope(true,false)
#endif
#ifdef PIONEER_Q38_RESIDENT_OUTPUT_OWNER
  || ResidentOutputInitializationSourceScope(false,false) || ResidentOutputInitializationSourceScope(true,false)
#endif
#ifdef PIONEER_Q38_GDN_STATE_INIT_NATIVE
  || RecurrentZeroSourceScope()
#endif
#ifdef PIONEER_Q38_LEARNED_RMS_NATIVE
  || LearnedRmsSourceScope(false)
#endif
  ;}
#endif
 bool NonRootHeldMappingStable(){return !coreMapLost && NonRootHeldOwner();}
 void DramTlbWrite(unsigned slot,unsigned word,uint32 value){
#ifdef PIONEER_Q38_MIXED_LIVE_NATIVE_FACTORY_FIXTURE
  if(UseMixedLiveIO()){mixedLiveTransport->Write32(MixedLiveNativeTransport::SetTlb,slot,value,word);return;}
#endif
#ifdef PIONEER_Q38_PROJECTION_NATIVE_FACTORY_FIXTURE
  if(UseProjectionIO()){projectionTransport->Write32(ProjectionNativeTransport::SetTlb,slot,value,word);return;}
#endif
  Touch();StoreMock::DramTlbWrite(slot,word,value);}
 uint32 DramTlbRead(unsigned slot,unsigned word){
#ifdef PIONEER_Q38_MIXED_LIVE_NATIVE_FACTORY_FIXTURE
  if(UseMixedLiveIO())return mixedLiveTransport->Read32(MixedLiveNativeTransport::Tlb,slot,word);
#endif
#ifdef PIONEER_Q38_PROJECTION_NATIVE_FACTORY_FIXTURE
  if(UseProjectionIO())return projectionTransport->Read32(ProjectionNativeTransport::Tlb,slot,word);
#endif
  Touch();return StoreMock::DramTlbRead(slot,word);}
 bool SelectRuntimeCore(unsigned core){
#ifdef PIONEER_Q38_MIXED_LIVE_NATIVE_FACTORY_FIXTURE
  if(UseMixedLiveIO()){if(core>=120 || (!fleetEnabled && core))return false;bool okay=mixedLiveTransport->Select(core);if(okay)runtimeCore=core;return okay;}
#endif
#ifdef PIONEER_Q38_PROJECTION_NATIVE_FACTORY_FIXTURE
  if(UseProjectionIO()){if(core>=120 || (!fleetEnabled && core))return false;bool okay=projectionTransport->Select(core);if(okay)runtimeCore=core;return okay;}
#endif
  Touch();if(core)return false;
  return BlackholeDramConfigure(*this,0,0,1,2,false) && BlackholeDramConfigure(*this,1,0xffa00000,1,2,false);
 }
 uint32 ResetRead(){
#ifdef PIONEER_Q38_MIXED_LIVE_NATIVE_FACTORY_FIXTURE
  if(UseMixedLiveIO())return mixedLiveTransport->Read32(MixedLiveNativeTransport::Reset);
#endif
#ifdef PIONEER_Q38_PROJECTION_NATIVE_FACTORY_FIXTURE
  if(UseProjectionIO())return projectionTransport->Read32(ProjectionNativeTransport::Reset);
#endif
  Touch();bool root=false;
  for(bool readOnly:{false,true}){bool same=true;for(unsigned w=0;w<3;w++)same&=ProjectionZeroIO::tlb[1][w]==BlackholeDramFixedTlbWord(0xffa00000,1,2,readOnly,w);root|=same;}
#ifdef PIONEER_Q38_RESIDENT_OUTPUT_OWNER
  if(!root && outputNonrootFault)return 0x46800;
#endif
#ifdef PIONEER_Q38_RECURRENT_BLOCK_INITIALIZATION_FIXTURE
  if(!root && recurrentBlockNonrootFault)return 0x46800;
#endif
  return root?coreReset:0x47800;
 }
 uint32 SourceRead(unsigned word){
#ifdef PIONEER_Q38_MIXED_LIVE_NATIVE_FACTORY_FIXTURE
  if(UseMixedLiveIO())return mixedLiveTransport->Read32(MixedLiveNativeTransport::Source,word);
#endif
#ifdef PIONEER_Q38_PROJECTION_NATIVE_FACTORY_FIXTURE
  if(UseProjectionIO())return projectionTransport->Read32(ProjectionNativeTransport::Source,word);
#endif
  Touch();return ProjectionZeroIO::SourceRead(word);}
 uint32 ResidentL1Read(unsigned word){
#ifdef PIONEER_Q38_MIXED_LIVE_NATIVE_FACTORY_FIXTURE
  if(UseMixedLiveIO())return mixedLiveTransport->Read32(MixedLiveNativeTransport::Local,word);
#endif
#ifdef PIONEER_Q38_PROJECTION_NATIVE_FACTORY_FIXTURE
  if(UseProjectionIO())return projectionTransport->Read32(ProjectionNativeTransport::Local,word);
#endif
  Touch();assert(word<2048);return destination[word];}
 void ResidentL1Write(unsigned word,uint32 value){
#ifdef PIONEER_Q38_MIXED_LIVE_NATIVE_FACTORY_FIXTURE
  if(UseMixedLiveIO()){mixedLiveTransport->Write32(MixedLiveNativeTransport::SetLocal,word,value);return;}
#endif
#ifdef PIONEER_Q38_PROJECTION_NATIVE_FACTORY_FIXTURE
  if(UseProjectionIO()){projectionTransport->Write32(ProjectionNativeTransport::SetLocal,word,value);return;}
#endif
  Touch();assert(word<2048);destination[word]=value;}
 uint32 RuntimePcRead(){
#ifdef PIONEER_Q38_MIXED_LIVE_NATIVE_FACTORY_FIXTURE
  if(UseMixedLiveIO())return mixedLiveTransport->Read32(MixedLiveNativeTransport::Pc);
#endif
#ifdef PIONEER_Q38_PROJECTION_NATIVE_FACTORY_FIXTURE
  if(UseProjectionIO())return projectionTransport->Read32(ProjectionNativeTransport::Pc);
#endif
#ifdef PIONEER_Q38_LEARNED_RMS_NATIVE
  if(RmsMode()){Touch();return rmsPc;}
#endif
  return EmbeddingMode()?(Touch(),embeddingPc):ProjectionZeroIO::RuntimePcRead();}
 void RuntimeResetWrite(uint32 value);
 void QuarantineHostPage(){
#ifdef PIONEER_Q38_MIXED_LIVE_NATIVE_FACTORY_FIXTURE
  if(mixedLiveTransport)mixedLiveTransport->Quarantine();
#endif
#ifdef PIONEER_Q38_PROJECTION_NATIVE_FACTORY_FIXTURE
  if(projectionTransport)projectionTransport->Quarantine();
#endif
  PioneerHostPageIO::quarantined=true;ProjectionZeroIO::QuarantineHostPage();retained=true;}
 uint32& RawSlot(uint32 a){
  assert(PioneerQ38EmbeddingSnapshotContains(a));
  if(a==0x6000)return entry;
  if(a<0x6e000)return prefix[(a-0x64000)/4];
  if(a<0xac000)return view[(a-TT_Q38_PANEL_VIEW_BASE)/4];
  return destination[0x1fc0/4+(a-0x17ffc0)/4];
 }
#include "embedding_factory_mapping.inc"
#ifdef PIONEER_Q38_MIXED_LIVE_FINAL_RELEASE
#include "native_session_unmap.inc"
#include "native_release_output.inc"
#include "native_release_wide.inc"
 int32 ReleaseHostPage(){return PioneerHostPageIO::ReleaseHostPage();}
 void InitFinalVmViews(){
  area_id* views[]={&q38MixedLiveMemoryArea[0],&q38MixedLiveMemoryArea[1],&q38MixedLiveMemoryArea[2],&q38MixedLiveMemoryArea[3],&q38EmbeddingMemoryArea,
   &q38PanelArea,&q38FfnCodeArea,&runtimeAreas[0],&runtimeAreas[1],&runtimeAreas[2],&runtimeAreas[3],&runtimeAreas[4],&attentionArea,&bulkArea,&coordinatorArea,&layerArea,&fleetArea,
   &this->PioneerHostReadIO::scratchArea,&this->PioneerHostReadIO::writeArea,&hdmaArea,&inboundArea,&siiArea,&nocArea,&dataArea,&dramArea,&resetArea,&csmArea,&this->PioneerArcIO::scratchArea,&tlbArea,&this->PioneerMmioIO::area};
  for(unsigned i=0;i<30;i++)if(*views[i]<B_OK)*views[i]=2000+int(i); // simulated OS handles,not permission or device data
 }
 ~PioneerSessionIO(){
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38MixedLiveNativeMapDestroy.inc"
 }
#endif
};
#ifdef PIONEER_Q38_PROJECTION_NATIVE_FACTORY_FIXTURE
static bool ProjectionPointerAddress(const volatile uint32* p,uint32& address){
 if(!activeIO || !activeIO->UseProjectionIO())return false;
 const volatile uint32* base[]={activeIO->projectionPrefix.data(),activeIO->projectionPrimary.data(),activeIO->projectionShadow.data()};
 const unsigned count[]={10240,9216,9216},offset[]={0x64000,0xb0000,0xc0000};
 for(unsigned i=0;i<3;i++){
  uintptr_t v=reinterpret_cast<uintptr_t>(p),first=reinterpret_cast<uintptr_t>(base[i]);
  if(v>=first && v-first<count[i]*4 && !((v-first)&3)){address=offset[i]+uint32(v-first);return true;}
 }
 return false;
}
#endif
static uint32 PioneerRead32(const volatile uint32* p){assert(activeIO);activeIO->Touch();activeIO->nativeReads++;
#ifdef PIONEER_Q38_MIXED_LIVE_NATIVE_FACTORY_FIXTURE
 if(activeIO->UseMixedLiveIO()){
  const unsigned offsets[]={0x20000,0x40000,0x60000,0xac000};
  for(unsigned i=0;i<4;i++){const auto& alias=activeIO->mixedLiveAliases[i];uintptr_t at=reinterpret_cast<uintptr_t>(p),first=reinterpret_cast<uintptr_t>(alias.data());
   if(at>=first && at-first<alias.size()*4 && !((at-first)&3))return activeIO->mixedLiveTransport->MappedRead(offsets[i]+unsigned(at-first));}
 }
#endif
#if defined(PIONEER_Q38_RESIDENT_LAYER_NATIVE_CONTEXT) && defined(PIONEER_Q38_PROJECTION_NATIVE_FACTORY_FIXTURE)
 if(activeIO->projectionTransport && activeIO->q38ResidentLayerMemoryMapAttempted){
  const volatile uint32* base[]={activeIO->layerPrefixAlias.data(),activeIO->layerSuffixAlias.data()};
  const unsigned bytes[]={40960,811008},offset[]={0x64000,0xb8000};
  for(unsigned i=0;i<2;i++){uintptr_t at=reinterpret_cast<uintptr_t>(p),first=reinterpret_cast<uintptr_t>(base[i]);
   if(at>=first && at-first<bytes[i] && !((at-first)&3))return activeIO->projectionTransport->ResidentLayerRead(offset[i]+unsigned(at-first));}
 }
#elif defined(PIONEER_Q38_RESIDENT_OUTPUT_OWNER) && defined(PIONEER_Q38_PROJECTION_NATIVE_FACTORY_FIXTURE)
 if(activeIO->projectionTransport && activeIO->q38ResidentOutputMemoryMapAttempted){
  const volatile uint32* base[]={activeIO->outputPrefixAlias.data(),activeIO->outputSuffixAlias.data()};
  const unsigned bytes[]={40960,614400},offset[]={0x64000,0xb8000};
  for(unsigned i=0;i<2;i++){uintptr_t at=reinterpret_cast<uintptr_t>(p),first=reinterpret_cast<uintptr_t>(base[i]);
   if(at>=first && at-first<bytes[i] && !((at-first)&3))return activeIO->projectionTransport->ResidentOutputRead(offset[i]+unsigned(at-first));}
 }
#endif
#ifdef PIONEER_Q38_PROJECTION_NATIVE_FACTORY_FIXTURE
 uint32 address;if(ProjectionPointerAddress(p,address))return activeIO->projectionTransport->MappedRead(address);
#endif
#ifdef PIONEER_Q38_LEARNED_RMS_NATIVE
 if(activeIO->RmsMode() && ++activeIO->rmsReads==activeIO->rmsReadFault)return *p^1;
#endif
 return *p;}
static void PioneerWrite32(volatile uint32* p,uint32 value){assert(activeIO);activeIO->Touch();activeIO->nativeWrites++;
#ifdef PIONEER_Q38_MIXED_LIVE_NATIVE_FACTORY_FIXTURE
 if(activeIO->UseMixedLiveIO()){
  const unsigned offsets[]={0x20000,0x40000,0x60000,0xac000};
  for(unsigned i=0;i<4;i++){const auto& alias=activeIO->mixedLiveAliases[i];uintptr_t at=reinterpret_cast<uintptr_t>(p),first=reinterpret_cast<uintptr_t>(alias.data());
   if(at>=first && at-first<alias.size()*4 && !((at-first)&3)){activeIO->mixedLiveTransport->MappedWrite(offsets[i]+unsigned(at-first),value);return;}}
 }
#endif
#if defined(PIONEER_Q38_RESIDENT_LAYER_NATIVE_CONTEXT) && defined(PIONEER_Q38_PROJECTION_NATIVE_FACTORY_FIXTURE)
 if(activeIO->projectionTransport && activeIO->q38ResidentLayerMemoryMapAttempted){
  const volatile uint32* base[]={activeIO->layerPrefixAlias.data(),activeIO->layerSuffixAlias.data()};
  const unsigned bytes[]={40960,811008},offset[]={0x64000,0xb8000};
  for(unsigned i=0;i<2;i++){uintptr_t at=reinterpret_cast<uintptr_t>(p),first=reinterpret_cast<uintptr_t>(base[i]);
   if(at>=first && at-first<bytes[i] && !((at-first)&3)){
    activeIO->projectionTransport->ResidentLayerWrite(offset[i]+unsigned(at-first),value);return;}}
 }
#elif defined(PIONEER_Q38_RESIDENT_OUTPUT_OWNER) && defined(PIONEER_Q38_PROJECTION_NATIVE_FACTORY_FIXTURE)
 if(activeIO->projectionTransport && activeIO->q38ResidentOutputMemoryMapAttempted){
  const volatile uint32* base[]={activeIO->outputPrefixAlias.data(),activeIO->outputSuffixAlias.data()};
  const unsigned bytes[]={40960,614400},offset[]={0x64000,0xb8000};
  for(unsigned i=0;i<2;i++){uintptr_t at=reinterpret_cast<uintptr_t>(p),first=reinterpret_cast<uintptr_t>(base[i]);
   if(at>=first && at-first<bytes[i] && !((at-first)&3)){
    activeIO->projectionTransport->ResidentOutputWrite(offset[i]+unsigned(at-first),value);return;}}
 }
#endif
#ifdef PIONEER_Q38_PROJECTION_NATIVE_FACTORY_FIXTURE
 uint32 address;if(ProjectionPointerAddress(p,address)){activeIO->projectionTransport->MappedWrite(address,value);return;}
#endif
 *p=value;}
static area_id map_physical_memory(const char*,uint64 physical,unsigned bytes,int placement,unsigned flags,void** view){
 assert(activeIO);activeIO->Touch();activeIO->mapCalls++;
#ifdef PIONEER_Q38_MIXED_LIVE_NATIVE_FACTORY_FIXTURE
 if(activeIO->UseMixedLiveIO()){
  const unsigned offsets[]={0x19020000,0x19040000,0x19060000,0x190ac000},sizes[]={8192,131072,57344,868352};
  unsigned i=0;while(i<4 && physical!=Bar+offsets[i])i++;
  assert(i<4 && bytes==sizes[i] && placement==B_ANY_KERNEL_ADDRESS && flags==(B_KERNEL_READ_AREA|B_KERNEL_WRITE_AREA|B_UNCACHED_MEMORY));
  if(int(i)==activeIO->mixedLiveFailMap){activeIO->mixedLiveMapFaultFence=activeIO->touches;return -1;}
  activeIO->mixedLiveAliases[i].resize(bytes/4);*view=int(i)==activeIO->mixedLiveNullMap?nullptr:activeIO->mixedLiveAliases[i].data();
  if(!*view)activeIO->mixedLiveMapFaultFence=activeIO->touches;return 50+i;
 }
#endif
#if defined(PIONEER_Q38_RESIDENT_LAYER_NATIVE_CONTEXT) && defined(PIONEER_Q38_PROJECTION_NATIVE_FACTORY_FIXTURE)
 if(activeIO->projectionTransport && activeIO->q38ResidentLayerMemoryMapAttempted){
  assert(placement==B_ANY_KERNEL_ADDRESS && flags==(B_KERNEL_READ_AREA|B_KERNEL_WRITE_AREA|B_UNCACHED_MEMORY));
  unsigned i=physical==Bar+0x19064000?0:1;
  assert(physical==Bar+(i?0x190b8000:0x19064000) && bytes==(i?811008U:40960U));
  if(int(i)==activeIO->layerFailMap)return -1;
  if(i)activeIO->layerSuffixAlias.resize(811008/4);
  *view=int(i)==activeIO->layerNullMap?nullptr:i?activeIO->layerSuffixAlias.data():activeIO->layerPrefixAlias.data();return 40+i;
 }
#elif defined(PIONEER_Q38_RESIDENT_OUTPUT_OWNER) && defined(PIONEER_Q38_PROJECTION_NATIVE_FACTORY_FIXTURE)
 if(activeIO->projectionTransport && activeIO->q38ResidentOutputMemoryMapAttempted){
  assert(placement==B_ANY_KERNEL_ADDRESS && flags==(B_KERNEL_READ_AREA|B_KERNEL_WRITE_AREA|B_UNCACHED_MEMORY));
  unsigned i=physical==Bar+0x19064000?0:1;
  assert(physical==Bar+(i?0x190b8000:0x19064000) && bytes==(i?614400U:40960U));
  if(int(i)==activeIO->outputFailMap)return -1;
  if(i)activeIO->outputSuffixAlias.resize(614400/4);
  *view=int(i)==activeIO->outputNullMap?nullptr:i?activeIO->outputSuffixAlias.data():activeIO->outputPrefixAlias.data();return 30+i;
 }
#endif
#ifdef PIONEER_Q38_PROJECTION_NATIVE_FACTORY_FIXTURE
 if(activeIO->UseProjectionIO()){
  const unsigned offsets[]={0x19064000,0x190b0000,0x190c0000},sizes[]={40960,36864,36864};
  unsigned i=0;while(i<3 && physical!=Bar+offsets[i])i++;
  assert(i<3 && bytes==sizes[i] && placement==B_ANY_KERNEL_ADDRESS && flags==(B_KERNEL_READ_AREA|B_KERNEL_WRITE_AREA|B_UNCACHED_MEMORY));
  if(int(i)==activeIO->projectionFailMap)return -1;
  *view=int(i)==activeIO->projectionNullMap?nullptr:i==0?activeIO->projectionPrefix.data():i==1?activeIO->projectionPrimary.data():activeIO->projectionShadow.data();
  return 24+i;
 }
#endif
 assert(physical==Bar+0x19064000 && bytes==40960 && placement==B_ANY_KERNEL_ADDRESS && flags==(B_KERNEL_READ_AREA|B_KERNEL_WRITE_AREA|B_UNCACHED_MEMORY));
 if(activeIO->failMap)return -1;
#if defined(PIONEER_Q38_EMBEDDING_VALIDATION_NATIVE) || defined(PIONEER_Q38_LEARNED_RMS_VALIDATION_NATIVE)
 // Observe the embedding owner's entry baseline AFTER zero CLOSE. The
 // legacy resident lease still owns its old status trailer until BANK_END.
 for(unsigned i=0;i<73737;i++)activeIO->embeddingOriginal[i]=activeIO->RawSlot(PioneerQ38EmbeddingSnapshotAddress(i));
#endif
 *view=activeIO->nullMap?nullptr:activeIO->prefix.data();return 23;
}
struct FactoryFlags{bool saved=false,dirty=false,ready=false,running=false,unsafe=false,awaiting=false,closed=false;};
#ifdef PIONEER_Q38_MIXED_LIVE_END_FIXTURE
using FactoryClock=MixedLiveNativeClock;
struct NativeClockFields{
 NativeClockFields& control=*this;
 tt_arc_scratch_result arc=MixedLiveNativeClock::Arc();
 tt_arc_telemetry_result telemetry=MixedLiveNativeClock::Health();
};
#else
struct FactoryClock{
 tt_arc_clock_guard_result value={};bool required=true,quarantined=false;
 FactoryClock(){value.state=TT_ARC_CLOCK_BUSY_VERIFIED;value.busy_requested=value.busy_acknowledged=1;}
 const auto& Snapshot()const{return value;}bool RequiresExplicitEnd()const{return required;}
 void Quarantine(){quarantined=true;}
};
#endif
struct PioneerStoreContext {
 PioneerSessionIO& io;PioneerQ38EmbeddingNativeState* q38EmbeddingModel=nullptr;
#ifdef PIONEER_Q38_RECURRENT_BLOCK_NATIVE_CONTEXT
 PioneerQ38RecurrentBlockNativeState* q38RecurrentBlockNative=nullptr;
#endif
#ifdef PIONEER_Q38_MIXED_LIVE_NATIVE_CONTEXT
 PioneerQ38MixedLiveNativeState* q38MixedLiveNative=nullptr;
#endif
#ifdef PIONEER_Q38_RESIDENT_LAYER_NATIVE_CONTEXT
 PioneerQ38ResidentLayerNativeState* q38ResidentLayerNative=nullptr;
#endif
#ifdef PIONEER_Q38_RESIDENT_OUTPUT_OWNER
 PioneerQ38ResidentOutputNativeState* q38ResidentOutputNative=nullptr;
#endif
 PioneerResidentBanks& banks;PioneerResidentLease& lease;PioneerBulkUploadLease& bulk;
 PioneerOutputBufferLease& bulkHost;PioneerWideOutputLease& wideHost;tt_host_page_result& host;
 struct D{tt_dram_samples_result& dram;
#ifdef PIONEER_Q38_MIXED_LIVE_END_FIXTURE
 NativeClockFields control={};
#endif
 };struct B1{D baseline;tt_compute_noc_result& noc;};struct B2{B1 baseline;};struct B3{B2 baseline;
#ifdef PIONEER_Q38_MIXED_LIVE_END_FIXTURE
 tt_host_dma_audit_result audit={};
#endif
 };B3 baseline;
 uint8* initialized;uint32 hash=0,*hashes=&hash;
#if defined(PIONEER_Q38_RESIDENT_OUTPUT_VALIDATION_NATIVE) || defined(PIONEER_Q38_RESIDENT_LAYER_VALIDATION_NATIVE) || defined(PIONEER_Q38_MIXED_LIVE_NATIVE_CONTEXT) || defined(PIONEER_Q38_RECURRENT_BLOCK_NATIVE_CONTEXT)
 // Real production pointee types for complete-role CPU END/retention
 // gates. This role never constructs any competing legacy FFN owner.
 PioneerQ38CanonicalModelPlan* q38FfnModelPlan=nullptr;PioneerQ38GraphMutableBanks* q38GraphMutable=nullptr;
 PioneerQ38FfnZeroLease* q38FfnZero=nullptr;PioneerQ38FfnPersistentGraphOwner* q38FfnGraph=nullptr;
 PioneerQ38FfnGraphCodeOwner* q38FfnCode=nullptr;const uint8* q38FfnModelLedger=nullptr;unsigned q38FfnModelLayer=0;
#else
 void *q38FfnModelPlan=nullptr,*q38GraphMutable=nullptr,*q38FfnZero=nullptr,*q38FfnGraph=nullptr,*q38FfnCode=nullptr;
#endif
 PioneerQ38GdnBanks* q38GdnBanks=nullptr;PioneerQ38StateZeroLease* q38StateZero=nullptr;
 PioneerQ38DispatchLease* q38Dispatch=nullptr;void *q38Cohort=nullptr,*fleet=nullptr;
 PioneerQ38ProjectionIdentity q38FfnModelIdentity={},q38GdnIdentity={};
 FactoryFlags span,panelProbe,streamProbe;FactoryFlags* q38Span=&span;FactoryClock q38Clock;
#if defined(PIONEER_Q38_RESIDENT_OUTPUT_VALIDATION_NATIVE) || defined(PIONEER_Q38_RESIDENT_LAYER_VALIDATION_NATIVE) || defined(PIONEER_Q38_MIXED_LIVE_NATIVE_FACTORY_FIXTURE)
 PioneerQ38DeltaProbeLease q38Delta;
#else
 FactoryFlags q38Delta;
#endif
 int runtimeCoreMode=-1;
#ifdef PIONEER_Q38_MIXED_LIVE_FRESH_FINAL_TEST
 int q38GdnCallThread=-1; // ACTUAL call scope must acquire the seal;never fixture-assigned
#else
 int q38GdnCallThread=42;
#endif
 uint64 bytes=PioneerResidentBanks::Total;
 bool bankStorageOnly=true,dispatchOwnerRetained=false,otherUnsafe=false,q38DispatchRoutes=false;
#ifdef PIONEER_Q38_MIXED_LIVE_END_FIXTURE
 tt_pioneer_mmio_result pci={};tt_pioneer_mmio_transaction transaction={};unsigned q38EndPhase=0;
 tt_resident_session_idle_result idle={};
 auto& Pci(){return pci;}
#endif
 PioneerStoreContext(PioneerSessionIO& native,PanelOwners& owner,PioneerBulkUploadLease& b,PioneerOutputBufferLease& output,
  PioneerWideOutputLease& wide,uint8* bitmap):io(native),banks(owner.banks),lease(owner.store),bulk(b),bulkHost(output),wideHost(wide),host(owner.host),
   baseline{{{{owner.dram},owner.noc}}},initialized(bitmap){
  span.closed=true;io.q38MutableContext=this;io.dram.target=&owner.dram;io.noc.target=&owner.noc;
 }
 ~PioneerStoreContext(){
#ifdef PIONEER_Q38_MIXED_LIVE_FINAL_RELEASE
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38MixedLiveNativeDestroy.inc"
  finalDestroyed++;
#else
#ifdef PIONEER_Q38_RECURRENT_BLOCK_NATIVE_CONTEXT
  delete q38RecurrentBlockNative;
#endif
#ifdef PIONEER_Q38_MIXED_LIVE_NATIVE_CONTEXT
  delete q38MixedLiveNative;
#endif
#ifdef PIONEER_Q38_RESIDENT_LAYER_NATIVE_CONTEXT
  delete q38ResidentLayerNative;
#endif
#ifdef PIONEER_Q38_RESIDENT_OUTPUT_OWNER
  delete q38ResidentOutputNative;
#endif
  delete q38EmbeddingModel;
#endif
  delete q38Dispatch;
 } // SIMULATED context;real unknown contexts are retained,never destroyed
 bool Unsafe(bool zero=false,bool graph=false,bool controlledEmbedding=false){
  (void)zero;(void)graph;
#ifdef PIONEER_Q38_MIXED_LIVE_NATIVE_CONTEXT
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38MixedLiveNativeUnsafe.inc"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38MixedLiveNativeCohortUnsafe.inc"
#endif
#ifdef PIONEER_Q38_RESIDENT_LAYER_NATIVE_CONTEXT
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38ResidentLayerNativeUnsafe.inc"
#endif
#ifdef PIONEER_Q38_RESIDENT_OUTPUT_OWNER
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38ResidentOutputNativeUnsafe.inc"
#endif
#include "embedding_factory_unsafe.inc"
#ifdef PIONEER_Q38_PROJECTION_NATIVE_FACTORY_FIXTURE
#include "projection_factory_unsafe.inc"
  if(q38Dispatch && PioneerQ38DispatchUnsafe(*q38Dispatch))return true;
#endif
  return otherUnsafe || banks.Quarantined() || q38Clock.quarantined || bulk.unsafe || bulkHost.quarantined || wideHost.quarantined
#ifdef PIONEER_Q38_MIXED_LIVE_END_FIXTURE
   || q38Clock.Unsafe()
   || PioneerQ38MixedLiveFinalPciUncertain(*this)
#endif
   || lease.recovery_required || (q38EmbeddingModel && !zero && q38EmbeddingModel->zero.running);
 }
};
struct PioneerProbeCookie{PioneerStoreContext* context;uint32 magic=0x50313030;int team=7;};
static PioneerProbeCookie* sRuntimeOwner=nullptr;
static bool sColdRecoveryRequired=false,sReloadQuiesced=false,sBankStorageOnly=true;
static uint64 sReloadGeneration=4;
#ifndef PIONEER_Q38_MIXED_LIVE_FRESH_FINAL_TEST
static const void* sQ38LocalDeltaOwner=nullptr;
#endif
static int currentThread=42;static int find_thread(void*){return currentThread;}
#if defined(PIONEER_Q38_RESIDENT_DECODER_NATIVE) || defined(PIONEER_Q38_RESIDENT_OUTPUT_OWNER) || defined(PIONEER_Q38_MIXED_LIVE_NATIVE_CONTEXT) || defined(PIONEER_Q38_RECURRENT_BLOCK_NATIVE_CONTEXT)
static int currentTeam=7;static bool failThreadInfo=false;
struct thread_info{int team=-1;};
static int get_thread_info(int thread,thread_info* info){if(failThreadInfo || thread!=currentThread || !info)return -1;info->team=currentTeam;return B_OK;}
#endif
// Use actual native MEMORY-ONLY retention body,not a simulated ready seal.
#include "embedding_factory_retain.inc"
#ifdef PIONEER_Q38_MIXED_LIVE_FRESH_FINAL_TEST
static int sLock=1;static bool finalLockHeld=false;
static unsigned finalLockAcquired=0,finalLockReleased=0;
static status_t acquire_sem(int lock){assert(lock==sLock);if(finalLockHeld)return B_BUSY;finalLockHeld=true;finalLockAcquired++;return B_OK;}
static void release_sem(int lock){assert(lock==sLock && finalLockHeld);finalLockHeld=false;finalLockReleased++;}
#include "native_probe_lock.inc"
#include "native_call_scope.inc"
#endif
#ifdef PIONEER_Q38_RESIDENT_OUTPUT_OWNER
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38ResidentOutputNativeBind.inc"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38ResidentOutputNativeInitialize.inc"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38ResidentOutputNativeExecution.inc"
#endif
#ifdef PIONEER_Q38_RESIDENT_LAYER_NATIVE_CONTEXT
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38ResidentLayerNativeBind.inc"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38ResidentLayerNativeInitialize.inc"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38ResidentLayerNativeExecution.inc"
#endif
#include "embedding_factory_scope.inc"
#ifdef PIONEER_Q38_LEARNED_RMS_NATIVE
#include "rms_factory_scope.inc"
#endif
#ifdef PIONEER_Q38_PROJECTION_NATIVE_FACTORY_FIXTURE
#include "projection_factory_scope.inc"
#endif
#ifdef PIONEER_Q38_RESIDENT_DECODER_NATIVE
#include "decoder_factory_scope.inc"
#ifdef PIONEER_Q38_GDN_RECURRENT_NATIVE
#include "gdn_recurrent_scope.inc"
#endif
#ifdef PIONEER_Q38_GDN_STATE_INIT_NATIVE
#include "gdn_state_zero_gate.inc"
#endif
#endif
#include "embedding_factory_factory.inc"
#ifdef PIONEER_Q38_RECURRENT_BLOCK_NATIVE_CONTEXT
#include "recurrent_block_factory_bind.inc"
#ifdef PIONEER_Q38_RECURRENT_BLOCK_INITIALIZATION_FIXTURE
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38RecurrentBlockNativeInitialize.inc"
#include "recurrent_block_zero_callbacks.inc"
#endif
#endif
#ifdef PIONEER_Q38_MIXED_LIVE_NATIVE_CONTEXT
#ifdef PIONEER_Q38_MIXED_LIVE_NATIVE_FACTORY_FIXTURE
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38MixedLiveNative.inc"
#else
#include "mixed_live_factory_bind.inc"
#endif
#endif
#ifdef PIONEER_Q38_LEARNED_RMS_NATIVE
#include "rms_factory_factory.inc"
#endif
#ifdef PIONEER_Q38_PROJECTION_NATIVE_FACTORY_FIXTURE
#include "projection_factory_factory.inc"
#endif
#ifdef PIONEER_Q38_RESIDENT_DECODER_NATIVE
#include "decoder_factory_factory.inc"
#endif
#if defined(PIONEER_Q38_PROJECTION_NATIVE_FACTORY_FIXTURE) || defined(PIONEER_Q38_MIXED_LIVE_NATIVE_FACTORY_FIXTURE)
#include "projection_factory_complete_end.inc"
static bool EndFactory(PioneerStoreContext& c){bool clean=true;tt_resident_session_result result={};
 return EndQ38DispatchOwnership(c,result,clean) && clean;
}
#else
static bool EndFactory(PioneerStoreContext& c){bool clean=true;tt_resident_session_result result={};
#ifdef PIONEER_Q38_RECURRENT_BLOCK_NATIVE_CONTEXT
#include "recurrent_block_factory_end.inc"
#endif
#ifdef PIONEER_Q38_MIXED_LIVE_NATIVE_CONTEXT
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38MixedLiveNativeEnd.inc"
#endif
#ifdef PIONEER_Q38_RESIDENT_LAYER_NATIVE_CONTEXT
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38ResidentLayerNativeEnd.inc"
#endif
#ifdef PIONEER_Q38_RESIDENT_OUTPUT_OWNER
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38ResidentOutputNativeEnd.inc"
#endif
#include "embedding_factory_end.inc"
 return clean;
}
#endif
#ifdef PIONEER_Q38_MIXED_LIVE_NATIVE_FACTORY_FIXTURE
bool PioneerSessionIO::UseMixedLiveIO()const{
 auto* c=q38MutableContext;auto* state=c?c->q38MixedLiveNative:nullptr;
 return mixedLiveTransport && state && state->phase>=PioneerQ38MixedLiveNativeState::DispatchPreparing;
} // TEST byte endpoint selection observes genuine private phase; NEVER sets it
void PioneerSessionIO::ConfigureMixedLiveTestTransport(){
 auto* state=q38MutableContext?q38MutableContext->q38MixedLiveNative:nullptr;
 assert(state && state->phase==PioneerQ38MixedLiveNativeState::MemoryInstalled && state->plan && mixedLiveTransport);
 mixedLiveTransport->Configure(*state->plan,wholeWorkspace.data(),5120);
} // TEST observer/RAW device bytes only; not a production setter or numerical grant
#endif
#ifdef PIONEER_Q38_PROJECTION_NATIVE_FACTORY_FIXTURE
bool PioneerSessionIO::UseProjectionIO()const{
 bool active=projectionTransport && q38MutableContext && q38MutableContext->q38EmbeddingModel
#ifdef PIONEER_Q38_RESIDENT_DECODER_NATIVE
  && (q38MutableContext->q38EmbeddingModel->phase==PioneerQ38EmbeddingNativeState::ProjectionPreparing
   || q38MutableContext->q38EmbeddingModel->phase==PioneerQ38EmbeddingNativeState::Projection
   || q38MutableContext->q38EmbeddingModel->phase==PioneerQ38EmbeddingNativeState::ProjectionComplete
   || q38MutableContext->q38EmbeddingModel->phase==PioneerQ38EmbeddingNativeState::Decoder
   || q38MutableContext->q38EmbeddingModel->phase==PioneerQ38EmbeddingNativeState::DecoderHeld
   || q38MutableContext->q38EmbeddingModel->phase==PioneerQ38EmbeddingNativeState::DecoderComplete);
#else
  && q38MutableContext->q38EmbeddingModel->phase>=PioneerQ38EmbeddingNativeState::ProjectionPreparing;
#endif
#if defined(PIONEER_Q38_PROJECTION_VALIDATION_NATIVE) || defined(PIONEER_Q38_RESIDENT_DECODER_VALIDATION_NATIVE) || defined(PIONEER_Q38_GDN_INPUT_VALIDATION_NATIVE) || defined(PIONEER_Q38_GDN_RECURRENT_VALIDATION_NATIVE) || defined(PIONEER_Q38_GDN_RENDEZVOUS_VALIDATION_NATIVE) || defined(PIONEER_Q38_GDN_OUTPUT_VALIDATION_NATIVE) || defined(PIONEER_Q38_RESIDENT_OUTPUT_VALIDATION_NATIVE) || defined(PIONEER_Q38_RESIDENT_LAYER_VALIDATION_NATIVE)
 if(active && projectionSeedRequired){
  projectionSeedRequired=false;auto& endpoint=*const_cast<PioneerSessionIO*>(this);
  std::vector<uint32> addresses(73737),values(73737);uint32 sources[16],commands[10],counts[8];
  for(unsigned i=0;i<73737;i++){addresses[i]=PioneerQ38EmbeddingSnapshotAddress(i);values[i]=endpoint.RawSlot(addresses[i]);}
  for(unsigned i=0;i<16;i++)sources[i]=i?source[i]:config;
  for(unsigned i=0;i<10;i++)commands[i]=command[i];
  for(unsigned i=0;i<8;i++)counts[i]=endpoint.PanelIO::ResidentNocRead(PioneerQ38SpanCounter(i));
  projectionTransport->SeedRoot(addresses.data(),values.data(),unsigned(values.size()),sources,commands,counts);
 }
#endif
 return active;
}
#endif
bool PioneerSessionIO::EmbeddingMode()const{
 return q38MutableContext && q38MutableContext->q38EmbeddingModel
  && q38MutableContext->q38EmbeddingModel->phase==PioneerQ38EmbeddingNativeState::Embedding;
}
#if defined(PIONEER_Q38_EMBEDDING_VALIDATION_NATIVE) || defined(PIONEER_Q38_LEARNED_RMS_VALIDATION_NATIVE)
bool PioneerSessionIO::ValidationTrafficMode()const{
#ifdef PIONEER_Q38_LEARNED_RMS_VALIDATION_NATIVE
 return RmsMode() && rmsStops && coreReset==0x47800;
#else
 return EmbeddingMode() && embeddingStops && coreReset==0x47800;
#endif
}
void PioneerSessionIO::ResidentNocWrite(uint32 offset,uint32 value){
 if(!ValidationTrafficMode()){ProjectionZeroIO::ResidentNocWrite(offset,value);return;}
 Touch();
 if(offset!=0x40){StoreMock::ResidentNocWrite(offset,value);return;}
#ifdef PIONEER_Q38_LEARNED_RMS_VALIDATION_NATIVE
 const unsigned maxPages=10,poison=0x7fc20000;
#else
 const unsigned maxPages=5,poison=0x7fc10000;
#endif
 assert(value==1 && !validationPending && validationReads<maxPages && pciCommand[0]==0x402 && pciCommand[1]==0x402);
 unsigned plane=validationReads/5,page=validationReads++%5;
 unsigned sourceOffset=0;
#ifdef PIONEER_Q38_LEARNED_RMS_VALIDATION_NATIVE
 sourceOffset=plane?TT_Q38_RMS_BACKUP:0;
#endif
 const uint32 expected[]={workspaceBase+sourceOffset+page*4096,0,PioneerQ38ProjectionRoute(workspaceBank),0x80000+page*4096,0,129,0,0,4096,0};
 for(unsigned i=0;i<10;i++)assert(command[i]==expected[i]);
 if(validationReads==validationFailRead){validationPending=true;return;}
 for(unsigned w=0;w<1024;w++){
  auto& local=RawSlot(0x80000+page*4096+w*4);
  assert(local==poison+plane*5120+page*1024+w); // destination genuinely poisoned BEFORE GDDR request
  if(!(validationReads==validationPoisonRead && w==1023))local=wholeWorkspace[sourceOffset/4+page*1024+w];
 }
 counters[0]++;counters[1]+=64;accepted++;counters[3]++;
 if(validationReads==validationBadCounter)writeCounters[0]++;
 if(validationReads==validationSourceFault)source[11]^=1;
}
#endif
#if defined(PIONEER_Q38_RESIDENT_OUTPUT_OWNER) && !defined(PIONEER_Q38_PROJECTION_NATIVE_FACTORY_FIXTURE)
void PioneerSessionIO::RuntimeCodeWrite(unsigned i,uint32 value){
#ifdef PIONEER_Q38_RESIDENT_LAYER_NATIVE_CONTEXT
 auto* layerState=q38MutableContext?q38MutableContext->q38ResidentLayerNative:nullptr;
 if(layerState && ((outputGdnCloseFault && layerState->phase==PioneerQ38ResidentLayerNativeState::RecurrentClosing)
  || (outputHistoryCloseFault && layerState->phase==PioneerQ38ResidentLayerNativeState::HistoryClosing))){Touch();return;}
#endif
 auto* state=q38MutableContext?q38MutableContext->q38ResidentOutputNative:nullptr;
 if(state && ((outputGdnCloseFault && state->phase==PioneerQ38ResidentOutputNativeState::RecurrentClosing)
  || (outputHistoryCloseFault && state->phase==PioneerQ38ResidentOutputNativeState::HistoryClosing))){Touch();return;}
 ProjectionZeroIO::RuntimeCodeWrite(i,value);
}
#endif
#ifdef PIONEER_Q38_RESIDENT_OUTPUT_OWNER
bool PioneerSessionIO::ReadState(unsigned bank,uint32_t address,uint32_t* out,unsigned words){
 if(!out || !words || (address&3))return false;
 for(unsigned i=0;i<672;i++){const auto& e=outputGdnExtents[i];
  if(e.bank==bank && address>=e.address && address-e.address<e.bytes && words<=(e.bytes-(address-e.address))/4){
   unsigned at=outputGdnFirst[i]*1024+(address-e.address)/4;
   for(unsigned w=0;w<words;w++)out[w]=outputGdnWords.at(at+w);return true;}}
 for(unsigned i=0;i<336;i++){const auto& e=outputHistoryExtents[i];
  if(e.bank==bank && address>=e.address && address-e.address<e.bytes && words<=(e.bytes-(address-e.address))/4){
   for(unsigned w=0;w<words;w++)out[w]=outputHistoryWords.at(i*5120+(address-e.address)/4+w);return true;}}
 return false;
}
bool PioneerSessionIO::WriteState(unsigned bank,uint32_t address,const uint32_t* input,unsigned words){
 if(!input || !words || (address&3))return false;
 for(unsigned i=0;i<672;i++){const auto& e=outputGdnExtents[i];
  if(e.bank==bank && address>=e.address && address-e.address<e.bytes && words<=(e.bytes-(address-e.address))/4){
   unsigned at=outputGdnFirst[i]*1024+(address-e.address)/4;
   for(unsigned w=0;w<words;w++)outputGdnWords.at(at+w)=input[w];return true;}}
 for(unsigned i=0;i<336;i++){const auto& e=outputHistoryExtents[i];
  if(e.bank==bank && address>=e.address && address-e.address<e.bytes && words<=(e.bytes-(address-e.address))/4){
   for(unsigned w=0;w<words;w++)outputHistoryWords.at(i*5120+(address-e.address)/4+w)=input[w];return true;}}
 return false;
}
#endif
void PioneerSessionIO::RuntimeResetWrite(uint32 value){
#ifdef PIONEER_Q38_RECURRENT_BLOCK_INITIALIZATION_FIXTURE
 auto* blockState=q38MutableContext?q38MutableContext->q38RecurrentBlockNative:nullptr;
 if(blockState){
  bool gdn=blockState->phase==PioneerQ38RecurrentBlockNativeState::RecurrentZeroing;
  bool history=blockState->phase==PioneerQ38RecurrentBlockNativeState::HistoryZeroing;
  bool closing=blockState->phase==PioneerQ38RecurrentBlockNativeState::RecurrentClosing
   || blockState->phase==PioneerQ38RecurrentBlockNativeState::HistoryClosing;
  if(gdn || history || closing){
   // Device-byte bridge ONLY:execute the actual frozen image and observe
   // its output. Never fabricate completion,coverage,publication or seals.
   auto* m=q38MutableContext->q38EmbeddingModel;unsigned region=0,first=0,words=0;
   if(value==0x46800){
    assert(!closing && !publicationCounterFabricOnly);
    region=(gdn?blockState->recurrentZero.nonce:blockState->historyZero.nonce)-1;
    if(gdn){
     if(recurrentBlockGdnWords.empty()){recurrentBlockGdnWords.assign(73728*1024,0xa55aa55a);recurrentBlockGdnTouched.assign(73728,0);}
     auto* g=m->graph.Recurrent(m->identity);unsigned pages=0;
     assert(g && region<672 && g->InitializationPages(m->identity,region,pages)
      && g->InitializeSpan(m->identity,region,0,pages,recurrentBlockGdnExtents[region]));
     const auto& e=recurrentBlockGdnExtents[region];assert(e.bank==l1[7] && e.address==l1[5] && e.bytes==l1[4] && e.route==l1[8]);
     first=recurrentBlockGdnPages;recurrentBlockGdnFirst[region]=first;words=pages*1024;recurrentBlockGdnRuns++;
     if(recurrentBlockGdnRuns==recurrentBlockFailRun)packetFault=1;
    }else{
     if(recurrentBlockHistoryWords.empty()){recurrentBlockHistoryWords.assign(1680*1024,0xa55aa55a);recurrentBlockHistoryTouched.assign(1680,0);}
     auto* h=m->graph.ConvHistory(m->identity);assert(h && region<336 && h->InitializeSpan(m->identity,region,0,5,recurrentBlockHistoryExtents[region]));
     const auto& e=recurrentBlockHistoryExtents[region];assert(e.bank==l1[7] && e.address==l1[5] && e.bytes==l1[4] && e.route==l1[8]);
     first=region*5;words=5120;recurrentBlockHistoryRuns++;if(recurrentBlockHistoryRuns==recurrentBlockHistoryFailRun)packetFault=1;
    }
   }
   ZeroControlIO::RuntimeResetWrite(value);
   if(value==0x46800){
    assert(lastDeviceWords.size()==words);auto& bytes=gdn?recurrentBlockGdnWords:recurrentBlockHistoryWords;
    auto& touched=gdn?recurrentBlockGdnTouched:recurrentBlockHistoryTouched;
    for(unsigned i=0;i<words;i++)bytes[first*1024+i]=lastDeviceWords[i];
    for(unsigned i=first;i<first+words/1024;i++)touched[i]++;
    if(gdn){recurrentBlockGdnPages+=words/1024;
     if(recurrentBlockGdnRuns%96==0)printf("QRB_NATIVE_ACTUAL_ZERO GDN%u/pages%u;NO publication before clean CLOSE\n",recurrentBlockGdnRuns,recurrentBlockGdnPages);}
    if(recurrentBlockLoseTeamRun && (gdn?recurrentBlockGdnRuns:recurrentBlockHistoryRuns)==recurrentBlockLoseTeamRun)currentTeam++;
   }
   return;
  }
 }
#endif
#ifdef PIONEER_Q38_MIXED_LIVE_NATIVE_FACTORY_FIXTURE
 if(UseMixedLiveIO()){mixedLiveTransport->Write32(MixedLiveNativeTransport::SetReset,0,value);return;}
#endif
#ifdef PIONEER_Q38_RESIDENT_OUTPUT_OWNER
 auto* outputState=q38MutableContext?q38MutableContext->q38ResidentOutputNative:nullptr;
#ifdef PIONEER_Q38_RESIDENT_LAYER_NATIVE_CONTEXT
 auto* layerState=q38MutableContext?q38MutableContext->q38ResidentLayerNative:nullptr;
 if(outputState || layerState){
#else
 if(outputState){
#endif
  // TEST transport observes the actual native lease/descriptor only. It
  // NEVER publishes progress, writes a ready bit or synthesizes success.
  bool gdn=outputState && outputState->phase==PioneerQ38ResidentOutputNativeState::RecurrentZeroing;
  bool history=outputState && outputState->phase==PioneerQ38ResidentOutputNativeState::HistoryZeroing;
  bool closing=outputState && (outputState->phase==PioneerQ38ResidentOutputNativeState::RecurrentClosing
   || outputState->phase==PioneerQ38ResidentOutputNativeState::HistoryClosing);
#ifdef PIONEER_Q38_RESIDENT_LAYER_NATIVE_CONTEXT
  if(layerState){assert(!outputState);gdn=layerState->phase==PioneerQ38ResidentLayerNativeState::RecurrentZeroing;
   history=layerState->phase==PioneerQ38ResidentLayerNativeState::HistoryZeroing;
   closing=layerState->phase==PioneerQ38ResidentLayerNativeState::RecurrentClosing
    || layerState->phase==PioneerQ38ResidentLayerNativeState::HistoryClosing;
  } // TEST observes the actual NEW private lease;never creates a seal.
#endif
  if(gdn || history || closing){
   auto* model=q38MutableContext->q38EmbeddingModel;unsigned region=0,first=0,words=0;
   if(value==0x46800){
    assert(!closing && !publicationCounterFabricOnly);
#ifdef PIONEER_Q38_RESIDENT_LAYER_NATIVE_CONTEXT
    if(layerState)region=(gdn?layerState->recurrentZero.nonce:layerState->historyZero.nonce)-1;
    else
#endif
    region=(gdn?outputState->recurrentZero.nonce:outputState->historyZero.nonce)-1;
    if(gdn){
     if(outputGdnWords.empty()){outputGdnWords.assign(73728*1024,0xa55aa55a);outputGdnTouched.assign(73728,0);}
     auto* g=model->graph.Recurrent(model->identity);unsigned pages=0;
     assert(g && region<672 && g->InitializationPages(model->identity,region,pages)
      && g->InitializeSpan(model->identity,region,0,pages,outputGdnExtents[region]));
     const auto& e=outputGdnExtents[region];assert(e.bank==l1[7] && e.address==l1[5] && e.bytes==l1[4] && e.route==l1[8]);
     first=outputGdnPages;outputGdnFirst[region]=first;words=pages*1024;outputGdnRuns++;
     if(outputGdnRuns==outputGdnFailRun)packetFault=1;
    }else{
     if(outputHistoryWords.empty()){outputHistoryWords.assign(1680*1024,0xa55aa55a);outputHistoryTouched.assign(1680,0);}
     auto* h=model->graph.ConvHistory(model->identity);
     assert(h && region<336 && h->InitializeSpan(model->identity,region,0,5,outputHistoryExtents[region]));
     const auto& e=outputHistoryExtents[region];assert(e.bank==l1[7] && e.address==l1[5] && e.bytes==l1[4] && e.route==l1[8]);
     first=region*5;words=5120;outputHistoryRuns++;if(outputHistoryRuns==outputHistoryFailRun)packetFault=1;
    }
   }
   ZeroControlIO::RuntimeResetWrite(value); // ACTUAL frozen RV32 zero + fresh reads + independent receipts
   if(value==0x46800){
    assert(lastDeviceWords.size()==words);auto& bytes=gdn?outputGdnWords:outputHistoryWords;auto& touched=gdn?outputGdnTouched:outputHistoryTouched;
    for(unsigned i=0;i<words;i++)bytes[first*1024+i]=lastDeviceWords[i];
    for(unsigned i=first;i<first+words/1024;i++)touched[i]++;
    if(gdn){outputGdnPages+=words/1024;if(outputGdnRuns%96==0)printf("NEW_NATIVE_GDN_ACTUAL_ZERO runs%u/pages%u;NO publication before clean CLOSE\n",outputGdnRuns,outputGdnPages);}
    if(outputLoseTeamRun && (gdn?outputGdnRuns:outputHistoryRuns)==outputLoseTeamRun)currentTeam++;
   }
   return;
  }
 }
#endif
#ifdef PIONEER_Q38_RESIDENT_DECODER_NATIVE
 auto* decoderModel=q38MutableContext?q38MutableContext->q38EmbeddingModel:nullptr;
#ifdef PIONEER_Q38_GDN_STATE_INIT_NATIVE
 if(decoderModel && decoderModel->phase==PioneerQ38EmbeddingNativeState::GdnZeroing){
  unsigned first=recurrentPages,words=0;
  if(value==0x46800){
   unsigned region=decoderModel->gdnZero.nonce-1,pages=0;auto* g=decoderModel->graph.Recurrent(decoderModel->identity);
   PioneerQ38GdnBankExtent extent;
   assert(g && region<672 && g->InitializationPages(decoderModel->identity,region,pages)
    && g->InitializeSpan(decoderModel->identity,region,0,pages,extent)
    && extent.address==l1[5] && extent.bank==l1[7] && extent.bytes==l1[4] && extent.route==l1[8]);
#ifdef PIONEER_Q38_GDN_RECURRENT_NATIVE
   recurrentExtents[region]=extent;recurrentFirst[region]=first;
#endif
   words=pages*1024;assert(first+pages<=73728);recurrentRuns++;
   if(recurrentRuns==recurrentFailRun)packetFault=1;
  }
  ZeroControlIO::RuntimeResetWrite(value); // ACTUAL frozen RV32,NEVER publicationCounterFabricOnly
  if(value==0x47800 && recurrentRuns==672 && recurrentCloseFault)view[(0x72000-TT_Q38_SPAN_VIEW_BASE)/4]^=1;
  if(value==0x46800){
   assert(!publicationCounterFabricOnly && lastDeviceWords.size()==words);
   for(unsigned i=0;i<words;i++)recurrentWords[first*1024+i]=lastDeviceWords[i];
   for(unsigned page=first;page<first+words/1024;page++)recurrentTouched[page]++;
   recurrentPages+=words/1024;
   if(recurrentRuns%96==0)printf("NATIVE_GDN_ACTUAL_ZERO runs%u/pages%u;NO ready publication before CLOSE\n",recurrentRuns,recurrentPages);
  }
  return;
 }
#endif
 if(decoderModel && decoderModel->phase==PioneerQ38EmbeddingNativeState::HistoryZeroing){
  unsigned region=decoderModel->historyZero.nonce-1;
  if(value==0x46800){assert(region<336);auto* h=decoderModel->graph.ConvHistory(decoderModel->identity);
   assert(h && h->InitializeSpan(decoderModel->identity,region,0,5,historyExtents[region]));}
  ZeroControlIO::RuntimeResetWrite(value);
  if(value==0x46800){assert(lastDeviceWords.size()==5120);for(unsigned i=0;i<5120;i++)historyWords[region*5120+i]=lastDeviceWords[i];}
  return;
 }
#endif
#ifdef PIONEER_Q38_PROJECTION_NATIVE_FACTORY_FIXTURE
 if(UseProjectionIO()){projectionTransport->Write32(ProjectionNativeTransport::SetReset,0,value);return;}
#endif
#ifdef PIONEER_Q38_LEARNED_RMS_NATIVE
 if(RmsMode()){RmsResetWrite(value);return;}
#endif
 if(!EmbeddingMode()){
  auto* m=q38MutableContext?q38MutableContext->q38EmbeddingModel:nullptr;
  if(m && value==0x46800){
   workspaceBank=m->bank;workspaceBase=m->plan.Base+m->plan.Workspace(m->bank)->offset;projectionLease=&m->zero;
   if(rootStarts+1==zeroFailRun)packetFault=1;
  }
  ProjectionZeroIO::RuntimeResetWrite(value);return;
 }
 Touch();resets.push_back(value);
 if(value==0x87ffff){if(coreReset==0x46800)embeddingStops++;coreReset=value;return;}
 if(value==0x47800){rootHolds++;coreReset=value;return;}
 assert(value==0x46800 && coreReset==0x87ffff);coreReset=value;embeddingStarts++;
 EmbeddingExecutionBridge t;t.l1.resize(73737);
 t.originalTiledWeights=originalEmbeddingWeights;
#if defined(PIONEER_Q38_LEARNED_RMS_NATIVE) || defined(PIONEER_Q38_MIXED_LIVE_NATIVE_FACTORY_FIXTURE)
 t.boundedEmbedding=true;
#endif
 // Emulator transport:clone EXACT installed bytes,not host arithmetic.
 // Raw endpoint copies here are DEVICE traffic,not controller PCI reads.
 for(unsigned i=0;i<73737;i++)t.l1[i]=RawSlot(PioneerQ38EmbeddingSnapshotAddress(i));
 for(unsigned i=0;i<16;i++)t.source[i]=i?source[i]:config;
 for(unsigned i=0;i<10;i++)t.command[i]=command[i];
 for(unsigned i=0;i<8;i++)t.counts[i]=PanelIO::ResidentNocRead(PioneerQ38SpanCounter(i));
 for(unsigned i=0;i<5120;i++)t.resident[i]=wholeWorkspace[i];
 t.failPacket=embeddingPacketFault;ExecuteEmbeddingBridge(embeddingImage,t);embeddingPc=t.pc;
 for(unsigned i=0;i<73737;i++)RawSlot(PioneerQ38EmbeddingSnapshotAddress(i))=t.l1[i];
 for(unsigned i=0;i<10;i++)command[i]=t.command[i];
 counters[0]=t.counts[0];counters[1]=t.counts[1];accepted=t.counts[2];counters[3]=t.counts[3];
 writeCounters[0]=t.counts[4];writeCounters[1]=t.counts[5];writeCounters[3]=t.counts[7];
 for(unsigned i=0;i<5120;i++)wholeWorkspace[i]=t.resident[i];
#if defined(PIONEER_Q38_EMBEDDING_VALIDATION_NATIVE) || defined(PIONEER_Q38_LEARNED_RMS_VALIDATION_NATIVE)
 computedEmbedding=t.resident;
#endif
 if(embeddingSummaryFault<32)RawSlot(TT_Q38_FFN_RESIDENT_SUMMARY+embeddingSummaryFault*4)^=1;
}
#ifdef PIONEER_Q38_MIXED_LIVE_END_FIXTURE
static void dprintf(const char*,...) {} // diagnostic metadata only
#include "native_clock_service.inc"
#include "native_bulk_close.inc"
#endif
#ifdef PIONEER_Q38_MIXED_LIVE_FRESH_FINAL_TEST
static uint32 FinalAuditRead(void* object,MixedFinalAuditEndpoints::Kind kind,unsigned a,unsigned b,unsigned n){
 auto& io=*static_cast<PioneerSessionIO*>(object);assert(&io==activeIO && io.UseMixedLiveIO() && !finalDeleteFailed && !finalPciFailed);
 unsigned count=++finalAuditReads[kind];
 if(int(kind)==finalAuditFailureKind && count==finalAuditFailureOrdinal)return UINT32_MAX;
 using E=MixedFinalAuditEndpoints;
 auto selected=[&](unsigned slot,auto word){for(unsigned w=0;w<3;w++)assert(io.DramTlbRead(slot,w)==word(w));};
 switch(kind){
  case E::Tlb:return io.DramTlbRead(a,b);
  case E::Niu:return io.SourceRead(0);
  case E::Reset:return io.ResetRead();
  case E::Source:return io.SourceRead(a);
  case E::Compute:return io.ComputeNocRead(a);
  case E::Pci:return io.HostPciRead(a,uint16_t(b),uint8(n));
  case E::Scratch:selected(1,BlackholeArcScratchTlbWord);switch(a){case 2:return 5;case 4:return 0;case 11:return 0x10003000;case 12:return 0x10002000;case 13:return 0x10001000;default:assert(false);return 0;}
  case E::Csm:{
   selected(1,BlackholeTelemetryTlbWord);
   if(a==0x10001000)return 0x10000;if(a==0x10001004)return TT_ARC_TELEMETRY_COUNT;
   if(a>=0x10001008 && a<0x10001008+TT_ARC_TELEMETRY_COUNT*4){unsigned i=(a-0x10001008)/4;return (i<<16)|BlackholeTelemetryTag(i);}
   assert(a>=0x10002000 && a<0x10002000+TT_ARC_TELEMETRY_COUNT*4);
   static constexpr uint32 values[]={0x12050001,0,0,0,0,0,0xf7,0x5515,0,0xfff,0,0};return values[(a-0x10002000)/4];
  }
  case E::DramId:{
   const unsigned ys[]={12,15,18,12,15,18,21};
   for(unsigned bank=0;bank<7;bank++){
    bool same=true;for(unsigned w=0;w<3;w++)same&=io.DramTlbRead(1,w)==BlackholeDramFixedTlbWord(0xffa00000,bank<3?18:17,ys[bank],true,w);
    if(!same)continue;assert(a<TT_DRAM_ID_WORDS);uint32 xy=BlackholeDramPhysicalX(bank)|(BlackholeDramPhysicalY(bank)<<6);
    if(a==0)return 0x1c611000|xy;if(a==1)return 0x80000|BlackholeDramBank(bank);return a==2?0xe005:xy;
   }assert(false && "raw DRAM ID requires its actual selected TLB");return 0;
  }
  case E::Inbound:assert(a<2);return a?0:0x2f;
  case E::Sii:selected(0,BlackholeHostSiiTlbWord);assert(a<8);return a?0:0x400000;
  case E::Hdma:selected(0,BlackholeHostDbiTlbWord);assert(a<4);return a%2?3:0;
 }assert(false);return 0;
}
static void FinalAuditTlbWrite(void* object,unsigned slot,unsigned word,uint32 value){
 auto& io=*static_cast<PioneerSessionIO*>(object);assert(&io==activeIO && !finalDeleteFailed && !finalPciFailed);finalAuditWrites++;
 if(slot==1 && int(word)==finalAuditDropRestoreWord && value==finalAuditBeforeTlb[slot][word])return; // raw dropped control write,never a result setter
 io.DramTlbWrite(slot,word,value);
}
static bool FinalFreshAudit(uint32 id){
 auto& io=*activeIO;auto* c=io.q38MutableContext;assert(c && &c->io==&io && finalLockHeld);finalAuditCalls++;
 assert(c->q38Clock.Snapshot().state==TT_ARC_CLOCK_RESTORED && c->q38Clock.Snapshot().after_mhz==800 && !c->q38Clock.RequiresExplicitEnd());
 auto& dram=c->baseline.baseline.baseline.baseline.dram;auto& noc=c->baseline.baseline.baseline.noc;
 assert(&dram==io.dram.target && &noc==io.noc.target);
 MixedFinalAuditEndpoints endpoints{&io,FinalAuditRead,FinalAuditTlbWrite};
 MixedFinalAuditResults results{io.arc,io.telemetry,io.tensix,dram,noc,io.path,io.audit,io.PioneerArcIO::dirty,
  io.telemetryAttempted,io.resetAttempted,io.dramAttempted,io.nocAttempted,io.pathAttempted,io.auditAttempted};
 for(unsigned slot=0;slot<2;slot++)for(unsigned word=0;word<3;word++)finalAuditBeforeTlb[slot][word]=io.DramTlbRead(slot,word);
 bool passed=ExecuteMixedFinalAudit(endpoints,results,id);finalAuditReturned=true;return passed;
}
#endif
struct NativeFactoryFixture {
 PioneerSessionIO io;PanelOwners owner;
 PioneerOutputBufferLease transfer;PioneerBulkUploadLease bulk;PioneerWideOutputLease wide;
 std::vector<uint8> learned=std::vector<uint8>(PioneerResidentBanks::Pages);
 std::unique_ptr<PioneerStoreContext> c;
 PioneerProbeCookie cookie{nullptr};PioneerQ38ProjectionZeroResult zero;PioneerQ38EmbeddingResult result;
 unsigned token,bank;
 NativeFactoryFixture(const char* zeroImage,const char* embedding,unsigned selectedToken=0,unsigned destination=0):io(zeroImage,embedding),owner(io),token(selectedToken),bank(destination){
  assert(PioneerPrepareOutputBuffer(io,owner.host,transfer) && PioneerBulkUploadPrepare(io,owner.dram,owner.noc,owner.host,owner.store,transfer,bulk)
   && PioneerPrepareWideOutput(io,owner.host,transfer,wide));
  auto plan=std::make_unique<PioneerQ38CanonicalModelPlan>();assert(plan->Build());
  if(token<248320)for(unsigned s=0;s<7;s++){
   const auto* p=plan->ProjectionStripe(64,0,s);if(token<p->firstRow || token-p->firstRow>=p->rows)continue;
   unsigned offset=p->offset+(token-p->firstRow)/16*46080,first=p->bank*unsigned(plan->Capacity/4096)+offset/4096;
   for(unsigned i=0;i<((offset%4096)+46080+4095)/4096;i++)learned[first+i]=1;
  }
  c.reset(new PioneerStoreContext(io,owner,bulk,transfer,wide,learned.data()));cookie.context=c.get();
  activeIO=&io;sRuntimeOwner=&cookie;sColdRecoveryRequired=sReloadQuiesced=false;sBankStorageOnly=true;currentThread=42;
#ifdef PIONEER_Q38_MIXED_LIVE_END_FIXTURE
  // Initial simulated baseline proof DATA,not a private clock/producer
  // publication. ACTUAL Begin must create its private lease via messages.
  owner.dram.cores_checked=120;c->baseline.audit.version=1;c->baseline.audit.state=TT_HOST_DMA_AUDIT_PASSED;c->baseline.audit.tlb_restored=1;
  c->q38Span=nullptr;assert(BeginQ38NormalClock(*c));c->q38Span=&c->span;
#endif
 }
 bool Bind(){return BindQ38EmbeddingModel(*c,token,8191,bank);}
 bool Initialize(){return InitializeQ38EmbeddingModel(*c,zero);}
 bool Run(){return RunQ38EmbeddingModel(*c,result);}
 void Unknown(){
  auto* m=c->q38EmbeddingModel;assert(m && m->Unsafe() && !m->Closed() && !m->embedding.Completed()
   && owner.banks.Quarantined() && owner.store.recovery_required && io.HostReadMock::quarantined
   && io.PioneerHostPageIO::quarantined && transfer.quarantined && wide.quarantined && c->q38Clock.quarantined && sColdRecoveryRequired);
  unsigned n=io.touches;assert(!Bind() && !Initialize() && !Run() && !EndFactory(*c) && io.touches==n);
 }
};
#ifndef PIONEER_Q38_EMBEDDING_FACTORY_FIXTURE_ONLY
int main(int argc,char** argv){
 assert(argc==3 || argc==4);setvbuf(stdout,nullptr,_IOLBF,0);
 (void)&Input;(void)&Prepare;(void)&Upload;(void)&Close;(void)&PreparePanel;(void)&Run;(void)&Job;(void)&ClosePanel;(void)&Finish;
 unsigned positives=0,faults=0;
 if(argc==3){
  for(unsigned source=0;source<7;source++){
   NativeFactoryFixture f(argv[1],argv[2],PioneerQ38EmbeddingFirst(source)+PioneerQ38EmbeddingRows(source)-1,(source+3)%7);
   assert(!f.c->q38EmbeddingModel && !f.io.q38EmbeddingMemoryMapAttempted && !f.c->Unsafe());unsigned n=f.io.touches;
   assert(f.Bind() && f.io.touches==n && f.c->q38EmbeddingModel->phase==PioneerQ38EmbeddingNativeState::Bound);
   auto* m=f.c->q38EmbeddingModel;assert(m->graph.CanonicalModel(m->identity)==&m->plan && !m->zero.closed && !m->embedding.Completed());
   assert(f.Initialize() && m->zero.nonce==2 && m->zero.closed && f.io.rootStarts==2 && !m->embedding.Completed());
   for(auto value:f.io.wholeWorkspace)assert(!value);
   std::vector<uint32> saved(73737);for(unsigned i=0;i<73737;i++)saved[i]=f.io.RawSlot(PioneerQ38EmbeddingSnapshotAddress(i));
   auto learned=f.learned;assert(f.Run() && m->Closed() && m->embedding.Completed() && m->phase==PioneerQ38EmbeddingNativeState::Complete
    && f.io.embeddingStarts==1 && f.io.embeddingStops==1 && f.io.mapCalls==1 && !f.c->Unsafe() && EndFactory(*f.c));
   for(unsigned i=0;i<73737;i++)assert(saved[i]==f.io.RawSlot(PioneerQ38EmbeddingSnapshotAddress(i)));
   assert(f.learned==learned && !f.io.pciWrites && !f.io.HostReadMock::bmeWrites && !f.io.wideReleased);
   bool nonzero=false;for(unsigned i=0;i<5120;i++)nonzero|=f.io.wholeWorkspace[i]!=0;assert(nonzero);
   for(unsigned i=5120;i<f.io.wholeWorkspace.size();i++)assert(!f.io.wholeWorkspace[i]);
   printf("PASS ACTUAL native factory sourcebank%u->workspace%u:unclaimed->bind->actual zero2MiB/native callbacks->actual map/QPE1->private close/END,ALL73737 root words restored,learned unchanged,NO BME\n",source,(source+3)%7);positives++;
  }
 }else{
  for(unsigned fault=0;fault<7;fault++){
   NativeFactoryFixture f(argv[1],argv[2]);assert(f.Bind());
   if(fault<2){f.io.zeroFailRun=fault+1;assert(!f.Initialize() && !f.io.embeddingStarts);}
   else{
    assert(f.Initialize());if(fault==2)f.io.failMap=true;if(fault==3)f.io.nullMap=true;
    if(fault==4)f.io.embeddingPacketFault=22;if(fault==5)f.io.embeddingSummaryFault=31;
    if(fault==6)f.c->q38GdnCallThread=-1;
    assert(!f.Run());if(fault==2 || fault==3 || fault==6)assert(!f.io.embeddingStarts);
   }
   f.Unknown();faults++;
  }
 }
 printf("PASS ACTUAL native embedding FACTORY integration positives%u/faults%u;REAL native state/actual source+zero+scope+mapping+retention+END/all instructions,unclaimed banks,NO copied ready facade;SIMULATED_HARDWARE_NOT_CARD_OR_MODEL_SPEED\n",positives,faults);
}
#endif
