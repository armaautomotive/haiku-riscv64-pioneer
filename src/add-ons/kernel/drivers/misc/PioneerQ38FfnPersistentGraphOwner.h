/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_FFN_PERSISTENT_GRAPH_OWNER_H
#define PIONEER_Q38_FFN_PERSISTENT_GRAPH_OWNER_H
#if !defined(PIONEER_Q38_FFN_GRAPH_INPUT_ROOT) || PIONEER_Q38_FFN_GRAPH_INPUT_ROOT != 1 || !defined(PIONEER_Q38_FFN_PERSISTENT_GRAPH) || PIONEER_Q38_FFN_PERSISTENT_GRAPH != 1
#error Exact QFG2 initial-F32-producing full FFN code required
#endif
#include <stddef.h>
#include "PioneerQ38FfnGraphCodeOwner.h"
#include "PioneerQ38FfnInputGraphImages.h"
#include "PioneerQ38FfnGraphShape.h"
#include "PioneerQ38GraphMutableBanks.h"
// PRIVATE HEAP ONLY. ACTUAL persistent full-FFN lifecycle,not the tag34
// STOP-only qualifier. Save/install/boot119 ONCE;multiple complete QFG2
// initial-input/gate/up/SiLU/down runs;STOP+ALL terminals before first hold
// and exact restoration ONLY at Close. No native selector is implied.
// IO.FfnGraphScope must bind the SAME real cookie/global-lock/thread,
// model/upload ledger/graph/code/clock/ALL host+maps,excluding ALL other
// writers,including old matrix/GDN paths. This private callback is NOT a
// public ready flag or source authentication. The native adapter is owed.
// No CPU learned products or intermediate numeric reference validation.
struct PioneerQ38FfnPersistentGraphResult {
 uint32_t version=1,phase=0,worker=0,word=0,unsafe=0,prepared=0,runs=0,closed=0;
 uint32_t savedWords=0,installedWords=0,restoredWords=0,savedFrames=0,restoredFrames=0;
 uint32_t savedMetadata=0,restoredMetadata=0,savedStack=0,restoredStack=0;
 uint32_t armed=0,released=0,ready=0,stopped=0,held=0,rootStarts=0,rootHolds=0;
 uint32_t rootReads=0,rootWrites=0,rootReadFlits=0,rootWriteFlits=0;
};
class PioneerQ38FfnPersistentGraphOwner {
 PioneerQ38FfnGraphCodeOwner* code=nullptr;
 const tt_dram_samples_result* proof=nullptr;
 PioneerResidentBanks* banks=nullptr;
 PioneerQ38DispatchLease* cohort=nullptr;
 PioneerQ38GraphMutableBanks* graph=nullptr;
 const PioneerQ38CanonicalModelPlan* plan=nullptr;
 PioneerQ38FfnBanks* state=nullptr;
 const uint8_t* learned=nullptr;unsigned learnedPages=0;
 PioneerQ38FfnIdentity identity={};
 uint32_t resets[120]={},frames[119][96]={},metadata[12288]={},stack[2048]={};
 uint32_t command[120][10]={},counters[120][8]={},sequence[119]={};
 uint32_t manifest[TT_Q38_FFN_GRAPH_WORDS]={},sources[119*16]={},stats[64]={},input[5120]={},output[5120]={};
 bool running[120]={},bootCommand[120]={};
 bool started=false,ready=false,unsafe=false,closed=false,rootTraffic=false,rootVerified=false,outputReady=false;
 uint32_t nonce=0;unsigned layer=64;uint64_t position=0;
 static unsigned Route(unsigned w){return PioneerWorkerX(w)|(PioneerWorkerY(w)<<6);}
 template<class IO> bool Fail(IO& io,PioneerQ38FfnPersistentGraphResult& r) {
  unsafe=true;ready=outputReady=false;r.unsafe=1;
  if(code)code->unsafe=true;
  if(state)state->Fault();
  if(graph)graph->Fault();
  if(cohort && banks)PioneerQ38DispatchQuarantine(*cohort,*banks);
  io.QuarantineHostPage();io.FfnGraphRetainHostOwners();return false; // MEMORY ONLY;NO following IO/STOP/retry
 }
 template<class IO> bool Scope(IO& io)const {
  return started && !unsafe && !closed && code && !code->unsafe && !code->closed && proof && banks && cohort && graph && plan && state
   && code->proof==proof && code->banks==banks && code->cohort==cohort && !cohort->unsafe && !cohort->closed
   && !cohort->bootAttempted && !cohort->booted && cohort->prepared==120 && cohort->rootRunning==running[0]
   && graph->CanonicalModel(identity)==plan && graph->Ffn(identity)==state
   && graph->MatchesInitializationOwner(*banks,*proof,identity) && state->OwnsGraphLease(this)
   && state->InitializationPublished(identity) && io.FfnGraphScope(*this,*graph,*plan,identity,learned,learnedPages);
 }
 template<class IO> bool Guard(IO& io,unsigned w) {
  if(w>=120 || !Scope(io) || !banks->Matches(*proof))return false;
  auto& n=cohort->worker[w];const auto& s=n.Store();
  if(!n.image.saved || !n.image.ready || n.image.unsafe || n.image.running!=running[w] || !s.ready
   || s.pending || s.recovery_required || n.noc.recovery_required || !io.SelectRuntimeCore(w)
   || !io.Q38PanelMappingStable() || !io.FfnGraphCodeMappingStable(w)
   || io.ResetRead()!=resets[w] || io.ResetRead()!=resets[w])return false;
  for(unsigned i=0;i<16;i++)if(io.ResidentNocRead(BlackholeDramSourceOffset(i))!=cohort->authority.source[w][i])return false;
  for(unsigned i=0;i<TT_DRAM_SOURCE_WORDS;i++)if(io.SourceRead(i)!=proof->source[i])return false;
  for(unsigned p=0;p<2;p++)if(!PioneerQ38GraphCsrPciQuiet(io,p))return false;
  if(!w && rootTraffic && !rootVerified)return true; // trusted ROOT owns commands/counters until physical terminal proof
  if(!PioneerQ38GraphCsrIdle(io))return false;
  if(w<code->sampled){
   for(unsigned i=0;i<10;i++)if(io.ResidentNocRead(i*4)!=command[w][i])return false;
   for(unsigned i=0;i<8;i++)if(io.ResidentNocRead(PioneerQ38PanelCounter(i))!=counters[w][i])return false;
  }
  return true;
 }
 template<class IO> bool Image(IO& io,unsigned w,bool full=false) {
  for(unsigned i=0;i<PioneerQ38FfnGraphCodeWords(w);i++) {
   uint32_t a=PioneerQ38FfnGraphCodeAddress(w,i),value;
   if((full || PioneerQ38FfnGraphProgramImmutable(w,a))
    && (!io.FfnGraphProgramReadChecked(w,a,value) || value!=PioneerQ38FfnInputGraphProgramWord(w,a)))return false;
  }
  return true;
 }
 template<class IO> bool Stage(IO& io,uint32_t a,const uint32_t* data,unsigned words) {
  for(unsigned i=0;i<words;i++){io.Q38PanelWrite(a+i*4,data[i]);if(io.Q38PanelRead(a+i*4)!=data[i])return false;}return true;
 }
 template<class IO> int PeerStatus(IO& io,unsigned w,bool stop) {
  const uint32_t seq=sequence[w-1];uint32_t marker=io.Q38PanelRead(0x7414c);
  if(marker!=TT_Q38_FFN_PERSISTENT_MARKER && (stop || seq || (marker && marker!=0xdeadbeef)))return -1;
  bool done=false;
  if(marker==TT_Q38_FFN_PERSISTENT_MARKER){
   if(io.Q38PanelRead(0x74148))return -1;
   uint32_t end=io.Q38PanelRead(0x74144),begin=io.Q38PanelRead(0x74140);
   if(end>seq || begin>seq || end>begin)return -1;
   uint32_t op=io.Q38PanelRead(0x74150);if(op && (!stop || op!=2))return -1;
   for(unsigned i=5;i<16;i++)if(io.Q38PanelRead(0x74140+i*4))return -1;
   done=begin==seq && end==seq && op==(stop?2U:0U);
  }
  bool terminal=true;
  for(unsigned i=0;i<2;i++){io.Pause();uint32_t pc=io.RuntimePcRead();
   if(!PioneerQ38FfnInputGraphExecutableAddress(w,pc) || pc==TT_Q38_PEER_FAULT_ENTRY || pc==TT_Q38_PEER_FAULT_PARK
    || (!stop && PioneerQ38PeerTerminalPc(pc)))return -1;
   terminal &= stop?PioneerQ38PeerTerminalPc(pc):pc==TT_Q38_PEER_WAIT_LOAD || pc==TT_Q38_PEER_WAIT_BRANCH;
  }
  return done && terminal?1:0;
 }
 template<class IO> bool PeerFrame(IO& io,unsigned w,bool stop) {
  for(unsigned i=0;i<8;i++)if(io.Q38PanelRead(0x74040+i*4)!=(i==0?unsigned(TT_Q38_FFN_PERSISTENT_MARKER):i==1?w:i==2?Route(w):i==3?cohort->authority.source[w][0]:0))return false;
  if(stop)for(unsigned i=0;i<16;i++)if(io.Q38PanelRead(0x74000+i*4)!=(i==10?unsigned(TT_Q38_FFN_PERSISTENT_MARKER):i==11?w:i==12?sequence[w-1]:i==13?2:0))return false;
  return io.Q38PanelRead(0x74100)==sequence[w-1];
 }
 template<class IO> bool WaitPeers(IO& io,bool stop,PioneerQ38FfnPersistentGraphResult& r) {
  bool done[119]={};unsigned count=0;uint64_t began=io.Q38CohortNow();if(began>UINT64_MAX-UINT64_C(30000000))return false;
  for(unsigned attempt=0;attempt<65536 && count<119;attempt++){
   uint64_t now=io.Q38CohortNow();if(now<began || now-began>=UINT64_C(30000000))return false;
   for(unsigned w=1;w<120;w++)if(!done[w-1]){r.worker=w;if(!Guard(io,w))return false;
    int status=PeerStatus(io,w,stop);if(status<0)return false;
    if(status==1){if(!Image(io,w) || !PeerFrame(io,w,stop) || !Guard(io,w))return false;
     done[w-1]=true;count++;if(stop)r.stopped++;else r.ready++;}}
   io.Pause();
  }
  return count==119;
 }
 bool Manifest(const PioneerQ38FfnCanonicalJobs& jobs,unsigned selected,uint64_t token) {
  if(!graph->MatchInitializedFfn(identity,jobs,selected,token,learned,learnedPages) || nonce>0x7ffffff8)return false;
  PioneerQ38FfnExtent workspace;
  if(!state->InitializeSpan(identity,0,0,74,workspace))return false;
  for(auto& v:manifest)v=0;
  auto* d=manifest;d[0]=TT_Q38_FFN_GRAPH_MARKER;d[1]=2;d[2]=67108864;d[3]=workspace.bank;d[4]=workspace.address;
  d[5]=1;d[6]=nonce;d[7]=3;d[8]=workspace.route;d[9]=129;d[10]=TT_Q38_FFN_GRAPH_MANIFEST+256;
  d[11]=4352;d[12]=TT_Q38_FFN_GRAPH_MANIFEST+768;d[13]=nonce+5;d[14]=1;
  for(unsigned i=0;i<16;i++)d[16+i]=cohort->authority.source[0][i];
  const uint64_t fields[]={identity.session,identity.sequence,identity.modelEpoch,identity.driverGeneration};
  for(unsigned i=0;i<4;i++){d[32+i*2]=uint32_t(fields[i]);d[33+i*2]=uint32_t(fields[i]>>32);}
  d[40]=uint32_t(token);d[41]=uint32_t(token>>32);d[42]=selected;
  for(unsigned w=0;w<119;w++)d[64+w]=sequence[w];
  for(unsigned role=0;role<3;role++){
   const auto* j=jobs.Job(role);if(!j || j->peers!=119 || j->type!=12)return false;
   auto* m=d+PioneerQ38FfnGraphRoleOffset(role);m[0]=role;m[1]=j->type;m[2]=119;m[3]=role==2?17408:5120;m[4]=role==2?5120:17408;
   for(unsigned w=0;w<119;w++){auto* p=m+16+w*8;p[0]=uint32_t(PioneerResidentBanks::Base+j->part[w].offset);
    p[1]=j->part[w].bank;p[2]=j->part[w].groups;p[4]=PioneerQ38SpanRoute(p[1]);p[5]=sources[w*16];m[16+119*8+w]=j->first[w];}
  }
  return PioneerQ38FfnGraphManifest(d,sources);
 }
 unsigned OutputPackets(unsigned role)const {
  const auto* m=manifest+PioneerQ38FfnGraphRoleOffset(role);unsigned row=0;
  for(unsigned w=0;w<119;w++){if(m[16+119*8+w]!=row)return 119;row+=m[16+w*8+2]*16;}
  return (m[4]*4+16383)/16384;
 }
 template<class IO> bool RootTerminal(IO& io) {
  if(io.RuntimeStatusRead(2)!=TT_Q38_FFN_GRAPH_MARKER || io.RuntimeStatusRead(0)!=0x7200c
   || io.RuntimeStatusRead(1)!=(manifest[5]==1?5120U:119U))return false;
  for(unsigned i=0;i<2;i++){io.Pause();if(!PioneerQ38RootTerminalPc(io.RuntimePcRead()))return false;}return true;
 }
 template<class IO> bool RootProof(IO& io,PioneerQ38FfnPersistentGraphResult& r) {
  if(!Guard(io,0) || !RootTerminal(io) || !PioneerQ38GraphCsrIdle(io))return false;
  // ROOT verifies original/shadow/full sources/ALL peer physical proofs
  // itself. This first implementation additionally checks exact captured
  // manifest bytes. It never reads peer numeric inputs/results on the CPU.
  for(unsigned i=0;i<TT_Q38_FFN_GRAPH_WORDS;i++){
   if(io.Q38PanelRead(TT_Q38_FFN_GRAPH_MANIFEST+i*4)!=manifest[i]
    || io.Q38PanelRead(TT_Q38_FFN_GRAPH_SHADOW+i*4)!=manifest[i])return false;
  }
  const bool work=manifest[5]==1;
  const unsigned writes=work?982+OutputPackets(0)+OutputPackets(1)+OutputPackets(2):238;
  // Input gather reads20+68 scale lines and1+3 contiguous operand chunks,
  // NOT two packets per producer. Matrix/producer packet overhead is
  // fixed;only the recorded status/physical-WAIT polls vary.
  const unsigned writeFlits=work?4065:238,minWaits=work?652:357;
  const unsigned constant=work?119*16+88*(16+7+1+16+7)+3*119*(7+1+16+7)+8+92+2:7378;
  for(unsigned i=0;i<64;i++){
   r.word=i;
   uint32_t v=io.Q38PanelRead(TT_Q38_FFN_GRAPH_STATS+i*4);stats[i]=v;
   if(i==0){if(v!=TT_Q38_FFN_GRAPH_MARKER)return false;}
   else if(i==1){if(v!=2)return false;}
   else if(i==2){if(v!=manifest[5])return false;}
   else if(i==6){if(v<constant+4*minWaits || v>constant+4*minWaits*65536U)return false;}
   else if(i==7){if(v!=writes)return false;}
   else if(i==8){if(v!=stats[6]+(work?5714:0))return false;}
   else if(i==9){if(v!=writeFlits)return false;}
   else if(i==10){if(v<minWaits || v>minWaits*65536U)return false;}
   else if(i==11){if(v<minWaits || v>stats[10] || (!work && v!=357)
    || stats[6]<stats[10]+3*v+constant || stats[6]>4*stats[10]+constant)return false;}
   else if(i==12 || i==54){if(v!=manifest[13])return false;}
   else if(i>=16 && i<24){if(v!=counters[0][i-16])return false;}
   else if(i>=24 && i<32){unsigned c=i-24,delta=c==0 || c==3?stats[6]:c==1?stats[8]:c==2 || c==6?stats[6]+stats[7]:c==5?stats[9]:stats[7];
    if(v!=uint32_t(counters[0][c]+delta))return false;}
   else if(i>=32 && i<48){if(v!=manifest[16+i-32])return false;}
   else if(i==48){if(v!=unsigned(work))return false;}
   else if(i==49){if(v!=(work?8U:1U))return false;}
   else if(i>=50 && i<=53){const unsigned expect[]={3,20,68,20480};if(v!=(work?expect[i-50]:0U))return false;}
   else if(i==55){if(v!=manifest[42])return false;}
   else if(i==56){if(v!=1)return false;}
   else if(i>=57){const unsigned receipt[]={0x32494651,20480,2,2,manifest[13],manifest[40],manifest[41]};if(v!=(work?receipt[i-57]:0U))return false;}
   else if(v)return false;
  }
  for(unsigned w=0;w<119;w++)if(io.Q38PanelRead(TT_Q38_FFN_GRAPH_SEQUENCE+w*4)!=PioneerQ38FfnGraphNextPeer(manifest,w))return false;
  const uint32_t last[]={0xffb20228,0,Route(119),0x75008,0,129,0,0,4,0};
  for(unsigned i=0;i<10;i++)if(io.ResidentNocRead(i*4)!=last[i])return false;
  for(unsigned i=0;i<8;i++)if(io.ResidentNocRead(PioneerQ38PanelCounter(i))!=stats[24+i])return false;
  for(unsigned i=0;i<10;i++)command[0][i]=last[i];
  for(unsigned i=0;i<8;i++)counters[0][i]=stats[24+i];
  rootVerified=true;r.rootReads=stats[6];r.rootWrites=stats[7];r.rootReadFlits=stats[8];r.rootWriteFlits=stats[9];return Guard(io,0);
 }
 void AdvancePeers(bool stop) {
  for(unsigned w=1;w<120;w++){
   sequence[w-1]=PioneerQ38FfnGraphNextPeer(manifest,w-1);bootCommand[w]=false;
   if(stop)continue;
   uint32_t reads=0,flits=0,writes=0,writeFlits=0;
   if(w<=20){reads++;flits+=16;writes+=2;writeFlits+=9;}
   if(w<=68){reads+=2;flits+=32;writes+=2;writeFlits+=9;}
   for(unsigned role=0;role<3;role++){
    const auto* m=manifest+PioneerQ38FfnGraphRoleOffset(role);const auto* p=m+16+(w-1)*8;
    unsigned k=m[3],blocks=k/256;reads+=p[2]*blocks+(k*2+16383)/16384+1;
    const unsigned tile=m[1]==12?2304:3392;
    flits+=p[2]*blocks*tile/64+(k*2+((blocks*4+63)&~63U))/64;
    if(role==2){const uint32_t last[]={p[0]+(p[2]*blocks-1)*tile,0,p[4],0x90000,0,Route(w),0,0,tile,0};
     for(unsigned i=0;i<10;i++)command[w][i]=last[i];}
   }
   for(unsigned i=0;i<8;i++)counters[w][i]+=i==0 || i==3?reads:i==1?flits:i==2 || i==6?reads+writes:i==5?writeFlits:writes;
  }
 }
 template<class IO> bool Execute(IO& io,PioneerQ38FfnPersistentGraphResult& r) {
  r.phase=6;r.worker=0;
  if(!Guard(io,0) || !Stage(io,TT_Q38_FFN_GRAPH_MANIFEST,manifest,TT_Q38_FFN_GRAPH_WORDS))return Fail(io,r);
  auto& store=cohort->worker[0].Store();store.l1_dirty=true;store.l1_restored=false;
  for(unsigned i=0;i<3;i++){io.RuntimeStatusWrite(i,0);if(io.RuntimeStatusRead(i))return Fail(io,r);}
  if(!Guard(io,0))return Fail(io,r);
  r.phase=7;rootVerified=false;running[0]=cohort->worker[0].image.running=cohort->rootRunning=true;
  resets[0]=0x87ffff;io.RuntimeResetWrite(resets[0]);io.Pause();if(!Guard(io,0))return Fail(io,r);
  uint64_t began=io.Q38CohortNow();if(began>UINT64_MAX-UINT64_C(30000000))return Fail(io,r);
  rootTraffic=true;resets[0]=0x46800;r.rootStarts++;io.RuntimeResetWrite(resets[0]);
  bool complete=false;
  for(unsigned attempt=0;attempt<65536;attempt++){
   if(!Guard(io,0))return Fail(io,r);
   uint64_t now=io.Q38CohortNow();if(now<began || now-began>=UINT64_C(30000000))return Fail(io,r);
   uint32_t marker=io.RuntimeStatusRead(2);if(marker && marker!=TT_Q38_FFN_GRAPH_MARKER)return Fail(io,r);
   if(marker){if(!RootProof(io,r))return Fail(io,r);complete=true;break;}
   io.Pause();uint32_t pc=io.RuntimePcRead();if(!PioneerQ38FfnInputGraphExecutableAddress(0,pc) || (pc>=0x72060 && pc<0x72080))return Fail(io,r);
  }
  if(!complete)return Fail(io,r);
  AdvancePeers(manifest[5]==2);
  // ROOT is physically terminal before holds. STOP additionally owes ALL
  //119 independent physical terminals BEFORE the FIRST participant hold.
  if(manifest[5]==2 && !WaitPeers(io,true,r))return Fail(io,r);
  r.worker=0;if(!Guard(io,0) || !RootTerminal(io))return Fail(io,r);
  resets[0]=0x87ffff;io.RuntimeResetWrite(resets[0]);io.Pause();if(!Guard(io,0))return Fail(io,r);
  resets[0]=0x47800;io.RuntimeResetWrite(resets[0]);io.Pause();if(!Guard(io,0))return Fail(io,r);
  running[0]=cohort->worker[0].image.running=cohort->rootRunning=false;r.rootHolds++;nonce=manifest[13];return true;
 }
public:
 PioneerQ38FfnPersistentGraphOwner()=default;
 PioneerQ38FfnPersistentGraphOwner(const PioneerQ38FfnPersistentGraphOwner&)=delete;
 PioneerQ38FfnPersistentGraphOwner& operator=(const PioneerQ38FfnPersistentGraphOwner&)=delete;
 bool Unsafe()const{return unsafe;}
 bool Ready()const{return ready && !unsafe && !closed;}
 bool Closed()const{return closed;}
 bool RootRunning()const{return running[0];}
 // CPU-only structural check for the ACTUAL native context. This is NOT
 // an IO grant:its adapter still owes cookie/global-call seal,clock,ALL
 // hosts/mappings/competing writers and physical guards. In particular,
 // never set the OLD dispatch booted flag to excuse these NEW running
 // peers;compare against this private owner's actual evolving state.
 bool MatchesNativeOwner(const tt_dram_samples_result& dram,const PioneerResidentBanks& owner,
  const PioneerQ38DispatchLease& participants,const PioneerQ38FfnGraphCodeOwner& codeOwner,
  const PioneerQ38GraphMutableBanks& mutableOwner,const PioneerQ38CanonicalModelPlan& canonical,
  const PioneerQ38FfnIdentity& id,const uint8_t* initialized,unsigned pages)const {
  if(!started || unsafe || closed || proof!=&dram || banks!=&owner || cohort!=&participants || code!=&codeOwner
   || graph!=&mutableOwner || plan!=&canonical || !(identity==id) || learned!=initialized || learnedPages!=pages
   || !initialized || pages!=PioneerResidentBanks::Pages || codeOwner.proof!=proof || codeOwner.banks!=banks
   || codeOwner.cohort!=cohort || codeOwner.Unsafe() || codeOwner.Closed()
   || participants.unsafe || participants.closed || participants.bootAttempted || participants.booted || participants.prepared!=120
   || participants.rootRunning!=running[0] || !owner.Matches(dram) || mutableOwner.CanonicalModel(id)!=&canonical
   || !mutableOwner.MatchesInitializationOwner(owner,dram,id) || !state || !state->OwnsGraphLease(this)
   || !state->InitializationPublished(id))return false;
  for(unsigned w=0;w<120;w++){
   const auto& n=participants.worker[w];const auto& s=n.master?*n.master:n.store;
   if(!n.image.saved || !n.image.ready || n.image.unsafe || n.image.running!=running[w]
    || !s.ready || s.pending || s.recovery_required || n.noc.recovery_required)return false;
  }
  return true;
 }
 template<class IO> bool Prepare(IO& io,const tt_dram_samples_result& dram,PioneerResidentBanks& owner,
  PioneerQ38DispatchLease& participants,PioneerQ38FfnGraphCodeOwner& codeOwner,PioneerQ38GraphMutableBanks& mutableOwner,
  const PioneerQ38CanonicalModelPlan& canonical,const PioneerQ38FfnIdentity& id,const uint8_t* initialized,unsigned pages,
  const PioneerQ38FfnCanonicalJobs& initialJobs,unsigned selected,uint64_t token,PioneerQ38FfnPersistentGraphResult& r) {
  if(started || unsafe || closed || code || codeOwner.started || codeOwner.unsafe || codeOwner.closed || codeOwner.proof
   || codeOwner.banks || codeOwner.cohort || participants.unsafe || participants.closed || participants.rootRunning
   || participants.prepared!=120 || participants.bootAttempted || participants.booted || !owner.Matches(dram)
   || mutableOwner.CanonicalModel(id)!=&canonical || !mutableOwner.MatchesInitializationOwner(owner,dram,id)
   || !mutableOwner.MatchInitializedFfn(id,initialJobs,selected,token,initialized,pages) || r.version!=1
   || r.phase || r.worker || r.word || r.unsafe || r.prepared || r.runs || r.closed || r.savedWords || r.installedWords
   || r.restoredWords || r.savedFrames || r.restoredFrames || r.savedMetadata || r.restoredMetadata || r.savedStack
   || r.restoredStack || r.armed || r.released || r.ready || r.stopped || r.held || r.rootStarts || r.rootHolds
   || r.rootReads || r.rootWrites || r.rootReadFlits || r.rootWriteFlits)return false;
  auto* ffn=mutableOwner.Ffn(id);if(!ffn || !ffn->ClaimGraph(id,this))return false;
  started=true;code=&codeOwner;proof=&dram;banks=&owner;cohort=&participants;graph=&mutableOwner;plan=&canonical;
  state=ffn;identity=id;learned=initialized;learnedPages=pages;layer=selected;position=token;
  code->started=true;code->proof=proof;code->banks=banks;code->cohort=cohort;
  for(auto& reset:resets)reset=0x47800;
  r.phase=1;
  for(unsigned w=0;w<120;w++){r.worker=w;if(!Guard(io,w) || !PioneerQ38GraphCsrCode(io,w))return Fail(io,r);
   for(unsigned i=0;i<10;i++)command[w][i]=code->command[w][i]=io.ResidentNocRead(i*4);
   for(unsigned i=0;i<8;i++){counters[w][i]=code->counters[w][i]=io.ResidentNocRead(PioneerQ38PanelCounter(i));
    if(i==6 && counters[w][2]!=counters[w][6])return Fail(io,r);}
   code->sampled=w+1;
   for(unsigned i=0;i<PioneerQ38FfnGraphCodeWords(w);i++){r.word=i;uint32_t value;
    if(!io.FfnGraphProgramReadChecked(w,PioneerQ38FfnGraphCodeAddress(w,i),value))return Fail(io,r);
    code->saved[PioneerQ38FfnGraphCodeBase(w)+i]=value;r.savedWords++;}
   if(w){for(unsigned i=0;i<96;i++){frames[w-1][i]=io.Q38PanelRead(0x74000+i*4);r.savedFrames++;}}
   else{for(unsigned i=0;i<12288;i++){metadata[i]=io.Q38PanelRead(0x74000+i*4);r.savedMetadata++;}
    for(unsigned i=0;i<2048;i++){stack[i]=io.Q38PanelRead(0x6e000+i*4);r.savedStack++;}}
   if(!Guard(io,w))return Fail(io,r);
  }
  if(r.savedWords!=867448 || r.savedFrames!=11424 || r.savedMetadata!=12288 || r.savedStack!=2048)return Fail(io,r);
  r.phase=2;
  for(unsigned w=0;w<120;w++){r.worker=w;if(!Guard(io,w))return Fail(io,r);
   for(unsigned i=0;i<PioneerQ38FfnGraphCodeWords(w);i++){r.word=i;uint32_t value,a=PioneerQ38FfnGraphCodeAddress(w,i);
    if(!io.FfnGraphProgramReadChecked(w,a,value) || value!=code->saved[PioneerQ38FfnGraphCodeBase(w)+i])return Fail(io,r);}
   for(unsigned i=0;i<PioneerQ38FfnGraphCodeWords(w);i++){r.word=i;uint32_t a=PioneerQ38FfnGraphCodeAddress(w,i),actual;
    if(!io.FfnGraphProgramWrite(w,a,PioneerQ38FfnInputGraphProgramWord(w,a)) || !io.FfnGraphProgramReadChecked(w,a,actual)
     || actual!=PioneerQ38FfnInputGraphProgramWord(w,a))return Fail(io,r);
    r.installedWords++;}
   if(!Guard(io,w))return Fail(io,r);
  }
  r.phase=3;for(unsigned w=0;w<120;w++){r.worker=w;if(!Guard(io,w) || !Image(io,w,true))return Fail(io,r);}
  r.phase=4;
  for(unsigned w=1;w<120;w++){r.worker=w;if(!Guard(io,w))return Fail(io,r);
   uint32_t boot[]={TT_Q38_FFN_PERSISTENT_MARKER,w,Route(w),cohort->authority.source[w][0],0,0,0,0},zero[16]={},sentinel[16];
   for(auto& v:sentinel)v=0xdeadbeef;
   if(!Stage(io,0x74040,boot,8) || !Stage(io,0x74000,zero,16) || !Stage(io,0x74100,zero,16) || !Stage(io,0x74140,sentinel,16))return Fail(io,r);
   for(unsigned i=0;i<10;i++){command[w][i]=i==5?Route(w):0;io.ResidentNocWrite(i*4,command[w][i]);if(io.ResidentNocRead(i*4)!=command[w][i])return Fail(io,r);}
   bootCommand[w]=true;if(!Guard(io,w))return Fail(io,r);
  }
  r.phase=5;
  for(unsigned w=1;w<120;w++){r.worker=w;if(!Guard(io,w))return Fail(io,r);resets[w]=0x87ffff;r.armed++;io.RuntimeResetWrite(resets[w]);io.Pause();if(!Guard(io,w))return Fail(io,r);}
  for(unsigned w=1;w<120;w++){r.worker=w;if(!Guard(io,w))return Fail(io,r);running[w]=cohort->worker[w].image.running=true;resets[w]=0x46800;r.released++;io.RuntimeResetWrite(resets[w]);if(!Guard(io,w))return Fail(io,r);}
  if(!WaitPeers(io,false,r))return Fail(io,r);
  for(unsigned w=1;w<120;w++)for(unsigned i=0;i<16;i++)sources[(w-1)*16+i]=cohort->authority.source[w][i];
  if(!Manifest(initialJobs,selected,token) || !Guard(io,0))return Fail(io,r);
  for(unsigned w=1;w<120;w++)if(!Stage(io,0x7d800+(w-1)*64,cohort->authority.source[w],16))return Fail(io,r);
  ready=true;r.prepared=1;return true;
 }
 template<class IO> bool Run(IO& io,const PioneerQ38FfnCanonicalJobs& jobs,unsigned selected,uint64_t token,
  const uint32_t (&values)[5120],PioneerQ38FfnPersistentGraphResult& r) {
  if(!Ready() || running[0] || !r.prepared || r.closed || r.unsafe)return false;
  if(!Scope(io))return Fail(io,r);
  if(!graph->MatchInitializedFfn(identity,jobs,selected,token,learned,learnedPages))return false; // configuration,NO IO
  for(unsigned i=0;i<5120;i++)if((values[i]&0x7f800000U)==0x7f800000U)return false;
  for(unsigned i=0;i<5120;i++)input[i]=values[i];
  if(!Manifest(jobs,selected,token))return false;
  outputReady=false;r.phase=6;r.worker=0;
  if(!Guard(io,0))return Fail(io,r);
  cohort->worker[0].image.dirty=true; // outer saved panel ALSO owns the staged input/output scratch
  if(!Stage(io,0x80000,input,5120) || !Execute(io,r))return Fail(io,r);
  layer=selected;position=token;outputReady=true;r.runs++;return true;
 }
 // Complete final F32 result ONLY;gate/up/activation/quantization stay on
 // card. Atomic host publication after all finite/wire/scope checks. This
 // is not a CPU learned-product reference or an arbitrary L1 read API.
 template<class IO> bool CopyOutput(IO& io,uint32_t (&destination)[5120],PioneerQ38FfnPersistentGraphResult& r) {
  if(!Ready() || !outputReady || running[0])return false;
  if(!Scope(io))return Fail(io,r);
  if(!Guard(io,0))return Fail(io,r);
  // Root L1 is compact WORKER order. GDDR was already scattered into
  // canonical row order by the real root. Reconstruct the HOST boundary
  // only from this same authorized down manifest,never assume workers
  // own consecutive semantic rows (canonical bank round-robin does not).
  const auto* down=manifest+PioneerQ38FfnGraphRoleOffset(2);unsigned packed=0;
  for(unsigned w=0;w<119;w++){
   unsigned first=down[16+119*8+w],rows=down[16+w*8+2]*16;
   for(unsigned row=0;row<rows;row++){uint32_t v=io.Q38PanelRead(0x80000+(packed+row)*4);
    if((v&0x7f800000U)==0x7f800000U)return Fail(io,r);
    output[first+row]=v;}
   packed+=rows;
  }
  if(!Guard(io,0))return Fail(io,r);
  for(unsigned i=0;i<5120;i++)destination[i]=output[i];
  return true;
 }
 template<class IO> bool Close(IO& io,PioneerQ38FfnPersistentGraphResult& r) {
  if(!Ready() || running[0] || !r.prepared || r.closed || r.unsafe)return false;
  if(!Scope(io))return Fail(io,r);
  outputReady=false;manifest[5]=2;manifest[6]=nonce;manifest[13]=nonce+1;manifest[14]=0;
  for(unsigned w=0;w<119;w++)manifest[64+w]=sequence[w];
  if(!PioneerQ38FfnGraphManifest(manifest,sources) || !Execute(io,r))return Fail(io,r);
  r.phase=8;
  for(unsigned w=1;w<120;w++){r.worker=w;if(!Guard(io,w) || !Image(io,w) || !PeerFrame(io,w,true) || PeerStatus(io,w,true)!=1)return Fail(io,r);
   resets[w]=0x87ffff;io.RuntimeResetWrite(resets[w]);io.Pause();if(!Guard(io,w))return Fail(io,r);
   resets[w]=0x47800;io.RuntimeResetWrite(resets[w]);io.Pause();if(!Guard(io,w))return Fail(io,r);
   running[w]=cohort->worker[w].image.running=false;r.held++;}
  r.phase=9;
  for(unsigned w=0;w<120;w++){r.worker=w;if(!Guard(io,w) || !Image(io,w))return Fail(io,r);
   for(unsigned i=0;i<PioneerQ38FfnGraphCodeWords(w);i++){r.word=i;uint32_t a=PioneerQ38FfnGraphCodeAddress(w,i),value=code->saved[PioneerQ38FfnGraphCodeBase(w)+i],actual;
    if(!io.FfnGraphProgramWrite(w,a,value) || !io.FfnGraphProgramReadChecked(w,a,actual) || actual!=value)return Fail(io,r);
    r.restoredWords++;}
   if(w){if(!Stage(io,0x74000,frames[w-1],96))return Fail(io,r);r.restoredFrames+=96;}
   else{if(!Stage(io,0x74000,metadata,12288) || !Stage(io,0x6e000,stack,2048))return Fail(io,r);r.restoredMetadata=12288;r.restoredStack=2048;}
   for(unsigned i=0;i<10;i++){command[w][i]=code->command[w][i];io.ResidentNocWrite(i*4,command[w][i]);if(io.ResidentNocRead(i*4)!=command[w][i])return Fail(io,r);}
   bootCommand[w]=false;if(!Guard(io,w) || !PioneerQ38GraphCsrCode(io,w))return Fail(io,r);
  }
  r.phase=10;
  for(unsigned w=0;w<120;w++){r.worker=w;if(!Guard(io,w))return Fail(io,r);
   for(unsigned i=0;i<PioneerQ38FfnGraphCodeWords(w);i++){r.word=i;uint32_t actual;
    if(!io.FfnGraphProgramReadChecked(w,PioneerQ38FfnGraphCodeAddress(w,i),actual) || actual!=code->saved[PioneerQ38FfnGraphCodeBase(w)+i])return Fail(io,r);}
   for(unsigned i=0;i<(w?96U:12288U);i++)if(io.Q38PanelRead(0x74000+i*4)!=(w?frames[w-1][i]:metadata[i]))return Fail(io,r);
   if(!w)for(unsigned i=0;i<2048;i++)if(io.Q38PanelRead(0x6e000+i*4)!=stack[i])return Fail(io,r);
  }
  if(r.held!=119 || r.stopped!=119 || r.restoredWords!=867448 || r.restoredFrames!=11424 || r.restoredMetadata!=12288 || r.restoredStack!=2048
   || !Guard(io,0) || !state->CloseGraph(identity,this))return Fail(io,r);
  closed=code->closed=true;ready=false;r.closed=1;return true;
 }
};
static_assert(sizeof(PioneerQ38FfnPersistentGraphOwner)>150000 && sizeof(PioneerQ38FfnPersistentGraphOwner)<196608,"complete persistent graph owner is HEAP ONLY");
#endif
