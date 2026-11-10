/* SPDX-License-Identifier: MIT */
// Real full-token journal algorithm;simulated CSR/L1 endpoints ONLY.
#ifndef PIONEER_Q38_GDN_MUTABLE_STATE
#define PIONEER_Q38_GDN_MUTABLE_STATE
#endif
// Select the SAME mock panel ABI before ANY shared header is included.
// Load production metadata before the fixture's global Input() helper.
#define TT_PIONEER_EXPERIMENTAL_Q38_AFFINE
#define TT_PIONEER_EXPERIMENTAL_Q38_BLOCK
#ifndef TT_PIONEER_EXPERIMENTAL_Q38_STREAM
#define TT_PIONEER_EXPERIMENTAL_Q38_STREAM
#endif
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38TokenMemory.h"
#if defined(PIONEER_Q38_QGW8_PROGRAMS_INSPECTION) || defined(PIONEER_Q38_QGWD_QGW8_JOURNAL_FIXTURE)
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38Qgw8UploadReceipt.h"
#include "../PioneerQ38HybridHeadModelPlan.h"
#endif
#ifdef PIONEER_Q38_QGWD_RECEIPT_JOURNAL_INSPECTION
#include "../PioneerQ38DenseQ8MatrixManifest.h"
#include "../PioneerQ38TokenRuntimePlan.h"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38TokenNativeOwner.h"
#endif
#ifdef PIONEER_Q38_HYBRID_LAYOUT_DIAGNOSTIC
#include "../PioneerQ38HybridJobBinding.h"
#endif
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
 uint32_t RuntimePcRead(){
#ifdef PIONEER_Q38_QGWD_RECEIPT_JOURNAL_INSPECTION
  if(runtimeMode && current && C().coreReset==0x46800)return 0x2004c;
#endif
  assert(false && "held-only journal cannot use a PC sample as STOP/cleanup authority");return 0;
 }
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
#if defined(PIONEER_Q38_TOKEN_NATIVE_CONTEXT) || defined(PIONEER_Q38_QGWD_RECEIPT_JOURNAL_INSPECTION)
 // Explicit simulated FIFO endpoints for testing the real native-mode
 // journal template. No actual device read, launch or owner authority.
 bool TokenFifoReadChecked(unsigned worker,bool overflow,uint32_t& output){
  Touch();assert(worker==current);
  output=overflow?0:(BootstrapIO::ResidentNocRead(BlackholeDramSourceOffset(0))&0x10000U)?0x10101010U:0;return true;
 }
#endif
#ifdef PIONEER_Q38_QGWD_RECEIPT_JOURNAL_INSPECTION
 // Runtime admission fixture deliberately has no saved panel storage. If
 // admission succeeds, the real owner must fail closed at its next check.
 bool TokenRuntimeScope(const PioneerQ38TokenNativeOwner&,const PioneerQ38TokenMemory&,
  const PioneerQ38CanonicalModelPlan&,const PioneerQ38TokenStartupPlan&,const PioneerQ38GdnIdentity&,
  const uint8_t*,unsigned){return true;}
 bool omitPanelSaved=false,runtimeMode=false;
 uint32_t runtimeWorkspace=0;unsigned runtimeRequests=0,runtimeCorruptRequest=0,bootCorruptWorker=0;
 std::vector<uint32_t> runtimeBank;
 uint32_t* Q38PanelSavedWords(){return omitPanelSaved?nullptr:BootstrapIO::Q38PanelSavedWords();}
 const uint32_t* BulkSavedWords(){return runtimeMode?C().BulkMock::BulkSavedWords():nullptr;}
 bool BulkMappingStable(){return runtimeMode;}
 bool TokenPeerStoreSelect(unsigned worker){return SelectRuntimeCore(worker);}
 uint64_t Q38CohortNow(){return CohortIO::Q38CohortNow();}
 void ResidentNocWrite(uint32_t offset,uint32_t value){
  if(!runtimeMode){CodeIO::ResidentNocWrite(offset,value);return;}
  assert(!quarantined);
  if(offset!=0x40){BootstrapIO::ResidentNocWrite(offset,value);return;}
  assert(current==0);
  auto& node=C();assert(value==1 && !node.pending && node.command[8] && !(node.command[8]&63)
   && node.command[8]<=16384 && !node.command[1] && !node.command[4] && !node.command[6] && !node.command[9]);
  const bool writing=node.command[7]==0x212;assert(writing || !node.command[7]);
  const unsigned scratch=writing?node.command[0]:node.command[3];
  const unsigned address=writing?node.command[3]:node.command[0];
  assert(scratch==PioneerQ38GlobalChainLayout::R::Scratch && address>=runtimeWorkspace
   && uint64_t(address-runtimeWorkspace)+node.command[8]<=runtimeBank.size()*4);
  const unsigned first=(address-runtimeWorkspace)/4;
  for(unsigned i=0;i<node.command[8]/4;i++){
   if(writing)runtimeBank[first+i]=FullRead(0,scratch+i*4);
   else FullWrite(0,scratch+i*4,runtimeBank[first+i]);
  }
  runtimeRequests++;
  if(writing && runtimeRequests==runtimeCorruptRequest)runtimeBank[first]^=1;
  node.doorbells++;node.doorbellRead=false;
  if(writing){node.writeCounters[0]++;node.writeCounters[1]+=node.command[8]/64;node.writeCounters[3]++;}
  else{node.counters[0]++;node.counters[1]+=node.command[8]/64;node.counters[3]++;}
  node.accepted++;
 }
 void RuntimeResetWrite(uint32_t value){
  if(!runtimeMode){CodeIO::RuntimeResetWrite(value);return;}
  assert(!quarantined && current && (value==0x87ffff || value==0x46800));
  auto& node=C();node.coreReset=value;
  if(value==0x87ffff){armed++;return;}
  assert(armed==119);released++;
  FullWrite(current,PioneerQ38SelectedGlobalWorker::Status+12,
   PioneerQ38SelectedGlobalWorker::Magic^(current==bootCorruptWorker?1U:0U));
 }
#endif
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
  if((badKind==2 && AtWord())
#if defined(PIONEER_Q38_HYBRID_LAYOUT_DIAGNOSTIC) || defined(PIONEER_Q38_QGW8_PROGRAMS_INSPECTION) || defined(PIONEER_Q38_QGWD_QGW8_JOURNAL_FIXTURE)
   || (badKind==13 && AtMemory() && address==0x16c400+badOrdinal*4)
#endif
#ifdef PIONEER_Q38_QGWD_RECEIPT_JOURNAL_INSPECTION
   || (badKind==14 && AtMemory() && address==0x16dc00+badOrdinal*4)
#endif
   ){output^=1;observed=touches;}return true;
 }
 bool TokenMemoryWrite(unsigned worker,uint32_t address,uint32_t value){
  Touch();writes++;
#ifdef PIONEER_Q38_QGWD_RECEIPT_JOURNAL_INSPECTION
  if(runtimeMode){assert(TOKL::SnapshotContains(worker,address));FullWrite(worker,address,value);return true;}
#endif
  assert(memoryState && memoryState->savedWords==TOKL::SavedWords && memoryState->verifiedOriginalWords>=TOKL::SavedWords);
  if(memoryState->phase==3){uint32_t expected;assert(TOKP::ProgramWord(worker,address,expected) && expected==value);}
  else assert((memoryState->phase==6
#if defined(PIONEER_Q38_HYBRID_LAYOUT_DIAGNOSTIC) || defined(PIONEER_Q38_QGW8_PROGRAMS_INSPECTION) || defined(PIONEER_Q38_QGWD_QGW8_JOURNAL_FIXTURE)
   || (memoryState->phase==8 && worker==0 && address>=0x16c400 && address<0x16c400+64*3*7*4)
#endif
#ifdef PIONEER_Q38_QGWD_RECEIPT_JOURNAL_INSPECTION
   || (memoryState->phase==10 && worker==0 && address>=0x16dc00 && address<0x16dc00+16*3*7*3*4)
#endif
   ) && TOKL::SnapshotContains(worker,address));
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
 void Bind(const PioneerQ38DispatchLease& cohort,PioneerResidentBanks& banks,const tt_dram_samples_result& proof){
  (void)banks;(void)proof;
  const auto* current=&cohort.authority.source[0][0];
  if(ready){for(unsigned i=0;i<1920;i++)assert(source[i]==current[i]);return;}
  for(unsigned i=0;i<1920;i++)source[i]=current[i];
  const uint32_t prompt[]={248319,17};
#if defined(PIONEER_Q38_QGW8_PROGRAMS_INSPECTION) || defined(PIONEER_Q38_QGWD_QGW8_JOURNAL_FIXTURE)
  // Derive exactly the receipt-owned relocated layout; this synthetic
  // upload is geometry/ledger evidence, not real model or hardware bytes.
  std::vector<uint8_t> pages(PioneerResidentBanks::Pages);std::vector<uint32_t> hashes(PioneerResidentBanks::Pages);
  auto receipt=std::make_unique<PioneerQ38Qgw8UploadReceipt>();int cookie=0;
  assert(receipt->Begin(&cohort,&cookie,hashes.data(),pages.data(),1,4,banks,proof,pages.size()));
  PioneerQ38SpanLease span;unsigned bank=0,length=0,spans=0;uint64_t offset=0;
  while(receipt->Next(bank,offset,length)){
   assert(receipt->Before(&cohort,hashes.data(),pages.data(),&span,bank,offset,length));
   const unsigned first=unsigned(uint64_t(bank)*(PioneerResidentBanks::Capacity/4096)+offset/4096);
   for(unsigned p=0;p<length/4096;p++){
    const unsigned i=first+p;const uint32_t hash=i*2654435761U;pages[i]=1;hashes[i]=hash;assert(receipt->RecordPage(i,hash));
   }
   span.saved=span.ready=span.dirty=true;span.nonce=++spans;assert(receipt->After());
  }
  span.closed=true;span.ready=span.dirty=false;
  assert(spans==22035 && receipt->Seal(span)
   && receipt->DeriveBoundCanonical(*canonical,&cohort,&cookie,hashes.data(),pages.data(),1,4));
#elif defined(PIONEER_Q38_HYBRID_LAYOUT_DIAGNOSTIC)
  assert(canonical->BuildHybridDiagnostic());
#else
  assert(canonical->Build());
#endif
  assert(startup->Build(*canonical,0,{1,2,3,4},source.data(),prompt,2,3,false,248318,248317));ready=true;
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
#ifdef PIONEER_Q38_QGWD_RECEIPT_JOURNAL_INSPECTION
 std::vector<uint32_t> hashes=std::vector<uint32_t>(PioneerResidentBanks::Pages);
 std::unique_ptr<PioneerQ38DenseQ8UploadReceiptInspection> receipt=std::make_unique<PioneerQ38DenseQ8UploadReceiptInspection>();
 PioneerQ38SpanLease span;int receiptContext=0,cookie=0;
#endif
 PioneerQ38GdnIdentity id={1,2,3,4};PioneerQ38TokenMemoryResult result;
 TokenMemoryFixture(){
  dram=BankProof(*io.core[0]);assert(banks.Bind(dram));
  for(unsigned worker=0;worker<120;worker++){
   owners.emplace_back(new PanelOwners(*io.core[worker]));auto& node=cohort->worker[worker];node.master=&owners[worker]->store;
   node.noc=owners[worker]->noc;if(worker)node.noc.version=PioneerWorkerVersion(worker);
   assert(PioneerQ38DispatchPrepareRole(io,dram,banks,*cohort,worker));
  }
  auto& data=Geometry();data.Bind(*cohort,banks,dram);
#ifdef PIONEER_Q38_QGWD_RECEIPT_JOURNAL_INSPECTION
  assert(receipt->Begin(&receiptContext,&cookie,hashes.data(),ledger.data(),1,4,banks,dram,ledger.size()));
  unsigned bank=0,length=0,spans=0;uint64_t offset=0;
  while(receipt->Next(bank,offset,length)){
   assert(receipt->Before(&receiptContext,hashes.data(),ledger.data(),&span,bank,offset,length));
   const unsigned first=unsigned(uint64_t(bank)*(PioneerResidentBanks::Capacity/4096)+offset/4096);
   for(unsigned p=0;p<length/4096;p++){
    const unsigned i=first+p;const uint32_t hash=i*2654435761U;ledger[i]=1;hashes[i]=hash;assert(receipt->RecordPage(i,hash));
   }
   span.saved=span.ready=span.dirty=true;span.nonce=++spans;assert(receipt->After());
  }
  span.closed=true;span.ready=span.dirty=false;
  assert(spans==21907 && receipt->Seal(span));
  assert(!data.startup->LearnedPagesPresent(ledger.data(),ledger.size())); // old logical extent has deliberate holes
  assert(&receiptContext!=reinterpret_cast<void*>(cohort.get()));
  assert(receipt->MatchesLogical(*data.canonical,&receiptContext,&cookie,hashes.data(),ledger.data(),1,4,banks,dram));
  assert(!receipt->MatchesLogical(*data.canonical,cohort.get(),&cookie,hashes.data(),ledger.data(),1,4,banks,dram));
  assert(!memory->BindDenseReceiptInspection(*receipt,nullptr,cohort.get(),&cookie,hashes.data(),1,4));
  assert(!memory->BindDenseReceiptInspection(*receipt,&receiptContext,nullptr,&cookie,hashes.data(),1,4));
  assert(memory->BindDenseReceiptInspection(*receipt,&receiptContext,cohort.get(),&cookie,hashes.data(),1,4));
#else
  for(unsigned i=0;i<data.canonical->Count();i++){
   const auto* part=data.canonical->Part(i);if(part->kind!=data.canonical->WeightKind)continue;
   unsigned first=part->bank*unsigned(data.canonical->Capacity/4096)+part->offset/4096;
   for(unsigned page=0;page<part->allocated/4096;page++)ledger[first+page]=1;
  }
#endif
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
 assert(argc==1 || (argc==2 && (!strcmp(argv[1],"--faults") || !strcmp(argv[1],"--capsule-faults")
#if defined(PIONEER_Q38_QGW8_PROGRAMS_INSPECTION) || defined(PIONEER_Q38_QGWD_QGW8_JOURNAL_FIXTURE)
  || !strcmp(argv[1],"--qgw8-journal")
#endif
#ifdef PIONEER_Q38_HYBRID_JOURNAL_DIAGNOSTIC
  || !strcmp(argv[1],"--hybrid-faults")
#endif
#ifdef PIONEER_Q38_HYBRID_LAYOUT_DIAGNOSTIC
  || !strcmp(argv[1],"--hybrid-manifest")
#endif
  )));setvbuf(stdout,nullptr,_IOLBF,0);
 if(argc==1){
  TokenMemoryFixture f;auto before=f.io.data;std::vector<uint32_t> panels;
  f.io.track=false;for(unsigned worker=0;worker<120;worker++){
   assert(f.io.SelectRuntimeCore(worker));panels.push_back(f.io.BootstrapIO::RuntimeCodeRead(0));
   for(unsigned i=0;i<4096;i++)panels.push_back(f.io.BootstrapIO::Q38PanelRead(0x70000+i*4));
  }
  f.io.track=true;const bool prepared=f.PrepareMemory();
#ifdef PIONEER_Q38_FULL_OFFLOAD_FAST_DECODE_PROFILE
  if(!prepared)fprintf(stderr,"MOCK journal stopped phase%u worker%u word%u saved%u verified%u installed%u unsafe%u\n",
   f.result.phase,f.result.worker,f.result.word,f.result.savedWords,f.result.verifiedOriginalWords,f.result.installedWords,f.result.unsafe);
#endif
  assert(prepared && f.memory->Installed() && !f.memory->Unsafe());
  assert(f.result.savedWords==TOKL::SavedWords && f.result.verifiedOriginalWords==TOKL::SavedWords
   && f.result.installedWords==TOKP::CodeWords(0)+119*TOKP::CodeWords(1) && !f.io.armed && !f.io.released);
#if defined(PIONEER_Q38_HYBRID_PROGRAMS_DIAGNOSTIC) && !defined(PIONEER_Q38_HYBRID_LAYOUT_DIAGNOSTIC)
  const unsigned beforeWrongLayout=f.io.touches;
  assert(!f.memory->InstallHybridManifestHeld(f.io,f.result) && f.io.touches==beforeWrongLayout);
#endif
  assert(f.CloseMemory() && f.memory->Closed() && f.io.data==before && f.result.restoredWords==TOKL::SavedWords
   && f.result.verifiedOriginalWords==2*TOKL::SavedWords);
  unsigned touches=f.io.touches;assert(!f.PrepareMemory() && !f.CloseMemory() && f.io.touches==touches);
  f.io.track=false;unsigned at=0;for(unsigned worker=0;worker<120;worker++){
   assert(f.io.SelectRuntimeCore(worker));assert(f.io.BootstrapIO::RuntimeCodeRead(0)==panels[at++]);
   for(unsigned i=0;i<4096;i++)assert(f.io.BootstrapIO::Q38PanelRead(0x70000+i*4)==panels[at++]);
  }
  printf("PASS DISTINCT full-token HELD journal/ALL120/saved%u/installed%u/restored%u/secondOriginal%u/arbitrary+UINT32_MAX/NEW terminal60000+60004/fullstack+runtime+padding/startup+851page binding;SIMULATED endpoints,NO start/reset/NoC/native execution/Work/TPS grant\n",f.result.savedWords,f.result.installedWords,f.result.restoredWords,f.result.verifiedOriginalWords);return 0;
 }
#if defined(PIONEER_Q38_QGW8_PROGRAMS_INSPECTION) || defined(PIONEER_Q38_QGWD_QGW8_JOURNAL_FIXTURE)
 if(!strcmp(argv[1],"--qgw8-journal")){
  {
   TokenMemoryFixture f;auto before=f.io.data;std::vector<uint32_t> panels;
   f.io.track=false;for(unsigned worker=0;worker<120;worker++){
    assert(f.io.SelectRuntimeCore(worker));panels.push_back(f.io.BootstrapIO::RuntimeCodeRead(0));
    for(unsigned i=0;i<4096;i++)panels.push_back(f.io.BootstrapIO::Q38PanelRead(0x70000+i*4));
   }
   f.io.track=true;
   assert(PioneerQ38TokenStartupPlan::WorkerLayoutCompatible(*Geometry().canonical));
   auto original=std::make_unique<PioneerQ38CanonicalModelPlan>();assert(original->Build()
    && !PioneerQ38TokenStartupPlan::WorkerLayoutCompatible(*original));
   PioneerQ38HybridHeadModelPlan::Proposal host;assert(PioneerQ38HybridHeadModelPlan::Build(*original,host));
   std::array<uint32_t,PioneerQ38HybridHeadModelPlan::ManifestWords> expected={};
   assert(PioneerQ38HybridHeadModelPlan::Manifest(*original,host,expected.data(),expected.size()));
#ifdef PIONEER_Q38_QGWD_RECEIPT_JOURNAL_INSPECTION
   // The receipt's driver context and its worker cohort are different
   // objects. Substituting either, or changing generation, must refuse
   // before any L1 I/O.
   for(unsigned mismatch=0;mismatch<3;mismatch++){
    auto rejected=std::make_unique<PioneerQ38TokenMemory>();auto other=std::make_unique<PioneerQ38DispatchLease>();
    int foreignContext=0;PioneerQ38TokenMemoryResult untouched;const unsigned touches=f.io.touches;
    assert(rejected->BindDenseReceiptInspection(*f.receipt,mismatch==0?&foreignContext:&f.receiptContext,
     mismatch==1?other.get():f.cohort.get(),&f.cookie,f.hashes.data(),1,mismatch==2?5:4));
    assert(!rejected->PrepareHeld(f.io,f.dram,f.banks,*f.cohort,*Geometry().canonical,*Geometry().startup,
     f.id,f.ledger.data(),f.ledger.size(),untouched));
    assert(!untouched.started && !untouched.unsafe && f.io.touches==touches);
   }
#endif
   assert(f.PrepareMemory() && f.memory->Installed() && f.result.savedWords==TOKL::SavedWords
    && f.result.installedWords==TOKP::CodeWords(0)+119*TOKP::CodeWords(1));
#ifdef PIONEER_Q38_QGWD_RECEIPT_JOURNAL_INSPECTION
   const unsigned beforeRuntimeTouches=f.io.touches;
   assert(!f.memory->VerifyDenseManifestForRuntimeInspection(f.io) && f.io.touches==beforeRuntimeTouches);
#endif
#ifdef PIONEER_Q38_DENSE_Q8_MATRIX_FOOTPRINT_INSPECTION
   // The QGWD image itself occupies part of the extra range. Preserve its
   // integrity: a never-launched close deliberately checks all code first.
   // Mutate only non-code padding, then verify original data is restored.
   f.io.track=false;for(unsigned worker:{1U,119U}){
    assert(f.io.SelectRuntimeCore(worker));
    for(unsigned address:{0x23000U,0x23358U,0x23ffcU}){
     uint32_t code;if(!TOKP::ProgramWord(worker,address,code))f.io.FullWrite(worker,address,0x12345678U^address);
     else assert(f.io.FullRead(worker,address)==code);
    }
   }f.io.track=true;
#endif
   std::array<uint32_t,64> boot={};
   assert(Geometry().startup->ReadBoot(1,boot.data()) && PioneerQ38SelectedGlobalWorker::BootShape(boot.data())
    && boot[0]==PioneerQ38SelectedGlobalWorker::Magic && boot[49]==0 && boot[50]==0x1ffc0000);
   assert(f.memory->InstallHybridManifestHeld(f.io,f.result));
   for(unsigned i=0;i<expected.size();i++)assert(f.io.data[TokenMemoryIO::Index(0,0x16c400+i*4)]==expected[i]);
   assert(f.memory->VerifyHybridManifestHeld(f.io,f.result));
#ifdef PIONEER_Q38_QGWD_RECEIPT_JOURNAL_INSPECTION
   PioneerQ38HybridDenseQ8MatrixPlan::Proposal denseHost;
   std::array<uint32_t,PioneerQ38DenseQ8UploadReceiptInspection::DenseManifestWords> denseExpected={};
   assert(PioneerQ38HybridDenseQ8MatrixPlan::Build(*original,denseHost)
    && PioneerQ38DenseQ8MatrixManifest::Build(*original,denseHost,denseExpected.data(),denseExpected.size()));
   assert(f.memory->InstallDenseManifestHeld(f.io,f.result));
   for(unsigned i=0;i<denseExpected.size();i++)assert(f.io.data[TokenMemoryIO::Index(0,0x16dc00+i*4)]==denseExpected[i]);
   assert(f.memory->VerifyDenseManifestHeld(f.io,f.result));
   assert(f.memory->VerifyDenseManifestForRuntimeInspection(f.io));
   // The actual runtime packet planner must bind the same published startup
   // and generate every fixed packet; this is still host-only DATA testing.
   PioneerQ38TokenRuntimePlan runtime;
   assert(runtime.Bind(*Geometry().canonical,*Geometry().startup,f.id,&f.cohort->authority.source[0][0]));
   std::array<uint8_t,16384> bytes={};uint64_t total=0;unsigned kinds[7]={};
   for(unsigned i=0;i<PioneerQ38TokenRuntimePlan::Chunks;i++){
    PioneerQ38TokenRuntimePlan::Chunk chunk;
    assert(runtime.Describe(i,chunk) && chunk.kind>=1 && chunk.kind<=6 && chunk.bytes<=bytes.size()
     && runtime.Read(i,bytes.data(),chunk.bytes));
    kinds[chunk.kind]++;total+=chunk.bytes;
   }
   assert(total==PioneerQ38TokenRuntimePlan::PayloadBytes && kinds[1]==2048 && kinds[2]==128
    && kinds[3]==1 && kinds[4]==1 && kinds[5]==2 && kinds[6]==128);
#endif
   const bool closeOk=f.CloseMemory();
   if(!closeOk || !f.memory->Closed() || f.io.data!=before || f.result.restoredWords!=TOKL::SavedWords){
    fprintf(stderr,"journal close=%d closed=%d restored=%u expected=%u phase=%u unsafe=%d\n",int(closeOk),int(f.memory->Closed()),f.result.restoredWords,TOKL::SavedWords,f.result.phase,int(f.result.unsafe));
    if(f.io.data!=before)for(size_t i=0;i<before.size();i++)if(f.io.data[i]!=before[i]){fprintf(stderr,"first differing word %zu: %08x != %08x\n",i,f.io.data[i],before[i]);break;}
   }
   assert(closeOk && f.memory->Closed() && f.io.data==before && f.result.restoredWords==TOKL::SavedWords);
   f.io.track=false;unsigned at=0;for(unsigned worker=0;worker<120;worker++){
    assert(f.io.SelectRuntimeCore(worker) && f.io.BootstrapIO::RuntimeCodeRead(0)==panels[at++]);
    for(unsigned i=0;i<4096;i++)assert(f.io.BootstrapIO::Q38PanelRead(0x70000+i*4)==panels[at++]);
   }
  }
  {
   TokenMemoryFixture f;assert(f.PrepareMemory());
   f.io.badKind=3;f.io.badPhase=8;f.io.badWorker=0;f.io.badOrdinal=64*3*7-1;
   assert(!f.memory->InstallHybridManifestHeld(f.io,f.result));f.Unknown();
  }
  {
   TokenMemoryFixture f;assert(f.PrepareMemory() && f.memory->InstallHybridManifestHeld(f.io,f.result));
   f.io.badKind=13;f.io.badPhase=9;f.io.badWorker=0;f.io.badOrdinal=64*3*7-1;
   assert(!f.memory->VerifyHybridManifestHeld(f.io,f.result));f.Unknown();
  }
#ifdef PIONEER_Q38_DENSE_Q8_MATRIX_FOOTPRINT_INSPECTION
  {
   TokenMemoryFixture f;assert(f.PrepareMemory());
   f.io.badKind=3;f.io.badPhase=6;f.io.badWorker=119;
   f.io.badOrdinal=TokenMemoryIO::Index(119,0x23358)-TOKL::SnapshotBase(119);
   assert(!f.CloseMemory());f.Unknown();
  }
#endif
#ifdef PIONEER_Q38_QGWD_QGW8_JOURNAL_FIXTURE
  {
   TokenMemoryFixture f;assert(f.PrepareMemory());
   // The expanded area contains actual code, not just mutable padding.
   // An altered installed instruction must quarantine before restoration.
   uint32_t code;assert(TOKP::ProgramWord(119,0x23000,code));
   f.io.track=false;assert(f.io.SelectRuntimeCore(119));f.io.FullWrite(119,0x23000,code^1);f.io.track=true;
   assert(!f.CloseMemory() && f.result.phase==5 && f.result.restoredWords==0);
   // A direct data mutation is not one of the mock's instrumented I/O faults;
   // begin the no-further-I/O check at the returned failure boundary.
   f.io.observed=f.io.touches;f.Unknown();
  }
#ifdef PIONEER_Q38_QGWD_RECEIPT_JOURNAL_INSPECTION
  {
   TokenMemoryFixture f;assert(f.PrepareMemory() && f.memory->InstallHybridManifestHeld(f.io,f.result)
    && f.memory->VerifyHybridManifestHeld(f.io,f.result) && f.memory->InstallDenseManifestHeld(f.io,f.result));
   f.io.runtimeWorkspace=Geometry().startup->Workspace();
   f.io.runtimeBank.resize(PioneerQ38GlobalChainLayout::WorkspaceBytes/4);f.io.runtimeMode=true;
   auto owner=std::make_unique<PioneerQ38TokenNativeOwner>();PioneerQ38TokenRuntimeResult runtimeResult;
   assert(owner->PrepareHeldRuntime(f.io,f.dram,f.banks,*f.cohort,*f.memory,*Geometry().canonical,
    *Geometry().startup,f.id,f.ledger.data(),f.ledger.size(),runtimeResult));
   assert(owner->Staged() && !owner->Unsafe() && runtimeResult.staged && !runtimeResult.unsafe
    && runtimeResult.boots==120 && runtimeResult.packets==2*PioneerQ38TokenRuntimePlan::Chunks
    && runtimeResult.writes==PioneerQ38TokenRuntimePlan::Chunks && runtimeResult.reads==runtimeResult.writes
    && runtimeResult.bytesWritten==PioneerQ38TokenRuntimePlan::PayloadBytes
    && runtimeResult.bytesRead==runtimeResult.bytesWritten && !f.io.armed && !f.io.released);
   PioneerQ38TokenBootstrapResult bootstrap;
   assert(owner->BootstrapWorkers(f.io,bootstrap) && owner->WorkersReady() && !owner->Unsafe()
    && bootstrap.armed==119 && bootstrap.released==119 && bootstrap.workers==119 && bootstrap.ready
    && f.io.armed==119 && f.io.released==119 && f.io.core[0]->coreReset==0x47800);
   // Bootstrap status and PCs are synthetic, not executed worker code.
   // Destruction here is simulated-only; no post-launch restoration or
   // physical ownership/launch permission follows from this fixture.
  }
  {
   TokenMemoryFixture f;assert(f.PrepareMemory() && f.memory->InstallHybridManifestHeld(f.io,f.result)
    && f.memory->VerifyHybridManifestHeld(f.io,f.result) && f.memory->InstallDenseManifestHeld(f.io,f.result));
   f.io.runtimeWorkspace=Geometry().startup->Workspace();f.io.runtimeBank.resize(PioneerQ38GlobalChainLayout::WorkspaceBytes/4);
   f.io.runtimeMode=true;f.io.runtimeCorruptRequest=3;
   auto owner=std::make_unique<PioneerQ38TokenNativeOwner>();PioneerQ38TokenRuntimeResult runtimeResult;
   assert(!owner->PrepareHeldRuntime(f.io,f.dram,f.banks,*f.cohort,*f.memory,*Geometry().canonical,
    *Geometry().startup,f.id,f.ledger.data(),f.ledger.size(),runtimeResult));
   assert(runtimeResult.started && runtimeResult.unsafe && !runtimeResult.staged && runtimeResult.packets==4
    && owner->Unsafe() && f.memory->Unsafe() && f.banks.Quarantined() && f.io.quarantined && f.io.retained
    && !f.io.armed && !f.io.released);
  }
  {
   TokenMemoryFixture f;assert(f.PrepareMemory() && f.memory->InstallHybridManifestHeld(f.io,f.result)
    && f.memory->VerifyHybridManifestHeld(f.io,f.result) && f.memory->InstallDenseManifestHeld(f.io,f.result));
   f.io.runtimeWorkspace=Geometry().startup->Workspace();f.io.runtimeBank.resize(PioneerQ38GlobalChainLayout::WorkspaceBytes/4);
   f.io.runtimeMode=true;f.io.bootCorruptWorker=60;
   auto owner=std::make_unique<PioneerQ38TokenNativeOwner>();PioneerQ38TokenRuntimeResult runtimeResult;
   assert(owner->PrepareHeldRuntime(f.io,f.dram,f.banks,*f.cohort,*f.memory,*Geometry().canonical,
    *Geometry().startup,f.id,f.ledger.data(),f.ledger.size(),runtimeResult));
   PioneerQ38TokenBootstrapResult bootstrap;assert(!owner->BootstrapWorkers(f.io,bootstrap));
   assert(bootstrap.started && bootstrap.unsafe && !bootstrap.ready && bootstrap.armed==119 && bootstrap.released==119
    && owner->Unsafe() && f.memory->Unsafe() && f.banks.Quarantined() && f.io.quarantined && f.io.retained);
  }
  {
   TokenMemoryFixture f;assert(f.PrepareMemory() && f.memory->InstallHybridManifestHeld(f.io,f.result)
    && f.memory->VerifyHybridManifestHeld(f.io,f.result) && f.memory->InstallDenseManifestHeld(f.io,f.result));
   f.io.runtimeWorkspace=Geometry().startup->Workspace();f.io.runtimeBank.resize(PioneerQ38GlobalChainLayout::WorkspaceBytes/4);
   f.io.runtimeMode=true;
   auto owner=std::make_unique<PioneerQ38TokenNativeOwner>();PioneerQ38TokenRuntimeResult runtimeResult;
   assert(owner->PrepareHeldRuntime(f.io,f.dram,f.banks,*f.cohort,*f.memory,*Geometry().canonical,
    *Geometry().startup,f.id,f.ledger.data(),f.ledger.size(),runtimeResult));
   assert(f.io.SelectRuntimeCore(0));const uint32_t tableWord=f.io.FullRead(0,0x16dc00);
   f.io.FullWrite(0,0x16dc00,tableWord^1);
   PioneerQ38TokenBootstrapResult bootstrap;assert(!owner->BootstrapWorkers(f.io,bootstrap));
   assert(bootstrap.started && bootstrap.unsafe && !bootstrap.ready && !bootstrap.armed && !bootstrap.released
    && !f.io.armed && !f.io.released && owner->Unsafe() && f.memory->Unsafe() && f.banks.Quarantined());
  }
  {
   TokenMemoryFixture f;assert(f.PrepareMemory());
   auto owner=std::make_unique<PioneerQ38TokenNativeOwner>();PioneerQ38TokenRuntimeResult runtimeResult;
   const unsigned touches=f.io.touches;
   assert(!owner->PrepareHeldRuntime(f.io,f.dram,f.banks,*f.cohort,*f.memory,*Geometry().canonical,
    *Geometry().startup,f.id,f.ledger.data(),f.ledger.size(),runtimeResult));
   assert(!runtimeResult.started && !runtimeResult.unsafe && f.io.touches==touches);
   assert(f.memory->InstallHybridManifestHeld(f.io,f.result) && f.memory->VerifyHybridManifestHeld(f.io,f.result)
    && f.memory->InstallDenseManifestHeld(f.io,f.result));
   f.io.omitPanelSaved=true;
   assert(!owner->PrepareHeldRuntime(f.io,f.dram,f.banks,*f.cohort,*f.memory,*Geometry().canonical,
    *Geometry().startup,f.id,f.ledger.data(),f.ledger.size(),runtimeResult));
   // The admitted owner reaches the deliberately missing saved-panel owner,
   // then quarantines without starting traffic; no execution is simulated.
   assert(runtimeResult.started && runtimeResult.unsafe && runtimeResult.phase==1 && !runtimeResult.packets
    && owner->Unsafe() && f.memory->Unsafe() && f.banks.Quarantined() && f.io.quarantined && f.io.retained);
  }
  {
   TokenMemoryFixture f;assert(f.PrepareMemory());
   f.hashes[0]^=1;assert(!f.memory->InstallHybridManifestHeld(f.io,f.result));
   f.io.observed=f.io.touches;f.Unknown();
  }
  {
   TokenMemoryFixture f;assert(f.PrepareMemory() && f.memory->InstallHybridManifestHeld(f.io,f.result)
    && f.memory->VerifyHybridManifestHeld(f.io,f.result));
   f.io.badKind=3;f.io.badPhase=10;f.io.badWorker=0;f.io.badOrdinal=16*3*7*3-1;
   assert(!f.memory->InstallDenseManifestHeld(f.io,f.result));f.Unknown();
  }
  {
   TokenMemoryFixture f;assert(f.PrepareMemory() && f.memory->InstallHybridManifestHeld(f.io,f.result)
    && f.memory->VerifyHybridManifestHeld(f.io,f.result) && f.memory->InstallDenseManifestHeld(f.io,f.result));
   f.io.badKind=14;f.io.badPhase=11;f.io.badWorker=0;f.io.badOrdinal=16*3*7*3-1;
   assert(!f.memory->VerifyDenseManifestHeld(f.io,f.result));f.Unknown();
  }
  puts("PASS QGWD receipt-bound held runtime/ALL2308 packet readbacks/119-worker mock bootstrap and fault quarantine; SIMULATED ONLY");return 0;
#else
  puts("PASS QGWD bundle held journal/expanded worker restore and corruption quarantine; QGW8 logical fixture, SIMULATED ONLY");return 0;
#endif
#else
  puts("PASS QGW8 held journal and manifest install/verify/restore and fault quarantine; SIMULATED ONLY");return 0;
#endif
 }
#endif
#ifdef PIONEER_Q38_HYBRID_JOURNAL_DIAGNOSTIC
#ifdef PIONEER_Q38_HYBRID_LAYOUT_DIAGNOSTIC
 if(!strcmp(argv[1],"--hybrid-manifest")){
  {
   TokenMemoryFixture f;auto before=f.io.data;
   auto original=std::make_unique<PioneerQ38CanonicalModelPlan>();assert(original->Build());
   auto wrongStartup=std::make_unique<PioneerQ38TokenStartupPlan>();const uint32_t prompt[]={248319,17};
   assert(wrongStartup->Build(*original,0,f.id,&f.cohort->authority.source[0][0],prompt,2,3,false,248318,248317));
   const auto priorTouches=f.io.touches;
   assert(!PioneerQ38TokenStartupPlan::WorkerLayoutCompatible(*original)
    && !f.memory->PrepareHeld(f.io,f.dram,f.banks,*f.cohort,*original,*wrongStartup,f.id,f.ledger.data(),f.ledger.size(),f.result)
    && f.io.touches==priorTouches && !f.result.started && !f.result.unsafe);
   assert(f.PrepareMemory() && f.memory->Installed());
   std::array<uint32_t,64> boot={};
   assert(Geometry().startup->ReadBoot(1,boot.data()) && PioneerQ38SelectedGlobalWorker::BootShape(boot.data())
    && boot[0]==0x35574751 && boot[49]==0 && boot[50]==0x1fff0000);
   PioneerQ38HybridJobBinding binding;std::array<uint32_t,64*3*7> expected={};
   assert(binding.Bind(*original,UINT64_C(0x1fff000000000000)) && binding.RootManifest(expected.data(),expected.size()));
   assert(f.memory->InstallHybridManifestHeld(f.io,f.result));
   for(unsigned i=0;i<expected.size();i++)assert(f.io.data[TokenMemoryIO::Index(0,0x16c400+i*4)]==expected[i]);
   assert(f.memory->VerifyHybridManifestHeld(f.io,f.result));
   const auto touches=f.io.touches;assert(!f.memory->InstallHybridManifestHeld(f.io,f.result) && f.io.touches==touches);
   assert(f.CloseMemory() && f.memory->Closed() && f.io.data==before);
  }
  {
   TokenMemoryFixture f;assert(f.PrepareMemory());
   f.io.badKind=3;f.io.badPhase=8;f.io.badWorker=0;f.io.badOrdinal=64*3*7-1;
   assert(!f.memory->InstallHybridManifestHeld(f.io,f.result));f.Unknown();
  }
  {
   TokenMemoryFixture f;assert(f.PrepareMemory() && f.memory->InstallHybridManifestHeld(f.io,f.result));
   f.io.badKind=13;f.io.badPhase=9;f.io.badWorker=0;f.io.badOrdinal=64*3*7-1;
   assert(!f.memory->VerifyHybridManifestHeld(f.io,f.result));f.Unknown();
  }
  puts("PASS QGW5 boot and 1344-word held manifest staging/reverification/restore; late write and later read corruption quarantined; SIMULATED ONLY");return 0;
 }
#endif
 if(!strcmp(argv[1],"--hybrid-faults")){
  auto fault=[](unsigned kind,unsigned phase,unsigned worker,unsigned address){
   TokenMemoryFixture f;f.io.badKind=kind;f.io.badPhase=phase;f.io.badWorker=worker;
   assert(TOKL::SnapshotContains(worker,address));
   f.io.badOrdinal=TokenMemoryIO::Index(worker,address)-TOKL::SnapshotBase(worker);
   if(phase<3){assert(!f.PrepareMemory() && !f.io.writes);}
   else{assert(f.PrepareMemory());assert(!f.CloseMemory());}
   f.Unknown();
  };
  fault(1,1,1,0x12c000); // new BFP8 input region: fail before any write
  fault(2,2,1,0x13c000); // new input verification mismatch: still no write
  fault(3,6,119,0x170000); // relocated GDN restore failure: retain/quarantine
#ifdef PIONEER_Q38_HYBRID_PROGRAMS_DIAGNOSTIC
  puts("PASS hybrid journal new-region read/verify/restore fault retention;SIMULATED QGW5 DATA,NO DEVICE EXECUTION");
#else
  puts("PASS hybrid journal new-region read/verify/restore fault retention;SIMULATED QTP4 DATA,NO QGW5 DEVICE EXECUTION");
#endif
  return 0;
 }
#endif
 if(!strcmp(argv[1],"--capsule-faults")){
#if defined(PIONEER_Q38_TOKEN_PRECISE_PROGRAMS) || defined(PIONEER_Q38_TOKEN_PUSHED_PROGRAMS) || defined(PIONEER_Q38_TOKEN_BF16_PROGRAMS)
  unsigned cases=0;
#ifdef PIONEER_Q38_TOKEN_BF16_PROGRAMS
  constexpr unsigned family=4;
#elif defined(PIONEER_Q38_TOKEN_PUSHED_PROGRAMS)
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
#ifdef PIONEER_Q38_TOKEN_BF16_PROGRAMS
   const bool bf16Extent=worker?address>=0x50000 && address<0x60000:address>=0x20000 && address<0x30000;
#else
   const bool bf16Extent=false;
#endif
   assert(TOKL::SnapshotContains(worker,address) && (precise || extendedFront || bf16Extent));
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
#ifdef PIONEER_Q38_TOKEN_BF16_PROGRAMS
  fault(1,1,0,0x20000);fault(1,1,0,0x2fffc);fault(1,2,0,0x2fffc);
  fault(3,3,0,0x20000);fault(5,3,0,0x20000);fault(2,3,0,0x20000);
  fault(3,6,0,0x20000);fault(3,6,0,0x2fffc);fault(1,7,0,0x2fffc);
  fault(1,1,1,0x50000);fault(1,1,119,0x5fffc);fault(1,2,1,0x5fffc);
  fault(3,3,1,0x50000);fault(5,3,1,0x50000);fault(2,3,119,0x50000);
  fault(3,6,1,0x50000);fault(3,6,119,0x5fffc);fault(1,7,119,0x5fffc);
  assert(cases==27);
#endif
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
 return 0;
}
