/* SPDX-License-Identifier: MIT */
// Actual whole ROOT/119workers/STOP/holds/readback owner. ONLY raw endpoints
// are simulated;no fabricated status/PC/nonce/ready reaches a close phase.
#define PIONEER_Q38_RECURRENT_BLOCK_VALIDATION_READBACK 1
#define PIONEER_Q38_RECURRENT_BLOCK_OWNER_ACTUAL_FIXTURE_ONLY
#include "pioneer_q38_recurrent_block_owner_actual_test.cpp"
struct RecurrentReadbackIO:RecurrentOwnerActualIO {
 using RecurrentOwnerActualIO::RecurrentOwnerActualIO;
 unsigned fault=0,readPackets=0,numericReads=0;bool reading=false,sourceScope=true,hit=false;
 bool AtReadback()const{return execution && execution->phase==7 && holds==120;}
 bool Scratch(uint32_t a)const{return a>=0x80000 && a<0x81000;}
 void Hit(){hit=true;observed=touches;}
 bool RecurrentBlockExecutionScope(const PioneerQ38RecurrentBlockOwner&,const PioneerQ38RecurrentBlockMemory&,
  const PioneerQ38CanonicalModelPlan&,const PioneerQ38MixedRecurrentBlockPlan&,const PioneerQ38GdnIdentity&,const uint8_t*,unsigned){return sourceScope;}
 bool RecurrentBlockMemoryMappingStable(unsigned w){
  bool okay=RecurrentMemoryIO::RecurrentBlockMemoryMappingStable(w);
  if(AtReadback() && reading && fault==1){Hit();return false;}return okay;
 }
 bool RecurrentBlockMemoryReadChecked(unsigned w,uint32_t a,uint32_t& output){
  if(!RecurrentOwnerActualIO::RecurrentBlockMemoryReadChecked(w,a,output))return false;
  if(!AtReadback() || !Scratch(a))return true;
  if(!reading){if(fault==4){output^=1;Hit();}return true;}
  if(fault==2){Hit();return false;}
  if(fault>=5 && fault<=7){output=fault==5?0x7f800000:fault==6?1:0x477fe001;Hit();return true;}
  numericReads++;if(fault==14){sourceScope=false;Hit();}if(fault==8)Hit();return true;
 }
 bool RecurrentBlockMemoryWrite(unsigned w,uint32_t a,uint32_t value){
  if(AtReadback() && Scratch(a)){if(a==0x80000)reading=false;if(fault==3){Touch();Hit();return false;}}
  return RecurrentOwnerActualIO::RecurrentBlockMemoryWrite(w,a,value);
 }
 uint32_t ResidentNocRead(uint32_t a){
  auto value=RecurrentMemoryIO::ResidentNocRead(a);if(!AtReadback() || !reading)return value;
  if((fault==9 && a==PioneerQ38PanelCounter(0)) || (fault==11 && a==0x840) || (fault==12 && a==0)){Hit();return fault==9?value+1:fault==11?1:value^1;}
  return fault==10 && a==0x40?1:value;
 }
 uint64_t Q38CohortNow(){if(AtReadback() && reading && fault==10){Hit();return UINT64_MAX;}return now;}
 uint32_t ResetRead(){auto value=RecurrentMemoryIO::ResetRead();if(AtReadback() && reading && fault==16){Hit();return value^1;}return value;}
 void ResidentNocWrite(uint32_t a,uint32_t value){
  if(!AtReadback() || a!=0x40 || C().command[7]){RecurrentOwnerActualIO::ResidentNocWrite(a,value);return;}
  Touch();assert(!current && value==1 && readPackets<5 && execution->stopped==119 && execution->held==120 && !memoryState->restoredWords);
  for(unsigned w=0;w<120;w++)assert(core[w]->coreReset==0x47800);
  const auto* o=machine.block->Outer();const unsigned next[]={o[9]+RB::FinalOffset+readPackets*4096,0,o[8],0x80000,0,129,0,0,4096,0};
  for(unsigned i=0;i<10;i++)assert(C().command[i]==next[i]);
  if(fault!=8)for(unsigned i=0;i<1024;i++)PhysicalWrite(0,0x80000+i*4,machine.Get(o[8],next[0]+i*4));
  auto& c=C();c.counters[0]++;c.counters[1]+=64;c.counters[3]++;c.accepted++;readPackets++;reading=true;
  if(fault==13){sourceScope=false;Hit();}if(fault==15){const_cast<uint32_t*>(o)[9]^=4096;Hit();}
 }
};
int main(int argc,char** argv){
 assert(argc==3 || (argc==4 && !strcmp(argv[3],"--plan-only")));setvbuf(stdout,nullptr,_IOLBF,0);
 (void)&RecurrentOwnerUnusedInput;(void)&Prepare;(void)&Upload;(void)&Close;(void)&PreparePanel;(void)&Run;(void)&Job;(void)&ClosePanel;(void)&Finish;
 (void)&Refusals;(void)&LateRefusals;(void)&RunPersistentGdnPair;
 auto io=std::make_unique<RecurrentReadbackIO>(argv[1],argv[2]);auto cohort=std::make_unique<PioneerQ38DispatchLease>();
 auto memory=std::make_unique<PioneerQ38RecurrentBlockMemory>();auto owner=std::make_unique<PioneerQ38RecurrentBlockOwner>();
 std::vector<std::unique_ptr<PanelOwners>> leases;auto dram=BankProof(*io->core[0]);PioneerResidentBanks banks;assert(banks.Bind(dram));
 for(unsigned w=0;w<120;w++){
  leases.emplace_back(new PanelOwners(*io->core[w]));auto& n=cohort->worker[w];n.master=&leases[w]->store;n.noc=leases[w]->noc;
  if(w)n.noc.version=PioneerWorkerVersion(w);assert(PioneerQ38DispatchPrepareRole(*io,dram,banks,*cohort,w));
 }
 unsigned geometry=0,mutations=0;
 for(unsigned bank=0;bank<7;bank++){
  auto operation=std::make_unique<PioneerQ38MixedRecurrentBlockPlan>();const auto& c=*io->machine.canonical;
  assert(operation->Build(c,0,0,0,bank,0,1001,1,{1,2,3,4},&cohort->authority.source[0][0],0,c.Base+c.Workspace(0)->offset));
  PioneerQ38RecurrentBlockReadbackPlan plan;PioneerQ38RecurrentBlockReadbackPage page,initial;
  assert(!plan.Page(0,page) && !memcmp(&page,&initial,sizeof(page)) && plan.Build(c,*operation) && plan.PageCount()==5 && !plan.Build(c,*operation));
  for(unsigned i=0;i<5;i++){
   assert(plan.Page(i,page) && page.bank==bank && page.address==operation->Outer()[9]+RB::FinalOffset+i*4096
    && page.route==PioneerQ38ProjectionRoute(bank) && page.firstWord==i*1024 && page.words==1024);geometry++;
  }
  auto before=page;assert(!plan.Page(5,page) && !memcmp(&page,&before,sizeof(page)));
  for(unsigned word=0;word<128;word++){
   auto* data=const_cast<uint32_t*>(operation->Outer());data[word]^=1;
   assert(!plan.Page(0,page) && !memcmp(&page,&before,sizeof(page)));data[word]^=1;mutations++;
  }
 }
 printf("PASS DISTINCT recurrent readback immutable geometry%u/all7banks/5pages/128all-word atomic refusals per bank%u;DATA only,NO source/STOP/native/card grant\n",geometry,mutations);
 if(argc==4)return 0;
 std::vector<uint8_t> ledger(PioneerResidentBanks::Pages,1);PioneerQ38GdnIdentity id={1,2,3,4};
 PioneerQ38RecurrentBlockMemoryResult journal;PioneerQ38RecurrentBlockResult result;io->memoryState=&journal;io->track=true;
 for(unsigned a=0x80000;a<0x81000;a+=4)assert(PioneerQ38RecurrentBlockSnapshotContains(0,a) && !PioneerQ38RecurrentBlockCodeContains(0,a));
 assert(memory->PrepareHeld(*io,dram,banks,*cohort,*io->machine.canonical,id,ledger.data(),unsigned(ledger.size()),journal));
 io->execution=&result;assert(owner->Prepare(*io,dram,banks,*cohort,*memory,*io->machine.canonical,*io->machine.block,id,ledger.data(),unsigned(ledger.size()),result));
 assert(owner->Run(*io,result));io->machine.VerifyBlock();
 puts("PASS actual QRB whole instruction RUN BEFORE any validation readback/STOP/hold/fault;NO fabricated ready context");
 std::vector<uint32_t> values(5120,0xdeadbeef);PioneerQ38RecurrentBlockReadbackResult out;
 auto inert=[&](uint32_t* p,unsigned n){auto before=io->touches;assert(!owner->CloseForValidation(*io,result,journal,p,n,out) && io->touches==before && !owner->Unsafe() && !out.started);};
 inert(nullptr,5120);inert(values.data(),5119);inert(reinterpret_cast<uint32_t*>(owner.get()),5120);
 inert(reinterpret_cast<uint32_t*>(memory.get()),5120);inert(reinterpret_cast<uint32_t*>(io->machine.block.get()),5120);
 for(unsigned fault=1;fault<=16;fault++){
  fflush(nullptr);pid_t child=fork();assert(child>=0);if(!child){
   io->fault=fault;
   assert(!owner->CloseForValidation(*io,result,journal,values.data(),unsigned(values.size()),out) && io->hit && owner->Unsafe() && memory->Unsafe()
    && cohort->unsafe && banks.Quarantined() && io->quarantined && io->retained && !out.completed && !journal.restoredWords && io->holds==120 && io->observed==io->touches);
   for(auto value:values)assert(value==0xdeadbeef);auto touches=io->touches;
   assert(!owner->CloseForValidation(*io,result,journal,values.data(),unsigned(values.size()),out) && !owner->Close(*io,result,journal) && io->touches==touches);
   printf("PASS actual QRB readback fault%u/real STOP+120holds/ALL owners retained ZERO following IO/restore/retry/caller output\n",fault);_exit(0);
  }int status=0;assert(waitpid(child,&status,0)==child && WIFEXITED(status) && !WEXITSTATUS(status));
 }
 assert(owner->CloseForValidation(*io,result,journal,values.data(),unsigned(values.size()),out)
  && owner->MatchesClosedNativeOwner(*memory,*io->machine.block) && out.completed && out.pages==5 && out.words==5120
  && io->readPackets==5 && io->numericReads==5120 && journal.restoredWords==TT_Q38_RECURRENT_BLOCK_SAVED_WORDS);
 const auto* o=io->machine.block->Outer();for(unsigned i=0;i<5120;i++)assert(values[i]==io->machine.Get(o[8],o[9]+RB::FinalOffset+i*4));
 puts("PASS DISTINCT recurrent validation readback/ACTUAL whole instruction owner+STOP+120holds/5fixed NoC READ packets/capture5120/full original restoration+second proof/16genuine readback faults+5inert aliases;RAW endpoints SIMULATED,NOT genuine native/card/Work/TPS qualification");
}
