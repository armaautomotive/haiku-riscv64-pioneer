/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_FFN_ROOT_LIFETIME_OWNER_H
#define PIONEER_Q38_FFN_ROOT_LIFETIME_OWNER_H
#if !defined(PIONEER_Q38_FFN_GRAPH_INPUT_ROOT) || PIONEER_Q38_FFN_GRAPH_INPUT_ROOT != 1
#error Separate exact QFG2 root lifetime role required
#endif
#include <stddef.h>
#include "PioneerQ38FfnGraphCodeOwner.h"
#include "PioneerQ38FfnInputGraphImages.h"
#include "PioneerQ38FfnGraphShape.h"
// PRIVATE,HEAP ONLY. One fixed QFG2 ROOT+ALL119 QFP2 boot/STOP lifetime,
// NOT model/FFN/input/learned/GDDR/host-DMA compute. No caller job/address/
// completed/clean hint exists. A REAL native caller must retain/exclude ALL
// context/code/maps/clock/host owners under its actual cookie/global lock.
// Root executes real NoC traffic:its counters are NEVER blindly restored.
// On ANY uncertainty retain BOTH heaps and the whole native context;NO
// later read/STOP/hold/restore/END/reset/retry/release. This class is NOT
// itself a native scope grant and no native route selects it yet.
struct PioneerQ38FfnRootLifetimeResult {
 uint32_t version=1,started=0,passed=0,unsafe=0,phase=0,worker=0,word=0;
 uint32_t savedWords=0,installedWords=0,restoredWords=0,savedFrameWords=0,restoredFrameWords=0;
 uint32_t savedRootWords=0,restoredRootWords=0,savedStackWords=0,restoredStackWords=0;
 uint32_t armed=0,released=0,ready=0,stopped=0,held=0,rootArmed=0,rootStarted=0,rootCompleted=0,rootHeld=0;
 uint32_t rootReads=0,rootWrites=0;
};
class PioneerQ38FfnRootLifetimeOwner {
 PioneerQ38FfnGraphCodeOwner* code=nullptr;
 const tt_dram_samples_result* proof=nullptr;
 PioneerResidentBanks* banks=nullptr;
 PioneerQ38DispatchLease* cohort=nullptr;
 uint32_t reset[120]={},frames[119][96]={},metadata[12288]={},stack[2048]={};
 uint32_t manifest[TT_Q38_FFN_GRAPH_WORDS]={},sources[119*16]={},stats[64]={},rootAfter[8]={};
 bool running[120]={},bootCommand[120]={};
 bool started=false,unsafe=false,closed=false,rootTraffic=false,rootVerified=false,rootCommandRestored=false;
 static unsigned Route(unsigned w){return PioneerWorkerX(w)|(PioneerWorkerY(w)<<6);}
 template<class IO> bool Fail(IO& io,PioneerQ38FfnRootLifetimeResult& r) {
  unsafe=true;r.unsafe=1;code->unsafe=true;PioneerQ38DispatchQuarantine(*cohort,*banks);io.QuarantineHostPage();return false;
 }
 template<class IO> bool Guard(IO& io,unsigned w) {
  if(w>=120 || unsafe || closed || !code || code->unsafe || code->closed || code->proof!=proof || code->banks!=banks
   || code->cohort!=cohort || cohort->unsafe || cohort->closed || cohort->rootRunning!=running[0]
   || cohort->prepared!=120 || cohort->bootAttempted || cohort->booted || !banks->Matches(*proof))return false;
  auto& n=cohort->worker[w];const auto& s=n.Store();
  if(!n.image.saved || !n.image.ready || n.image.unsafe || n.image.running!=running[w] || !s.ready
   || s.pending || s.recovery_required || n.noc.recovery_required || !io.SelectRuntimeCore(w)
   || !io.Q38PanelMappingStable() || !io.FfnGraphCodeMappingStable(w)
   || io.ResetRead()!=reset[w] || io.ResetRead()!=reset[w])return false;
  for(unsigned i=0;i<16;i++)if(io.ResidentNocRead(BlackholeDramSourceOffset(i))!=cohort->authority.source[w][i])return false;
  for(unsigned i=0;i<TT_DRAM_SOURCE_WORDS;i++)if(io.SourceRead(i)!=proof->source[i])return false;
  for(unsigned p=0;p<2;p++)if(!PioneerQ38GraphCsrPciQuiet(io,p))return false;
  // During ROOT execution its OWN trusted code writes commands/counters.
  // Never pretend they remain at a held-state baseline,or read them as
  // completion authority before its actual physical terminal is proved.
  if(!w && rootTraffic && !rootVerified)return true;
  if(!PioneerQ38GraphCsrIdle(io))return false;
  if(w<code->sampled) {
   for(unsigned i=0;i<10;i++) {
    const uint32_t last[]={0xffb20228,0,Route(119),0x75008,0,129,0,0,4,0};
    uint32_t expected=!w && rootTraffic && !rootCommandRestored?last[i]:bootCommand[w]?(i==5?Route(w):0):code->command[w][i];
    if(io.ResidentNocRead(i*4)!=expected)return false;
   }
   for(unsigned i=0;i<8;i++)if(io.ResidentNocRead(PioneerQ38PanelCounter(i))!=(!w && rootTraffic?rootAfter[i]:code->counters[w][i]))return false;
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
 template<class IO> bool Stage(IO& io,uint32_t a,const uint32_t* words,unsigned n) {
  for(unsigned i=0;i<n;i++){io.Q38PanelWrite(a+i*4,words[i]);if(io.Q38PanelRead(a+i*4)!=words[i])return false;}return true;
 }
 template<class IO> int PeerStatus(IO& io,unsigned w,bool stop) {
  uint32_t marker=io.Q38PanelRead(0x7414c);
  if(marker!=TT_Q38_FFN_PERSISTENT_MARKER && (stop || (marker && marker!=0xdeadbeef)))return -1;
  bool done=false;
  if(marker==TT_Q38_FFN_PERSISTENT_MARKER) {
   if(io.Q38PanelRead(0x74148))return -1;
   uint32_t finish=io.Q38PanelRead(0x74144);if(finish>unsigned(stop))return -1;
   uint32_t begin=io.Q38PanelRead(0x74140);if(begin>unsigned(stop) || finish>begin)return -1;
   uint32_t op=io.Q38PanelRead(0x74150);if(op && (!stop || op!=2))return -1;
   for(unsigned i=5;i<16;i++)if(io.Q38PanelRead(0x74140+i*4))return -1;
   done=stop?begin==1 && finish==1 && op==2:!begin && !finish && !op;
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
  for(unsigned i=0;i<16;i++){unsigned expected=!stop?0:i==10?unsigned(TT_Q38_FFN_PERSISTENT_MARKER):i==11?w:i==12?1:i==13?2:0;
   if(io.Q38PanelRead(0x74000+i*4)!=expected)return false;}
  return io.Q38PanelRead(0x74100)==unsigned(stop);
 }
 template<class IO> bool WaitPeers(IO& io,bool stop,PioneerQ38FfnRootLifetimeResult& r) {
  bool done[119]={};unsigned count=0;uint64_t began=io.Q38CohortNow();
  if(began>UINT64_MAX-UINT64_C(30000000))return false;
  for(unsigned attempt=0;attempt<65536 && count<119;attempt++) {
   uint64_t now=io.Q38CohortNow();if(now<began || now-began>=UINT64_C(30000000))return false;
   for(unsigned w=1;w<120;w++)if(!done[w-1]){r.worker=w;if(!Guard(io,w))return false;
    int state=PeerStatus(io,w,stop);if(state<0)return false;
    if(state==1){if(!Image(io,w) || !PeerFrame(io,w,stop) || !Guard(io,w))return false;
     done[w-1]=true;count++;if(stop)r.stopped++;else r.ready++;}}
   io.Pause();
  }
  return count==119;
 }
 bool BuildStop() {
  // FIXED diagnostic DATA only;dummy learned geometry is NEVER read by
  // mode2. This is deliberately NOT a canonical model/input manifest API.
  auto* d=manifest;d[0]=TT_Q38_FFN_GRAPH_MARKER;d[1]=2;d[2]=67108864;d[3]=0;d[4]=0x04000000;
  d[5]=2;d[7]=3;d[8]=PioneerQ38SpanRoute(0);d[9]=129;d[10]=TT_Q38_FFN_GRAPH_MANIFEST+256;
  d[11]=4352;d[12]=TT_Q38_FFN_GRAPH_MANIFEST+768;d[13]=1;
  for(unsigned i=0;i<16;i++)d[16+i]=cohort->authority.source[0][i];
  d[32]=1;d[34]=1;d[36]=0x53544f50;d[38]=1;
  for(unsigned role=0;role<3;role++) {
   auto* m=d+PioneerQ38FfnGraphRoleOffset(role);m[0]=role;m[1]=12;m[2]=119;m[3]=role==2?17408:5120;m[4]=role==2?5120:17408;
   unsigned groups=m[4]/16,row=0;
   for(unsigned w=0;w<119;w++){auto* p=m+16+w*8;p[0]=0x10000040+role*0x10000000+(w/7)*0x00800000;
    p[1]=w%7;p[2]=groups/119+unsigned(w<groups%119);p[4]=PioneerQ38SpanRoute(p[1]);p[5]=cohort->authority.source[w+1][0];
    m[16+119*8+w]=row;row+=p[2]*16;}
  }
  for(unsigned w=0;w<119;w++)for(unsigned i=0;i<16;i++)sources[w*16+i]=cohort->authority.source[w+1][i];
  return PioneerQ38FfnGraphManifest(d,sources);
 }
 template<class IO> bool RootFrame(IO& io,bool shadow) {
  for(unsigned i=0;i<TT_Q38_FFN_GRAPH_WORDS;i++) {
   if(io.Q38PanelRead(TT_Q38_FFN_GRAPH_MANIFEST+i*4)!=manifest[i])return false;
   if(shadow && io.Q38PanelRead(TT_Q38_FFN_GRAPH_SHADOW+i*4)!=manifest[i])return false;
  }
  for(unsigned w=1;w<120;w++)for(unsigned i=0;i<16;i++)if(io.Q38PanelRead(0x7d800+(w-1)*64+i*4)!=cohort->authority.source[w][i])return false;
  return true;
 }
 template<class IO> bool RootTerminal(IO& io) {
  if(io.RuntimeStatusRead(2)!=TT_Q38_FFN_GRAPH_MARKER || io.RuntimeStatusRead(0)!=0x7200c || io.RuntimeStatusRead(1)!=119)return false;
  for(unsigned i=0;i<2;i++){io.Pause();if(!PioneerQ38RootTerminalPc(io.RuntimePcRead()))return false;}return true;
 }
 template<class IO> bool RootProof(IO& io,PioneerQ38FfnRootLifetimeResult& r) {
  if(!Guard(io,0) || !Image(io,0) || !RootTerminal(io) || !PioneerQ38GraphCsrIdle(io) || !RootFrame(io,true))return false;
  // Validate EACH observed word before reading another. Variable poll
  // counts are bounded,then coupled to exact independently sampled MASTER
  // deltas;they are never caller-provided completion/cleanup hints.
  for(unsigned i=0;i<64;i++) {
   uint32_t value=io.Q38PanelRead(TT_Q38_FFN_GRAPH_STATS+i*4);stats[i]=value;
   if(i==0){if(value!=TT_Q38_FFN_GRAPH_MARKER)return false;}
   else if(i==1 || i==2){if(value!=2)return false;}
   else if(i==6){if(value<8806 || value>4*(357*65536U)+7378)return false;}
   else if(i==7 || i==9){if(value!=238)return false;}
   else if(i==8){if(value!=stats[6])return false;}
   else if(i==10){if(value<357 || value>357*65536U || stats[6]<value+8449 || stats[6]>4*value+7378)return false;}
   else if(i==11){if(value!=357)return false;}
   else if(i==12){if(value!=1)return false;}
   else if(i>=16 && i<24){if(value!=code->counters[0][i-16])return false;}
   else if(i>=24 && i<32){unsigned c=i-24,delta=c==0 || c==1 || c==3?stats[6]:c==2 || c==6?stats[6]+238:238;
    if(value!=uint32_t(code->counters[0][c]+delta))return false;}
   else if(i>=32 && i<48){if(value!=cohort->authority.source[0][i-32])return false;}
   else if(value!=(i==49 || i==54 || i==56?1U:0U))return false;
  }
  for(unsigned w=0;w<119;w++)if(io.Q38PanelRead(TT_Q38_FFN_GRAPH_SEQUENCE+w*4)!=1)return false;
  const uint32_t command[]={0xffb20228,0,Route(119),0x75008,0,129,0,0,4,0};
  for(unsigned i=0;i<10;i++)if(io.ResidentNocRead(i*4)!=command[i])return false;
  for(unsigned i=0;i<8;i++) {
   uint32_t observed=io.ResidentNocRead(PioneerQ38PanelCounter(i));
   unsigned delta=i==0 || i==1 || i==3?stats[6]:i==2 || i==6?stats[6]+238:238;
   if(stats[16+i]!=code->counters[0][i] || stats[24+i]!=observed || uint32_t(observed-code->counters[0][i])!=delta)return false;
   if(rootVerified && observed!=rootAfter[i])return false;
   rootAfter[i]=observed;
  }
  rootVerified=true;r.rootReads=stats[6];r.rootWrites=238;return Guard(io,0);
 }
public:
 PioneerQ38FfnRootLifetimeOwner()=default;
 PioneerQ38FfnRootLifetimeOwner(const PioneerQ38FfnRootLifetimeOwner&)=delete;
 PioneerQ38FfnRootLifetimeOwner& operator=(const PioneerQ38FfnRootLifetimeOwner&)=delete;
 bool Unsafe()const{return unsafe;}
 bool Closed()const{return closed;}
 template<class IO> bool Qualify(IO& io,const tt_dram_samples_result& dram,PioneerResidentBanks& owner,
  PioneerQ38DispatchLease& participants,PioneerQ38FfnGraphCodeOwner& codeOwner,PioneerQ38FfnRootLifetimeResult& r) {
  if(started || unsafe || closed || code || codeOwner.started || codeOwner.unsafe || codeOwner.closed || codeOwner.proof || codeOwner.banks
   || codeOwner.cohort || participants.unsafe || participants.closed || participants.rootRunning || participants.prepared!=120
   || participants.bootAttempted || participants.booted || !owner.Matches(dram) || r.version!=1)return false;
  if(r.started || r.passed || r.unsafe || r.phase || r.worker || r.word || r.savedWords || r.installedWords || r.restoredWords
   || r.savedFrameWords || r.restoredFrameWords || r.savedRootWords || r.restoredRootWords || r.savedStackWords || r.restoredStackWords
   || r.armed || r.released || r.ready || r.stopped || r.held || r.rootArmed || r.rootStarted || r.rootCompleted || r.rootHeld
   || r.rootReads || r.rootWrites)return false;
  for(unsigned w=0;w<120;w++){auto& n=participants.worker[w];const auto& s=n.Store();
   if(!n.image.saved || !n.image.ready || n.image.unsafe || n.image.running || !s.ready || s.pending || s.recovery_required || n.noc.recovery_required)return false;}
  started=true;code=&codeOwner;proof=&dram;banks=&owner;cohort=&participants;r.started=1;
  code->started=true;code->proof=proof;code->banks=banks;code->cohort=cohort;
  for(auto& value:reset)value=0x47800;
  r.phase=1;
  for(unsigned w=0;w<120;w++){r.worker=w;if(!Guard(io,w) || !PioneerQ38GraphCsrCode(io,w))return Fail(io,r);
   for(unsigned i=0;i<10;i++)code->command[w][i]=io.ResidentNocRead(i*4);
   for(unsigned i=0;i<8;i++){code->counters[w][i]=io.ResidentNocRead(PioneerQ38PanelCounter(i));if(i==6 && code->counters[w][2]!=code->counters[w][6])return Fail(io,r);}
   code->sampled=w+1;
   for(unsigned i=0;i<PioneerQ38FfnGraphCodeWords(w);i++){r.word=i;uint32_t value;
    if(!io.FfnGraphProgramReadChecked(w,PioneerQ38FfnGraphCodeAddress(w,i),value))return Fail(io,r);
    code->saved[PioneerQ38FfnGraphCodeBase(w)+i]=value;r.savedWords++;}
   if(w)for(unsigned i=0;i<96;i++){frames[w-1][i]=io.Q38PanelRead(0x74000+i*4);r.savedFrameWords++;}
   else {for(unsigned i=0;i<12288;i++){metadata[i]=io.Q38PanelRead(0x74000+i*4);r.savedRootWords++;}
    for(unsigned i=0;i<2048;i++){stack[i]=io.Q38PanelRead(0x6e000+i*4);r.savedStackWords++;}}
   if(!Guard(io,w))return Fail(io,r);
  }
  if(r.savedWords!=867448 || r.savedFrameWords!=11424 || r.savedRootWords!=12288 || r.savedStackWords!=2048)return Fail(io,r);
  r.phase=2;
  for(unsigned w=0;w<120;w++){r.worker=w;if(!Guard(io,w))return Fail(io,r);
   for(unsigned i=0;i<PioneerQ38FfnGraphCodeWords(w);i++){r.word=i;uint32_t value;
    if(!io.FfnGraphProgramReadChecked(w,PioneerQ38FfnGraphCodeAddress(w,i),value) || value!=code->saved[PioneerQ38FfnGraphCodeBase(w)+i])return Fail(io,r);}
   for(unsigned i=0;i<PioneerQ38FfnGraphCodeWords(w);i++){r.word=i;uint32_t a=PioneerQ38FfnGraphCodeAddress(w,i),actual;
    if(!io.FfnGraphProgramWrite(w,a,PioneerQ38FfnInputGraphProgramWord(w,a)) || !io.FfnGraphProgramReadChecked(w,a,actual)
     || actual!=PioneerQ38FfnInputGraphProgramWord(w,a))return Fail(io,r);
    r.installedWords++;}
   if(!Guard(io,w))return Fail(io,r);
  }
  r.phase=3;for(unsigned w=0;w<120;w++){r.worker=w;if(!Guard(io,w) || !Image(io,w,true))return Fail(io,r);}
  r.phase=4;
  for(unsigned w=1;w<120;w++){r.worker=w;if(!Guard(io,w) || !Image(io,w))return Fail(io,r);
   for(unsigned i=0;i<96;i++)if(io.Q38PanelRead(0x74000+i*4)!=frames[w-1][i])return Fail(io,r);
   uint32_t boot[]={TT_Q38_FFN_PERSISTENT_MARKER,w,Route(w),cohort->authority.source[w][0],0,0,0,0},zero[16]={},sentinel[16];
   for(auto& v:sentinel)v=0xdeadbeef;
   if(!Stage(io,0x74040,boot,8) || !Stage(io,0x74000,zero,16) || !Stage(io,0x74100,zero,16) || !Stage(io,0x74140,sentinel,16))return Fail(io,r);
   for(unsigned i=0;i<10;i++){unsigned value=i==5?Route(w):0;io.ResidentNocWrite(i*4,value);if(io.ResidentNocRead(i*4)!=value)return Fail(io,r);}
   bootCommand[w]=true;if(!Guard(io,w))return Fail(io,r);
  }
  r.phase=5;
  for(unsigned w=1;w<120;w++){r.worker=w;if(!Guard(io,w))return Fail(io,r);reset[w]=0x87ffff;r.armed++;io.RuntimeResetWrite(reset[w]);io.Pause();if(!Guard(io,w))return Fail(io,r);}
  for(unsigned w=1;w<120;w++){r.worker=w;if(!Guard(io,w))return Fail(io,r);running[w]=cohort->worker[w].image.running=true;reset[w]=0x46800;r.released++;io.RuntimeResetWrite(reset[w]);if(!Guard(io,w))return Fail(io,r);}
  if(!WaitPeers(io,false,r))return Fail(io,r);
  r.phase=6;for(unsigned w=1;w<120;w++){r.worker=w;if(!Guard(io,w) || !Image(io,w) || !PeerFrame(io,w,false) || PeerStatus(io,w,false)!=1)return Fail(io,r);}
  r.worker=0;if(!Guard(io,0) || !Image(io,0) || !BuildStop())return Fail(io,r);
  for(unsigned i=0;i<12288;i++)if(io.Q38PanelRead(0x74000+i*4)!=metadata[i])return Fail(io,r);
  for(unsigned i=0;i<2048;i++)if(io.Q38PanelRead(0x6e000+i*4)!=stack[i])return Fail(io,r);
  if(!Stage(io,TT_Q38_FFN_GRAPH_MANIFEST,manifest,TT_Q38_FFN_GRAPH_WORDS))return Fail(io,r);
  for(unsigned w=1;w<120;w++)if(!Stage(io,0x7d800+(w-1)*64,cohort->authority.source[w],16))return Fail(io,r);
  for(unsigned i=0;i<3;i++){io.RuntimeStatusWrite(i,0);if(io.RuntimeStatusRead(i))return Fail(io,r);}
  if(!RootFrame(io,false) || !Guard(io,0) || !Image(io,0))return Fail(io,r);
  r.phase=7;running[0]=cohort->worker[0].image.running=cohort->rootRunning=true;
  reset[0]=0x87ffff;r.rootArmed=1;io.RuntimeResetWrite(reset[0]);io.Pause();if(!Guard(io,0))return Fail(io,r);
  uint64_t began=io.Q38CohortNow();if(began>UINT64_MAX-UINT64_C(30000000))return Fail(io,r);
  rootTraffic=true;reset[0]=0x46800;r.rootStarted=1;io.RuntimeResetWrite(reset[0]);
  bool complete=false;
  for(unsigned attempt=0;attempt<65536;attempt++) {
   if(!Guard(io,0))return Fail(io,r);
   uint64_t now=io.Q38CohortNow();if(now<began || now-began>=UINT64_C(30000000))return Fail(io,r);
   uint32_t marker=io.RuntimeStatusRead(2);if(marker && marker!=TT_Q38_FFN_GRAPH_MARKER)return Fail(io,r);
   if(marker){if(!RootProof(io,r))return Fail(io,r);complete=true;r.rootCompleted=1;break;}
   io.Pause();uint32_t pc=io.RuntimePcRead();if(!PioneerQ38FfnInputGraphExecutableAddress(0,pc) || (pc>=0x72060 && pc<0x72080))return Fail(io,r);
  }
  if(!complete || !WaitPeers(io,true,r))return Fail(io,r);
  r.phase=8;
  for(unsigned w=1;w<120;w++){r.worker=w;if(!Guard(io,w) || !Image(io,w) || !PeerFrame(io,w,true) || PeerStatus(io,w,true)!=1)return Fail(io,r);}
  r.worker=0;if(!RootProof(io,r))return Fail(io,r); // ALL terminals BEFORE first hold
  reset[0]=0x87ffff;io.RuntimeResetWrite(reset[0]);io.Pause();if(!Guard(io,0))return Fail(io,r);
  reset[0]=0x47800;io.RuntimeResetWrite(reset[0]);io.Pause();if(!Guard(io,0))return Fail(io,r);
  running[0]=cohort->worker[0].image.running=cohort->rootRunning=false;r.rootHeld=1;
  for(unsigned w=1;w<120;w++){r.worker=w;if(!Guard(io,w) || !Image(io,w) || !PeerFrame(io,w,true) || PeerStatus(io,w,true)!=1)return Fail(io,r);
   reset[w]=0x87ffff;io.RuntimeResetWrite(reset[w]);io.Pause();if(!Guard(io,w))return Fail(io,r);
   reset[w]=0x47800;io.RuntimeResetWrite(reset[w]);io.Pause();if(!Guard(io,w))return Fail(io,r);
   running[w]=cohort->worker[w].image.running=false;r.held++;}
  r.phase=9;
  for(unsigned w=0;w<120;w++){r.worker=w;if(!Guard(io,w) || !Image(io,w))return Fail(io,r);
   for(unsigned i=0;i<PioneerQ38FfnGraphCodeWords(w);i++){r.word=i;uint32_t a=PioneerQ38FfnGraphCodeAddress(w,i),value=code->saved[PioneerQ38FfnGraphCodeBase(w)+i],actual;
    if(!io.FfnGraphProgramWrite(w,a,value) || !io.FfnGraphProgramReadChecked(w,a,actual) || actual!=value)return Fail(io,r);
    r.restoredWords++;}
   if(w){if(!Stage(io,0x74000,frames[w-1],96))return Fail(io,r);r.restoredFrameWords+=96;}
   else {if(!Stage(io,0x74000,metadata,12288) || !Stage(io,0x6e000,stack,2048))return Fail(io,r);r.restoredRootWords=12288;r.restoredStackWords=2048;}
   for(unsigned i=0;i<10;i++){io.ResidentNocWrite(i*4,code->command[w][i]);if(io.ResidentNocRead(i*4)!=code->command[w][i])return Fail(io,r);}
   bootCommand[w]=false;if(!w)rootCommandRestored=true;
   if(!Guard(io,w) || !PioneerQ38GraphCsrCode(io,w))return Fail(io,r);
  }
  r.phase=10;
  for(unsigned w=0;w<120;w++){r.worker=w;if(!Guard(io,w))return Fail(io,r);
   for(unsigned i=0;i<PioneerQ38FfnGraphCodeWords(w);i++){r.word=i;uint32_t value;
    if(!io.FfnGraphProgramReadChecked(w,PioneerQ38FfnGraphCodeAddress(w,i),value) || value!=code->saved[PioneerQ38FfnGraphCodeBase(w)+i])return Fail(io,r);}
   if(w){for(unsigned i=0;i<96;i++)if(io.Q38PanelRead(0x74000+i*4)!=frames[w-1][i])return Fail(io,r);}
   else {for(unsigned i=0;i<12288;i++)if(io.Q38PanelRead(0x74000+i*4)!=metadata[i])return Fail(io,r);
    for(unsigned i=0;i<2048;i++)if(io.Q38PanelRead(0x6e000+i*4)!=stack[i])return Fail(io,r);}
  }
  if(r.armed!=119 || r.released!=119 || r.ready!=119 || r.stopped!=119 || r.held!=119 || r.installedWords!=867448 || r.restoredWords!=867448
   || r.restoredFrameWords!=11424 || r.restoredRootWords!=12288 || r.restoredStackWords!=2048 || !Guard(io,0))return Fail(io,r);
  closed=code->closed=true;r.passed=1;return true;
 }
};
static_assert(sizeof(PioneerQ38FfnRootLifetimeOwner)>110000 && sizeof(PioneerQ38FfnRootLifetimeOwner)<131072,"ROOT metadata+stack+ALL119 frames are HEAP ONLY");
#endif
