/* SPDX-License-Identifier: MIT */
// Actual DISTINCT QRB1 held journal;simulated PCI/L1/source endpoints.
// No fake native scope,STOP/compute/driver/Work or speed qualification.
#define PIONEER_Q38_GDN_MUTABLE_STATE
#define PIONEER_Q38_FFN_CODE_OWNER_MOCK_ONLY
#include "pioneer_q38_ffn_graph_code_owner_test.cpp"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38RecurrentBlockMemory.h"
struct RecurrentMemoryIO:CodeIO {
 uint32_t RuntimePcRead(){assert(false && "held journal cannot use PC as authority");return 0;}
 const PioneerQ38RecurrentBlockMemoryResult* memoryState=nullptr;
 bool scopeAllowed=true,retained=false,dropPending=false;
 unsigned badKind=0,badPhase=0,badWorker=0,badOrdinal=0;
 std::vector<uint32_t> data=std::vector<uint32_t>(TT_Q38_RECURRENT_BLOCK_SAVED_WORDS);
 RecurrentMemoryIO(){for(unsigned i=0;i<data.size();i++)data[i]=i%17?0x9e3779b9U*(i+1)^0xa56b4e37U:UINT32_MAX;}
 static unsigned Index(unsigned w,uint32_t a){
  unsigned ordinal=0;for(unsigned n=0;n<PioneerQ38RecurrentBlockSnapshotSpans(w);n++){
   const auto s=PioneerQ38RecurrentBlockSnapshotSpan(w,n);
   if(a>=s.address && a-s.address<s.words*4)return PioneerQ38RecurrentBlockSnapshotBase(w)+ordinal+(a-s.address)/4;
   ordinal+=s.words;
  }
  assert(false);return 0;
 }
 bool AtMemory()const{return memoryState && memoryState->phase==badPhase && current==badWorker;}
 bool AtWord()const{return AtMemory() && memoryState->word==badOrdinal;}
 bool RecurrentBlockMemoryScope(const PioneerQ38RecurrentBlockMemory&,const PioneerQ38CanonicalModelPlan&,
  const PioneerQ38GdnIdentity&,const uint8_t*,unsigned){if(!scopeAllowed)observed=touches;return scopeAllowed;}
 void RecurrentBlockRetainHostOwners(){retained=true;}
 bool RecurrentBlockMemoryMappingStable(unsigned w){Touch();assert(w==current);if(badKind==4 && AtMemory()){observed=touches;return false;}return true;}
 uint32_t ResetRead(){auto value=CodeIO::ResetRead();if(badKind==6 && AtMemory()){observed=touches;return value^1;}return value;}
 uint32_t SourceRead(unsigned i){auto value=CodeIO::SourceRead(i);if(badKind==7 && AtMemory()){observed=touches;return value^1;}return value;}
 uint32_t ResidentNocRead(uint32_t a){auto value=CodeIO::ResidentNocRead(a);
  if(AtMemory() && ((badKind==8 && a==0x130) || (badKind==10 && a==0x208) || (badKind==11 && a==0) || (badKind==12 && a==0x210))){observed=touches;return value^1;}return value;
 }
 uint32_t HostPciRead(unsigned p,uint16_t a,uint8_t bytes){auto value=CodeIO::HostPciRead(p,a,bytes);
  if(badKind==9 && AtMemory() && a==4){observed=touches;return value^1;}return value;
 }
 uint32_t FullRead(unsigned w,uint32_t a){
  assert(w==current && PioneerQ38RecurrentBlockSnapshotContains(w,a));
  if(a==0x6000)return BootstrapIO::RuntimeCodeRead(0);
  if(a>=0x70000 && a<0x74000)return BootstrapIO::Q38PanelRead(a);
  return data.at(Index(w,a));
 }
 void FullWrite(unsigned w,uint32_t a,uint32_t value){
  assert(w==current && PioneerQ38RecurrentBlockSnapshotContains(w,a));
  if(a==0x6000)BootstrapIO::RuntimeCodeWrite(0,value);
  else if(a>=0x70000 && a<0x74000)BootstrapIO::Q38PanelWrite(a,value);
  else data.at(Index(w,a))=value;
 }
 bool RecurrentBlockMemoryReadChecked(unsigned w,uint32_t a,uint32_t& output){
  Touch();reads++;if(badKind==1 && AtWord()){observed=touches;return false;}
  output=FullRead(w,a);if(dropPending){dropPending=false;observed=touches;}
  if(badKind==2 && AtWord()){output^=1;observed=touches;}return true;
 }
 bool RecurrentBlockMemoryWrite(unsigned w,uint32_t a,uint32_t value){
  Touch();writes++;
  assert(memoryState && memoryState->savedWords==TT_Q38_RECURRENT_BLOCK_SAVED_WORDS
   && memoryState->verifiedOriginalWords>=TT_Q38_RECURRENT_BLOCK_SAVED_WORDS);
  assert(memoryState->phase==3?PioneerQ38RecurrentBlockCodeContains(w,a):memoryState->phase==6 && PioneerQ38RecurrentBlockSnapshotContains(w,a));
  if(badKind==3 && AtWord()){observed=touches;return false;}
  if(badKind==5 && AtWord()){assert(FullRead(w,a)!=value);dropPending=true;return true;}
  FullWrite(w,a,value);return true;
 }
};
struct RecurrentMemoryFixture {
 RecurrentMemoryIO io;
 std::unique_ptr<PioneerQ38DispatchLease> cohort=std::make_unique<PioneerQ38DispatchLease>();
 std::unique_ptr<PioneerQ38RecurrentBlockMemory> memory=std::make_unique<PioneerQ38RecurrentBlockMemory>();
 std::unique_ptr<PioneerQ38CanonicalModelPlan> plan=std::make_unique<PioneerQ38CanonicalModelPlan>();
 std::vector<std::unique_ptr<PanelOwners>> owners;
 tt_dram_samples_result dram;PioneerResidentBanks banks;
 std::vector<uint8_t> ledger=std::vector<uint8_t>(PioneerResidentBanks::Pages);
 PioneerQ38GdnIdentity id={1,2,3,4};PioneerQ38RecurrentBlockMemoryResult result;
 RecurrentMemoryFixture(){
  dram=BankProof(*io.core[0]);assert(banks.Bind(dram) && plan->Build());
  for(unsigned w=0;w<120;w++){
   owners.emplace_back(new PanelOwners(*io.core[w]));auto& node=cohort->worker[w];node.master=&owners[w]->store;
   node.noc=owners[w]->noc;if(w)node.noc.version=PioneerWorkerVersion(w);assert(PioneerQ38DispatchPrepareRole(io,dram,banks,*cohort,w));
  }
  io.memoryState=&result;io.track=true;
 }
 bool PrepareMemory(){return memory->PrepareHeld(io,dram,banks,*cohort,*plan,id,ledger.data(),unsigned(ledger.size()),result);}
 bool CloseMemory(){return memory->CloseHeldNoLaunch(io,result);}
 void Unknown(){
  assert(result.unsafe && memory->Unsafe() && !memory->Closed() && !memory->Installed() && cohort->unsafe && banks.Quarantined()
   && io.quarantined && io.retained && !io.armed && !io.released && io.observed==io.touches);
  for(const auto& node:cohort->worker)assert(node.image.unsafe && (node.master?*node.master:node.store).recovery_required);
  unsigned touches=io.touches,reads=io.reads,writes=io.writes;
  assert(!PrepareMemory() && !CloseMemory() && touches==io.touches && reads==io.reads && writes==io.writes);
 }
};
#ifndef PIONEER_Q38_RECURRENT_BLOCK_MEMORY_FIXTURE_ONLY
int main(int argc,char** argv){
 (void)&Input;(void)&Prepare;(void)&Upload;(void)&Close;(void)&PreparePanel;(void)&Run;(void)&Job;(void)&ClosePanel;(void)&Finish;
 assert(argc==1 || (argc==2 && !strcmp(argv[1],"--faults")));setvbuf(stdout,nullptr,_IOLBF,0);
 if(argc==1){
  RecurrentMemoryFixture f;auto before=f.io.data;std::vector<uint32_t> panels;
  f.io.track=false;for(unsigned w=0;w<120;w++){assert(f.io.SelectRuntimeCore(w));panels.push_back(f.io.BootstrapIO::RuntimeCodeRead(0));
   for(unsigned i=0;i<4096;i++)panels.push_back(f.io.BootstrapIO::Q38PanelRead(0x70000+i*4));}
  f.io.track=true;assert(f.PrepareMemory() && f.memory->Installed() && !f.memory->Unsafe());
  assert(f.result.savedWords==TT_Q38_RECURRENT_BLOCK_SAVED_WORDS && f.result.verifiedOriginalWords==TT_Q38_RECURRENT_BLOCK_SAVED_WORDS
   && f.result.installedWords==TT_Q38_RECURRENT_BLOCK_CODE_WORDS && !f.io.armed && !f.io.released);
  assert(f.CloseMemory() && f.memory->Closed() && f.io.data==before && f.result.restoredWords==TT_Q38_RECURRENT_BLOCK_SAVED_WORDS
   && f.result.verifiedOriginalWords==2*TT_Q38_RECURRENT_BLOCK_SAVED_WORDS);
  unsigned touches=f.io.touches;assert(!f.PrepareMemory() && !f.CloseMemory() && f.io.touches==touches);
  f.io.track=false;unsigned at=0;for(unsigned w=0;w<120;w++){assert(f.io.SelectRuntimeCore(w));assert(f.io.BootstrapIO::RuntimeCodeRead(0)==panels[at++]);
   for(unsigned i=0;i<4096;i++)assert(f.io.BootstrapIO::Q38PanelRead(0x70000+i*4)==panels[at++]);}
  printf("PASS DISTINCT QRB1 held journal ALL120/saved%u/installed%u/restored%u/secondOriginal%u/arbitrary+UINT32_MAX/64000Conv+padding+fullstacks+runtime+shadows+trailers;NO reset,start,NoC,compute,native or Work grant\n",
   f.result.savedWords,f.result.installedWords,f.result.restoredWords,f.result.verifiedOriginalWords);return 0;
 }
 unsigned faults=0,refusals=0;
 const std::pair<unsigned,unsigned> tails[]={{0,0x64000},{0,0x6bffc},{0,0x17f7fc},{119,0xc0ffc}};
 for(auto tail:tails)for(unsigned phase:{1U,2U,6U,7U}){
  RecurrentMemoryFixture f;f.io.badKind=phase==6?3:1;f.io.badPhase=phase;f.io.badWorker=tail.first;
  f.io.badOrdinal=RecurrentMemoryIO::Index(tail.first,tail.second)-PioneerQ38RecurrentBlockSnapshotBase(tail.first);
  if(phase<=2){assert(!f.PrepareMemory() && !f.io.writes);}else{assert(f.PrepareMemory());assert(!f.CloseMemory());}
  f.Unknown();faults++;
 }
 for(unsigned phase:{1U,2U,3U,4U,5U,6U,7U})for(unsigned worker:{0U,119U}){
  RecurrentMemoryFixture f;f.io.badKind=phase==3 || phase==6?3:1;f.io.badPhase=phase;f.io.badWorker=worker;
  f.io.badOrdinal=phase==1 || phase==2 || phase==6 || phase==7?PioneerQ38RecurrentBlockSnapshotWords(worker)-1:PioneerQ38RecurrentBlockCodeWords(worker)-1;
  if(phase<=4)assert(!f.PrepareMemory());else{assert(f.PrepareMemory());assert(!f.CloseMemory());}f.Unknown();faults++;
 }
 for(unsigned phase:{2U,3U,6U}){
  RecurrentMemoryFixture f;f.io.badKind=2;f.io.badPhase=phase;f.io.badWorker=0;
  f.io.badOrdinal=phase==3?PioneerQ38RecurrentBlockCodeWords(0)-1:PioneerQ38RecurrentBlockSnapshotWords(0)-1;
  if(phase<=3)assert(!f.PrepareMemory());else{assert(f.PrepareMemory());assert(!f.CloseMemory());}f.Unknown();faults++;
 }
 for(unsigned phase:{1U,3U,6U}){
  RecurrentMemoryFixture f;f.io.badKind=4;f.io.badPhase=phase;f.io.badWorker=119;
  if(phase<=3)assert(!f.PrepareMemory());else{assert(f.PrepareMemory());assert(!f.CloseMemory());}f.Unknown();faults++;
 }
 for(unsigned phase:{3U,6U}){
  RecurrentMemoryFixture f;f.io.badKind=5;f.io.badPhase=phase;f.io.badWorker=0;
  // A silent drop is observable only for a genuinely changed code word.
  // Untouched padding has the same preimage and restore value; its failed
  // read/write cases above remain mandatory,not relaxed or deleted.
  unsigned address=0x64000;
  f.io.badOrdinal=phase==3?1+13149:RecurrentMemoryIO::Index(0,address);
  if(phase==3)assert(!f.PrepareMemory());else{assert(f.PrepareMemory());assert(!f.CloseMemory());}f.Unknown();faults++;
 }
 {RecurrentMemoryFixture f;f.io.scopeAllowed=false;assert(!f.PrepareMemory() && !f.io.touches);f.Unknown();faults++;}
 for(unsigned kind=6;kind<=12;kind++){
  RecurrentMemoryFixture f;f.io.badKind=kind;f.io.badPhase=3;f.io.badWorker=119;assert(!f.PrepareMemory());f.Unknown();faults++;
 }
 {RecurrentMemoryFixture f;auto original=f.result;
  auto* words=reinterpret_cast<uint32_t*>(&f.result);static_assert(sizeof(f.result)==48,"all result bytes have explicit pristine values");
  for(unsigned i=0;i<12;i++){
   words[i]^=1;assert(!f.PrepareMemory() && !f.io.touches && !f.io.reads && !f.io.writes);f.result=original;refusals++;
  }
  f.id.session=0;assert(!f.PrepareMemory() && !f.io.touches);f.id.session=1;refusals++;
  for(unsigned w:{0U,119U}){
   auto& n=f.cohort->worker[w];auto& s=n.Store();
   auto deny=[&](auto& field,bool bad){auto before=field;field=bad;
    assert(!f.PrepareMemory() && !f.io.touches && !f.io.reads && !f.io.writes && !memcmp(&f.result,&original,sizeof(original)));field=before;refusals++;};
   deny(n.image.saved,false);deny(n.image.ready,false);deny(n.image.unsafe,true);deny(n.image.running,true);
   deny(s.ready,false);deny(s.pending,true);deny(s.recovery_required,true);deny(n.noc.recovery_required,true);
  }
 }
 printf("PASS DISTINCT QRB1 held journal faults%u/preconditionRefusals%u/Conv-window+padding endpoints+all7phases/ALL originals twice BEFORE firstwrite/FIRST unknown retains all snapshots+hosts,NO following IO/retry/restore;SIMULATED endpoints,NOT genuine native/compute/Work/TPS\n",faults,refusals);
}
#endif
