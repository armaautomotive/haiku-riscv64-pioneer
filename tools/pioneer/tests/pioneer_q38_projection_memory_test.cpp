/* SPDX-License-Identifier: MIT */
// Actual new memory lifecycle,simulated guarded PCI/L1/source endpoints.
// No native mapping/learned arithmetic/input producer/launch authority.
#define PIONEER_Q38_GDN_MUTABLE_STATE
#define PIONEER_Q38_FFN_CODE_OWNER_MOCK_ONLY
#include "pioneer_q38_ffn_graph_code_owner_test.cpp"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38ProjectionGraphMemory.h"
struct ProjectionMemoryIO:CodeIO {
 const PioneerQ38ProjectionGraphMemoryResult* memoryState=nullptr;
 bool scopeAllowed=true,retained=false,dropPending=false;
 unsigned badKind=0,badPhase=0,badWorker=0,badOrdinal=0;
 std::vector<uint32_t> data=std::vector<uint32_t>(8134072);
 ProjectionMemoryIO(){for(unsigned i=0;i<data.size();i++)data[i]=i%17?0x9e3779b9U*(i+1)^0xa56b4e37U:UINT32_MAX;}
 static unsigned Index(unsigned w,uint32_t a){
  unsigned i=0;for(unsigned n=0;n<PioneerQ38ProjectionGraphSpanCount(w);n++){
   auto s=PioneerQ38ProjectionGraphSnapshotSpan(w,n);if(a>=s.address && a-s.address<s.words*4)return PioneerQ38ProjectionGraphSnapshotBase(w)+i+(a-s.address)/4;i+=s.words;
  }
  assert(false);return 0;
 }
 bool AtMemory()const{return memoryState && memoryState->phase==badPhase && current==badWorker;}
 bool AtWord()const{return AtMemory() && memoryState->word==badOrdinal;}
 bool ProjectionGraphMemoryScope(const PioneerQ38ProjectionGraphMemory&,const PioneerQ38CanonicalModelPlan&,
  const PioneerQ38GdnIdentity&,const uint8_t*,unsigned){if(!scopeAllowed)observed=touches;return scopeAllowed;}
 void ProjectionGraphRetainHostOwners(){retained=true;}
 bool ProjectionGraphMemoryMappingStable(unsigned w){Touch();assert(w==current);if(badKind==4 && AtMemory()){observed=touches;return false;}return true;}
 uint32_t ResetRead(){auto v=CodeIO::ResetRead();if(badKind==6 && AtMemory()){observed=touches;return v^1;}return v;}
 uint32_t SourceRead(unsigned i){auto v=CodeIO::SourceRead(i);if(badKind==7 && AtMemory()){observed=touches;return v^1;}return v;}
 uint32_t ResidentNocRead(uint32_t a){auto v=CodeIO::ResidentNocRead(a);
  if(AtMemory() && ((badKind==8 && a==0x130) || (badKind==10 && a==0x208) || (badKind==11 && a==0) || (badKind==12 && a==0x210))){observed=touches;return v^1;}return v;
 }
 uint32_t HostPciRead(unsigned p,uint16_t a,uint8_t bytes){auto v=CodeIO::HostPciRead(p,a,bytes);
  if(badKind==9 && AtMemory() && a==4){observed=touches;return v^1;}return v;
 }
 uint32_t FullRead(unsigned w,uint32_t a){
  assert(w==current && PioneerQ38ProjectionGraphSnapshotContains(w,a));
  if(a==0x6000)return BootstrapIO::RuntimeCodeRead(0);
  if(a>=0x70000 && a<0x74000)return BootstrapIO::Q38PanelRead(a);
  return data.at(Index(w,a));
 }
 void FullWrite(unsigned w,uint32_t a,uint32_t value){
  assert(w==current && PioneerQ38ProjectionGraphSnapshotContains(w,a));
  if(a==0x6000)BootstrapIO::RuntimeCodeWrite(0,value);
  else if(a>=0x70000 && a<0x74000)BootstrapIO::Q38PanelWrite(a,value);
  else data.at(Index(w,a))=value;
 }
 bool ProjectionGraphMemoryReadChecked(unsigned w,uint32_t a,uint32_t& output){
  Touch();reads++;if(badKind==1 && AtWord()){observed=touches;return false;}
  output=FullRead(w,a);if(dropPending){dropPending=false;observed=touches;}
  if(badKind==2 && AtWord()){output^=1;observed=touches;}return true;
 }
 bool ProjectionGraphMemoryWrite(unsigned w,uint32_t a,uint32_t value){
  Touch();writes++;assert(memoryState && memoryState->savedWords==8134072 && memoryState->verifiedOriginalWords>=8134072);
  assert(memoryState->phase==3?PioneerQ38ProjectionGraphProgramAddress(w,a):memoryState->phase==6 && PioneerQ38ProjectionGraphSnapshotContains(w,a));
  if(badKind==3 && AtWord()){observed=touches;return false;}
  if(badKind==5 && AtWord()){assert(FullRead(w,a)!=value);dropPending=true;return true;}
  FullWrite(w,a,value);return true;
 }
};
struct ProjectionMemoryFixture {
 ProjectionMemoryIO io;
 std::unique_ptr<PioneerQ38DispatchLease> cohort=std::make_unique<PioneerQ38DispatchLease>();
 std::unique_ptr<PioneerQ38ProjectionGraphMemory> memory=std::make_unique<PioneerQ38ProjectionGraphMemory>();
 std::unique_ptr<PioneerQ38CanonicalModelPlan> plan=std::make_unique<PioneerQ38CanonicalModelPlan>();
 std::vector<std::unique_ptr<PanelOwners>> owners;
 tt_dram_samples_result dram;PioneerResidentBanks banks;
 std::vector<uint8_t> ledger=std::vector<uint8_t>(PioneerResidentBanks::Pages);
 PioneerQ38GdnIdentity id={1,2,3,4};PioneerQ38ProjectionGraphMemoryResult result;
 ProjectionMemoryFixture(){
  dram=BankProof(*io.core[0]);assert(banks.Bind(dram) && plan->Build());
  for(unsigned w=0;w<120;w++){
   owners.emplace_back(new PanelOwners(*io.core[w]));auto& n=cohort->worker[w];n.master=&owners[w]->store;
   n.noc=owners[w]->noc;if(w)n.noc.version=PioneerWorkerVersion(w);assert(PioneerQ38DispatchPrepareRole(io,dram,banks,*cohort,w));
  }
  io.memoryState=&result;io.track=true;
 }
 bool PrepareMemory(){return memory->PrepareHeld(io,dram,banks,*cohort,*plan,id,ledger.data(),unsigned(ledger.size()),result);}
 bool CloseMemory(){return memory->CloseHeldNoLaunch(io,result);}
 void Unknown(){
  assert(result.unsafe && memory->Unsafe() && !memory->Closed() && !memory->Installed() && cohort->unsafe && banks.Quarantined()
   && io.quarantined && io.retained && !io.armed && !io.released && io.observed==io.touches);
  unsigned t=io.touches,r=io.reads,w=io.writes;assert(!PrepareMemory() && !CloseMemory() && io.touches==t && io.reads==r && io.writes==w);
 }
};
#ifndef PIONEER_Q38_PROJECTION_MEMORY_FIXTURE_ONLY
int main(int argc,char** argv){
 (void)&Input;(void)&Prepare;(void)&Upload;(void)&Close;(void)&PreparePanel;(void)&Run;(void)&Job;(void)&ClosePanel;(void)&Finish;
 assert(argc==1 || (argc==2 && !strcmp(argv[1],"--faults")));setvbuf(stdout,nullptr,_IOLBF,0);
 if(argc==1){
  ProjectionMemoryFixture f;auto data=f.io.data;std::vector<uint32_t> panels;
  f.io.track=false;for(unsigned w=0;w<120;w++){assert(f.io.SelectRuntimeCore(w));panels.push_back(f.io.BootstrapIO::RuntimeCodeRead(0));
   for(unsigned i=0;i<4096;i++)panels.push_back(f.io.BootstrapIO::Q38PanelRead(0x70000+i*4));}
  f.io.track=true;assert(f.PrepareMemory() && f.memory->Installed() && !f.memory->Unsafe());
  assert(f.result.savedWords==8134072 && f.result.verifiedOriginalWords==8134072 && f.result.installedWords==989304 && !f.io.armed && !f.io.released);
  assert(f.CloseMemory() && f.memory->Closed() && f.io.data==data && f.result.restoredWords==8134072 && f.result.verifiedOriginalWords==2*8134072);
  {unsigned touches=f.io.touches;assert(!f.PrepareMemory() && !f.CloseMemory() && f.io.touches==touches);}
  f.io.track=false;unsigned at=0;for(unsigned w=0;w<120;w++){assert(f.io.SelectRuntimeCore(w));assert(f.io.BootstrapIO::RuntimeCodeRead(0)==panels[at++]);
   for(unsigned i=0;i<4096;i++)assert(f.io.BootstrapIO::Q38PanelRead(0x70000+i*4)==panels[at++]);}
  printf("PASS actual projection memory lifecycle ALL120/saved%u/installed%u/restored%u/verifiedOriginal%u;arbitrary+UINT32_MAX data/exact full stacks+runtime+primary+shadow+trailers,SECOND full pass before FIRST write;NO_RESET_BOOT_NOC_OR_NATIVE_EXECUTION\n",
   f.result.savedWords,f.result.installedWords,f.result.restoredWords,f.result.verifiedOriginalWords);
  return 0;
 }
 unsigned faults=0,refusals=0;
 for(unsigned phase:{1U,2U,3U,4U,5U,6U,7U})for(unsigned worker:{0U,119U}){
  ProjectionMemoryFixture f;f.io.badKind=phase==3 || phase==6?3:1;f.io.badPhase=phase;f.io.badWorker=worker;
  f.io.badOrdinal=phase==1 || phase==2 || phase==6 || phase==7?PioneerQ38ProjectionGraphSnapshotWords(worker)-1:PioneerQ38ProjectionGraphCodeWords(worker)-1;
  if(phase<=4)assert(!f.PrepareMemory());else{assert(f.PrepareMemory());assert(!f.CloseMemory());}
  f.Unknown();faults++;
 }
 for(unsigned phase:{2U,3U,6U}){
  ProjectionMemoryFixture f;f.io.badKind=2;f.io.badPhase=phase;f.io.badWorker=0;f.io.badOrdinal=phase==3?PioneerQ38ProjectionGraphCodeWords(0)-1:PioneerQ38ProjectionGraphSnapshotWords(0)-1;
  if(phase<=3)assert(!f.PrepareMemory());else{assert(f.PrepareMemory());assert(!f.CloseMemory());}f.Unknown();faults++;
 }
 for(unsigned phase:{1U,3U,6U}){
  ProjectionMemoryFixture f;f.io.badKind=4;f.io.badPhase=phase;f.io.badWorker=119;
  if(phase<=3)assert(!f.PrepareMemory());else{assert(f.PrepareMemory());assert(!f.CloseMemory());}f.Unknown();faults++;
 }
 for(unsigned phase:{3U,6U}){
  ProjectionMemoryFixture f;f.io.badKind=5;f.io.badPhase=phase;f.io.badWorker=0;f.io.badOrdinal=1;
  if(phase==3)assert(!f.PrepareMemory());else{assert(f.PrepareMemory());assert(!f.CloseMemory());}f.Unknown();faults++;
 }
 {ProjectionMemoryFixture f;f.io.scopeAllowed=false;assert(!f.PrepareMemory() && !f.io.touches);f.Unknown();faults++;}
 for(unsigned kind=6;kind<=12;kind++){
  ProjectionMemoryFixture f;f.io.badKind=kind;f.io.badPhase=3;f.io.badWorker=119;assert(!f.PrepareMemory());f.Unknown();faults++;
 }
 {ProjectionMemoryFixture f;auto original=f.result;
  for(unsigned i=0;i<13;i++){
   if(i==0)f.result.version=2;if(i==1)f.result.started=1;if(i==2)f.result.phase=1;if(i==3)f.result.worker=1;if(i==4)f.result.word=1;
   if(i==5)f.result.unsafe=1;if(i==6)f.result.installed=1;if(i==7)f.result.closed=1;if(i==8)f.result.savedWords=1;
   if(i==9)f.result.verifiedOriginalWords=1;if(i==10)f.result.installedWords=1;if(i==11)f.result.restoredWords=1;if(i==12)f.id.session=0;
   assert(!f.PrepareMemory() && !f.io.touches && !f.io.reads && !f.io.writes);f.result=original;f.id.session=1;refusals++;
  }
  for(unsigned w:{0U,119U}){
   auto& n=f.cohort->worker[w];auto& s=n.Store();
   auto deny=[&](auto& field,bool bad){auto saved=field;field=bad;assert(!f.PrepareMemory() && !f.io.touches && !f.io.reads && !f.io.writes
    && !memcmp(&f.result,&original,sizeof(original)));field=saved;refusals++;};
   deny(n.image.saved,false);deny(n.image.ready,false);deny(n.image.unsafe,true);deny(n.image.running,true);
   deny(s.ready,false);deny(s.pending,true);deny(s.recovery_required,true);deny(n.noc.recovery_required,true);
  }
 }
 printf("PASS actual projection memory lifecycle faults%u/preconditionRefusals%u;FIRST failed read/write/map/scope/preimage/proof/restore retains ALL snapshots+host owners,NO following IO/reset/read/restore/retry;SIMULATED_ENDPOINTS_NOT_NATIVE_OR_SPEED\n",faults,refusals);
}
#endif
