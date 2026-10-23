/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_PROJECTION_GRAPH_OWNER_H
#define PIONEER_Q38_PROJECTION_GRAPH_OWNER_H
#if !defined(PIONEER_Q38_PROJECTION_EXECUTION_OWNER) || PIONEER_Q38_PROJECTION_EXECUTION_OWNER != 1 || !defined(PIONEER_Q38_LEARNED_RMS_NATIVE) || PIONEER_Q38_LEARNED_RMS_NATIVE != 1
#error Private complete projection execution owner requires exact RMS producer role
#endif
#include <new>
#include <stddef.h>
#include "PioneerQ38ProjectionGraphMemory.h"
#include "PioneerQ38ProjectionGraphPlan.h"
#include "PioneerQ38ProjectionGraphProof.h"
#include "PioneerQ38LearnedRmsController.h"
// HEAP ONLY. Same workers/code/state across complete first-layer attention
// projections. NO CPU numeric input/output/reference/fallback. This first
// producer is layer0/attn_norm ONLY;later layers/FFN/vocabulary need their
// actual residual/normalized producers,not a caller layer or input epoch.
struct PioneerQ38ProjectionGraphResult {
 uint32_t version=1,phase=0,worker=0,word=0,unsafe=0,prepared=0,runs=0,closed=0;
 uint32_t armed=0,released=0,ready=0,stopped=0,held=0,rootStarts=0,rootHolds=0;
 uint32_t rootReads=0,rootWrites=0,rootReadFlits=0,rootWriteFlits=0;
 PioneerQ38ProjectionGraphMemoryResult memory;
};
class PioneerQ38ProjectionGraphOwner {
 friend class PioneerQ38ResidentDecoderOwner;
 const tt_dram_samples_result* proof=nullptr;PioneerResidentBanks* banks=nullptr;
 PioneerQ38DispatchLease* cohort=nullptr;PioneerQ38ProjectionGraphMemory* memory=nullptr;
 PioneerQ38ProjectionMutableBanks* graph=nullptr;PioneerQ38ProjectionBanks* state=nullptr;
 const PioneerQ38CanonicalModelPlan* canonical=nullptr;const PioneerQ38LearnedRmsController* input=nullptr;
 const uint8_t* ledger=nullptr;unsigned pages=0,token=UINT32_MAX,position=UINT32_MAX,bank=7,role=14,nonce=0;
 PioneerQ38ProjectionIdentity identity={};
 PioneerQ38ProjectionJobs jobs;PioneerQ38ProjectionGraphPlan operation;
 uint32_t command[120][10]={},counts[120][8]={},resets[120]={},sequence[119]={},sources[119][16]={};
 bool running[120]={},started=false,ready=false,unsafe=false,closed=false,traffic=false,verified=false,output=false;
 static unsigned Route(unsigned w){return PioneerWorkerX(w)|(PioneerWorkerY(w)<<6);}
 static bool Role(unsigned r){return r==5 || r==6 || r==10 || r==11;}
 bool MatchesCompleted(const PioneerQ38ProjectionMutableBanks& owner,const PioneerQ38CanonicalModelPlan& plan,
  const PioneerQ38ProjectionIdentity& id,unsigned selectedToken,unsigned selectedPosition,unsigned selectedBank,unsigned selectedRole)const{
  return Ready() && output && !running[0] && graph==&owner && canonical==&plan && identity==id
   && token==selectedToken && position==selectedPosition && bank==selectedBank && role==selectedRole
   && state && state->OwnsExecution(id,this,input); // private resident-output proof,NOT CPU metadata authority
 }
 template<class IO> bool Fail(IO& io,PioneerQ38ProjectionGraphResult& r){
  unsafe=true;ready=output=false;r.unsafe=1;
  if(memory)memory->Fault();
  if(graph)graph->Fault();
  if(banks && cohort)PioneerQ38DispatchQuarantine(*cohort,*banks);
  io.QuarantineHostPage();io.ProjectionGraphRetainHostOwners();return false; // MEMORY ONLY,NO following I/O
 }
 template<class IO> bool Scope(IO& io)const{
  return started && !unsafe && !closed && proof && banks && cohort && memory && graph && state && canonical && input
   && MatchesNativeOwner(*proof,*banks,*cohort,*memory,*graph,*canonical,*input,identity,ledger,pages)
   && io.ProjectionGraphExecutionScope(*this,*memory,*graph,*canonical,*input,identity,ledger,pages);
 }
 template<class IO> bool Guard(IO& io,unsigned w){
  if(w>=120 || !Scope(io) || !io.SelectRuntimeCore(w) || !io.Q38PanelMappingStable() || !io.ProjectionGraphMemoryMappingStable(w)
   || io.ResetRead()!=resets[w] || io.ResetRead()!=resets[w])return false;
  for(unsigned i=0;i<16;i++)if(io.ResidentNocRead(BlackholeDramSourceOffset(i))!=cohort->authority.source[w][i])return false;
  for(unsigned i=0;i<TT_DRAM_SOURCE_WORDS;i++)if(io.SourceRead(i)!=proof->source[i])return false;
  for(unsigned p=0;p<2;p++)if(!PioneerQ38GraphCsrPciQuiet(io,p))return false;
  if(!w && traffic && !verified)return true;
  if(!PioneerQ38GraphCsrIdle(io))return false;
  for(unsigned i=0;i<10;i++)if(io.ResidentNocRead(i*4)!=command[w][i])return false;
  for(unsigned i=0;i<8;i++)if(io.ResidentNocRead(PioneerQ38PanelCounter(i))!=counts[w][i])return false;
  return true;
 }
 template<class IO> bool Image(IO& io,unsigned w){
  for(unsigned i=0;i<PioneerQ38ProjectionGraphCodeWords(w);i++){
   uint32_t a=PioneerQ38ProjectionGraphCodeAddress(w,i),v;
   if(PioneerQ38ProjectionGraphProgramImmutable(w,a)
    && (!io.ProjectionGraphMemoryReadChecked(w,a,v) || v!=PioneerQ38ProjectionGraphProgramWord(w,a)))return false;
  }
  return true;
 }
 template<class IO> bool Stage(IO& io,uint32_t a,const uint32_t* data,unsigned n){
  for(unsigned i=0;i<n;i++)if(!io.ProjectionGraphMemoryWrite(0,a+i*4,data[i]))return false;
  for(unsigned i=0;i<n;i++){uint32_t v;if(!io.ProjectionGraphMemoryReadChecked(0,a+i*4,v) || v!=data[i])return false;}
  return true;
 }
 template<class IO> int Peer(IO& io,unsigned w,bool stop){
  uint32_t seq=sequence[w-1],marker=io.Q38PanelRead(0x7414c);
  if(marker!=TT_Q38_FFN_PERSISTENT_MARKER && (stop || seq || (marker && marker!=0xdeadbeef)))return -1;
  bool done=false;
  if(marker==TT_Q38_FFN_PERSISTENT_MARKER){
   if(io.Q38PanelRead(0x74148))return -1;
   uint32_t end=io.Q38PanelRead(0x74144),begin=io.Q38PanelRead(0x74140),op=io.Q38PanelRead(0x74150);
   if(end>seq || begin>seq || end>begin || (op && (!stop || op!=2)))return -1;
   for(unsigned i=5;i<16;i++)if(io.Q38PanelRead(0x74140+i*4))return -1;
   done=begin==seq && end==seq && op==(stop?2U:0U);
  }
  bool terminal=true;
  for(unsigned i=0;i<2;i++){io.Pause();uint32_t pc=io.RuntimePcRead();
   if(!PioneerQ38ProjectionGraphExecutableAddress(w,pc) || pc==TT_Q38_PEER_FAULT_ENTRY || pc==TT_Q38_PEER_FAULT_PARK
    || (!stop && PioneerQ38PeerTerminalPc(pc)))return -1;
   terminal &= stop?PioneerQ38PeerTerminalPc(pc):pc==TT_Q38_PEER_WAIT_LOAD || pc==TT_Q38_PEER_WAIT_BRANCH;
  }
  return done && terminal?1:0;
 }
 template<class IO> bool PeerFrame(IO& io,unsigned w){
  for(unsigned i=0;i<8;i++)if(io.Q38PanelRead(0x74040+i*4)!=(i==0?unsigned(TT_Q38_FFN_PERSISTENT_MARKER):i==1?w:i==2?Route(w):i==3?cohort->authority.source[w][0]:0))return false;
  return io.Q38PanelRead(0x74100)==sequence[w-1];
 }
 template<class IO> bool WaitPeers(IO& io,bool stop,PioneerQ38ProjectionGraphResult& r){
  bool done[119]={};unsigned finished=0;uint64_t began=io.Q38CohortNow();if(began>UINT64_MAX-UINT64_C(30000000))return false;
  for(unsigned attempt=0;attempt<65536 && finished<119;attempt++){
   uint64_t now=io.Q38CohortNow();if(now<began || now-began>=UINT64_C(30000000))return false;
   for(unsigned w=1;w<120;w++)if(!done[w-1]){
    r.worker=w;if(!Guard(io,w))return false;
    int p=Peer(io,w,stop);if(p<0)return false;
    if(p==1){if(!Image(io,w) || !PeerFrame(io,w) || !Guard(io,w))return false;done[w-1]=true;finished++;if(stop)r.stopped++;else r.ready++;}
   }
   io.Pause();
  }
  return finished==119;
 }
 bool Manifest(unsigned selected,bool stop){
  if(!Role(selected) || nonce>0x7ffffff0)return false;
  // Rebuild ONLY between known-clean ROOT entries,never while a peer job
  // may still access a prior manifest. These are private HEAP DATA objects.
  operation.~PioneerQ38ProjectionGraphPlan();new(&operation) PioneerQ38ProjectionGraphPlan;
  jobs.~PioneerQ38ProjectionJobs();new(&jobs) PioneerQ38ProjectionJobs;
  return jobs.Build(*canonical,0,selected,position) && jobs.LearnedPagesPresent(ledger,pages)
   && graph->MatchInitializedProjection(identity,jobs,0,selected,position,ledger,pages)
   && operation.Build(*canonical,jobs,0,selected,position,bank,identity,cohort->authority.source[0],sources,sequence,nonce,stop?0:1,stop);
 }
 void Advance(bool stop){
  const auto* m=operation.Manifest();unsigned k=m[44],type=m[48],blocks=k>>(type==8?5:8),tile=type==8?576:type==12?2304:3392;
  for(unsigned w=1;w<120;w++){
   sequence[w-1]=PioneerQ38ProjectionGraphNextPeer(m,w-1);if(stop)continue;
   uint32_t reads=0,flits=0,writes=0,writeFlits=0;
   if(w<=k/256){reads=1;flits=16;writes=2;writeFlits=10;
    const uint32_t last[]={0x90e00,0,Route(w),m[4]+TT_Q38_PROJECTION_LINES_OFFSET+(w-1)*128,0,m[8],0,0x212,128,0};
    for(unsigned i=0;i<10;i++)command[w][i]=last[i];}
   for(unsigned wave=0;wave<jobs.Waves();wave++)if(w<=jobs.WaveWorkers(wave)){
    const auto& p=*jobs.Part(wave*119+w-1);reads+=p.groups*blocks+(k*2+16383)/16384+1;
    flits+=p.groups*blocks*tile/64+(k*2+((blocks*4+63)&~63U))/64;
    const uint32_t last[]={uint32_t(canonical->Base+p.offset+(p.groups*blocks-1)*tile),0,PioneerQ38ProjectionRoute(p.bank),0x90000,0,Route(w),0,0,tile,0};
    for(unsigned i=0;i<10;i++)command[w][i]=last[i];
   }
   for(unsigned i=0;i<8;i++)counts[w][i]+=i==0 || i==3?reads:i==1?flits:i==2 || i==6?reads+writes:i==5?writeFlits:writes;
  }
 }
 template<class IO> bool RootTerminal(IO& io){
  const auto* m=operation.Manifest();
  if(io.RuntimeStatusRead(2)!=TT_Q38_PROJECTION_GRAPH_MARKER || io.RuntimeStatusRead(0)!=0x7200c || io.RuntimeStatusRead(1)!=(m[5]==1?m[45]:119U))return false;
  for(unsigned i=0;i<2;i++){io.Pause();if(!PioneerQ38RootTerminalPc(io.RuntimePcRead()))return false;}return true;
 }
 template<class IO> bool RootProof(IO& io,PioneerQ38ProjectionGraphResult& r){
  r.word=1000;if(!Guard(io,0))return false;
  r.word=1001;if(!RootTerminal(io))return false;
  r.word=1002;if(!PioneerQ38GraphCsrIdle(io))return false;
  r.word=1003;if(!Image(io,0))return false;
  const auto* m=operation.Manifest();uint32_t stats[64],after[8];
  for(unsigned i=0;i<TT_Q38_PROJECTION_GRAPH_WORDS;i++){
   r.word=10000+i;
   uint32_t a,b;if(!io.ProjectionGraphMemoryReadChecked(0,TT_Q38_PROJECTION_GRAPH_MANIFEST+i*4,a)
    || a!=m[i] || !io.ProjectionGraphMemoryReadChecked(0,TT_Q38_PROJECTION_GRAPH_SHADOW+i*4,b) || b!=m[i])return false;
  }
  for(unsigned i=0;i<64;i++)stats[i]=io.Q38PanelRead(TT_Q38_PROJECTION_GRAPH_STATS+i*4);
  for(unsigned i=0;i<8;i++)after[i]=io.ResidentNocRead(PioneerQ38PanelCounter(i));
  r.word=2001;if(!PioneerQ38ProjectionGraphReceipt(stats,m,counts[0],after))return false;
  for(unsigned w=0;w<119;w++){r.word=3000+w;if(io.Q38PanelRead(TT_Q38_PROJECTION_GRAPH_SEQUENCE+w*4)!=PioneerQ38ProjectionGraphNextPeer(m,w))return false;}
  unsigned lastWorker=m[5]==2?119:PioneerQ38ProjectionGraphWorkers(m,m[7]-1);
  const uint32_t last[]={0xffb20228,0,Route(lastWorker),0x75008,0,129,0,0,4,0};
  for(unsigned i=0;i<10;i++){r.word=4000+i;if(io.ResidentNocRead(i*4)!=last[i])return false;}
  for(unsigned i=0;i<8;i++)counts[0][i]=after[i];
  for(unsigned i=0;i<10;i++)command[0][i]=last[i];
  verified=true;r.rootReads=stats[6];r.rootWrites=stats[7];r.rootReadFlits=stats[8];r.rootWriteFlits=stats[9];r.word=5000;return Guard(io,0);
 }
 template<class IO> bool Execute(IO& io,bool stop,PioneerQ38ProjectionGraphResult& r){
  const auto* m=operation.Manifest();r.phase=6;r.worker=0;
  if(!m || !Guard(io,0) || !Stage(io,TT_Q38_PROJECTION_GRAPH_MANIFEST,m,TT_Q38_PROJECTION_GRAPH_WORDS))return Fail(io,r);
  for(unsigned i=0;i<3;i++){io.RuntimeStatusWrite(i,0);if(io.RuntimeStatusRead(i))return Fail(io,r);}
  if(!Guard(io,0))return Fail(io,r);
  r.phase=7;verified=false;running[0]=cohort->rootRunning=cohort->worker[0].image.running=true;
  resets[0]=0x87ffff;io.RuntimeResetWrite(resets[0]);io.Pause();if(!Guard(io,0))return Fail(io,r);
  uint64_t began=io.Q38CohortNow();if(began>UINT64_MAX-UINT64_C(30000000))return Fail(io,r);
  traffic=true;resets[0]=0x46800;r.rootStarts++;io.RuntimeResetWrite(resets[0]);bool done=false;
  for(unsigned attempt=0;attempt<65536;attempt++){
   if(!Guard(io,0))return Fail(io,r);
   uint64_t now=io.Q38CohortNow();if(now<began || now-began>=UINT64_C(30000000))return Fail(io,r);
   uint32_t marker=io.RuntimeStatusRead(2);if(marker && marker!=TT_Q38_PROJECTION_GRAPH_MARKER)return Fail(io,r);
   if(marker){if(!RootProof(io,r))return Fail(io,r);done=true;break;}
   io.Pause();uint32_t pc=io.RuntimePcRead();if(!PioneerQ38ProjectionGraphExecutableAddress(0,pc) || (pc>=0x72060 && pc<0x72080))return Fail(io,r);
  }
  if(!done)return Fail(io,r);
  Advance(stop);
  // STOP owes ALL119 independent physical terminals BEFORE FIRST hold.
  if(!WaitPeers(io,stop,r) || !Guard(io,0) || !RootTerminal(io))return Fail(io,r);
  resets[0]=0x87ffff;io.RuntimeResetWrite(resets[0]);io.Pause();if(!Guard(io,0))return Fail(io,r);
  resets[0]=0x47800;io.RuntimeResetWrite(resets[0]);io.Pause();if(!Guard(io,0))return Fail(io,r);
  running[0]=cohort->rootRunning=cohort->worker[0].image.running=false;traffic=false;r.rootHolds++;nonce=m[13];return true;
 }
public:
 PioneerQ38ProjectionGraphOwner()=default;
 PioneerQ38ProjectionGraphOwner(const PioneerQ38ProjectionGraphOwner&)=delete;
 PioneerQ38ProjectionGraphOwner& operator=(const PioneerQ38ProjectionGraphOwner&)=delete;
 bool Ready()const{return ready && !unsafe && !closed;}bool Unsafe()const{return unsafe;}bool Closed()const{return closed;}
 void Fault(){unsafe=true;ready=output=false;} // MEMORY ONLY
 bool MatchesNativeOwner(const tt_dram_samples_result& dram,const PioneerResidentBanks& resident,const PioneerQ38DispatchLease& participants,
  const PioneerQ38ProjectionGraphMemory& code,const PioneerQ38ProjectionMutableBanks& mutableOwner,const PioneerQ38CanonicalModelPlan& plan,
  const PioneerQ38LearnedRmsController& producer,const PioneerQ38ProjectionIdentity& id,const uint8_t* initialized,unsigned count)const{
  if(!started || unsafe || closed || proof!=&dram || banks!=&resident || cohort!=&participants || memory!=&code || graph!=&mutableOwner
   || canonical!=&plan || input!=&producer || !(identity==id) || ledger!=initialized || pages!=count || !initialized || count!=PioneerResidentBanks::Pages
   || !state || !state->OwnsExecution(id,this,&producer) || !producer.MatchesCompleted(mutableOwner,plan,id,token,position,bank)
   || !resident.Matches(dram) || mutableOwner.CanonicalModel(id)!=&plan || !mutableOwner.MatchesInitializationOwner(resident,dram,id)
   || code.unsafe || code.closed || code.proof!=&dram || code.banks!=&resident || code.cohort!=&participants || code.plan!=&plan
   || !(code.identity==id) || code.ledger!=initialized || participants.unsafe || participants.closed || participants.prepared!=120
   || participants.bootAttempted || participants.booted || participants.rootRunning!=running[0])return false;
  for(unsigned w=0;w<120;w++){const auto& n=participants.worker[w];const auto& s=n.master?*n.master:n.store;
   if(!n.image.saved || !n.image.ready || n.image.unsafe || n.image.running!=running[w] || !s.ready || s.pending || s.recovery_required || n.noc.recovery_required)return false;}
  return true;
 }
 template<class IO> bool Prepare(IO& io,const tt_dram_samples_result& dram,PioneerResidentBanks& resident,PioneerQ38DispatchLease& participants,
  PioneerQ38ProjectionGraphMemory& code,PioneerQ38ProjectionMutableBanks& mutableOwner,const PioneerQ38CanonicalModelPlan& plan,
  const PioneerQ38LearnedRmsController& producer,const PioneerQ38ProjectionIdentity& id,const uint8_t* initialized,unsigned count,
  unsigned selectedToken,unsigned selectedPosition,unsigned selectedBank,unsigned initialRole,PioneerQ38ProjectionGraphResult& r){
  if(started || unsafe || closed || !id.Valid() || !initialized || count!=PioneerResidentBanks::Pages || code.started || code.unsafe || code.closed
   || !Role(initialRole)
   || !producer.MatchesCompleted(mutableOwner,plan,id,selectedToken,selectedPosition,selectedBank)
   || participants.unsafe || participants.closed || participants.rootRunning || participants.prepared!=120 || participants.bootAttempted || participants.booted
   || !resident.Matches(dram) || r.version!=1 || r.phase || r.worker || r.word || r.unsafe || r.prepared || r.runs || r.closed
   || r.armed || r.released || r.ready || r.stopped || r.held || r.rootStarts || r.rootHolds || r.rootReads || r.rootWrites || r.rootReadFlits || r.rootWriteFlits
   || r.memory.version!=1 || r.memory.started || r.memory.phase || r.memory.worker || r.memory.word || r.memory.unsafe || r.memory.installed
   || r.memory.closed || r.memory.savedWords || r.memory.verifiedOriginalWords || r.memory.installedWords || r.memory.restoredWords)return false;
  for(unsigned i=0;i<16;i++)if(participants.authority.source[0][i]!=producer.source[i])return false; // same physical ROOT source across producer handoff
  if(!jobs.Build(plan,0,initialRole,selectedPosition) || !jobs.LearnedPagesPresent(initialized,count)
   || !mutableOwner.MatchInitializedProjection(id,jobs,0,initialRole,selectedPosition,initialized,count)){
   jobs.~PioneerQ38ProjectionJobs();new(&jobs) PioneerQ38ProjectionJobs;return false; // inert configuration refusal BEFORE claims/I/O
  }
  auto* reservation=mutableOwner.Projection(id);if(!reservation || !reservation->ClaimExecution(id,this,&producer))return false;
  started=true;proof=&dram;banks=&resident;cohort=&participants;memory=&code;graph=&mutableOwner;state=reservation;canonical=&plan;input=&producer;
  identity=id;ledger=initialized;pages=count;token=selectedToken;position=selectedPosition;bank=selectedBank;role=initialRole;r.phase=1;
  if(!code.PrepareHeld(io,dram,resident,participants,plan,id,initialized,count,r.memory))return Fail(io,r);
  for(unsigned w=0;w<120;w++){resets[w]=0x47800;
   for(unsigned i=0;i<10;i++)command[w][i]=code.command[w][i];
   for(unsigned i=0;i<8;i++)counts[w][i]=code.counters[w][i];
   if(w)for(unsigned i=0;i<16;i++)sources[w-1][i]=participants.authority.source[w][i];}
  r.phase=2;
  for(unsigned w=1;w<120;w++){r.worker=w;if(!Guard(io,w) || !Image(io,w))return Fail(io,r);
   const uint32_t boot[]={TT_Q38_FFN_PERSISTENT_MARKER,w,Route(w),participants.authority.source[w][0],0,0,0,0};
   for(unsigned i=0;i<96;i++){unsigned a=0x74000+i*4;uint32_t v=i>=16 && i<24?boot[i-16]:i>=80?0xdeadbeef:0,actual;
    if(!io.ProjectionGraphMemoryWrite(w,a,v) || !io.ProjectionGraphMemoryReadChecked(w,a,actual) || actual!=v)return Fail(io,r);}
   for(unsigned i=0;i<10;i++){command[w][i]=i==5?Route(w):0;io.ResidentNocWrite(i*4,command[w][i]);if(io.ResidentNocRead(i*4)!=command[w][i])return Fail(io,r);}
   if(!Guard(io,w))return Fail(io,r);
  }
  code.launched=true; // BEFORE first reset:held-only Close is NOW permanently forbidden
  r.phase=3;
  for(unsigned w=1;w<120;w++){r.worker=w;if(!Guard(io,w))return Fail(io,r);resets[w]=0x87ffff;r.armed++;io.RuntimeResetWrite(resets[w]);io.Pause();if(!Guard(io,w))return Fail(io,r);}
  for(unsigned w=1;w<120;w++){r.worker=w;if(!Guard(io,w))return Fail(io,r);resets[w]=0x46800;running[w]=participants.worker[w].image.running=true;r.released++;io.RuntimeResetWrite(resets[w]);if(!Guard(io,w))return Fail(io,r);}
  r.phase=4;if(!WaitPeers(io,false,r) || !Guard(io,0))return Fail(io,r);
  for(unsigned w=1;w<120;w++)if(!Stage(io,TT_Q38_PROJECTION_GRAPH_SOURCES+(w-1)*64,sources[w-1],16))return Fail(io,r);
  ready=true;r.prepared=1;return true;
 }
 template<class IO> bool Run(IO& io,unsigned selected,PioneerQ38ProjectionGraphResult& r){
  if(!Ready() || running[0] || !r.prepared || r.closed || r.unsafe || !Role(selected))return false;
  if(!Scope(io))return Fail(io,r);
  if(!Manifest(selected,false))return false; // configuration refused before device IO
  output=false;if(!Execute(io,false,r))return false;
  role=selected;output=true;r.runs++;return true;
 }
 template<class IO> bool Close(IO& io,PioneerQ38ProjectionGraphResult& r){
  if(!Ready() || running[0] || !r.prepared || r.closed || r.unsafe)return false;
  if(!Scope(io) || !Manifest(role,true))return Fail(io,r);
  output=false;if(!Execute(io,true,r))return false;r.phase=8;
  for(unsigned w=1;w<120;w++){r.worker=w;if(!Guard(io,w) || !Image(io,w) || !PeerFrame(io,w) || Peer(io,w,true)!=1)return Fail(io,r);
   resets[w]=0x87ffff;io.RuntimeResetWrite(resets[w]);io.Pause();if(!Guard(io,w))return Fail(io,r);
   resets[w]=0x47800;io.RuntimeResetWrite(resets[w]);io.Pause();if(!Guard(io,w))return Fail(io,r);
   running[w]=cohort->worker[w].image.running=false;r.held++;}
  // Never call CloseHeldNoLaunch after compute. This owner independently
  // proved STOP/ALL physical terminals/holds and now restores the entire
  // original footprint with its OWN current post-compute counters/commands.
  r.phase=9;
  for(unsigned w=0;w<120;w++){r.worker=w;if(!Guard(io,w) || !Image(io,w))return Fail(io,r);
   for(unsigned i=0;i<PioneerQ38ProjectionGraphSnapshotWords(w);i++){r.word=i;uint32_t a=PioneerQ38ProjectionGraphSnapshotAddress(w,i),v;
    uint32_t original=memory->saved[PioneerQ38ProjectionGraphSnapshotBase(w)+i];
    if(!io.ProjectionGraphMemoryWrite(w,a,original) || !io.ProjectionGraphMemoryReadChecked(w,a,v) || v!=original)return Fail(io,r);
    r.memory.restoredWords++;}
   for(unsigned i=0;i<10;i++){command[w][i]=memory->command[w][i];io.ResidentNocWrite(i*4,command[w][i]);if(io.ResidentNocRead(i*4)!=command[w][i])return Fail(io,r);}
   if(!Guard(io,w) || !PioneerQ38GraphCsrCode(io,w))return Fail(io,r);
  }
  r.phase=10;
  for(unsigned w=0;w<120;w++){r.worker=w;if(!Guard(io,w) || !memory->VerifyOriginal(io,w,r.memory) || !Guard(io,w))return Fail(io,r);}
  if(r.held!=119 || r.stopped!=119 || r.memory.restoredWords!=8134072 || r.memory.verifiedOriginalWords!=2*8134072
   || !Guard(io,0) || !state->CloseExecution(identity,this,input))return Fail(io,r);
  closed=memory->closed=true;memory->installed=false;ready=false;r.closed=r.memory.closed=1;return true;
 }
};
static_assert(sizeof(PioneerQ38ProjectionGraphOwner)>65536 && sizeof(PioneerQ38ProjectionGraphOwner)<131072,"persistent projection execution owner HEAP ONLY");
#endif
