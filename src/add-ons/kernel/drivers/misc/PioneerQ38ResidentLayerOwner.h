/* SPDX-License-Identifier: MIT */
#include "PioneerQ38ResidentLayerOwnerRole.h"
#if !defined(PIONEER_Q38_DECODER_RUNTIME) || PIONEER_Q38_DECODER_RUNTIME != 1 || !defined(PIONEER_Q38_GDN_MUTABLE_STATE)
#error Resident-layer execution needs the complete genuine joint runtime reservation
#endif
#if defined(PIONEER_Q38_RESIDENT_DECODER_OWNER) || defined(PIONEER_Q38_RESIDENT_DECODER_NATIVE) || defined(PIONEER_Q38_GDN_RECURRENT_NATIVE) || defined(PIONEER_Q38_GDN_INPUT_OWNER)
#error New complete resident-layer lifetime is NOT an old per-worker decoder alias
#endif
#ifndef PIONEER_Q38_RESIDENT_LAYER_OWNER_H
#define PIONEER_Q38_RESIDENT_LAYER_OWNER_H
#include "PioneerQ38ProjectionGraphOwner.h"
#include "PioneerQ38ResidentLayerMemory.h"
#include "PioneerQ38ResidentLayerPlan.h"
#include "PioneerQ38ResidentLayerProof.h"
// HEAP ONLY. Full NEW first-layer Conv/GDN/output/residual+RMS/FFN/final-residual lifetime.
// This is the actual execution/STOP/restore controller,not a boot-only smoke test.
// A genuine NEW native state/factory/IO scope and original-GGUF qualification
// remain REQUIRED. No existing driver or old68 context installs/selects it.
// A separate genuine native factory/IO scope/mapping implementation remains
// mandatory. The driver intentionally refuses this prototype role. No
// public request, fabricated ready bit or CPU arithmetic can mint its input.
// GDN/history initialization MUST finish before the actual QPS1 producer;
// accepted physical source/command/counters are checked before FIRST write.
// Full execution qualification is owed before any native/card selection.
struct PioneerQ38ResidentLayerResult {
 uint32_t version=1,phase=0,worker=0,word=0,started=0,prepared=0,unsafe=0,completed=0,closed=0;
 uint32_t armed=0,released=0,waiting=0,stopped=0,held=0,rootStarts=0,rootHolds=0;
 PioneerQ38ResidentLayerMemoryResult memory;
};
class PioneerQ38ResidentLayerOwner {
 PioneerQ38ProjectionGraphOwner* projection=nullptr;
 PioneerQ38ResidentProjectionInput* input=nullptr;
 PioneerQ38ResidentLayerMemory* memory=nullptr;
 PioneerQ38ProjectionGraphMemory* previousMemory=nullptr;
 PioneerQ38ConvHistoryBanks* history=nullptr;PioneerQ38GdnBanks* recurrent=nullptr;
 const tt_dram_samples_result* proof=nullptr;PioneerResidentBanks* banks=nullptr;PioneerQ38DispatchLease* cohort=nullptr;
 PioneerQ38ProjectionMutableBanks* graph=nullptr;const PioneerQ38CanonicalModelPlan* canonical=nullptr;
 const uint8_t* ledger=nullptr;PioneerQ38GdnIdentity identity={};
 PioneerQ38ResidentLayerPlan operation;
 uint32_t sources[120][16]={},command[120][10]={},counts[120][8]={},before[120][8]={},resets[120]={},matrixBefore[119][8]={},downBefore[119][8]={};
 uint32_t activeFfn[TT_Q38_FFN_RESIDUAL_GRAPH_WORDS]={},handoff[TT_Q38_RESIDENT_LAYER_RECEIPT_WORDS]={},savedRecurrent[68][32]={};
 bool running[120]={},accepted[120]={},stopped[119]={};
 uint32_t token=UINT32_MAX,position=UINT32_MAX,bank=7,phase=0;
 uint64_t began=0;bool started=false,ready=false,unsafe=false,closed=false,ran=false,traffic=false,rootVerified=false;
 bool Bound(bool closedState=false)const{
  if(!started || unsafe || closed!=closedState || !projection || !input || !memory || memory->unsafe || memory->closed!=closedState
   || !previousMemory || previousMemory->Unsafe() || !previousMemory->Closed() || !history || !recurrent || !proof || !banks || !cohort
   || !graph || !canonical || !ledger || !identity.Valid() || !canonical->Ready() || !banks->Matches(*proof)
   || projection->Unsafe() || !projection->Closed() || input!=&projection->closedResidentInput || input->owner!=projection || input->consumer || input->layerConsumer!=this
   || input->graph!=graph || input->canonical!=canonical || input->ledger!=ledger || !(input->identity==identity)
   || input->token!=token || input->position!=position || input->bank!=bank || input->phase+1!=phase || position
   || projection->memory!=previousMemory || projection->token!=token || projection->position!=position || projection->bank!=bank || projection->nonce+1!=phase
   || input->rms!=projection->input || !input->rms || !input->rms->Completed() || projection->proof!=proof || projection->banks!=banks
   || projection->cohort!=cohort || projection->graph!=graph || projection->canonical!=canonical || projection->ledger!=ledger
   || !(projection->identity==identity) || input->pages!=PioneerResidentBanks::Pages || projection->pages!=PioneerResidentBanks::Pages
   || graph->CanonicalModel(identity)!=canonical || !graph->MatchesInitializationOwner(*banks,*proof,identity)
   || graph->ConvHistory(identity)!=history || graph->Recurrent(identity)!=recurrent
   || !history->InitializationPublished(identity) || !recurrent->InitializationPublished(identity)
   || history->residentOutputOwner || recurrent->residentOutputOwner
   || history->residentLayerOwner!=this || recurrent->residentLayerOwner!=this
   || history->residentLayerPhase!=phase || recurrent->residentLayerPhase!=phase
   || history->residentLayerNextPublished!=ran || recurrent->residentLayerNextPublished!=ran
   || cohort->unsafe || cohort->closed || cohort->prepared!=120 || cohort->bootAttempted || cohort->booted || cohort->rootRunning!=running[0]
   || (ready && (!memory->started || !memory->installed || !memory->launched)))return false;
  if(closedState && (!ran || !rootVerified || ready || traffic || !memory->started || !memory->launched || memory->installed))return false;
  if(memory->started && (memory->proof!=proof || memory->banks!=banks || memory->cohort!=cohort || memory->plan!=canonical
   || !(memory->identity==identity) || memory->ledger!=ledger))return false;
  const auto* workspace=canonical->Workspace(bank);if(!workspace)return false;const uint32_t base=canonical->Base+workspace->offset;
  if(input->raw!=base+PioneerQ38ProjectionResultOffset(6) || input->z!=base+PioneerQ38ProjectionResultOffset(5)
   || input->a!=base+PioneerQ38ProjectionResultOffset(10) || input->b!=base+PioneerQ38ProjectionResultOffset(11))return false;
  for(unsigned i=0;i<16;i++)if(input->source[i]!=sources[0][i])return false;
  for(unsigned w=0;w<120;w++){
   const auto& n=cohort->worker[w];const auto& s=n.master?*n.master:n.store;
   if(!n.image.saved || !n.image.ready || n.image.unsafe || n.image.running!=running[w] || !s.ready || s.pending || s.recovery_required || n.noc.recovery_required)return false;
   if(closedState && (running[w] || resets[w]!=0x47800 || (w && (!accepted[w] || !stopped[w-1]))))return false;
   for(unsigned i=0;i<16;i++)if(cohort->authority.source[w][i]!=sources[w][i])return false;
  }
  // Deliberately NOT input->Current()/old MatchesClosedNativeOwner: the
  // old record is consumed, and new workers legitimately run after claim.
  return true;
 }
 template<class IO> bool Scope(IO& io)const{
  return Bound() && io.ResidentLayerExecutionScope(*this,*projection,*memory,*graph,*canonical,identity,ledger,PioneerResidentBanks::Pages);
 }
 template<class IO> bool Fail(IO& io,PioneerQ38ResidentLayerResult& r){
  unsafe=true;ready=false;r.unsafe=1;
  if(memory)memory->Fault();
  if(projection)projection->Fault();
  if(graph)graph->Fault();
  if(banks && cohort)PioneerQ38DispatchQuarantine(*cohort,*banks);
  io.QuarantineHostPage();io.ResidentLayerRetainHostOwners();return false; // MEMORY ONLY;NO following IO/reset/restore/retry
 }
 template<class IO> bool Guard(IO& io,unsigned w){
  if(w>=120 || !Scope(io) || !io.SelectRuntimeCore(w) || !io.Q38PanelMappingStable() || !io.ResidentLayerMemoryMappingStable(w)
   || io.ResetRead()!=resets[w] || io.ResetRead()!=resets[w])return false;
  for(unsigned i=0;i<16;i++)if(io.ResidentNocRead(BlackholeDramSourceOffset(i))!=sources[w][i])return false;
  for(unsigned i=0;i<TT_DRAM_SOURCE_WORDS;i++)if(io.SourceRead(i)!=proof->source[i])return false;
  for(unsigned p=0;p<2;p++)if(!PioneerQ38GraphCsrPciQuiet(io,p))return false;
  if(!w && traffic && !rootVerified)return true; // only genuine active ROOT may have in-flight packets
  if(!PioneerQ38GraphCsrIdle(io))return false;
  // Before accepting a completed peer, its NEW arithmetic receipt owes
  // exact continuity from before[w],not the old pre-execution counters.
  if(w && rootVerified && !accepted[w])return true;
  for(unsigned i=0;i<10;i++)if(io.ResidentNocRead(i*4)!=command[w][i])return false;
  for(unsigned i=0;i<8;i++)if(io.ResidentNocRead(PioneerQ38PanelCounter(i))!=counts[w][i])return false;
  return true;
 }
 template<class IO> bool Read(IO& io,unsigned w,uint32_t a,uint32_t* out,unsigned n){
  for(unsigned i=0;i<n;i++)if(!io.ResidentLayerMemoryReadChecked(w,a+i*4,out[i]))return false;
  return true;
 }
 template<class IO> bool Stage(IO& io,unsigned w,uint32_t a,const uint32_t* data,unsigned n){
  for(unsigned i=0;i<n;i++)if(!io.ResidentLayerMemoryWrite(w,a+i*4,data[i]))return false;
  for(unsigned i=0;i<n;i++){uint32_t v;if(!io.ResidentLayerMemoryReadChecked(w,a+i*4,v) || v!=data[i])return false;}return true;
 }
 template<class IO> bool Image(IO& io,unsigned w){
  for(unsigned i=0;i<PioneerQ38ResidentLayerCodeWords(w);i++){
   uint32_t a=PioneerQ38ResidentLayerCodeAddress(w,i),actual,expected;
   if(!PioneerQ38ResidentLayerProgramWord(w,a,expected) || !io.ResidentLayerMemoryReadChecked(w,a,actual) || actual!=expected)return false;
  }
  return true;
 }
 template<class IO> bool Budget(IO& io,uint64_t start){uint64_t now=io.Q38CohortNow();return now>=start && now-start<UINT64_C(30000000);}
 template<class IO> int Peer(IO& io,unsigned w,unsigned sequence,bool stop){
  uint32_t status[16],boot[8];
  if(!Read(io,w,0x74140,status,16))return -1;
  if(status[3]!=TT_Q38_FFN_PERSISTENT_MARKER){
   if(!sequence && !stop && (status[3]==0 || status[3]==0xdeadbeef))return 0;
   return -1;
  }
  if(status[2] || status[0]>sequence || status[1]>status[0] || (stop && (!sequence || status[0]<sequence-1 || status[1]<sequence-1))
   || (status[4] && (!stop || status[4]!=2)))return -1;
  for(unsigned i=5;i<16;i++)if(status[i])return -1;
  uint32_t pc[2];for(unsigned i=0;i<2;i++){
   io.Pause();pc[i]=io.RuntimePcRead();
   if(!PioneerQ38ResidentLayerProgramImmutable(w,pc[i]) || pc[i]==TT_Q38_PEER_FAULT_ENTRY || pc[i]==TT_Q38_PEER_FAULT_PARK
    || (!stop && PioneerQ38PeerTerminalPc(pc[i])))return -1;
  }
  if(status[0]!=sequence || status[1]!=sequence || status[4]!=(stop?2U:0U))return 0;
  for(unsigned i=0;i<2;i++)if(stop?!PioneerQ38PeerTerminalPc(pc[i]):pc[i]!=0x7204c && pc[i]!=0x72050)return 0;
  if(!Read(io,w,0x74040,boot,8) || io.Q38PanelRead(0x74100)!=sequence)return -1;
  for(unsigned i=0;i<8;i++)if(boot[i]!=(i==0?unsigned(TT_Q38_FFN_PERSISTENT_MARKER):i==1?w:i==2?PioneerQ38ConvPeer(w):i==3?sources[w][0]:0))return -1;
  return 1;
 }
 template<class IO> bool InitialPeers(IO& io,PioneerQ38ResidentLayerResult& r){
  bool done[119]={};unsigned finished=0;
  for(unsigned attempt=0;attempt<65536 && finished<119;attempt++){
   if(!Budget(io,began))return false;
   for(unsigned w=1;w<120;w++)if(!done[w-1]){
    r.worker=w;if(!Guard(io,w))return false;int state=Peer(io,w,0,false);if(state<0)return false;
    if(state==1){if(!Image(io,w) || !Guard(io,w))return false;done[w-1]=true;finished++;r.waiting++;}
   }
   io.Pause();
  }
  return finished==119;
 }
 template<class IO> bool RootPhysical(IO& io){
  uint32_t status[3],pc[2];for(unsigned i=0;i<3;i++)status[i]=io.RuntimeStatusRead(i);
  for(unsigned i=0;i<2;i++){io.Pause();pc[i]=io.RuntimePcRead();if(!PioneerQ38ResidentLayerRootTerminalPc(pc[i]))return false;}
  return PioneerQ38ResidentLayerRootPhysical(status,pc,io.ResetRead());
 }
 template<class IO> bool RootReceipt(IO& io,PioneerQ38ResidentLayerResult& r){
  if(!Guard(io,0) || !RootPhysical(io) || !PioneerQ38GraphCsrIdle(io) || !Image(io,0))return false;
  const uint32_t* manifests[]={operation.Manifest(),operation.PostManifest(),operation.LayerManifest()};
  const unsigned addresses[]={TT_Q38_COHORT_MANIFEST,TT_Q38_OUTPUT_COHORT_MANIFEST,TT_Q38_RESIDENT_LAYER_MANIFEST};
  const unsigned shadows[]={TT_Q38_COHORT_SHADOW,TT_Q38_OUTPUT_COHORT_SHADOW,TT_Q38_RESIDENT_LAYER_SHADOW};
  const unsigned words[]={TT_Q38_COHORT_WORDS,TT_Q38_OUTPUT_COHORT_WORDS,TT_Q38_RESIDENT_LAYER_WORDS};
  for(unsigned p=0;p<3;p++)for(unsigned i=0;i<words[p];i++){
   r.word=i;uint32_t v;if(!Read(io,0,addresses[p]+i*4,&v,1) || v!=manifests[p][i]
    || !Read(io,0,shadows[p]+i*4,&v,1) || v!=manifests[p][i])return false;
  }
  if(!Read(io,0,TT_Q38_RESIDENT_LAYER_RECEIPT,handoff,TT_Q38_RESIDENT_LAYER_RECEIPT_WORDS)
   || !Read(io,0,TT_Q38_FFN_RESIDUAL_GRAPH_MANIFEST,activeFfn,TT_Q38_FFN_RESIDUAL_GRAPH_WORDS))return false;
  for(unsigned i=0;i<TT_Q38_FFN_RESIDUAL_GRAPH_WORDS;i++){
   uint32_t v;if(!Read(io,0,TT_Q38_FFN_RESIDUAL_GRAPH_SHADOW+i*4,&v,1) || v!=activeFfn[i])return false;
  }
  uint32_t stats[64],ffn[64],after[8],last[10];
  if(!Read(io,0,TT_Q38_COHORT_STATS,stats,64) || !Read(io,0,TT_Q38_FFN_RESIDUAL_GRAPH_STATS,ffn,64))return false;
  for(unsigned i=0;i<8;i++)after[i]=io.ResidentNocRead(PioneerQ38PanelCounter(i));
  for(unsigned i=0;i<10;i++)last[i]=io.ResidentNocRead(i*4);
  if(!PioneerQ38ResidentLayerRootReceipt(manifests[2],manifests[0],manifests[1],handoff,stats,activeFfn,ffn,before[0],after,last))return false;
  for(unsigned w=1;w<120;w++){
   r.worker=w;uint32_t seq,a;
   if(!Read(io,0,TT_Q38_COHORT_SEQUENCE+(w-1)*4,&seq,1) || seq!=PioneerQ38ResidentLayerSequence(manifests[0],w)
    || !Read(io,0,TT_Q38_FFN_RESIDUAL_GRAPH_SEQUENCE+(w-1)*4,&a,1) || a!=seq
    || !Read(io,0,TT_Q38_COHORT_COUNTERS+(w-1)*32,matrixBefore[w-1],8)
    || !Read(io,0,TT_Q38_FFN_RESIDUAL_GRAPH_COUNTERS+(w-1)*32,downBefore[w-1],8))return false;
   if(w<=68 && !Read(io,0,TT_Q38_RESIDENT_LAYER_GDN_SAVED+(w-1)*128,savedRecurrent[w-1],32))return false;
  }
  for(unsigned i=0;i<8;i++)counts[0][i]=after[i];
  for(unsigned i=0;i<10;i++)command[0][i]=last[i];
  rootVerified=accepted[0]=true;return Guard(io,0);
 }
 template<class IO> bool AcceptPeer(IO& io,unsigned w,PioneerQ38ResidentLayerResult& r){
  if(!Guard(io,w) || Peer(io,w,PioneerQ38ResidentLayerSequence(operation.Manifest(),w),false)!=1 || !Image(io,w))return false;
  uint32_t p[64],first[32]={},state[32]={},gated[32]={},clock[16]={},after[8],last[10];
  if(!Read(io,w,0x83f00,p,64))return false;
  if(w<=96){
   if(!Read(io,w,TT_Q38_GDN_INPUT_SUMMARY,first,32) || !Read(io,w,TT_Q38_GDN_GATED_SUMMARY,gated,32)
    || !Read(io,w,TT_Q38_GDN_RENDEZVOUS_CLOCK,clock,16))return false;
   if(w<=68){for(unsigned i=0;i<32;i++)state[i]=savedRecurrent[w-1][i];}
   else if(!Read(io,w,TT_Q38_RGDN_SUMMARY,state,32))return false;
  }
  for(unsigned i=0;i<8;i++)after[i]=io.ResidentNocRead(PioneerQ38PanelCounter(i));
  for(unsigned i=0;i<10;i++)last[i]=io.ResidentNocRead(i*4);
  if(!PioneerQ38ResidentLayerPeerReceipt(operation.LayerManifest(),operation.Manifest(),operation.PostManifest(),w,before[w],after,last,p,
   matrixBefore[w-1],downBefore[w-1],first,state,gated,clock))return false;
  for(unsigned i=0;i<8;i++)counts[w][i]=after[i];
  for(unsigned i=0;i<10;i++)command[w][i]=last[i];
  accepted[w]=true;r.completed++;return Guard(io,w);
 }
 template<class IO> bool Hold(IO& io,unsigned w){
  if(!Guard(io,w))return false;
  resets[w]=0x87ffff;io.RuntimeResetWrite(resets[w]);io.Pause();if(!Guard(io,w))return false;
  resets[w]=0x47800;io.RuntimeResetWrite(resets[w]);io.Pause();if(!Guard(io,w))return false;
  running[w]=cohort->worker[w].image.running=false;if(!w){cohort->rootRunning=false;traffic=false;}return true;
 }
public:
 PioneerQ38ResidentLayerOwner()=default;
 PioneerQ38ResidentLayerOwner(const PioneerQ38ResidentLayerOwner&)=delete;
 PioneerQ38ResidentLayerOwner& operator=(const PioneerQ38ResidentLayerOwner&)=delete;
 bool Unsafe()const{return unsafe;}bool Closed()const{return closed;}
 bool Claimed()const{return started;} // CPU metadata ONLY,never an IO grant
 bool Completed()const{return ready && ran && !unsafe && !closed;} // private lineage still REQUIRED by every consumer
 bool MatchesNativeOwner(const PioneerQ38ProjectionGraphOwner& producer,const PioneerQ38ResidentLayerMemory& code)const{
  return projection==&producer && memory==&code && Bound(); // CPU binding ONLY,not native scope/IO authority
 }
 bool MatchesClosedNativeOwner(const PioneerQ38ProjectionGraphOwner& producer,const PioneerQ38ResidentLayerMemory& code)const{
  return projection==&producer && memory==&code && Bound(true); // private NEW full-layer lineage,not old input Current
 }
 template<class IO> bool Prepare(IO& io,PioneerQ38ProjectionGraphOwner& producer,PioneerQ38ResidentLayerMemory& code,PioneerQ38ResidentLayerResult& r){
  if(started || unsafe || closed || code.started || code.unsafe || code.closed || code.launched || code.installed
   || !producer.ResidentInput() || r.version!=1 || r.phase || r.worker || r.word || r.started || r.prepared || r.unsafe || r.completed || r.closed
   || r.armed || r.released || r.waiting || r.stopped || r.held || r.rootStarts || r.rootHolds
   || r.memory.version!=1 || r.memory.started || r.memory.phase || r.memory.worker || r.memory.word || r.memory.unsafe || r.memory.installed
   || r.memory.closed || r.memory.savedWords || r.memory.verifiedOriginalWords || r.memory.installedWords || r.memory.restoredWords)return false;
  auto& v=producer.closedResidentInput;auto* h=producer.graph->ConvHistory(v.identity);auto* g=producer.graph->Recurrent(v.identity);
  if(!h || !g || !h->InitializationPublished(v.identity) || !g->InitializationPublished(v.identity)
   || h->residentOutputOwner || g->residentOutputOwner || h->residentLayerOwner || g->residentLayerOwner || v.consumer || v.layerConsumer || v.phase>=0x7fffffef)return false;
  unsigned sequences[119]={};
  if(!operation.Build(*v.canonical,0,v.position,v.bank,0,v.token,v.phase+1,v.identity,&producer.cohort->authority.source[0][0],sequences)
   || !operation.LearnedPagesPresent(v.ledger,v.pages))return false; // inert before claims or physical IO
  projection=&producer;input=&v;memory=&code;previousMemory=producer.memory;history=h;recurrent=g;proof=producer.proof;banks=producer.banks;cohort=producer.cohort;
  graph=producer.graph;canonical=v.canonical;ledger=v.ledger;identity=v.identity;token=v.token;position=v.position;bank=v.bank;phase=v.phase+1;
  v.layerConsumer=this;h->residentLayerOwner=g->residentLayerOwner=this;h->residentLayerPhase=g->residentLayerPhase=phase;started=true;r.started=1;r.phase=1;
  for(unsigned w=0;w<120;w++){
   resets[w]=0x47800;for(unsigned i=0;i<16;i++)sources[w][i]=cohort->authority.source[w][i];
   for(unsigned i=0;i<10;i++)command[w][i]=producer.command[w][i];
   for(unsigned i=0;i<8;i++)counts[w][i]=before[w][i]=producer.counts[w][i];
  }
  // ALL accepted previous physical ledgers BEFORE any new snapshot/install
  // write. Initialization after QPS1 would change counters and fail here.
  for(unsigned w=0;w<120;w++){r.worker=w;if(!Guard(io,w) || !PioneerQ38GraphCsrCode(io,w))return Fail(io,r);}
  if(!code.PrepareHeld(io,*proof,*banks,*cohort,*canonical,identity,ledger,PioneerResidentBanks::Pages,r.memory))return Fail(io,r);
  for(unsigned w=0;w<120;w++){
   for(unsigned i=0;i<10;i++)if(code.command[w][i]!=command[w][i])return Fail(io,r);
   for(unsigned i=0;i<8;i++)if(code.counters[w][i]!=counts[w][i])return Fail(io,r);
  }
  r.phase=2;
  for(unsigned w=1;w<120;w++){
   r.worker=w;if(!Guard(io,w) || !Image(io,w))return Fail(io,r);
   for(unsigned i=0;i<96;i++){
    uint32_t value=i>=16 && i<24?(i==16?unsigned(TT_Q38_FFN_PERSISTENT_MARKER):i==17?w:i==18?PioneerQ38ConvPeer(w):i==19?sources[w][0]:0):i>=80?0xdeadbeef:0;
    if(!Stage(io,w,0x74000+i*4,&value,1))return Fail(io,r);
   }
   for(unsigned i=0;i<10;i++){command[w][i]=i==5?PioneerQ38ConvPeer(w):0;io.ResidentNocWrite(i*4,command[w][i]);if(io.ResidentNocRead(i*4)!=command[w][i])return Fail(io,r);}
   if(!Guard(io,w))return Fail(io,r);
  }
  if(!Guard(io,0) || !Stage(io,0,TT_Q38_COHORT_MANIFEST,operation.Manifest(),TT_Q38_COHORT_WORDS)
   || !Stage(io,0,TT_Q38_OUTPUT_COHORT_MANIFEST,operation.PostManifest(),TT_Q38_OUTPUT_COHORT_WORDS)
   || !Stage(io,0,TT_Q38_RESIDENT_LAYER_MANIFEST,operation.LayerManifest(),TT_Q38_RESIDENT_LAYER_WORDS))return Fail(io,r);
  for(unsigned i=0;i<3;i++){io.RuntimeStatusWrite(i,0);if(io.RuntimeStatusRead(i))return Fail(io,r);}
  if(!Guard(io,0) || !Image(io,0))return Fail(io,r);
  code.launched=true;r.phase=3;
  for(unsigned w=1;w<120;w++){r.worker=w;if(!Guard(io,w))return Fail(io,r);resets[w]=0x87ffff;io.RuntimeResetWrite(resets[w]);r.armed++;io.Pause();if(!Guard(io,w))return Fail(io,r);}
  began=io.Q38CohortNow();if(began>UINT64_MAX-UINT64_C(30000000))return Fail(io,r);
  for(unsigned w=1;w<120;w++){r.worker=w;if(!Guard(io,w) || !Budget(io,began))return Fail(io,r);
   resets[w]=0x46800;running[w]=cohort->worker[w].image.running=true;io.RuntimeResetWrite(resets[w]);r.released++;if(!Guard(io,w))return Fail(io,r);}
  if(!InitialPeers(io,r))return Fail(io,r);
  ready=true;r.prepared=1;r.phase=4;return true;
 }
 template<class IO> bool Run(IO& io,PioneerQ38ResidentLayerResult& r){
  if(!ready || unsafe || closed || ran || running[0] || !r.prepared || r.unsafe || r.closed || r.completed)return false;
  // Recheck the actual retained upload ledger before the first RUN MMIO.
  // A caller cannot remove learned pages between PREPARE and RUN and keep
  // the earlier canonical recipe as a substitute for current ownership.
  if(!Scope(io) || !operation.LearnedPagesPresent(ledger,PioneerResidentBanks::Pages))return Fail(io,r);
  if(!Guard(io,0) || !Budget(io,began))return Fail(io,r);
  for(unsigned i=0;i<3;i++){io.RuntimeStatusWrite(i,0);if(io.RuntimeStatusRead(i))return Fail(io,r);}
  running[0]=cohort->rootRunning=cohort->worker[0].image.running=true;resets[0]=0x87ffff;r.phase=5;io.RuntimeResetWrite(resets[0]);io.Pause();if(!Guard(io,0))return Fail(io,r);
  traffic=true;resets[0]=0x46800;io.RuntimeResetWrite(resets[0]);r.rootStarts++;
  bool done=false;
  for(unsigned attempt=0;attempt<65536;attempt++){
   if(!Guard(io,0) || !Budget(io,began))return Fail(io,r);
   uint32_t marker=io.RuntimeStatusRead(2);
   if(marker && marker!=TT_Q38_RESIDENT_LAYER_MARKER)return Fail(io,r);
   if(marker){if(!RootReceipt(io,r))return Fail(io,r);done=true;break;}
   io.Pause();uint32_t pc=io.RuntimePcRead();if(!PioneerQ38ResidentLayerProgramImmutable(0,pc) || PioneerQ38ResidentLayerRootRejectedPc(pc))return Fail(io,r);
  }
  if(!done)return Fail(io,r);
  r.phase=6;
  for(unsigned w=1;w<120;w++){r.worker=w;if(!Budget(io,began) || !AcceptPeer(io,w,r))return Fail(io,r);}
  if(!Guard(io,0) || !RootPhysical(io) || !Budget(io,began) || !Hold(io,0))return Fail(io,r);
  r.rootHolds++;ran=true;history->residentLayerNextPublished=recurrent->residentLayerNextPublished=true;r.phase=7;return true;
 }
public:
 template<class IO> bool Close(IO& io,PioneerQ38ResidentLayerResult& r){
  if(!Completed() || running[0] || !r.prepared || r.unsafe || r.closed || r.stopped || r.held || r.memory.restoredWords)return false;
  r.phase=8;const auto* m=operation.Manifest();
  for(unsigned w=1;w<120;w++){
   r.worker=w;const unsigned seq=PioneerQ38ResidentLayerSequence(m,w);
   if(!Guard(io,w) || !Image(io,w) || Peer(io,w,seq,false)!=1)return Fail(io,r);
   uint32_t job[16]={};job[10]=TT_Q38_FFN_PERSISTENT_MARKER;job[11]=w;job[12]=seq+1;job[13]=2;
   if(!Stage(io,w,0x74000,job,16) || !Guard(io,w) || !Stage(io,w,0x74100,job+12,1) || !Guard(io,w))return Fail(io,r);
  }
  const uint64_t stopBegan=io.Q38CohortNow();if(stopBegan>UINT64_MAX-UINT64_C(30000000))return Fail(io,r);
  unsigned finished=0;
  for(unsigned attempt=0;attempt<65536 && finished<119;attempt++){
   if(!Budget(io,stopBegan))return Fail(io,r);
   for(unsigned w=1;w<120;w++)if(!stopped[w-1]){
    r.worker=w;if(!Guard(io,w))return Fail(io,r);int state=Peer(io,w,PioneerQ38ResidentLayerSequence(m,w)+1,true);if(state<0)return Fail(io,r);
    if(state==1){if(!Image(io,w) || !Guard(io,w))return Fail(io,r);stopped[w-1]=true;finished++;r.stopped++;}
   }
   io.Pause();
  }
  if(finished!=119)return Fail(io,r);
  // ALL119 independently stopped and unheld BEFORE FIRST peer hold. A
  // held PC is NEVER read to justify cleanup or another participant.
  r.phase=9;
  for(unsigned w=1;w<120;w++){r.worker=w;if(!Guard(io,w) || Peer(io,w,PioneerQ38ResidentLayerSequence(m,w)+1,true)!=1 || !Hold(io,w))return Fail(io,r);r.held++;}
  r.phase=10;
  for(unsigned w=0;w<120;w++){
   r.worker=w;if(!Guard(io,w) || !Image(io,w))return Fail(io,r);
   for(unsigned i=0;i<PioneerQ38ResidentLayerSnapshotWords(w);i++){
    r.word=i;uint32_t original=memory->saved[PioneerQ38ResidentLayerSnapshotBase(w)+i];
    if(!Stage(io,w,PioneerQ38ResidentLayerSnapshotAddress(w,i),&original,1))return Fail(io,r);
    r.memory.restoredWords++;
   }
   for(unsigned i=0;i<10;i++){command[w][i]=memory->command[w][i];io.ResidentNocWrite(i*4,command[w][i]);if(io.ResidentNocRead(i*4)!=command[w][i])return Fail(io,r);}
   if(!Guard(io,w) || !PioneerQ38GraphCsrCode(io,w))return Fail(io,r);
  }
  r.phase=11;
  for(unsigned w=0;w<120;w++){r.worker=w;if(!Guard(io,w) || !memory->VerifyOriginal(io,w,r.memory) || !Guard(io,w))return Fail(io,r);}
  if(r.stopped!=119 || r.held!=119 || r.memory.restoredWords!=TT_Q38_RESIDENT_LAYER_SAVED_WORDS
   || r.memory.verifiedOriginalWords!=2*TT_Q38_RESIDENT_LAYER_SAVED_WORDS || !Guard(io,0))return Fail(io,r);
  closed=memory->closed=true;ready=memory->installed=false;r.closed=r.memory.closed=1;return true;
 }

};
static_assert(sizeof(PioneerQ38ResidentLayerOwner)>524288 && sizeof(PioneerQ38ResidentLayerOwner)<1048576,"complete owner/recipes/physical ledger HEAP ONLY");
#endif
