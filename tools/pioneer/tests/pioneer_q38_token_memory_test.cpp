/* SPDX-License-Identifier: MIT */
// Real full-token journal algorithm;simulated CSR/L1 endpoints ONLY.
#define PIONEER_Q38_GDN_MUTABLE_STATE
// Select the SAME mock panel ABI before ANY shared header is included.
// Load production metadata before the fixture's global Input() helper.
#define TT_PIONEER_EXPERIMENTAL_Q38_AFFINE
#define TT_PIONEER_EXPERIMENTAL_Q38_BLOCK
#define TT_PIONEER_EXPERIMENTAL_Q38_STREAM
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38TokenMemory.h"
#define PIONEER_Q38_FFN_CODE_OWNER_MOCK_ONLY
#include "pioneer_q38_ffn_graph_code_owner_test.cpp"
namespace TOKL=PioneerQ38TokenL1;namespace TOKP=PioneerQ38TokenPrograms;
struct TokenMemoryIO:CodeIO {
 const PioneerQ38TokenMemoryResult* memoryState=nullptr;
 const PioneerQ38TokenStartupPlan* startup=nullptr;
 bool scopeAllowed=true,retained=false,dropPending=false;
 unsigned badKind=0,badPhase=0,badWorker=0,badOrdinal=0;
 std::vector<uint32_t> data=std::vector<uint32_t>(TOKL::SavedWords);
 TokenMemoryIO(){for(unsigned i=0;i<data.size();i++)data[i]=i%17?0x9e3779b9U*(i+1)^0xa56b4e37U:UINT32_MAX;}
 uint32_t RuntimePcRead(){assert(false && "held-only journal cannot use a PC sample as STOP/cleanup authority");return 0;}
 static unsigned Index(unsigned worker,uint32_t address){
  unsigned ordinal=0;for(unsigned n=0;n<TOKL::SnapshotSpans(worker);n++){
   const auto span=TOKL::SnapshotSpan(worker,n);
   if(address>=span.address && address-span.address<span.words*4)return TOKL::SnapshotBase(worker)+ordinal+(address-span.address)/4;
   ordinal+=span.words;
  }
  assert(false);return 0;
 }
 bool AtMemory()const{return memoryState && memoryState->phase==badPhase && current==badWorker;}
 bool AtWord()const{return AtMemory() && memoryState->word==badOrdinal;}
 bool TokenMemoryScope(const PioneerQ38TokenMemory&,const PioneerQ38CanonicalModelPlan&,const PioneerQ38TokenStartupPlan& actual,
  const PioneerQ38GdnIdentity&,const uint8_t*,unsigned){assert(startup==&actual);if(!scopeAllowed)observed=touches;return scopeAllowed;}
 void TokenMemoryRetainHostOwners(){retained=true;}
 bool TokenMemoryMappingStable(unsigned worker){Touch();assert(worker==current);if(badKind==4 && AtMemory()){observed=touches;return false;}return true;}
 uint32_t ResetRead(){auto value=CodeIO::ResetRead();if(badKind==6 && AtMemory()){observed=touches;return value^1;}return value;}
 uint32_t SourceRead(unsigned i){auto value=CodeIO::SourceRead(i);if(badKind==7 && AtMemory()){observed=touches;return value^1;}return value;}
 uint32_t ResidentNocRead(uint32_t a){auto value=CodeIO::ResidentNocRead(a);
  if(AtMemory() && ((badKind==8 && a==0x130) || (badKind==10 && a==0x208) || (badKind==11 && a==0) || (badKind==12 && a==0x210))){observed=touches;return value^1;}return value;
 }
 uint32_t HostPciRead(unsigned peer,uint16_t address,uint8_t bytes){auto value=CodeIO::HostPciRead(peer,address,bytes);
  if(badKind==9 && AtMemory() && address==4){observed=touches;return value^1;}return value;
 }
 uint32_t FullRead(unsigned worker,uint32_t address){
  assert(worker==current && TOKL::SnapshotContains(worker,address));
  if(address==0x6000)return BootstrapIO::RuntimeCodeRead(0);
  if(address>=0x70000 && address<0x74000)return BootstrapIO::Q38PanelRead(address);
  return data.at(Index(worker,address));
 }
 void FullWrite(unsigned worker,uint32_t address,uint32_t value){
  assert(worker==current && TOKL::SnapshotContains(worker,address));
  if(address==0x6000)BootstrapIO::RuntimeCodeWrite(0,value);
  else if(address>=0x70000 && address<0x74000)BootstrapIO::Q38PanelWrite(address,value);
  else data.at(Index(worker,address))=value;
 }
 bool TokenMemoryReadChecked(unsigned worker,uint32_t address,uint32_t& output){
  Touch();reads++;if(badKind==1 && AtWord()){observed=touches;return false;}
  output=FullRead(worker,address);if(dropPending){dropPending=false;observed=touches;}
  if(badKind==2 && AtWord()){output^=1;observed=touches;}return true;
 }
 bool TokenMemoryWrite(unsigned worker,uint32_t address,uint32_t value){
  Touch();writes++;
  assert(memoryState && memoryState->savedWords==TOKL::SavedWords && memoryState->verifiedOriginalWords>=TOKL::SavedWords);
  if(memoryState->phase==3){uint32_t expected;assert(TOKP::ProgramWord(worker,address,expected) && expected==value);}
  else assert(memoryState->phase==6 && TOKL::SnapshotContains(worker,address));
  if(badKind==3 && AtWord()){observed=touches;return false;}
  if(badKind==5 && AtWord()){assert(FullRead(worker,address)!=value);dropPending=true;return true;}
  FullWrite(worker,address,value);return true;
 }
};
// Share ONLY immutable CPU geometry between fresh fixtures. No emulated
// processor state,device history,journal,nonce or readiness is transplanted.
struct TokenMemoryGeometry {
 std::unique_ptr<PioneerQ38CanonicalModelPlan> canonical=std::make_unique<PioneerQ38CanonicalModelPlan>();
 std::unique_ptr<PioneerQ38TokenStartupPlan> startup=std::make_unique<PioneerQ38TokenStartupPlan>();
 std::array<uint32_t,1920> source={};bool ready=false;
 void Bind(const PioneerQ38DispatchLease& cohort){
  const auto* current=&cohort.authority.source[0][0];
  if(ready){for(unsigned i=0;i<1920;i++)assert(source[i]==current[i]);return;}
  for(unsigned i=0;i<1920;i++)source[i]=current[i];
  const uint32_t prompt[]={248319,17};
  assert(canonical->Build() && startup->Build(*canonical,0,{1,2,3,4},source.data(),prompt,2,3,false,248318,248317));ready=true;
 }
};
static TokenMemoryGeometry& Geometry(){static TokenMemoryGeometry geometry;return geometry;}
struct TokenMemoryFixture {
 TokenMemoryIO io;
 std::unique_ptr<PioneerQ38DispatchLease> cohort=std::make_unique<PioneerQ38DispatchLease>();
 std::unique_ptr<PioneerQ38TokenMemory> memory=std::make_unique<PioneerQ38TokenMemory>();
 std::vector<std::unique_ptr<PanelOwners>> owners;
 tt_dram_samples_result dram;PioneerResidentBanks banks;
 std::vector<uint8_t> ledger=std::vector<uint8_t>(PioneerResidentBanks::Pages);
 PioneerQ38GdnIdentity id={1,2,3,4};PioneerQ38TokenMemoryResult result;
 TokenMemoryFixture(){
  dram=BankProof(*io.core[0]);assert(banks.Bind(dram));
  for(unsigned worker=0;worker<120;worker++){
   owners.emplace_back(new PanelOwners(*io.core[worker]));auto& node=cohort->worker[worker];node.master=&owners[worker]->store;
   node.noc=owners[worker]->noc;if(worker)node.noc.version=PioneerWorkerVersion(worker);
   assert(PioneerQ38DispatchPrepareRole(io,dram,banks,*cohort,worker));
  }
  auto& data=Geometry();data.Bind(*cohort);
  for(unsigned i=0;i<data.canonical->Count();i++){
   const auto* part=data.canonical->Part(i);if(part->kind!=data.canonical->WeightKind)continue;
   unsigned first=part->bank*unsigned(data.canonical->Capacity/4096)+part->offset/4096;
   for(unsigned page=0;page<part->allocated/4096;page++)ledger[first+page]=1;
  }
  io.startup=data.startup.get();io.memoryState=&result;io.track=true;
 }
 bool PrepareMemory(){auto& data=Geometry();return memory->PrepareHeld(io,dram,banks,*cohort,*data.canonical,*data.startup,id,ledger.data(),ledger.size(),result);}
 bool CloseMemory(){return memory->CloseHeldNoLaunch(io,result);}
 void Unknown(){
  assert(result.unsafe && memory->Unsafe() && !memory->Closed() && !memory->Installed() && cohort->unsafe && banks.Quarantined()
   && io.quarantined && io.retained && !io.armed && !io.released && io.observed==io.touches);
  for(const auto& node:cohort->worker)assert(node.image.unsafe && (node.master?*node.master:node.store).recovery_required);
  unsigned touches=io.touches,reads=io.reads,writes=io.writes;
  assert(!PrepareMemory() && !CloseMemory() && touches==io.touches && reads==io.reads && writes==io.writes);
 }
};
int main(int argc,char** argv){
 (void)&Input;(void)&Prepare;(void)&Upload;(void)&Close;(void)&PreparePanel;(void)&Run;(void)&Job;(void)&ClosePanel;(void)&Finish;
 assert(argc==1 || (argc==2 && (!strcmp(argv[1],"--faults") || !strcmp(argv[1],"--capsule-faults"))));setvbuf(stdout,nullptr,_IOLBF,0);
 if(argc==1){
  TokenMemoryFixture f;auto before=f.io.data;std::vector<uint32_t> panels;
  f.io.track=false;for(unsigned worker=0;worker<120;worker++){
   assert(f.io.SelectRuntimeCore(worker));panels.push_back(f.io.BootstrapIO::RuntimeCodeRead(0));
   for(unsigned i=0;i<4096;i++)panels.push_back(f.io.BootstrapIO::Q38PanelRead(0x70000+i*4));
  }
  f.io.track=true;assert(f.PrepareMemory() && f.memory->Installed() && !f.memory->Unsafe());
  assert(f.result.savedWords==TOKL::SavedWords && f.result.verifiedOriginalWords==TOKL::SavedWords
   && f.result.installedWords==TOKP::CodeWords(0)+119*TOKP::CodeWords(1) && !f.io.armed && !f.io.released);
  assert(f.CloseMemory() && f.memory->Closed() && f.io.data==before && f.result.restoredWords==TOKL::SavedWords
   && f.result.verifiedOriginalWords==2*TOKL::SavedWords);
  unsigned touches=f.io.touches;assert(!f.PrepareMemory() && !f.CloseMemory() && f.io.touches==touches);
  f.io.track=false;unsigned at=0;for(unsigned worker=0;worker<120;worker++){
   assert(f.io.SelectRuntimeCore(worker));assert(f.io.BootstrapIO::RuntimeCodeRead(0)==panels[at++]);
   for(unsigned i=0;i<4096;i++)assert(f.io.BootstrapIO::Q38PanelRead(0x70000+i*4)==panels[at++]);
  }
  printf("PASS DISTINCT full-token HELD journal/ALL120/saved%u/installed%u/restored%u/secondOriginal%u/arbitrary+UINT32_MAX/NEW terminal60000+60004/fullstack+runtime+padding/startup+851page binding;SIMULATED endpoints,NO start/reset/NoC/native execution/Work/TPS grant\n",f.result.savedWords,f.result.installedWords,f.result.restoredWords,f.result.verifiedOriginalWords);return 0;
 }
 if(!strcmp(argv[1],"--capsule-faults")){
#if defined(PIONEER_Q38_TOKEN_PRECISE_PROGRAMS) || defined(PIONEER_Q38_TOKEN_PUSHED_PROGRAMS)
  unsigned cases=0;
#ifdef PIONEER_Q38_TOKEN_PUSHED_PROGRAMS
  constexpr unsigned family=3;
#else
  constexpr unsigned family=2;
#endif
  auto fault=[&](unsigned kind,unsigned phase,unsigned worker,unsigned address){
   TokenMemoryFixture f;f.io.badKind=kind;f.io.badPhase=phase;f.io.badWorker=worker;
   const bool precise=address>=0x30000 && address<0x40000;
#ifdef PIONEER_Q38_TOKEN_PUSHED_PROGRAMS
   const bool extendedFront=address>=0x22000 && address<0x23000;
#else
   const bool extendedFront=false;
#endif
   assert(worker && TOKL::SnapshotContains(worker,address) && (precise || extendedFront));
   if(phase==3){
    unsigned index=0;while(index<TOKP::CodeWords(worker) && TOKP::CodeAddress(worker,index)!=address)index++;
    assert(index<TOKP::CodeWords(worker));f.io.badOrdinal=index;
   }else f.io.badOrdinal=TokenMemoryIO::Index(worker,address)-TOKL::SnapshotBase(worker);
   if(phase<=3){assert(!f.PrepareMemory());if(phase<3)assert(!f.io.writes);}
   else{assert(f.PrepareMemory());assert(!f.CloseMemory());}
   f.Unknown();cases++;
  };
  fault(1,1,1,0x30000);fault(1,1,119,0x3fffc);fault(1,2,1,0x3fffc);
  fault(3,3,1,0x30000);fault(5,3,1,0x30000);fault(2,3,1,0x30000);
  fault(3,6,1,0x30000);fault(3,6,119,0x3fffc);fault(1,7,119,0x3fffc);
  assert(cases==9);
#ifdef PIONEER_Q38_TOKEN_PUSHED_PROGRAMS
  uint32_t last;assert(TOKL::Family==0x33505451 && TOKL::SavedBytes==53913832 && TOKP::ProgramWord(119,0x223a4,last)
   && !TOKP::ProgramWord(119,0x223a8,last) && TOKL::SnapshotContains(119,0x22ffc));
  fault(1,1,1,0x22000);fault(1,1,119,0x22ffc);fault(1,2,1,0x22ffc);
  fault(3,3,1,0x22000);fault(5,3,1,0x223a4);fault(2,3,119,0x223a4);
  fault(3,6,1,0x22000);fault(3,6,119,0x22ffc);fault(1,7,119,0x22ffc);
  fault(5,6,119,0x223a4);fault(2,6,1,0x22000);
  assert(cases==20);
#endif
  printf("PASS QTP%u precise ROM+padding+applicable extended-front HELD-journal faults%u/worker1+119/save+verify+write-loss+silent-drop+readback-mismatch+restore+finalpass/real algorithm retained all owners/no following IO,retry or cleanup;NOT native,launch,model,Work or TPS\n",family,cases);return 0;
#else
  assert(false && "capsule faults require the distinct QTP2/QTP3 program DATA");
#endif
 }
 unsigned faults=0,refusals=0;
 const std::pair<unsigned,unsigned> endpoints[]={{0,0x60000},{0,0x60004},{0,0x64000},{0,0x6bffc},{0,0x17f7fc},{119,0xc0ffc}};
 for(auto endpoint:endpoints)for(unsigned phase:{1U,2U,6U,7U}){
  TokenMemoryFixture f;f.io.badKind=phase==6?3:1;f.io.badPhase=phase;f.io.badWorker=endpoint.first;
  f.io.badOrdinal=TokenMemoryIO::Index(endpoint.first,endpoint.second)-TOKL::SnapshotBase(endpoint.first);
  if(phase<=2){assert(!f.PrepareMemory() && !f.io.writes);}else{assert(f.PrepareMemory());assert(!f.CloseMemory());}
  f.Unknown();faults++;
 }
 for(unsigned phase:{1U,2U,3U,4U,5U,6U,7U})for(unsigned worker:{0U,119U}){
  TokenMemoryFixture f;f.io.badKind=phase==3 || phase==6?3:1;f.io.badPhase=phase;f.io.badWorker=worker;
  f.io.badOrdinal=phase==1 || phase==2 || phase==6 || phase==7?TOKL::SnapshotWords(worker)-1:TOKP::CodeWords(worker)-1;
  if(phase<=4)assert(!f.PrepareMemory());else{assert(f.PrepareMemory());assert(!f.CloseMemory());}f.Unknown();faults++;
 }
 for(unsigned phase:{2U,3U,6U}){
  TokenMemoryFixture f;f.io.badKind=2;f.io.badPhase=phase;f.io.badWorker=0;
  f.io.badOrdinal=phase==3?TOKP::CodeWords(0)-1:TOKL::SnapshotWords(0)-1;
  if(phase<=3)assert(!f.PrepareMemory());else{assert(f.PrepareMemory());assert(!f.CloseMemory());}f.Unknown();faults++;
 }
 for(unsigned phase:{3U,6U}){
  TokenMemoryFixture f;f.io.badKind=5;f.io.badPhase=phase;
  f.io.badOrdinal=phase==3?1+unsigned(sizeof(TOKP::RootText)/4):TokenMemoryIO::Index(0,0x60000);
  if(phase==3)assert(!f.PrepareMemory());else{assert(f.PrepareMemory());assert(!f.CloseMemory());}f.Unknown();faults++;
 }
 for(unsigned kind=4;kind<=12;kind++){
  if(kind==5)continue;
  TokenMemoryFixture f;f.io.badKind=kind;f.io.badPhase=3;f.io.badWorker=119;assert(!f.PrepareMemory());f.Unknown();faults++;
 }
 {TokenMemoryFixture f;f.io.scopeAllowed=false;assert(!f.PrepareMemory() && !f.io.touches);f.Unknown();faults++;}
 {TokenMemoryFixture f;auto original=f.result;auto* words=reinterpret_cast<uint32_t*>(&f.result);
  static_assert(sizeof(f.result)==48,"every result byte explicitly pristine");
  for(unsigned i=0;i<12;i++){
   words[i]^=1;assert(!f.PrepareMemory() && !f.io.touches && !f.io.reads && !f.io.writes);f.result=original;refusals++;
  }
  auto deny=[&](){assert(!f.PrepareMemory() && !f.result.started && !f.io.touches && !f.io.reads && !f.io.writes);refusals++;};
  f.id.driverGeneration++;deny();f.id.driverGeneration--;
  auto& c=*Geometry().canonical;
  unsigned last=0,norm=0;for(unsigned i=0;i<c.Count();i++){
   const auto* part=c.Part(i);if(part->kind!=c.WeightKind)continue;last=i;
   if(part->layer==64 && part->role==2)norm=i;
  }
  for(unsigned which:{0U,norm,last}){
   const auto* part=c.Part(which);assert(part && part->kind==c.WeightKind);
   unsigned page=part->bank*unsigned(c.Capacity/4096)+part->offset/4096;f.ledger[page]=0;deny();f.ledger[page]=1;
  }
  f.cohort->authority.source[119][15]^=1;deny();f.cohort->authority.source[119][15]^=1;
  for(unsigned worker:{0U,119U}){
   auto& node=f.cohort->worker[worker];auto& store=node.Store();
   auto field=[&](auto& value,bool bad){auto old=value;value=bad;deny();value=old;};
   field(node.image.saved,false);field(node.image.ready,false);field(node.image.unsafe,true);field(node.image.running,true);
   field(store.ready,false);field(store.pending,true);field(store.recovery_required,true);field(node.noc.recovery_required,true);
  }
 }
 printf("PASS full-token held-journal faults%u/inert preconditionRefusals%u/newterminal+aux+lastpeer/all7phases/read+write+mismatch+silentdrop/source+reset+PCI+counter+command+scope/first UNKNOWN retains all owners/no subsequent IO/retry/cleanup;SIMULATED endpoints,NOT genuine native/full-model/Work/TPS\n",faults,refusals);
}
