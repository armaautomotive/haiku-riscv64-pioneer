/* SPDX-License-Identifier: MIT */
#if defined(PIONEER_Q38_CPU_VALIDATION) || defined(PIONEER_Q38_RECURRENT_PREFIX_VALIDATION_REFERENCE) \
 || defined(PIONEER_Q38_RECURRENT_BLOCK_VALIDATION_REFERENCE) || defined(PIONEER_Q38_RECURRENT_BLOCK_VALIDATION_READBACK)
#error Prefix execution owner cannot import CPU oracle or old readback roles,including defined0
#endif
#ifdef PIONEER_Q38_RECURRENT_PREFIX_VALIDATION_READBACK
#include "PioneerQ38RecurrentPrefixReadbackRole.h"
#ifdef PIONEER_Q38_RECURRENT_PREFIX_OWNER_H
static_assert(TT_Q38_RECURRENT_PREFIX_READBACK_SELECTION==1,"late prefix readback selection change");
#endif
#else
#ifdef PIONEER_Q38_RECURRENT_PREFIX_OWNER_H
static_assert(TT_Q38_RECURRENT_PREFIX_READBACK_SELECTION==0,"late prefix readback selection removal");
#endif
#endif
#ifndef PIONEER_Q38_RECURRENT_PREFIX_OWNER_H
#define PIONEER_Q38_RECURRENT_PREFIX_OWNER_H
enum {TT_Q38_RECURRENT_PREFIX_READBACK_SELECTION=
#ifdef PIONEER_Q38_RECURRENT_PREFIX_VALIDATION_READBACK
 1
#else
 0
#endif
};
#ifdef PIONEER_Q38_RECURRENT_PREFIX_VALIDATION_READBACK
#include "PioneerQ38RecurrentPrefixReadbackPlan.h"
#endif
#include "PioneerQ38RecurrentPrefixMemory.h"
#include "../../../../../tools/pioneer/PioneerQ38RecurrentPrefixCounters.h"
// DISTINCT autonomous QPX1 owner algorithm; NOT the old QRB1 owner. NO old dense owner/role alias,CPU numeric
// products,public post-launch restore hint,model/token/state-slot commit.
// The native scope MUST prove the same real completed embedding producer,
// initialized recurrent/history leases,FD/thread/source/clock/ALL hosts.
enum PioneerQ38RecurrentPrefixWaitFailure {
 TT_Q38_RECURRENT_PREFIX_WAIT_CLOCK_OVERFLOW=0x57540001,TT_Q38_RECURRENT_PREFIX_WAIT_GUARD=0x57540002,
 TT_Q38_RECURRENT_PREFIX_WAIT_CLOCK_BACKWARDS=0x57540003,TT_Q38_RECURRENT_PREFIX_WAIT_DEADLINE=0x57540004,
 TT_Q38_RECURRENT_PREFIX_WAIT_STATUS=0x57540005,TT_Q38_RECURRENT_PREFIX_WAIT_POLL_LIMIT=0x57540006
}; // retained CPU-only result.word diagnostics, NOT a device query or recovery grant
struct PioneerQ38RecurrentPrefixResult {
 uint32_t version=1,started=0,phase=0,worker=0,word=0,unsafe=0,prepared=0,completed=0,stopped=0,held=0,closed=0;
 uint32_t armed=0,released=0,rootStarts=0,rootHolds=0,packets=0;
};
class PioneerQ38RecurrentPrefixOwner {
 const tt_dram_samples_result* proof=nullptr;PioneerResidentBanks* banks=nullptr;PioneerQ38DispatchLease* cohort=nullptr;
 PioneerQ38RecurrentPrefixMemory* memory=nullptr;const PioneerQ38CanonicalModelPlan* canonical=nullptr;
 const PioneerQ38RecurrentPrefixPlan* plan=nullptr;PioneerQ38GdnIdentity identity={};const uint8_t* ledger=nullptr;
 uint32_t counts[120][8]={},command[120][10]={},resets[120]={},stop[128]={},staging[4096]={};
 unsigned polls[120]={},observingPeer=0,completedPeers=0,heldPeers=0;
 bool running[120]={},started=false,unsafe=false,closed=false,launched=false,traffic=false,ran=false,rootCompleted=false,allStopped=false;
 uint64_t began=0,waitBudget=UINT64_C(30000000);
 bool Bound()const{
  if(!started || unsafe || closed || !proof || !banks || !cohort || !memory || !canonical || !plan || !ledger || !identity.Valid()
   || !banks->Matches(*proof) || memory->unsafe || memory->closed || !memory->installed || memory->launched!=launched
   || memory->proof!=proof || memory->banks!=banks || memory->cohort!=cohort || memory->plan!=canonical || memory->ledger!=ledger
   || !(memory->identity==identity) || !plan->MatchesBinding(*canonical,plan->Bank(),identity,&cohort->authority.source[0][0]) || cohort->unsafe || cohort->closed || cohort->prepared!=120
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
  return Bound() && io.RecurrentPrefixExecutionScope(*this,*memory,*canonical,*plan,identity,ledger,PioneerResidentBanks::Pages);
 }
 template<class IO> bool Fail(IO& io,PioneerQ38RecurrentPrefixResult& r){
  unsafe=true;r.unsafe=1;if(memory)memory->Fault();
  if(cohort && banks)PioneerQ38DispatchQuarantine(*cohort,*banks);
  io.QuarantineHostPage();io.RecurrentPrefixRetainHostOwners();return false; // MEMORY ONLY,no later IO/retry/cleanup
 }
 template<class IO> bool Guard(IO& io,unsigned w){
  if(w>=120 || !Scope(io) || !io.SelectRuntimeCore(w) || !io.RecurrentPrefixMemoryMappingStable(w)
   || io.ResetRead()!=resets[w] || io.ResetRead()!=resets[w])return false;
  for(unsigned i=0;i<16;i++)if(io.ResidentNocRead(BlackholeDramSourceOffset(i))!=plan->Boot(w)[16+i])return false;
  for(unsigned i=0;i<TT_DRAM_SOURCE_WORDS;i++)if(io.SourceRead(i)!=proof->source[i])return false;
  for(unsigned p=0;p<2;p++)if(!PioneerQ38GraphCsrPciQuiet(io,p))return false;
#ifdef PIONEER_Q38_RECURRENT_PREFIX_VALIDATION_READBACK
  if(!w && readbackTraffic)return ReadbackTransferGuard(io);
#endif
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
  for(unsigned i=0;i<n;i++)if(!io.RecurrentPrefixMemoryReadChecked(w,a+i*4,out[i]))return false;
  return true;
 }
 template<class IO> bool Stage(IO& io,unsigned w,unsigned a,const uint32_t* data,unsigned n){
  for(unsigned i=0;i<n;i++)if(!io.RecurrentPrefixMemoryWrite(w,a+i*4,data[i]))return false;
  for(unsigned i=0;i<n;i++){uint32_t actual;if(!io.RecurrentPrefixMemoryReadChecked(w,a+i*4,actual) || actual!=data[i])return false;}
  return true;
 }
 template<class IO> bool Image(IO& io,unsigned w){PioneerQ38RecurrentPrefixMemoryResult ignored;return memory->VerifyImage(io,w,ignored);}
 template<class IO> bool Budget(IO& io,uint64_t at){uint64_t now=io.Q38CohortNow();return now>=at && now-at<UINT64_C(30000000);}
 template<class IO> bool BeginWait(IO& io,PioneerQ38RecurrentPrefixResult& r,uint64_t budget=UINT64_C(30000000)){
  if(budget!=UINT64_C(30000000) && budget!=UINT64_C(90000000))return false;
  began=io.Q38CohortNow();waitBudget=budget;
  if(began>UINT64_MAX-waitBudget){r.word=TT_Q38_RECURRENT_PREFIX_WAIT_CLOCK_OVERFLOW;return false;}
  return true;
 } // 30s/real boot transition; one bounded90s complete3layer wait, NEVER rearmed inside Poll
 template<class IO> bool Packet(IO& io,unsigned address,unsigned bytes,PioneerQ38RecurrentPrefixResult& r){
  namespace L=PioneerQ38MixedRecurrentBlockRootLayout;
  if(!bytes || (bytes&63) || bytes>16384 || !Guard(io,0))return false;
  const auto* o=plan->Layer(0)->Outer();uint32_t expected[8];
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
  if(status[2] || status[3]!=magic || status[0]>nonce || status[1]>status[0] || status[4]>2 || status[5]>16)return -1;
  for(unsigned i=8;i<16;i++)if(status[i])return -1;
  if(status[0]!=nonce || status[1]!=nonce)return 0;
  if(status[4]!=(stopped?2U:0U) || status[5]!=(w?0U:stopped?15U:nonce?14U:0U))return -1;
  if(nonce && (status[6]!=2 || status[7]!=0))return -1;
  if(!nonce && (status[6] || status[7]))return -1;
  for(unsigned i=0;i<2;i++){
   if(io.ResetRead()!=0x46800)return -1;
   io.Pause();unsigned pc=io.RuntimePcRead();
   unsigned a=w?(stopped?0x20060U:0x2004cU):(stopped?0x40060U:0x4004cU);
   if(pc!=a && pc!=a+4)return 0; // speculative PC alone is never completion
  }
  return 1;
 }
 template<class IO> bool Poll(IO& io,unsigned w,unsigned nonce,bool stopped,PioneerQ38RecurrentPrefixResult& r){
  for(unsigned i=0;i<65536;i++){
   if(!Guard(io,w)){r.word=TT_Q38_RECURRENT_PREFIX_WAIT_GUARD;return false;}
   const uint64_t now=io.Q38CohortNow();
   if(now<began){r.word=TT_Q38_RECURRENT_PREFIX_WAIT_CLOCK_BACKWARDS;return false;}
   if(now-began>=waitBudget){r.word=TT_Q38_RECURRENT_PREFIX_WAIT_DEADLINE;return false;}
   int observed=Wait(io,w,nonce,stopped);
   if(observed<0){r.word=TT_Q38_RECURRENT_PREFIX_WAIT_STATUS;return false;}
   if(observed==1)return true;
   io.Pause();
  }
  r.word=TT_Q38_RECURRENT_PREFIX_WAIT_POLL_LIMIT;return false;
 }
 void PeerCommand(unsigned w,uint32_t* out)const{
  namespace M=PioneerQ38MixedWorkerLayout;const auto* p=plan->Layer(2)->Recipe(7)+80+(w-1)*8;
  const unsigned last[]={p[0]+(p[2]*68-1)*2304,0,p[4],0x90000,0,M::Peer(w),0,0,2304,0};
  for(unsigned i=0;i<10;i++)out[i]=last[i];
 }
 template<class IO> bool Peer(IO& io,unsigned w){
  namespace L=PioneerQ38MixedRecurrentBlockProjectionLayout;namespace M=PioneerQ38MixedWorkerLayout;
  unsigned n=3*L::RunDelta(w-1),sequence=n+1;uint32_t actual;
  const auto* boot=plan->Boot(w);const auto* d=plan->Layer(2)->Recipe(7);const auto* p=d+80+(w-1)*8;
  for(unsigned i=0;i<64;i++)if(!Read(io,w,M::Boot+i*4,&actual,1) || actual!=boot[i])return false;
  const unsigned head[]={M::Magic,sequence,2,w,M::Peer(w),d[42],d[40],d[6]+1};
  for(unsigned i=0;i<64;i++){
   unsigned expected=i<8?head[i]:i<16?d[24+i]:0;
   if(!Read(io,w,M::Job+i*4,&actual,1) || actual!=expected)return false;
  }
  if(!Read(io,w,M::Doorbell,&actual,1) || actual!=sequence)return false;
  uint32_t delta[8],before[8],tail[10];
  if(!PioneerQ38RecurrentPrefixCounterDelta(*plan,*canonical,w,polls[w],true,delta))return false;
  for(unsigned i=0;i<8;i++)before[i]=memory->counters[w][i]+delta[i];
  PeerCommand(w,tail);const unsigned receipt[]={p[2]*17408/16,n,w,0x32504651};
  for(unsigned i=0;i<64;i++){
   unsigned expected=i<8?before[i]:i<16?counts[w][i-8]:i<20?receipt[i-16]:i<30?tail[i-20]:i==30?boot[16]:i==31?M::Peer(w):i<48?boot[i-16]:0;
   if(!Read(io,w,0x83f00+i*4,&actual,1) || actual!=expected)return false;
  }
  return true; // BOTH actual STOP and retained LAST-down proof, not a fabricated intermediate
 }
 template<class IO> bool RootReceipt(IO& io){
  namespace L=PioneerQ38MixedRecurrentBlockRootLayout;namespace M=PioneerQ38MixedWorkerLayout;
  namespace H=PioneerQ38RecurrentHistoryLayout;
  const auto* o=plan->Layer(2)->Outer();const auto* b=plan->Boot(0);uint32_t actual;
  for(unsigned i=0;i<64;i++)if(!Read(io,0,L::Boot+i*4,&actual,1) || actual!=b[i])return false;
  stop[0]=L::Magic;stop[1]=1;stop[2]=128;stop[5]=129;stop[48]=4;stop[49]=119;stop[72]=L::Stop;
  for(unsigned i=0;i<16;i++)stop[16+i]=b[16+i];
  for(unsigned i=0;i<8;i++)stop[32+i]=b[8+i];
  if(!L::StopShape(stop))return false;
  for(unsigned i=0;i<128;i++)if(!Read(io,0,L::Outer+i*4,&actual,1) || actual!=stop[i])return false;
  if(!Read(io,0,L::Doorbell,&actual,1) || actual!=4 || !PioneerQ38GraphCsrIdle(io))return false;
  for(unsigned i=0;i<8;i++){counts[0][i]=io.ResidentNocRead(PioneerQ38PanelCounter(i));if(i==6 && counts[0][2]!=counts[0][6])return false;}
  const unsigned last[]={0xffb20228,0,M::Peer(119),L::Scratch+8,0,129,0,0,4,0};
  for(unsigned i=0;i<10;i++){command[0][i]=io.ResidentNocRead(i*4);if(command[0][i]!=last[i])return false;}
  const unsigned head[]={L::StopMagic,4,3,119,2,0,129,1};
  for(unsigned i=0;i<64;i++){
   unsigned expected=i<8?head[i]:i<16?counts[0][i-8]:i<26?command[0][i-16]:i<50?b[i-18]:0U;
   if(!Read(io,0,L::StopSummary+i*4,&actual,1) || actual!=expected)return false;
  }
  uint32_t summary[64],history=0;
  if(!Read(io,0,L::JointSummary,summary,64) || !H::Address(o[9],2,history) || summary[18]!=summary[22])return false;
  const unsigned runHead[]={L::Magic,3,2,0,5120,119,8,640,96,6144,14,0};
  const unsigned staged[]={H::Marker,history,H::Bytes,3,2,0,0,1};
  for(unsigned i=0;i<64;i++){
   if(i>=16 && i<24)continue; // past device-run counters, NOT recaptured as current authority
   unsigned expected=i<12?runHead[i]:i<32?0U:i<48?b[i-16]:i<56?b[i-40]:staged[i-56];
   if(summary[i]!=expected)return false;
  }
  for(unsigned w=1;w<120;w++)if(!Read(io,0,L::Nonces+(w-1)*4,&actual,1)
   || actual!=3*PioneerQ38MixedRecurrentBlockProjectionLayout::RunDelta(w-1)+1)return false;
  traffic=false;return Guard(io,0) && Image(io,0);
 }
 template<class IO> bool RootPeerCounters(IO& io){
  namespace L=PioneerQ38MixedRecurrentBlockRootLayout;
  if(!Guard(io,0))return false;
  for(unsigned w=1;w<120;w++)for(unsigned i=0;i<8;i++){uint32_t actual;
   if(!Read(io,0,L::Counters+((w-1)*8+i)*4,&actual,1) || actual!=counts[w][i])return false;
  }
  return Guard(io,0);
 }
 template<class IO> bool Hold(IO& io,unsigned w){
  if(!Guard(io,w))return false;
  resets[w]=0x87ffff;io.RuntimeResetWrite(resets[w]);io.Pause();if(!Guard(io,w))return false;
  resets[w]=0x47800;io.RuntimeResetWrite(resets[w]);io.Pause();if(!Guard(io,w))return false;
  running[w]=cohort->worker[w].image.running=false;if(!w)cohort->rootRunning=false;
  if(!Guard(io,w))return false;
  heldPeers++;return true;
 }
#ifdef PIONEER_Q38_RECURRENT_PREFIX_VALIDATION_READBACK
#include "PioneerQ38RecurrentPrefixValidationReadback.inc"
#endif
public:
 PioneerQ38RecurrentPrefixOwner()=default;
 PioneerQ38RecurrentPrefixOwner(const PioneerQ38RecurrentPrefixOwner&)=delete;
 PioneerQ38RecurrentPrefixOwner& operator=(const PioneerQ38RecurrentPrefixOwner&)=delete;
 bool Unsafe()const{return unsafe;}bool Closed()const{return closed;}bool Claimed()const{return started;}
 void Fault(){unsafe=true;if(memory)memory->Fault();} // MEMORY ONLY,never promotes cleanup
 bool MatchesNativeOwner(const PioneerQ38RecurrentPrefixMemory& m,const PioneerQ38RecurrentPrefixPlan& p)const{return memory==&m && plan==&p && Bound();}
 bool MatchesCohort(const PioneerQ38DispatchLease* c)const{return cohort==c && Bound();}
 bool MatchesClosedNativeOwner(const PioneerQ38RecurrentPrefixMemory& m,const PioneerQ38RecurrentPrefixPlan& p)const{
  return started && closed && !unsafe && launched && ran && rootCompleted && allStopped && completedPeers==119 && heldPeers==120
   && !observingPeer && memory==&m && plan==&p && m.Closed() && m.launched && m.proof==proof && m.banks==banks
   && m.cohort==cohort && m.plan==canonical && m.identity==identity && m.ledger==ledger && proof && banks && cohort && canonical
   && banks->Matches(*proof) && p.Matches(*canonical) && !cohort->unsafe && !cohort->rootRunning;
  // A later known-clean legacy dispatch CLOSE must not invalidate this
  // already-proven execution closure. It remains only a binding check;
  // final native source/PCI/clock/host disposal needs its OWN current proof.
 }
 template<class IO> bool Prepare(IO& io,const tt_dram_samples_result& dram,PioneerResidentBanks& owner,PioneerQ38DispatchLease& participants,
  PioneerQ38RecurrentPrefixMemory& mem,const PioneerQ38CanonicalModelPlan& c,const PioneerQ38RecurrentPrefixPlan& operation,
  const PioneerQ38GdnIdentity& id,const uint8_t* learned,unsigned pages,PioneerQ38RecurrentPrefixResult& r){
  namespace L=PioneerQ38MixedRecurrentBlockRootLayout;namespace M=PioneerQ38MixedWorkerLayout;namespace P=PioneerQ38RecurrentPrefixLayout;
  if(started || unsafe || closed || !operation.MatchesBinding(c,operation.Bank(),id,&participants.authority.source[0][0])
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
  if(!Guard(io,0) || !Stage(io,0,L::Outer,plan->Layer(0)->Outer(),128))return Fail(io,r);
  // ALL3 immutable packages BEFORE any worker start. No product/state input.
  for(unsigned layer=0;layer<3;layer++){uint32_t address;
   if(!plan->PackageAddress(layer,address))return Fail(io,r);
   for(unsigned at=0;at<P::Bytes;at+=16384){
    unsigned bytes=P::Bytes-at;if(bytes>16384)bytes=16384;
    auto* raw=reinterpret_cast<uint8_t*>(staging);
    if(!plan->ReadPackage(layer,at,raw,bytes))return Fail(io,r);
    for(unsigned i=0;i<bytes/4;i++)staging[i]=uint32_t(raw[i*4])|(uint32_t(raw[i*4+1])<<8)|(uint32_t(raw[i*4+2])<<16)|(uint32_t(raw[i*4+3])<<24);
    if(!Stage(io,0,L::Scratch,staging,bytes/4) || !Packet(io,address+at,bytes,r))return Fail(io,r);
   }
  }
  if(r.packets!=57)return Fail(io,r);
  launched=mem.launched=true;r.phase=2;
  for(unsigned w=1;w<120;w++){
   r.worker=w;if(!Guard(io,w) || !Image(io,w))return Fail(io,r);
   resets[w]=0x87ffff;io.RuntimeResetWrite(resets[w]);r.armed++;io.Pause();if(!Guard(io,w))return Fail(io,r);
   if(!BeginWait(io,r))return Fail(io,r); // distinct deadline BEFORE this virgin release
   resets[w]=0x46800;running[w]=participants.worker[w].image.running=true;io.RuntimeResetWrite(resets[w]);r.released++;
   if(!Poll(io,w,0,false,r))return Fail(io,r);
  }
  r.prepared=1;r.phase=3;return true;
 }
 template<class IO> bool Run(IO& io,PioneerQ38RecurrentPrefixResult& r){
  namespace L=PioneerQ38MixedRecurrentBlockRootLayout;
  if(!r.prepared || r.unsafe || r.completed || r.stopped || ran || rootCompleted || closed || unsafe || !launched || running[0])return false;
  r.worker=r.word=0;
  if(!Scope(io) || !plan->LearnedPagesPresent(ledger,PioneerResidentBanks::Pages) || !Guard(io,0) || !Image(io,0))return Fail(io,r);
  resets[0]=0x87ffff;io.RuntimeResetWrite(resets[0]);io.Pause();if(!Guard(io,0))return Fail(io,r);
  if(!BeginWait(io,r))return Fail(io,r);
  resets[0]=0x46800;running[0]=cohort->rootRunning=cohort->worker[0].image.running=true;io.RuntimeResetWrite(resets[0]);r.rootStarts++;
  if(!Poll(io,0,0,false,r))return Fail(io,r);
  uint32_t nonce=1;traffic=true;r.phase=4;
  // ONE host start. ROOT alone advances0->1->2 and synthesizes terminal STOP4.
  if(!BeginWait(io,r,UINT64_C(90000000)) || !Stage(io,0,L::Doorbell,&nonce,1)
   || !Poll(io,0,4,true,r) || !RootReceipt(io))return Fail(io,r);
  rootCompleted=true;
  for(unsigned w=1;w<120;w++){
   r.worker=w;observingPeer=w;
   if(!BeginWait(io,r) || !Poll(io,w,3*PioneerQ38MixedRecurrentBlockProjectionLayout::RunDelta(w-1)+1,true,r))return Fail(io,r);
   uint32_t actual[8];for(unsigned i=0;i<8;i++)actual[i]=io.ResidentNocRead(PioneerQ38PanelCounter(i));
   if(!PioneerQ38RecurrentPrefixCounterProof(*plan,*canonical,w,memory->counters[w],actual,false,polls[w]))return Fail(io,r);
   for(unsigned i=0;i<8;i++)counts[w][i]=actual[i];
   PeerCommand(w,command[w]);
   if(!Peer(io,w) || !Image(io,w))return Fail(io,r);
   observingPeer=0;if(!Guard(io,w))return Fail(io,r);
   completedPeers++;r.completed++;r.stopped++;
  }
  if(completedPeers!=119 || !RootPeerCounters(io))return Fail(io,r);
  allStopped=ran=true;r.phase=5;return true; // provisional PREFIX, never full-token state commit
 }
#ifdef PIONEER_Q38_RECURRENT_PREFIX_VALIDATION_READBACK
private:
 template<class IO> bool CloseImpl(IO& io,PioneerQ38RecurrentPrefixResult& r,PioneerQ38RecurrentPrefixMemoryResult& saved,
  uint32_t* values,PioneerQ38RecurrentPrefixReadbackResult* readback){
#else
 template<class IO> bool Close(IO& io,PioneerQ38RecurrentPrefixResult& r,PioneerQ38RecurrentPrefixMemoryResult& saved){
#endif
  if(!ran || !rootCompleted || !allStopped || completedPeers!=119 || observingPeer || unsafe || closed || !r.prepared || r.completed!=119
   || r.stopped!=119 || r.held || r.closed || saved.restoredWords)return false;
  r.worker=r.word=0;r.phase=6;
  if(!Guard(io,0) || !BeginWait(io,r) || !Poll(io,0,4,true,r) || !RootReceipt(io))return Fail(io,r);
  for(unsigned w=1;w<120;w++){
   r.worker=w;if(!Guard(io,w) || !BeginWait(io,r)
    || !Poll(io,w,3*PioneerQ38MixedRecurrentBlockProjectionLayout::RunDelta(w-1)+1,true,r)
    || !Peer(io,w) || !Image(io,w) || !Guard(io,w))return Fail(io,r);
  }
  if(!RootPeerCounters(io))return Fail(io,r);
  r.phase=7;
  // ALL real terminal STOP/status/PC/source/counter records precede the
  // FIRST hold. ALL120 proven holds precede the FIRST restore write.
  for(unsigned w=0;w<120;w++){r.worker=w;if(!Hold(io,w))return Fail(io,r);r.held++;if(!w)r.rootHolds++;}
  if(heldPeers!=120)return Fail(io,r);
#ifdef PIONEER_Q38_RECURRENT_PREFIX_VALIDATION_READBACK
  if(readback && !ValidationReadback(io,*readback,r))return Fail(io,r);
#endif
  r.phase=8;
  for(unsigned w=0;w<120;w++){
   r.worker=w;if(!Guard(io,w) || !Image(io,w))return Fail(io,r);
   for(unsigned i=0;i<PioneerQ38RecurrentPrefixSnapshotWords(w);i++){
    r.word=i;uint32_t original=memory->saved[PioneerQ38RecurrentPrefixSnapshotBase(w)+i];
    if(!Stage(io,w,PioneerQ38RecurrentPrefixSnapshotAddress(w,i),&original,1))return Fail(io,r);
    saved.restoredWords++;
   }
   for(unsigned i=0;i<10;i++){
    command[w][i]=memory->command[w][i];io.ResidentNocWrite(i*4,command[w][i]);if(io.ResidentNocRead(i*4)!=command[w][i])return Fail(io,r);
   }
   if(!Guard(io,w) || !PioneerQ38GraphCsrCode(io,w))return Fail(io,r);
  }
  r.phase=9;
  for(unsigned w=0;w<120;w++){r.worker=w;if(!Guard(io,w) || !memory->VerifyOriginal(io,w,saved) || !Guard(io,w))return Fail(io,r);}
  if(!allStopped || heldPeers!=120 || r.held!=120 || saved.restoredWords!=TT_Q38_RECURRENT_PREFIX_SAVED_WORDS
   || saved.verifiedOriginalWords!=2*TT_Q38_RECURRENT_PREFIX_SAVED_WORDS || !Guard(io,0))return Fail(io,r);
  memory->closed=closed=true;memory->installed=false;saved.closed=r.closed=1;
#ifdef PIONEER_Q38_RECURRENT_PREFIX_VALIDATION_READBACK
  if(readback){for(unsigned i=0;i<TT_Q38_RECURRENT_PREFIX_READBACK_WORDS;i++)values[i]=readbackValues[i];readback->completed=1;}
#endif
  return true;
 }
#ifdef PIONEER_Q38_RECURRENT_PREFIX_VALIDATION_READBACK
public:
 template<class IO> __attribute__((noinline)) bool Close(IO& io,PioneerQ38RecurrentPrefixResult& r,PioneerQ38RecurrentPrefixMemoryResult& saved){return CloseImpl(io,r,saved,nullptr,nullptr);}
 bool CanCloseForValidation(uint32_t* values,unsigned words,const PioneerQ38RecurrentPrefixReadbackResult& out,
  const PioneerQ38RecurrentPrefixResult& r,const PioneerQ38RecurrentPrefixMemoryResult& saved)const{return ReadbackRequest(values,words,out,r,saved);}
 template<class IO> __attribute__((noinline)) bool CloseForValidation(IO& io,PioneerQ38RecurrentPrefixResult& r,PioneerQ38RecurrentPrefixMemoryResult& saved,
  uint32_t* values,unsigned words,PioneerQ38RecurrentPrefixReadbackResult& out){
  if(!ReadbackRequest(values,words,out,r,saved))return false;
  if(!Scope(io) || !readbackPlan.Build(*canonical,*plan))return Fail(io,r);
  out.started=1;return CloseImpl(io,r,saved,values,&out);
 }
#endif
};
static_assert(sizeof(PioneerQ38RecurrentPrefixOwner)>24576 && sizeof(PioneerQ38RecurrentPrefixOwner)<65536,"execution proof state HEAP ONLY");
#endif
