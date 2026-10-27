/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_RECURRENT_BLOCK_OWNER_H
#define PIONEER_Q38_RECURRENT_BLOCK_OWNER_H
#include "PioneerQ38RecurrentBlockMemory.h"
#include "../../../../../tools/pioneer/PioneerQ38MixedRecurrentBlockCounters.h"
// DISTINCT complete QRB1 owner. NO old dense owner/role alias,CPU numeric
// products,public post-launch restore hint,model/token/state-slot commit.
// The native scope MUST prove the same real completed embedding producer,
// initialized recurrent/history leases,FD/thread/source/clock/ALL hosts.
struct PioneerQ38RecurrentBlockResult {
 uint32_t version=1,started=0,phase=0,worker=0,word=0,unsafe=0,prepared=0,completed=0,stopped=0,held=0,closed=0;
 uint32_t armed=0,released=0,rootStarts=0,rootHolds=0,packets=0;
};
class PioneerQ38RecurrentBlockOwner {
 const tt_dram_samples_result* proof=nullptr;PioneerResidentBanks* banks=nullptr;PioneerQ38DispatchLease* cohort=nullptr;
 PioneerQ38RecurrentBlockMemory* memory=nullptr;const PioneerQ38CanonicalModelPlan* canonical=nullptr;
 const PioneerQ38MixedRecurrentBlockPlan* plan=nullptr;PioneerQ38GdnIdentity identity={};const uint8_t* ledger=nullptr;
 uint32_t counts[120][8]={},command[120][10]={},resets[120]={},stop[128]={};
 unsigned polls[120]={},observingPeer=0,completedPeers=0,heldPeers=0;
 bool running[120]={},started=false,unsafe=false,closed=false,launched=false,traffic=false,ran=false,rootCompleted=false,allStopped=false;
 uint64_t began=0;
 bool Bound()const{
  if(!started || unsafe || closed || !proof || !banks || !cohort || !memory || !canonical || !plan || !ledger || !identity.Valid()
   || !banks->Matches(*proof) || memory->unsafe || memory->closed || !memory->installed || memory->launched!=launched
   || memory->proof!=proof || memory->banks!=banks || memory->cohort!=cohort || memory->plan!=canonical || memory->ledger!=ledger
   || !(memory->identity==identity) || !plan->Matches(*canonical) || cohort->unsafe || cohort->closed || cohort->prepared!=120
   || cohort->bootAttempted || cohort->booted || cohort->rootRunning!=running[0])return false;
  for(unsigned w=0;w<120;w++){
   const auto& n=cohort->worker[w];const auto& s=n.master?*n.master:n.store;
   if(!n.image.saved || !n.image.ready || n.image.unsafe || n.image.running!=running[w] || !s.ready || s.pending
    || s.recovery_required || n.noc.recovery_required)return false;
   for(unsigned i=0;i<16;i++)if(cohort->authority.source[w][i]!=plan->Boot(w)[16+i])return false;
  }
  return true;
 }
 template<class IO> bool Scope(IO& io)const{
  return Bound() && io.RecurrentBlockExecutionScope(*this,*memory,*canonical,*plan,identity,ledger,PioneerResidentBanks::Pages);
 }
 template<class IO> bool Fail(IO& io,PioneerQ38RecurrentBlockResult& r){
  unsafe=true;r.unsafe=1;if(memory)memory->Fault();
  if(cohort && banks)PioneerQ38DispatchQuarantine(*cohort,*banks);
  io.QuarantineHostPage();io.RecurrentBlockRetainHostOwners();return false; // MEMORY ONLY,no later IO/retry/cleanup
 }
 template<class IO> bool Guard(IO& io,unsigned w){
  if(w>=120 || !Scope(io) || !io.SelectRuntimeCore(w) || !io.RecurrentBlockMemoryMappingStable(w)
   || io.ResetRead()!=resets[w] || io.ResetRead()!=resets[w])return false;
  for(unsigned i=0;i<16;i++)if(io.ResidentNocRead(BlackholeDramSourceOffset(i))!=plan->Boot(w)[16+i])return false;
  for(unsigned i=0;i<TT_DRAM_SOURCE_WORDS;i++)if(io.SourceRead(i)!=proof->source[i])return false;
  for(unsigned p=0;p<2;p++)if(!PioneerQ38GraphCsrPciQuiet(io,p))return false;
  if(!w && traffic)return true; // SAME owned ROOT may have genuine in-flight packets
  if(!PioneerQ38GraphCsrIdle(io))return false;
  // Only AFTER the real ROOT completion receipt may one peer's changed
  // counters be observed. ALL8 must then pass the exact bounded packet
  // proof before adopting them;this is NOT arbitrary baseline recapture.
  if(w && observingPeer==w && rootCompleted && !allStopped)return true;
  for(unsigned i=0;i<10;i++)if(io.ResidentNocRead(i*4)!=command[w][i])return false;
  for(unsigned i=0;i<8;i++)if(io.ResidentNocRead(PioneerQ38PanelCounter(i))!=counts[w][i])return false;
  return true;
 }
 template<class IO> bool Read(IO& io,unsigned w,unsigned a,uint32_t* out,unsigned n){
  for(unsigned i=0;i<n;i++)if(!io.RecurrentBlockMemoryReadChecked(w,a+i*4,out[i]))return false;
  return true;
 }
 template<class IO> bool Stage(IO& io,unsigned w,unsigned a,const uint32_t* data,unsigned n){
  for(unsigned i=0;i<n;i++)if(!io.RecurrentBlockMemoryWrite(w,a+i*4,data[i]))return false;
  for(unsigned i=0;i<n;i++){uint32_t actual;if(!io.RecurrentBlockMemoryReadChecked(w,a+i*4,actual) || actual!=data[i])return false;}
  return true;
 }
 template<class IO> bool Image(IO& io,unsigned w){PioneerQ38RecurrentBlockMemoryResult ignored;return memory->VerifyImage(io,w,ignored);}
 template<class IO> bool Budget(IO& io,uint64_t at){uint64_t now=io.Q38CohortNow();return now>=at && now-at<UINT64_C(30000000);}
 template<class IO> bool Packet(IO& io,unsigned address,unsigned bytes,PioneerQ38RecurrentBlockResult& r){
  namespace L=PioneerQ38MixedRecurrentBlockRootLayout;
  if(!bytes || (bytes&63) || bytes>16384 || !Guard(io,0))return false;
  const auto* o=plan->Outer();uint32_t expected[8];
  for(unsigned i=0;i<8;i++)expected[i]=counts[0][i]+(i==5?bytes/64:i==2 || i==4 || i==6 || i==7?1U:0U);
  const uint32_t next[]={L::Scratch,0,129,address,0,o[8],0,0x212,bytes,0};
  for(unsigned i=0;i<10;i++){command[0][i]=next[i];io.ResidentNocWrite(i*4,next[i]);if(io.ResidentNocRead(i*4)!=next[i])return false;}
  const uint64_t at=io.Q38CohortNow();if(at>UINT64_MAX-UINT64_C(30000000))return false;
  traffic=true;io.ResidentNocWrite(0x40,1);r.packets++;bool done=false;
  for(unsigned poll=0;poll<65536;poll++){
   if(!Guard(io,0) || !Budget(io,at))return false;
   done=io.ResidentNocRead(0x40)==0;
   for(unsigned i=0;i<8;i++)done &= io.ResidentNocRead(PioneerQ38PanelCounter(i))==expected[i];
   if(done)break;
   io.Pause();
  }
  if(!done)return false;
  for(unsigned i=0;i<10;i++)if(io.ResidentNocRead(i*4)!=next[i])return false;
  for(unsigned i=0;i<8;i++)counts[0][i]=expected[i];
  traffic=false;return Guard(io,0);
 }
 template<class IO> int Wait(IO& io,unsigned w,unsigned nonce,bool stopped){
  namespace L=PioneerQ38MixedRecurrentBlockRootLayout;namespace M=PioneerQ38MixedWorkerLayout;
  uint32_t status[16];unsigned at=w?unsigned(M::Status):unsigned(L::Status);
  if(!Read(io,w,at,status,16))return -1;
  unsigned magic=w?unsigned(M::Magic):unsigned(L::Magic);
  bool zero=true;for(unsigned i=0;i<16;i++)zero &= !status[i];
  if(!nonce && zero)return 0; // not yet boot-published,NEVER a manufactured ready record
  if(status[2] || status[3]!=magic || status[0]>nonce || status[1]>status[0] || status[4]>2 || status[5]>15)return -1;
  for(unsigned i=8;i<16;i++)if(status[i])return -1;
  if(status[0]!=nonce || status[1]!=nonce)return 0;
  if(status[4]!=(stopped?2U:0U) || status[5]!=(w?0U:stopped?15U:nonce?14U:0U))return -1;
  if(nonce && (status[6]!=plan->Outer()[3] || status[7]!=plan->Outer()[4]))return -1;
  if(!nonce && (status[6] || status[7]))return -1;
  for(unsigned i=0;i<2;i++){
   if(io.ResetRead()!=0x46800)return -1;
   io.Pause();unsigned pc=io.RuntimePcRead();
   unsigned a=w?(stopped?0x20060U:0x2004cU):(stopped?0x40060U:0x4004cU);
   if(pc!=a && pc!=a+4)return 0; // speculative PC alone is never completion
  }
  return 1;
 }
 template<class IO> bool Poll(IO& io,unsigned w,unsigned nonce,bool stopped,uint64_t at){
  for(unsigned i=0;i<65536;i++){
   if(!Guard(io,w) || !Budget(io,at))return false;
   int observed=Wait(io,w,nonce,stopped);if(observed<0)return false;if(observed==1)return true;io.Pause();
  }
  return false;
 }
 void PeerCommand(unsigned w,uint32_t* out)const{
  namespace M=PioneerQ38MixedWorkerLayout;const auto* p=plan->Recipe(7)+80+(w-1)*8;
  const unsigned last[]={p[0]+(p[2]*68-1)*2304,0,p[4],0x90000,0,M::Peer(w),0,0,2304,0};
  for(unsigned i=0;i<10;i++)out[i]=last[i];
 }
 template<class IO> bool Peer(IO& io,unsigned w,bool stopped){
  namespace L=PioneerQ38MixedRecurrentBlockProjectionLayout;namespace M=PioneerQ38MixedWorkerLayout;
  unsigned n=L::RunDelta(w-1),sequence=n+unsigned(stopped);uint32_t actual;
  const auto* boot=plan->Boot(w);const auto* d=plan->Recipe(7);const auto* p=d+80+(w-1)*8;
  for(unsigned i=0;i<64;i++)if(!Read(io,w,M::Boot+i*4,&actual,1) || actual!=boot[i])return false;
  const unsigned head[]={M::Magic,sequence,stopped?2U:1U,w,M::Peer(w),d[42],d[40],d[6]+unsigned(stopped)};
  const unsigned capsule[]={0x31483851,12,2304,0,0,17408,p[0],p[2],p[4],1};
  for(unsigned i=0;i<64;i++){
   unsigned expected=i<8?head[i]:i<16?d[24+i]:!stopped && i<26?capsule[i-16]:!stopped && i==26?1U:0;
   if(!Read(io,w,M::Job+i*4,&actual,1) || actual!=expected)return false;
  }
  if(!Read(io,w,M::Doorbell,&actual,1) || actual!=sequence)return false;
  if(stopped)return true;
  uint32_t delta[8],before[8],tail[10];
  if(!PioneerQ38RecurrentBlockCounterDelta(*plan,*canonical,w,polls[w],true,delta))return false;
  for(unsigned i=0;i<8;i++)before[i]=memory->counters[w][i]+delta[i];
  PeerCommand(w,tail);const unsigned receipt[]={p[2]*17408/16,n,w,0x32504651};
  for(unsigned i=0;i<64;i++){
   unsigned expected=i<8?before[i]:i<16?counts[w][i-8]:i<20?receipt[i-16]:i<30?tail[i-20]:i==30?boot[16]:i==31?M::Peer(w):i<48?boot[i-16]:0;
   if(!Read(io,w,0x83f00+i*4,&actual,1) || actual!=expected)return false;
  }
  return true;
 }
 template<class IO> bool RootReceipt(IO& io,bool stopped){
  namespace L=PioneerQ38MixedRecurrentBlockRootLayout;namespace M=PioneerQ38MixedWorkerLayout;
  const auto* o=plan->Outer();const auto* b=plan->Boot(0);uint32_t actual;
  for(unsigned i=0;i<64;i++)if(!Read(io,0,L::Boot+i*4,&actual,1) || actual!=b[i])return false;
  for(unsigned i=0;i<128;i++)if(!Read(io,0,L::Outer+i*4,&actual,1) || actual!=(stopped?stop[i]:o[i]))return false;
  if(!Read(io,0,L::Doorbell,&actual,1) || actual!=(stopped?2U:1U) || !PioneerQ38GraphCsrIdle(io))return false;
  for(unsigned i=0;i<8;i++){counts[0][i]=io.ResidentNocRead(PioneerQ38PanelCounter(i));if(i==6 && counts[0][2]!=counts[0][6])return false;}
  const unsigned runLast[]={L::Scratch,0,129,o[9]+L::FinalOffset+16384,0,o[8],0,0x212,4096,0};
  const unsigned stopLast[]={0xffb20228,0,M::Peer(119),L::Scratch+8,0,129,0,0,4,0};
  for(unsigned i=0;i<10;i++){command[0][i]=io.ResidentNocRead(i*4);if(command[0][i]!=(stopped?stopLast[i]:runLast[i]))return false;}
  const unsigned runHead[]={L::Magic,1,o[3],o[4],5120,119,8,640,96,6144,14,o[12]};
  const unsigned stopHead[]={L::StopMagic,2,1,119,o[3],o[4],129,1};
  unsigned at=stopped?unsigned(L::StopSummary):unsigned(L::JointSummary);
  for(unsigned i=0;i<64;i++){
   unsigned expected=stopped?(i<8?stopHead[i]:i<16?counts[0][i-8]:i<26?command[0][i-16]:i<34?b[i-18]:i<50?b[i-18]:0U):
    i<12?runHead[i]:i<16?0U:i<24?counts[0][i-16]:i<32?0U:i<48?o[i-16]:i<56?o[i-16]:0U;
   if(!Read(io,0,at+i*4,&actual,1) || actual!=expected)return false;
  }
  traffic=false;return Guard(io,0) && Image(io,0);
 }
 template<class IO> bool Hold(IO& io,unsigned w){
  if(!Guard(io,w))return false;
  resets[w]=0x87ffff;io.RuntimeResetWrite(resets[w]);io.Pause();if(!Guard(io,w))return false;
  resets[w]=0x47800;io.RuntimeResetWrite(resets[w]);io.Pause();if(!Guard(io,w))return false;
  running[w]=cohort->worker[w].image.running=false;if(!w)cohort->rootRunning=false;
  if(!Guard(io,w))return false;
  heldPeers++;return true;
 }
public:
 PioneerQ38RecurrentBlockOwner()=default;
 PioneerQ38RecurrentBlockOwner(const PioneerQ38RecurrentBlockOwner&)=delete;
 PioneerQ38RecurrentBlockOwner& operator=(const PioneerQ38RecurrentBlockOwner&)=delete;
 bool Unsafe()const{return unsafe;}bool Closed()const{return closed;}bool Claimed()const{return started;}
 void Fault(){unsafe=true;if(memory)memory->Fault();} // MEMORY ONLY,never promotes cleanup
 bool MatchesNativeOwner(const PioneerQ38RecurrentBlockMemory& m,const PioneerQ38MixedRecurrentBlockPlan& p)const{return memory==&m && plan==&p && Bound();}
 bool MatchesCohort(const PioneerQ38DispatchLease* c)const{return cohort==c && Bound();}
 bool MatchesClosedNativeOwner(const PioneerQ38RecurrentBlockMemory& m,const PioneerQ38MixedRecurrentBlockPlan& p)const{
  return started && closed && !unsafe && launched && ran && rootCompleted && allStopped && completedPeers==119 && heldPeers==120
   && !observingPeer && memory==&m && plan==&p && m.Closed() && m.launched && m.proof==proof && m.banks==banks
   && m.cohort==cohort && m.plan==canonical && m.identity==identity && m.ledger==ledger && proof && banks && cohort && canonical
   && banks->Matches(*proof) && p.Matches(*canonical) && !cohort->unsafe && !cohort->rootRunning;
  // A later known-clean legacy dispatch CLOSE must not invalidate this
  // already-proven execution closure. It remains only a binding check;
  // final native source/PCI/clock/host disposal needs its OWN current proof.
 }
 template<class IO> bool Prepare(IO& io,const tt_dram_samples_result& dram,PioneerResidentBanks& owner,PioneerQ38DispatchLease& participants,
  PioneerQ38RecurrentBlockMemory& mem,const PioneerQ38CanonicalModelPlan& c,const PioneerQ38MixedRecurrentBlockPlan& operation,
  const PioneerQ38GdnIdentity& id,const uint8_t* learned,unsigned pages,PioneerQ38RecurrentBlockResult& r){
  namespace L=PioneerQ38MixedRecurrentBlockRootLayout;namespace M=PioneerQ38MixedWorkerLayout;
  const auto* o=operation.Outer();
  if(started || unsafe || closed || !o || o[3] || o[4] || o[12] || o[48]!=1 || !operation.Matches(c)
   || !operation.LearnedPagesPresent(learned,pages) || !mem.Installed()
   || !mem.MatchesNativeOwner(dram,owner,participants,c,id,learned,pages) || r.version!=1)return false;
  const auto* words=reinterpret_cast<const uint32_t*>(&r);for(unsigned i=1;i<sizeof(r)/4;i++)if(words[i])return false;
  started=true;proof=&dram;banks=&owner;cohort=&participants;memory=&mem;canonical=&c;plan=&operation;identity=id;ledger=learned;r.started=1;r.phase=1;
  for(unsigned w=0;w<120;w++){
   resets[w]=0x47800;for(unsigned i=0;i<8;i++)counts[w][i]=mem.counters[w][i];for(unsigned i=0;i<10;i++)command[w][i]=mem.command[w][i];
  }
  for(unsigned w=0;w<120;w++){r.worker=w;if(!Guard(io,w) || !Image(io,w))return Fail(io,r);}
  for(unsigned w=0;w<120;w++){
   r.worker=w;uint32_t zero[64]={};
   if(!Guard(io,w) || !Stage(io,w,w?unsigned(M::Boot):unsigned(L::Boot),plan->Boot(w),64)
    || !Stage(io,w,w?unsigned(M::Job):unsigned(L::Outer),zero,64) || !Stage(io,w,w?unsigned(M::Doorbell):unsigned(L::Doorbell),zero,1)
    || !Stage(io,w,w?unsigned(M::Status):unsigned(L::Status),zero,16))return Fail(io,r);
   if(w)for(unsigned i=0;i<10;i++){
    command[w][i]=i==5?M::Peer(w):0;io.ResidentNocWrite(i*4,command[w][i]);if(io.ResidentNocRead(i*4)!=command[w][i])return Fail(io,r);
   }
   if(!Guard(io,w))return Fail(io,r);
  }
  if(!Guard(io,0) || !Stage(io,0,L::Outer,o,128))return Fail(io,r);
  // Stage only immutable recipe/Conv/GDN DATA,never host intermediate
  // products or state. The actual ROOT reads all of it from owned GDDR.
  for(unsigned slot=0;slot<8;slot++)if(!Stage(io,0,L::Scratch,plan->Recipe(slot),1280)
   || !Packet(io,o[57]+slot*L::RecipeStride,L::RecipeStride,r))return Fail(io,r);
  const uint32_t* tables[]={plan->Conv(),plan->Gdn()};const unsigned sizes[]={o[60],o[61]},remote[]={o[58],o[59]};
  for(unsigned table=0;table<2;table++)for(unsigned at=0;at<sizes[table];at+=4096){
   unsigned n=sizes[table]-at;if(n>4096)n=4096;
   if(!Stage(io,0,L::Scratch,tables[table]+at,n) || !Packet(io,remote[table]+at*4,n*4,r))return Fail(io,r);
  }
  began=io.Q38CohortNow();if(began>UINT64_MAX-UINT64_C(30000000))return Fail(io,r);
  launched=mem.launched=true;r.phase=2;
  for(unsigned w=1;w<120;w++){
   r.worker=w;if(!Guard(io,w) || !Image(io,w))return Fail(io,r);
   resets[w]=0x87ffff;io.RuntimeResetWrite(resets[w]);r.armed++;io.Pause();if(!Guard(io,w))return Fail(io,r);
   resets[w]=0x46800;running[w]=participants.worker[w].image.running=true;io.RuntimeResetWrite(resets[w]);r.released++;
   if(!Poll(io,w,0,false,began))return Fail(io,r);
  }
  r.prepared=1;r.phase=3;return true;
 }
 template<class IO> bool Run(IO& io,PioneerQ38RecurrentBlockResult& r){
  namespace L=PioneerQ38MixedRecurrentBlockRootLayout;
  if(!r.prepared || r.unsafe || r.completed || ran || rootCompleted || closed || unsafe || !launched || running[0])return false;
  r.worker=r.word=0;
  if(!Scope(io) || !plan->LearnedPagesPresent(ledger,PioneerResidentBanks::Pages) || !Guard(io,0) || !Image(io,0))return Fail(io,r);
  began=io.Q38CohortNow();if(began>UINT64_MAX-UINT64_C(30000000))return Fail(io,r);
  resets[0]=0x87ffff;io.RuntimeResetWrite(resets[0]);io.Pause();if(!Guard(io,0))return Fail(io,r);
  resets[0]=0x46800;running[0]=cohort->rootRunning=cohort->worker[0].image.running=true;io.RuntimeResetWrite(resets[0]);r.rootStarts++;
  if(!Poll(io,0,0,false,began))return Fail(io,r);
  uint32_t nonce=1;traffic=true;r.phase=4;
  if(!Stage(io,0,L::Doorbell,&nonce,1) || !Poll(io,0,1,false,began) || !RootReceipt(io,false))return Fail(io,r);
  rootCompleted=true;
  for(unsigned w=1;w<120;w++){
   r.worker=w;observingPeer=w;
   if(!Poll(io,w,PioneerQ38MixedRecurrentBlockProjectionLayout::RunDelta(w-1),false,began))return Fail(io,r);
   uint32_t actual[8];for(unsigned i=0;i<8;i++)actual[i]=io.ResidentNocRead(PioneerQ38PanelCounter(i));
   if(!PioneerQ38RecurrentBlockCounterProof(*plan,*canonical,w,memory->counters[w],actual,false,polls[w]))return Fail(io,r);
   for(unsigned i=0;i<8;i++)counts[w][i]=actual[i];
   PeerCommand(w,command[w]);
   if(!Peer(io,w,false) || !Image(io,w))return Fail(io,r);
   observingPeer=0;if(!Guard(io,w))return Fail(io,r);
   completedPeers++;r.completed++;
  }
  if(completedPeers!=119 || !Guard(io,0))return Fail(io,r);
  ran=true;r.phase=5;return true; // provisional complete layer,NEVER model/token/state commit
 }
 template<class IO> bool Close(IO& io,PioneerQ38RecurrentBlockResult& r,PioneerQ38RecurrentBlockMemoryResult& saved){
  namespace L=PioneerQ38MixedRecurrentBlockRootLayout;
  if(!ran || !rootCompleted || completedPeers!=119 || observingPeer || unsafe || closed || !r.prepared || r.completed!=119
   || r.stopped || r.held || r.closed || saved.restoredWords)return false;
  r.worker=r.word=0;
  if(!Guard(io,0))return Fail(io,r);
  stop[0]=L::Magic;stop[1]=1;stop[2]=128;stop[5]=129;stop[48]=2;stop[49]=119;stop[72]=L::Stop;
  for(unsigned i=16;i<40;i++)stop[i]=plan->Outer()[i];
  if(!L::StopShape(stop))return Fail(io,r);
  began=io.Q38CohortNow();if(began>UINT64_MAX-UINT64_C(30000000))return Fail(io,r);
  uint32_t nonce=2;r.phase=6;
  if(!Stage(io,0,L::Outer,stop,128))return Fail(io,r);
  traffic=true;
  if(!Stage(io,0,L::Doorbell,&nonce,1) || !Poll(io,0,2,true,began) || !RootReceipt(io,true))return Fail(io,r);
  for(unsigned w=1;w<120;w++){
   r.worker=w;if(!Poll(io,w,PioneerQ38MixedRecurrentBlockProjectionLayout::RunDelta(w-1)+1,true,began)
    || !Peer(io,w,true) || !Image(io,w) || !Guard(io,w))return Fail(io,r);
   r.stopped++;
  }
  if(r.stopped!=119 || !Guard(io,0))return Fail(io,r);
  allStopped=true;r.phase=7;
  // ALL real terminal STOP/status/PC/source/counter records precede the
  // FIRST hold. ALL120 proven holds precede the FIRST restore write.
  for(unsigned w=0;w<120;w++){r.worker=w;if(!Hold(io,w))return Fail(io,r);r.held++;if(!w)r.rootHolds++;}
  if(heldPeers!=120)return Fail(io,r);
  r.phase=8;
  for(unsigned w=0;w<120;w++){
   r.worker=w;if(!Guard(io,w) || !Image(io,w))return Fail(io,r);
   for(unsigned i=0;i<PioneerQ38RecurrentBlockSnapshotWords(w);i++){
    r.word=i;uint32_t original=memory->saved[PioneerQ38RecurrentBlockSnapshotBase(w)+i];
    if(!Stage(io,w,PioneerQ38RecurrentBlockSnapshotAddress(w,i),&original,1))return Fail(io,r);
    saved.restoredWords++;
   }
   for(unsigned i=0;i<10;i++){
    command[w][i]=memory->command[w][i];io.ResidentNocWrite(i*4,command[w][i]);if(io.ResidentNocRead(i*4)!=command[w][i])return Fail(io,r);
   }
   if(!Guard(io,w) || !PioneerQ38GraphCsrCode(io,w))return Fail(io,r);
  }
  r.phase=9;
  for(unsigned w=0;w<120;w++){r.worker=w;if(!Guard(io,w) || !memory->VerifyOriginal(io,w,saved) || !Guard(io,w))return Fail(io,r);}
  if(!allStopped || heldPeers!=120 || r.held!=120 || saved.restoredWords!=TT_Q38_RECURRENT_BLOCK_SAVED_WORDS
   || saved.verifiedOriginalWords!=2*TT_Q38_RECURRENT_BLOCK_SAVED_WORDS || !Guard(io,0))return Fail(io,r);
  memory->closed=closed=true;memory->installed=false;saved.closed=r.closed=1;return true;
 }
};
static_assert(sizeof(PioneerQ38RecurrentBlockOwner)>8192 && sizeof(PioneerQ38RecurrentBlockOwner)<32768,"execution proof state HEAP ONLY");
#endif
