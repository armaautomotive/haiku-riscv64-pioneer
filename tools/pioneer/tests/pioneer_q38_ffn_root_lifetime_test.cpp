/* SPDX-License-Identifier: MIT */
// Actual controller,SYNTHETIC endpoints only. Native scope and actual
// root/worker instruction execution are NOT supplied by this test fabric.
#define PIONEER_Q38_TEST_QUALIFIED_LEGACY
#define PIONEER_Q38_FFN_CODE_OWNER_MOCK_ONLY
#include "pioneer_q38_ffn_graph_code_owner_test.cpp"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38FfnRootLifetimeOwner.h"
struct RootLifetimeIO:CodeIO {
 const PioneerQ38FfnRootLifetimeResult* life=nullptr;
 unsigned failure=0,wantedPhase=7,wantedWorker=0,point=0,resetWrites=0,commandWrites=0;
 bool rootDone=false,droppedMeta=false;
 uint32_t droppedAddress=0;
 bool AtLife()const{return track && life && life->phase==wantedPhase && current==wantedWorker;}
 uint32_t BadLife(uint32_t value){observed=touches;return value^1;}
 bool FfnGraphCodeMappingStable(unsigned w){Touch();assert(w==current);if(failure==3 && AtLife()){observed=touches;return false;}return true;}
 uint32_t FfnGraphProgramRead(unsigned w,uint32_t a){Touch();reads++;return RawRead(w,a);}
 bool FfnGraphProgramReadChecked(unsigned w,uint32_t a,uint32_t& output) {
  if(failure==1 && AtLife() && a==PioneerQ38FfnGraphCodeAddress(w,point)){Touch();observed=touches;return false;}
  output=FfnGraphProgramRead(w,a);return true;
 }
 bool FfnGraphProgramWrite(unsigned w,uint32_t a,uint32_t value) {
  Touch();writes++;assert(life && life->savedWords==867448 && (life->phase==2 || life->phase==9));
  if(failure==2 && AtLife() && a==PioneerQ38FfnGraphCodeAddress(w,point)){observed=touches;return false;}
  RawWrite(w,a,value);return true;
 }
 uint32_t ResetRead(){Touch();uint32_t v=BootstrapIO::ResetRead();return failure==4 && AtLife()?BadLife(v):v;}
 uint32_t SourceRead(unsigned i){Touch();uint32_t v=BootstrapIO::SourceRead(i);return failure==5 && AtLife()?BadLife(v):v;}
 uint32_t ResidentNocRead(uint32_t offset){Touch();uint32_t value=BootstrapIO::ResidentNocRead(offset);
  if(AtLife() && ((failure==6 && offset==0x130) || (failure==9 && rootDone && offset==point*4)
   || (failure==10 && rootDone && offset==PioneerQ38PanelCounter(point))))return BadLife(value);
  return value;}
 uint32_t HostPciRead(unsigned p,uint16_t o,uint8_t b){Touch();uint32_t v=BootstrapIO::HostPciRead(p,o,b);return failure==7 && AtLife() && o==4?BadLife(v):v;}
 uint32_t Q38PanelRead(uint32_t a){Touch();uint32_t value=BootstrapIO::Q38PanelRead(a);
  if(droppedMeta && a==droppedAddress){droppedMeta=false;observed=touches;return value;}
  if(failure==8 && AtLife() && rootDone && a==TT_Q38_FFN_GRAPH_STATS+point*4){observed=touches;return point==6 || point==10?0:value^1;}
  return value;
 }
 void Q38PanelWrite(uint32_t a,uint32_t value) {
  if(!track){BootstrapIO::Q38PanelWrite(a,value);return;}
  Touch();assert(!quarantined && life);
  if(failure==12 && AtLife() && a==point){droppedMeta=true;droppedAddress=a;return;}
  BootstrapIO::Q38PanelWrite(a,value);
 }
 uint32_t RuntimeStatusRead(unsigned i){Touch();uint32_t value=BootstrapIO::RuntimeStatusRead(i);
  return failure==13 && AtLife() && rootDone && i==point?BadLife(value):value;}
 void RuntimeStatusWrite(unsigned i,uint32_t value){Touch();BootstrapIO::RuntimeStatusWrite(i,value);}
 virtual uint32_t CurrentPc(){return current?(rootDone?TT_Q38_PEER_STOP:TT_Q38_PEER_WAIT_LOAD):rootDone?TT_Q38_DISPATCH_PARK:0x68000;}
 uint32_t RuntimePcRead(){Touch();if(failure==11 && AtLife()){observed=touches;return 0xdeadc0dc;}return CurrentPc();}
 uint64_t Q38CohortNow(){Touch();if(failure==14 && AtLife()){observed=touches;return UINT64_MAX-10;}return BootstrapIO::Q38CohortNow();}
 void ResidentNocWrite(uint32_t a,uint32_t value) {
  if(!track){CodeIO::ResidentNocWrite(a,value);return;}
  Touch();assert(!quarantined && life && a<40 && !(a&3) && (life->phase==4 || life->phase==9));commandWrites++;
  BootstrapIO::ResidentNocWrite(a,value);
 }
 virtual void ReleasedPeer() {
  for(unsigned i=0;i<16;i++)C().view[(0x74140-TT_Q38_PANEL_VIEW_BASE)/4+i]=i==3?unsigned(TT_Q38_FFN_PERSISTENT_MARKER):0;
 }
 virtual void ReleasedRoot() {
  // SYNTHETIC completion fabric,NOT actual program evidence. A separate
  // actual-instruction adapter must drive this same controller later.
  assert(!current && life->ready==119 && life->rootStarted && !rootDone);
  auto& c=C();uint32_t before[8];for(unsigned i=0;i<8;i++)before[i]=BootstrapIO::ResidentNocRead(PioneerQ38PanelCounter(i));
  const auto read=[&](unsigned a){return c.view[(a-TT_Q38_PANEL_VIEW_BASE)/4];};
  auto put=[&](unsigned a,uint32_t value){c.view[(a-TT_Q38_PANEL_VIEW_BASE)/4]=value;};
  for(unsigned i=0;i<TT_Q38_FFN_GRAPH_WORDS;i++)put(TT_Q38_FFN_GRAPH_SHADOW+i*4,read(TT_Q38_FFN_GRAPH_MANIFEST+i*4));
  for(unsigned w=1;w<120;w++) {
   auto& p=*core[w];auto peerPut=[&](unsigned a,uint32_t value){p.view[(a-TT_Q38_PANEL_VIEW_BASE)/4]=value;};
   for(unsigned i=0;i<16;i++){peerPut(0x74000+i*4,i==10?unsigned(TT_Q38_FFN_PERSISTENT_MARKER):i==11?w:i==12?1:i==13?2:0);
    peerPut(0x74140+i*4,i<2?1:i==3?unsigned(TT_Q38_FFN_PERSISTENT_MARKER):i==4?2:0);}
   peerPut(0x74100,1);put(TT_Q38_FFN_GRAPH_SEQUENCE+(w-1)*4,1);
  }
  c.counters[0]+=8806;c.counters[1]+=8806;c.counters[3]+=8806;
  c.writeCounters[0]+=238;c.writeCounters[1]+=238;c.writeCounters[3]+=238;c.accepted+=8806+238;
  const uint32_t last[]={0xffb20228,0,PioneerWorkerX(119)|(PioneerWorkerY(119)<<6),0x75008,0,129,0,0,4,0};
  for(unsigned i=0;i<10;i++)c.command[i]=last[i];
  for(unsigned i=0;i<64;i++)put(TT_Q38_FFN_GRAPH_STATS+i*4,0);
  const uint32_t header[]={TT_Q38_FFN_GRAPH_MARKER,2,2,0,0,0,8806,238,8806,238,357,357,1};
  for(unsigned i=0;i<13;i++)put(TT_Q38_FFN_GRAPH_STATS+i*4,header[i]);
  for(unsigned i=0;i<8;i++){put(TT_Q38_FFN_GRAPH_STATS+(16+i)*4,before[i]);put(TT_Q38_FFN_GRAPH_STATS+(24+i)*4,BootstrapIO::ResidentNocRead(PioneerQ38PanelCounter(i)));}
  for(unsigned i=0;i<16;i++)put(TT_Q38_FFN_GRAPH_STATS+(32+i)*4,read(TT_Q38_FFN_GRAPH_MANIFEST+(16+i)*4));
  for(unsigned i:{49U,54U,56U})put(TT_Q38_FFN_GRAPH_STATS+i*4,1);
  put(0x6e000,read(0x6e000)^1); // root stack really became dirty
  c.destination[2032]=0x7200c;c.destination[2033]=119;c.destination[2034]=TT_Q38_FFN_GRAPH_MARKER;rootDone=true;
 }
 void RuntimeResetWrite(uint32_t value) {
  if(!track){BootstrapIO::RuntimeResetWrite(value);return;}
  Touch();assert(life && !quarantined);resetWrites++;
  if(life->phase==8)assert(life->rootCompleted && life->stopped==119);
  C().coreReset=value;
  if(value==0x46800){if(current)ReleasedPeer();else ReleasedRoot();}
 }
};
template<class IO>struct RootLifetimeFixture {
 IO io;std::unique_ptr<PioneerQ38DispatchLease> cohort{new PioneerQ38DispatchLease};
 std::unique_ptr<PioneerQ38FfnGraphCodeOwner> code{new PioneerQ38FfnGraphCodeOwner};
 std::unique_ptr<PioneerQ38FfnRootLifetimeOwner> lifetime{new PioneerQ38FfnRootLifetimeOwner};
 std::vector<std::unique_ptr<PanelOwners>> owners;tt_dram_samples_result dram;PioneerResidentBanks banks;
 PioneerQ38FfnRootLifetimeResult result;
 RootLifetimeFixture(){dram=BankProof(*io.core[0]);assert(banks.Bind(dram));
  for(unsigned w=0;w<120;w++){owners.emplace_back(new PanelOwners(*io.core[w]));auto& n=cohort->worker[w];n.master=&owners[w]->store;
   n.noc=owners[w]->noc;if(w)n.noc.version=PioneerWorkerVersion(w);assert(PioneerQ38DispatchPrepareRole(io,dram,banks,*cohort,w));}
  io.life=&result;io.state=nullptr;io.track=true;}
 bool Run(){return lifetime->Qualify(io,dram,banks,*cohort,*code,result);}
 void Unknown(){assert(result.started && result.unsafe && !result.passed && lifetime->Unsafe() && !lifetime->Closed()
  && code->Unsafe() && !code->Closed() && cohort->unsafe && banks.Quarantined() && io.quarantined);
  assert(io.observed && io.observed==io.touches);unsigned before=io.touches;
  assert(!Run() && !PioneerQ38DispatchBootstrap(io,dram,banks,*cohort) && io.touches==before);
  for(auto& n:cohort->worker)assert(n.image.unsafe && n.Store().recovery_required);
 }
};
#ifndef PIONEER_Q38_FFN_ROOT_LIFETIME_FIXTURE_ONLY
int main() {
 (void)&Input;(void)&Prepare;(void)&Upload;(void)&Close;(void)&PreparePanel;(void)&Run;(void)&Job;(void)&ClosePanel;(void)&Finish;
 unsigned positive=0,faults=0,refusals=0;
 // Mutate every public result field WITHOUT walking across struct members
 // as a uint32_t array. No result flag/receipt may authorize or skip work.
 uint32_t PioneerQ38FfnRootLifetimeResult::* const invalidFields[]={
  &PioneerQ38FfnRootLifetimeResult::version,&PioneerQ38FfnRootLifetimeResult::started,&PioneerQ38FfnRootLifetimeResult::passed,
  &PioneerQ38FfnRootLifetimeResult::unsafe,&PioneerQ38FfnRootLifetimeResult::phase,&PioneerQ38FfnRootLifetimeResult::worker,
  &PioneerQ38FfnRootLifetimeResult::word,&PioneerQ38FfnRootLifetimeResult::savedWords,&PioneerQ38FfnRootLifetimeResult::installedWords,
  &PioneerQ38FfnRootLifetimeResult::restoredWords,&PioneerQ38FfnRootLifetimeResult::savedFrameWords,&PioneerQ38FfnRootLifetimeResult::restoredFrameWords,
  &PioneerQ38FfnRootLifetimeResult::savedRootWords,&PioneerQ38FfnRootLifetimeResult::restoredRootWords,&PioneerQ38FfnRootLifetimeResult::savedStackWords,
  &PioneerQ38FfnRootLifetimeResult::restoredStackWords,&PioneerQ38FfnRootLifetimeResult::armed,&PioneerQ38FfnRootLifetimeResult::released,
  &PioneerQ38FfnRootLifetimeResult::ready,&PioneerQ38FfnRootLifetimeResult::stopped,&PioneerQ38FfnRootLifetimeResult::held,
  &PioneerQ38FfnRootLifetimeResult::rootArmed,&PioneerQ38FfnRootLifetimeResult::rootStarted,&PioneerQ38FfnRootLifetimeResult::rootCompleted,
  &PioneerQ38FfnRootLifetimeResult::rootHeld,&PioneerQ38FfnRootLifetimeResult::rootReads,&PioneerQ38FfnRootLifetimeResult::rootWrites};
 for(auto field:invalidFields){RootLifetimeFixture<RootLifetimeIO> f;
  f.result.*field^=1;unsigned before=f.io.touches,value=f.result.*field;
  assert(!f.Run() && f.result.*field==value && f.io.touches==before && !f.io.quarantined);refusals++;
 }
 for(bool wrapped:{false,true}) {
  RootLifetimeFixture<RootLifetimeIO> f;auto prefix=f.io.prefix;std::vector<uint32_t> original,meta,stack;
  f.io.track=false;for(unsigned w=0;w<120;w++){f.io.SelectRuntimeCore(w);
   if(wrapped){auto& c=f.io.C();c.accepted=UINT32_MAX-w;c.counters[0]=UINT32_MAX-w;c.counters[1]=UINT32_MAX-w;c.counters[3]=UINT32_MAX-w;}
   for(unsigned i=0;i<PioneerQ38FfnGraphCodeWords(w);i++)original.push_back(f.io.RawRead(w,PioneerQ38FfnGraphCodeAddress(w,i)));
   for(unsigned i=0;i<(w?96:12288);i++)meta.push_back(f.io.BootstrapIO::Q38PanelRead(0x74000+i*4));
   if(!w)for(unsigned i=0;i<2048;i++)stack.push_back(f.io.BootstrapIO::Q38PanelRead(0x6e000+i*4));}
  f.io.track=true;bool okay=f.Run();if(!okay)fprintf(stderr,"ROOT_LIFETIME_FAIL phase%u worker%u word%u saved%u copied%u ready%u root%u/%u stopped%u held%u;SYNTHETIC_ENDPOINT_TEST_ONLY\n",f.result.phase,f.result.worker,f.result.word,f.result.savedWords,f.result.installedWords,f.result.ready,f.result.rootStarted,f.result.rootCompleted,f.result.stopped,f.result.held);
  assert(okay && f.result.passed && f.lifetime->Closed() && f.code->Closed() && f.result.rootReads==8806 && f.result.rootWrites==238);
  assert(f.result.savedWords==867448 && f.result.installedWords==867448 && f.result.restoredWords==867448
   && f.result.savedRootWords==12288 && f.result.restoredRootWords==12288 && f.result.savedStackWords==2048
   && f.result.restoredStackWords==2048 && f.result.savedFrameWords==11424 && f.result.restoredFrameWords==11424
   && f.result.armed==119 && f.result.released==119 && f.result.ready==119 && f.result.stopped==119 && f.result.held==119
   && f.result.rootArmed==1 && f.result.rootStarted==1 && f.result.rootCompleted==1 && f.result.rootHeld==1
   && f.io.resetWrites==480 && f.io.commandWrites==2390 && f.io.prefix==prefix);
  unsigned before=f.io.touches;assert(!f.Run() && f.io.touches==before);
  f.io.track=false;unsigned at=0;for(unsigned w=0;w<120;w++){f.io.SelectRuntimeCore(w);assert(f.io.ResetRead()==0x47800 && !f.cohort->worker[w].image.running);
   for(unsigned i=0;i<PioneerQ38FfnGraphCodeWords(w);i++)assert(f.io.RawRead(w,PioneerQ38FfnGraphCodeAddress(w,i))==original[PioneerQ38FfnGraphCodeBase(w)+i]);
   for(unsigned i=0;i<(w?96:12288);i++)assert(f.io.BootstrapIO::Q38PanelRead(0x74000+i*4)==meta[at++]);
   if(!w)for(unsigned i=0;i<2048;i++)assert(f.io.BootstrapIO::Q38PanelRead(0x6e000+i*4)==stack[i]);}
  f.io.rootDone=false;f.io.armed=f.io.released=0;assert(PioneerQ38DispatchBootstrap(f.io,f.dram,f.banks,*f.cohort));positive++;
 }
 for(unsigned field=0;field<64;field++){RootLifetimeFixture<RootLifetimeIO> f;f.io.failure=8;f.io.point=field;assert(!f.Run());f.Unknown();faults++;}
 for(unsigned phase=1;phase<=10;phase++)for(unsigned worker:{0U,60U,119U}) {
  if(!worker && (phase==4 || phase==5))continue; // ONLY peers read program in these phases.
  RootLifetimeFixture<RootLifetimeIO> f;f.io.failure=1;f.io.wantedPhase=phase;f.io.wantedWorker=worker;f.io.point=1;
  assert(!f.Run());f.Unknown();faults++;
 }
 for(unsigned phase:{2U,9U})for(unsigned worker:{0U,60U,119U}){RootLifetimeFixture<RootLifetimeIO> f;
  f.io.failure=2;f.io.wantedPhase=phase;f.io.wantedWorker=worker;f.io.point=1;assert(!f.Run());f.Unknown();faults++;}
 for(unsigned phase:{5U,7U,8U,9U,10U})for(unsigned worker:{0U,60U,119U})for(unsigned kind:{3U,4U,5U,6U,7U}) {
  if(phase==5 && !worker)continue; // ROOT is untouched while ALL peers arm/release.
  RootLifetimeFixture<RootLifetimeIO> f;f.io.failure=kind;f.io.wantedPhase=phase;f.io.wantedWorker=worker;assert(!f.Run());f.Unknown();faults++;
 }
 for(unsigned kind:{9U,10U})for(unsigned point=0;point<(kind==9?10U:8U);point++){RootLifetimeFixture<RootLifetimeIO> f;
  f.io.failure=kind;f.io.point=point;assert(!f.Run());f.Unknown();faults++;}
 for(unsigned point:{0x75800U,0x6e000U}){RootLifetimeFixture<RootLifetimeIO> f;f.io.failure=12;f.io.wantedPhase=9;f.io.point=point;assert(!f.Run());f.Unknown();faults++;}
 for(unsigned i=0;i<3;i++){RootLifetimeFixture<RootLifetimeIO> f;f.io.failure=13;f.io.point=i;assert(!f.Run());f.Unknown();faults++;}
 for(unsigned phase:{5U,7U}){RootLifetimeFixture<RootLifetimeIO> f;f.io.failure=14;f.io.wantedPhase=phase;f.io.wantedWorker=phase==5?119:0;assert(!f.Run());f.Unknown();faults++;}
 printf("PASS OFFLINE ROOT+ALL119 lifetime controller:positive%u/faults%u/inertResultRefusals%u,867448program/11424peer-control/12288root-meta/2048root-stack words restored;ROOT observed8806reads238writes,wrapped counters NEVER restored,ALLphysical terminals before FIRSThold,UNKNOWN memory quarantine and ZERO later IO;SYNTHETIC_ENDPOINTS_NOT_ACTUAL_PROGRAM_OR_NATIVE_SCOPE_MODEL_SPEED_AUTHORITY\n",positive,faults,refusals);
}
#endif
