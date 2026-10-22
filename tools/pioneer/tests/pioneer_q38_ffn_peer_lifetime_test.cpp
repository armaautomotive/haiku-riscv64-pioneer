/* SPDX-License-Identifier: MIT */
// Actual controller;synthetic hardware endpoints,NOT native qualification.
#define PIONEER_Q38_FFN_CODE_OWNER_MOCK_ONLY
#include "pioneer_q38_ffn_graph_code_owner_test.cpp"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38FfnPeerLifetimeOwner.h"
struct PeerLifetimeIO:CodeIO {
 const PioneerQ38FfnPeerLifetimeResult* life=nullptr;
 unsigned failure=0,wantedPhase=0,wantedWorker=1,wantedWord=1,resetWrites=0,commandWrites=0,statusReads=0;
 unsigned targetSourceReads=0,skipSourceReads=0,peerPc=TT_Q38_PEER_WAIT_LOAD;
 bool panelDropped=false,stopFetch=false;
 unsigned badStatusWord=2;
 bool AtLife()const{return track && life && life->phase==wantedPhase && current==wantedWorker;}
 uint32_t BadLife(uint32_t v){observed=touches;return v^1;}
 bool SelectRuntimeCore(unsigned w){return CodeIO::SelectRuntimeCore(w);}
 bool FfnGraphCodeMappingStable(unsigned w){Touch();assert(w==current);if(failure==4 && AtLife()){observed=touches;return false;}return true;}
 uint32_t FfnGraphProgramRead(unsigned w,uint32_t a){Touch();reads++;uint32_t v=RawRead(w,a);
  if(failure==1 && AtLife() && a==PioneerQ38FfnGraphCodeAddress(w,wantedWord))return BadLife(v);return v;}
 bool FfnGraphProgramReadChecked(unsigned w,uint32_t a,uint32_t& output){
  if(failure==25 && AtLife()){Touch();observed=touches;return false;}
  output=FfnGraphProgramRead(w,a);return true;}
 bool FfnGraphProgramWrite(unsigned w,uint32_t a,uint32_t v){Touch();writes++;
  assert(life && life->savedWords==867448 && (life->phase==2 || life->phase==8));
  if(failure==2 && AtLife() && a==PioneerQ38FfnGraphCodeAddress(w,wantedWord)){observed=touches;return false;}
  RawWrite(w,a,v);return true;}
 uint32_t ResetRead(){Touch();uint32_t v=BootstrapIO::ResetRead();return failure==5 && AtLife()?BadLife(v):v;}
 uint32_t SourceRead(unsigned i){Touch();uint32_t v=BootstrapIO::SourceRead(i);return failure==6 && AtLife()?BadLife(v):v;}
 uint32_t ResidentNocRead(uint32_t o){Touch();uint32_t v=BootstrapIO::ResidentNocRead(o);
  if(AtLife() && ((failure==7 && o==0x130 && ++targetSourceReads>skipSourceReads) || (failure==8 && o==0x208)
   || (failure==10 && o==12) || (failure==21 && o==0x210 && ++aliasReads==2)))return BadLife(v);
  if(panelDropped && o==20){panelDropped=false;observed=touches;}return v;}
 uint32_t HostPciRead(unsigned p,uint16_t o,uint8_t b){Touch();uint32_t v=BootstrapIO::HostPciRead(p,o,b);return failure==9 && AtLife() && o==4?BadLife(v):v;}
 uint32_t Q38PanelRead(uint32_t a){Touch();uint32_t v=BootstrapIO::Q38PanelRead(a);
  if(a>=0x74140 && a<0x74180)statusReads++;
  if(failure==12 && AtLife() && a==0x7414c){observed=touches;return 1;}
  if(failure==22 && AtLife() && a==0x74040)return BadLife(v);
  if(failure==24 && AtLife() && a==0x74140+badStatusWord*4){observed=touches;return badStatusWord<2?2:badStatusWord==4?3:1;}
  if(panelDropped && (a==0x74040 || a==0x74028 || a==0x74100)){panelDropped=false;observed=touches;}return v;}
 void Q38PanelWrite(uint32_t a,uint32_t v){Touch();assert(!quarantined);
  if(AtLife() && ((failure==13 && a==0x74040) || (failure==14 && a==0x74100)
   || ((failure==15 || failure==23) && a==0x74028))){panelDropped=true;return;}
  if(track && life && life->phase==6){assert(current && C().coreReset==0x46800 && ((a>=0x74000 && a<0x74040) || a==0x74100));C().view[(a-TT_Q38_PANEL_VIEW_BASE)/4]=v;}
  else BootstrapIO::Q38PanelWrite(a,v);
  if(track && life && life->phase==6 && a==0x74100 && v==1)Stopped();
 }
 virtual void Stopped(){for(unsigned i=0;i<16;i++)C().view[(0x74140-TT_Q38_PANEL_VIEW_BASE)/4+i]=i<2?1:i==3?unsigned(TT_Q38_FFN_PERSISTENT_MARKER):i==4?2:0;}
 virtual uint32_t CurrentPc(){return BootstrapIO::Q38PanelRead(0x74100)==1?(stopFetch?TT_Q38_PEER_STOP_FETCH:TT_Q38_PEER_STOP):peerPc;}
 uint32_t RuntimePcRead(){Touch();if(failure==11 && AtLife()){observed=touches;return 0xdeadc0dc;}
  if(failure==18 && AtLife()){observed=touches;return TT_Q38_PEER_WAIT_LOAD;}return CurrentPc();}
 virtual void Released(){for(unsigned i=0;i<16;i++)C().view[(0x74140-TT_Q38_PANEL_VIEW_BASE)/4+i]=i==3?unsigned(TT_Q38_FFN_PERSISTENT_MARKER):0;}
 void RuntimeResetWrite(uint32_t v){if(!track){BootstrapIO::RuntimeResetWrite(v);return;}Touch();assert(current && life && !quarantined);resetWrites++;
  assert(life->phase==5 || life->phase==7);
  if(life->phase==5){if(v==0x87ffff){armed++;assert(!released);}else{assert(v==0x46800 && armed==119);released++;}}
  else{assert(life->stopped==119 && (v==0x87ffff || v==0x47800));}
  C().coreReset=v;if(v==0x46800)Released();
 }
 void ResidentNocWrite(uint32_t o,uint32_t v){if(!track){CodeIO::ResidentNocWrite(o,v);return;}Touch();assert(!quarantined && life && o<40 && !(o&3));
  assert(life->phase==4 || life->phase==8);commandWrites++;
  if(failure==20 && AtLife() && o==20){panelDropped=true;return;}BootstrapIO::ResidentNocWrite(o,v);
 }
 uint64_t Q38CohortNow(){Touch();if(failure==16 && AtLife()){observed=touches;return UINT64_MAX-10;}return BootstrapIO::Q38CohortNow();}
};
template<class IO>struct PeerLifetimeFixture {
 IO io;std::unique_ptr<PioneerQ38DispatchLease> cohort{new PioneerQ38DispatchLease};
 std::unique_ptr<PioneerQ38FfnGraphCodeOwner> code{new PioneerQ38FfnGraphCodeOwner};
 std::unique_ptr<PioneerQ38FfnPeerLifetimeOwner> lifetime{new PioneerQ38FfnPeerLifetimeOwner};
 std::vector<std::unique_ptr<PanelOwners>> owners;tt_dram_samples_result dram;PioneerResidentBanks banks;
 PioneerQ38FfnPeerLifetimeResult result;
 PeerLifetimeFixture(){dram=BankProof(*io.core[0]);assert(banks.Bind(dram));
  for(unsigned w=0;w<120;w++){owners.emplace_back(new PanelOwners(*io.core[w]));auto& n=cohort->worker[w];n.master=&owners[w]->store;
   n.noc=owners[w]->noc;if(w)n.noc.version=PioneerWorkerVersion(w);assert(PioneerQ38DispatchPrepareRole(io,dram,banks,*cohort,w));}
  io.life=&result;io.state=nullptr;io.track=true;
 }
 bool Run(){return lifetime->Qualify(io,dram,banks,*cohort,*code,result);}
 void Unknown(){assert(result.started && result.unsafe && !result.passed && lifetime->Unsafe() && !lifetime->Closed()
  && code->Unsafe() && !code->Closed() && cohort->unsafe && banks.Quarantined() && io.quarantined);
  assert(io.observed && io.observed==io.touches);unsigned touches=io.touches,writes=io.writes,resets=io.resetWrites;
  assert(!Run() && !PioneerQ38DispatchBootstrap(io,dram,banks,*cohort));
  assert(touches==io.touches && writes==io.writes && resets==io.resetWrites);
  for(auto& n:cohort->worker)assert(n.image.unsafe && n.Store().recovery_required);
 }
};
#ifndef PIONEER_Q38_FFN_PEER_LIFETIME_FIXTURE_ONLY
int main(){
 (void)&Input;(void)&Prepare;(void)&Upload;(void)&Close;(void)&PreparePanel;(void)&Run;(void)&Job;(void)&ClosePanel;(void)&Finish;
 unsigned positive=0,faults=0;
 for(bool wrapped:{false,true}) {
  PeerLifetimeFixture<PeerLifetimeIO> f;f.io.stopFetch=wrapped;
  if(wrapped)std::fill(f.io.prefix.begin(),f.io.prefix.end(),UINT32_MAX); // valid data,NOT a mapping-error sentinel
  auto prefix=f.io.prefix;std::vector<uint32_t> original,frames;
  f.io.track=false;for(unsigned w=0;w<120;w++){f.io.SelectRuntimeCore(w);
   if(wrapped){auto& c=f.io.C();c.accepted=UINT32_MAX-w;c.counters[0]=UINT32_MAX-2-w;c.counters[1]=0xfff00000+w;}
   for(unsigned i=0;i<PioneerQ38FfnGraphCodeWords(w);i++)original.push_back(f.io.RawRead(w,PioneerQ38FfnGraphCodeAddress(w,i)));
   if(w)for(unsigned i=0;i<96;i++)frames.push_back(f.io.BootstrapIO::Q38PanelRead(0x74000+i*4));}
  f.io.track=true;assert(f.Run() && f.result.passed && !f.result.unsafe && f.lifetime->Closed() && f.code->Closed());
  assert(f.result.savedWords==867448 && f.result.installedWords==867448 && f.result.restoredWords==867448
   && f.result.armed==119 && f.result.released==119 && f.result.ready==119 && f.result.stopIssued==119
   && f.result.stopped==119 && f.result.held==119 && f.result.savedFrameWords==119*96 && f.result.restoredFrameWords==119*96
   && f.io.resetWrites==119*4 && f.io.commandWrites==119*20 && f.io.prefix==prefix);
  unsigned touches=f.io.touches;assert(!f.Run() && f.io.touches==touches);
  f.io.track=false;for(unsigned w=0;w<120;w++){f.io.SelectRuntimeCore(w);assert(f.io.ResetRead()==0x47800 && !f.cohort->worker[w].image.running);
   for(unsigned i=0;i<PioneerQ38FfnGraphCodeWords(w);i++)assert(f.io.RawRead(w,PioneerQ38FfnGraphCodeAddress(w,i))==original[PioneerQ38FfnGraphCodeBase(w)+i]);
   if(w)for(unsigned i=0;i<96;i++)assert(f.io.BootstrapIO::Q38PanelRead(0x74000+i*4)==frames[(w-1)*96+i]);}
  f.io.life=nullptr;f.io.armed=f.io.released=0; // reset TEST counters,not device/lease state
  assert(PioneerQ38DispatchBootstrap(f.io,f.dram,f.banks,*f.cohort));positive++;
 }
 for(unsigned phase:{2U,3U,4U,5U,6U,7U,8U,9U})for(unsigned worker:{0U,60U,119U}) {
  if(worker==0 && (phase==4 || phase==5 || phase==6 || phase==7))continue;
  PeerLifetimeFixture<PeerLifetimeIO> f;f.io.failure=phase==9?8:1;f.io.wantedPhase=phase;f.io.wantedWorker=worker;
  assert(!f.Run());f.Unknown();faults++;
 }
 for(unsigned phase:{2U,8U})for(unsigned worker:{0U,60U,119U}){PeerLifetimeFixture<PeerLifetimeIO> f;
  f.io.failure=2;f.io.wantedPhase=phase;f.io.wantedWorker=worker;assert(!f.Run());f.Unknown();faults++;}
 for(unsigned phase:{5U,6U,7U})for(unsigned kind:{4U,5U,6U,7U,8U,9U,10U})for(unsigned worker:{1U,60U,119U}) {
  PeerLifetimeFixture<PeerLifetimeIO> f;f.io.failure=kind;f.io.wantedPhase=phase;f.io.wantedWorker=worker;
  assert(!f.Run());f.Unknown();faults++;
 }
 for(unsigned phase:{5U,7U})for(unsigned kind:{11U,12U})for(unsigned worker:{1U,60U,119U}){PeerLifetimeFixture<PeerLifetimeIO> f;
  f.io.failure=kind;f.io.wantedPhase=phase;f.io.wantedWorker=worker;assert(!f.Run());f.Unknown();faults++;}
 for(unsigned kind:{13U,14U,15U,18U,20U})for(unsigned worker:{1U,60U,119U}){PeerLifetimeFixture<PeerLifetimeIO> f;
  f.io.failure=kind;f.io.wantedPhase=kind==13?4:kind==18?7:kind==20?8:6;f.io.wantedWorker=worker;assert(!f.Run());f.Unknown();faults++;}
 for(unsigned worker:{1U,60U,119U}){PeerLifetimeFixture<PeerLifetimeIO> f;f.io.failure=21;f.io.wantedPhase=1;f.io.wantedWorker=worker;
  assert(!f.Run());f.Unknown();faults++;}
 {PeerLifetimeFixture<PeerLifetimeIO> f;f.io.failure=16;f.io.wantedPhase=5;f.io.wantedWorker=119;assert(!f.Run());f.Unknown();faults++;}
 {PeerLifetimeFixture<PeerLifetimeIO> f;f.io.failure=7;f.io.wantedPhase=7;f.io.wantedWorker=1;f.io.skipSourceReads=2;
  assert(!f.Run() && !f.result.held);f.Unknown();faults++;}
 for(unsigned phase:{4U,9U})for(unsigned worker:{1U,60U,119U}){PeerLifetimeFixture<PeerLifetimeIO> f;
  f.io.failure=22;f.io.wantedPhase=phase;f.io.wantedWorker=worker;assert(!f.Run());f.Unknown();faults++;}
 for(unsigned worker:{1U,60U,119U}){PeerLifetimeFixture<PeerLifetimeIO> f;f.io.failure=23;f.io.wantedPhase=8;f.io.wantedWorker=worker;
  assert(!f.Run());f.Unknown();faults++;}
 for(unsigned phase:{5U,7U})for(unsigned word=0;word<16;word++)if(word!=3){PeerLifetimeFixture<PeerLifetimeIO> f;
  f.io.failure=24;f.io.wantedPhase=phase;f.io.wantedWorker=60;f.io.badStatusWord=word;assert(!f.Run());f.Unknown();faults++;}
 for(unsigned phase:{1U,2U,3U,4U,5U,6U,7U,8U,9U})for(unsigned worker:{1U,60U,119U}){PeerLifetimeFixture<PeerLifetimeIO> f;
  f.io.failure=25;f.io.wantedPhase=phase;f.io.wantedWorker=worker;assert(!f.Run());f.Unknown();faults++;}
 for(unsigned field=0;field<18;field++) {
  PeerLifetimeFixture<PeerLifetimeIO> f;auto& r=f.result;
  uint32_t* fields[]={&r.version,&r.started,&r.passed,&r.unsafe,&r.phase,&r.worker,&r.word,&r.savedWords,&r.installedWords,
   &r.restoredWords,&r.armed,&r.released,&r.ready,&r.stopIssued,&r.stopped,&r.held,&r.savedFrameWords,&r.restoredFrameWords};
  *fields[field]=field?1:2;auto before=r;assert(!f.Run() && !f.io.touches && !memcmp(&before,&r,sizeof(r)));faults++;
 }
 for(unsigned worker:{0U,60U,119U})for(unsigned fault=0;fault<7;fault++) {
  PeerLifetimeFixture<PeerLifetimeIO> f;auto& n=f.cohort->worker[worker];
  if(!fault)n.image.saved=false;if(fault==1)n.image.ready=false;if(fault==2)n.image.unsafe=true;if(fault==3)n.image.running=true;
  if(fault==4)n.Store().pending=true;if(fault==5)n.Store().recovery_required=true;if(fault==6)n.noc.recovery_required=true;
  assert(!f.Run() && !f.io.touches && !f.result.started && !f.code->Unsafe());faults++;
 }
 printf("PASS OFFLINE NEW FFN peer execution lifetime:positive%u/faults%u/ALL119stage+arm before release/current WAIT+both STOP PCs before ANY hold/867448program+11424frame words exact snapshot+install+restore/ALL master counters unchanged/root NEVER launched/NO_NoC_GDDR_hostDMA_CPUreference_MODEL_OR_SPEED_AUTHORITY\n",positive,faults);
}
#endif
