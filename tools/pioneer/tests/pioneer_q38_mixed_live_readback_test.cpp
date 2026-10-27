/* SPDX-License-Identifier: MIT */
// Actual fixed readback/owner algorithm,EXPLICIT SYNTHETIC raw endpoints.
// NOT native authorization,instruction execution,original-model or TPS.
#define PIONEER_Q38_MIXED_LIVE_SIGMOID_PROGRAMS 1
#define PIONEER_Q38_MIXED_LIVE_VALIDATION_READBACK 1
#define PIONEER_Q38_MIXED_LIVE_OWNER_FIXTURE_ONLY
#include "pioneer_q38_mixed_live_owner_test.cpp"
#include <sys/wait.h>
#include <unistd.h>
struct ReadbackIO:LiveOwnerIO {
 unsigned rbFault=0,readPackets=0,numericReads=0;
 bool reading=false;
 static uint32_t Value(unsigned word){return word%19==0?0x80000000U:0x3f000000U+word*97;}
 bool AtReadback()const{return execution && execution->phase==7 && holds==120;}
 bool Scratch(uint32_t a)const{return a>=0x80000 && a<0x81000;}
 bool MixedLiveMemoryMappingStable(unsigned w){
  bool valid=LiveOwnerIO::MixedLiveMemoryMappingStable(w);
  if(AtReadback() && reading && rbFault==1){observed=touches;return false;}return valid;
 }
 bool MixedLiveMemoryReadChecked(unsigned w,uint32_t a,uint32_t& value){
  if(!LiveOwnerIO::MixedLiveMemoryReadChecked(w,a,value))return false;
  if(!AtReadback() || !Scratch(a))return true;
  if(!reading){if(rbFault==4){value^=1;observed=touches;}return true;}
  if(rbFault==2){observed=touches;return false;}
  if(rbFault>=5 && rbFault<=7){value=rbFault==5?0x7f800000:rbFault==6?1:0x477fe001;observed=touches;return true;}
  numericReads++;
  if(rbFault==14){executionScope=false;observed=touches;}
  // Dropped DMA leaves NaN poison;the first scalar grammar check refuses.
  if(rbFault==8)observed=touches;
  return true;
 }
 bool MixedLiveMemoryWrite(unsigned w,uint32_t a,uint32_t value){
  if(AtReadback() && Scratch(a)){
   if(a==0x80000)reading=false;
   if(rbFault==3){Touch();observed=touches;return false;}
  }
  return LiveOwnerIO::MixedLiveMemoryWrite(w,a,value);
 }
 uint32_t ResidentNocRead(uint32_t a){
  uint32_t value=LiveOwnerIO::ResidentNocRead(a);
  if(!AtReadback() || !reading)return value;
  if((rbFault==9 && a==PioneerQ38PanelCounter(0)) || (rbFault==11 && a==0x840) || (rbFault==12 && a==0)){
   observed=touches;return rbFault==9?value+1:rbFault==11?1:value^1;
  }
  return rbFault==10 && a==0x40?1:value;
 }
 uint64_t Q38CohortNow(){
  auto value=LiveOwnerIO::Q38CohortNow();
  if(AtReadback() && reading && rbFault==10){observed=touches;return UINT64_MAX;}return value;
 }
 uint32_t ResetRead(){
  auto value=LiveOwnerIO::ResetRead();
  if(AtReadback() && reading && rbFault==16){observed=touches;return value^1;}return value;
 }
 void ResidentNocWrite(uint32_t a,uint32_t value){
  if(!execution || a!=0x40 || C().command[7]){LiveOwnerIO::ResidentNocWrite(a,value);return;}
  Touch();assert(!current && a==0x40 && value==1 && AtReadback() && readPackets<5);
  assert(execution->stopped==119 && execution->held==120 && !memoryState->restoredWords);
  for(unsigned w=0;w<120;w++)assert(core[w]->coreReset==0x47800 && FullReadSelected(w,(w?unsigned(MW::Status):unsigned(L::Status))+16)==2);
  const auto* o=operation->Outer();const unsigned next[]={o[9]+L::FinalOffset+readPackets*4096,0,o[8],0x80000,0,129,0,0,4096,0};
  for(unsigned i=0;i<10;i++)assert(C().command[i]==next[i]);
  if(rbFault!=8)for(unsigned word=0;word<1024;word++)FullWrite(0,0x80000+word*4,Value(readPackets*1024+word));
  auto& c=C();c.counters[0]++;c.counters[1]+=64;c.counters[3]++;c.accepted++;readPackets++;reading=true;
  if(rbFault==13){executionScope=false;observed=touches;}
  if(rbFault==15){const_cast<uint32_t*>(o)[9]^=4096;observed=touches;}
 }
 uint32_t FullReadSelected(unsigned w,uint32_t a){unsigned previous=current;current=w;auto v=FullRead(w,a);current=previous;return v;}
};
struct ReadbackFixture {
 ReadbackIO io;tt_dram_samples_result dram;PioneerResidentBanks banks;
 std::unique_ptr<PioneerQ38DispatchLease> cohort{new PioneerQ38DispatchLease};
 std::unique_ptr<PioneerQ38MixedLiveMemory> memory{new PioneerQ38MixedLiveMemory};
 std::unique_ptr<PioneerQ38CanonicalModelPlan> canonical{new PioneerQ38CanonicalModelPlan};
 std::unique_ptr<PioneerQ38MixedLiveBlockPlan> plan{new PioneerQ38MixedLiveBlockPlan};
 std::unique_ptr<PioneerQ38MixedLiveOwner> owner{new PioneerQ38MixedLiveOwner};
 std::vector<std::unique_ptr<PanelOwners>> owners;
 std::vector<uint8_t> ledger=std::vector<uint8_t>(PioneerResidentBanks::Pages,1);
 std::vector<uint32_t> values=std::vector<uint32_t>(5120,0xdeadbeef);
 PioneerQ38GdnIdentity id={1,2,3,4};PioneerQ38MixedLiveMemoryResult journal;PioneerQ38MixedLiveResult result;
 PioneerQ38MixedLiveReadbackResult report;
 ReadbackFixture(){
  dram=BankProof(*io.core[0]);assert(banks.Bind(dram) && canonical->Build());
  for(unsigned w=0;w<120;w++){
   owners.emplace_back(new PanelOwners(*io.core[w]));auto& n=cohort->worker[w];n.master=&owners[w]->store;
   n.noc=owners[w]->noc;if(w)n.noc.version=PioneerWorkerVersion(w);assert(PioneerQ38DispatchPrepareRole(io,dram,banks,*cohort,w));
  }
  assert(plan->Build(*canonical,3,0,0,0,2,1,id,&cohort->authority.source[0][0],0,canonical->Base+canonical->Workspace(0)->offset));
  io.memoryState=&journal;io.track=true;
  assert(memory->PrepareHeld(io,dram,banks,*cohort,*canonical,id,ledger.data(),unsigned(ledger.size()),journal));
  io.execution=&result;io.operation=plan.get();
  assert(owner->Prepare(io,dram,banks,*cohort,*memory,*canonical,*plan,id,ledger.data(),unsigned(ledger.size()),result));
  assert(owner->Run(io,result));
 }
 bool Close(){return owner->CloseForValidation(io,result,journal,values.data(),unsigned(values.size()),report);}
 void Unknown(){
  assert(result.unsafe && owner->Unsafe() && memory->Unsafe() && cohort->unsafe && banks.Quarantined() && io.quarantined && io.retained);
  for(auto v:values)assert(v==0xdeadbeef);
  unsigned t=io.touches;assert(!Close() && !owner->Close(io,result,journal) && io.touches==t && !report.completed);
 }
};
int main(){
 (void)&LiveOwnerUnusedInput;(void)&Prepare;(void)&Upload;(void)&Close;(void)&PreparePanel;(void)&Run;(void)&Job;(void)&ClosePanel;(void)&Finish;
 setvbuf(stdout,nullptr,_IOLBF,0);ReadbackFixture f;
 static_assert(sizeof(PioneerQ38MixedLiveOwner)>20480,"validation staging is a retained HEAP member");
 unsigned geometry=0;
 for(unsigned bank=0;bank<7;bank++){
  std::unique_ptr<PioneerQ38MixedLiveBlockPlan> operation(new PioneerQ38MixedLiveBlockPlan);
  assert(operation->Build(*f.canonical,3,0,0,bank,2,1,f.id,&f.cohort->authority.source[0][0],0,f.canonical->Base+f.canonical->Workspace(0)->offset));
  PioneerQ38MixedLiveReadbackPlan plan;PioneerQ38MixedLiveReadbackPage p,initial;
  assert(!plan.Page(0,p) && !memcmp(&p,&initial,sizeof(p)) && plan.Build(*f.canonical,*operation) && plan.PageCount()==5
   && !plan.Build(*f.canonical,*operation));
  for(unsigned page=0;page<5;page++){
   assert(plan.Page(page,p) && p.bank==bank && p.address==operation->Outer()[9]+L::FinalOffset+page*4096
    && p.route==PioneerQ38ProjectionRoute(bank) && p.firstWord==page*1024 && p.words==1024);geometry++;
  }
  auto before=p;assert(!plan.Page(5,p) && !memcmp(&before,&p,sizeof(p)));
  const unsigned original=operation->Outer()[9];const_cast<uint32_t*>(operation->Outer())[9]^=4096;
  assert(!plan.Page(0,p) && !memcmp(&before,&p,sizeof(p)));const_cast<uint32_t*>(operation->Outer())[9]=original;
 }
 for(unsigned address=0x80000;address<0x81000;address+=4)assert(PioneerQ38MixedLiveSnapshotContains(0,address));
 printf("PASS immutable final readback geometry%u/all7banks/exact5pages/source mutation and out-of-range refusals/scratch fully journalled\n",geometry);
 unsigned refused=0;
 auto inert=[&](uint32_t* values,unsigned words,PioneerQ38MixedLiveReadbackResult& out){
  const auto before=out;const unsigned t=f.io.touches;
  assert(!f.owner->CloseForValidation(f.io,f.result,f.journal,values,words,out) && f.io.touches==t && !f.owner->Unsafe()
   && !memcmp(&before,&out,sizeof(out)));refused++;
 };
 inert(nullptr,5120,f.report);inert(f.values.data(),5119,f.report);inert(f.values.data(),5121,f.report);
 inert(reinterpret_cast<uint32_t*>(reinterpret_cast<uintptr_t>(f.values.data())+1),5120,f.report);
 inert(reinterpret_cast<uint32_t*>(UINTPTR_MAX-3),5120,f.report);
 for(unsigned i=0;i<5;i++){
  auto* words=reinterpret_cast<uint32_t*>(&f.report);words[i]=i?1:2;inert(f.values.data(),5120,f.report);f.report={};
 }
 inert(reinterpret_cast<uint32_t*>(f.owner.get()),5120,f.report);inert(reinterpret_cast<uint32_t*>(f.memory.get()),5120,f.report);
 inert(reinterpret_cast<uint32_t*>(f.canonical.get()),5120,f.report);inert(reinterpret_cast<uint32_t*>(f.plan.get()),5120,f.report);
 inert(reinterpret_cast<uint32_t*>(&f.result),5120,f.report);inert(reinterpret_cast<uint32_t*>(&f.journal),5120,f.report);
 inert(reinterpret_cast<uint32_t*>(&f.report),5120,f.report);inert(reinterpret_cast<uint32_t*>(f.ledger.data()),5120,f.report);
 printf("PASS readback inert CPU refusals%u/null,count,alignment,overflow,all progress fields,retained-object+journal aliases\n",refused);
 unsigned faults=0;
 for(unsigned fault=1;fault<=18;fault++){
  const pid_t child=fork();assert(child>=0);
  if(!child){
   f.io.rbFault=fault;
   if(fault>=17){f.io.failure=2;f.io.failurePhase=fault==17?8:9;f.io.failureWorker=0;}
   bool closed=f.Close();
   if(closed)fprintf(stderr,"READBACK_FAULT_NOT_REACHED %u\n",fault);
   assert(!closed && f.io.observed==f.io.touches);f.Unknown();
   if(fault<=16)assert(!f.journal.restoredWords);else assert(f.report.pages==5 && f.report.words==5120);
   _exit(0);
  }
  int status=0;assert(waitpid(child,&status,0)==child && WIFEXITED(status) && !WEXITSTATUS(status));faults++;
 }
 assert(f.Close() && f.report.started && f.report.pages==5 && f.report.words==5120 && f.report.completed);
 assert(f.result.held==120 && f.result.stopped==119 && f.result.closed && f.io.readPackets==5 && f.io.numericReads==5120
  && f.journal.restoredWords==TT_Q38_MIXED_LIVE_SAVED_WORDS && f.journal.verifiedOriginalWords==2*TT_Q38_MIXED_LIVE_SAVED_WORDS);
 for(unsigned word=0;word<5120;word++)assert(f.values[word]==ReadbackIO::Value(word));
 printf("PASS guarded fixed5page/5120word readback AFTER ALL120holds/BEFORE restore;atomic publication ONLY after full11398712word restore+second proof;faults%u retain ALL with ZERO following IO/restore/retry;SYNTHETIC endpoints ONLY,NO native/physical/model/TPS qualification\n",faults);
}
