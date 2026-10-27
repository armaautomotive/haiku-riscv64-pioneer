/* SPDX-License-Identifier: MIT */
// Actual production owner/real fresh boot instructions;only FAILED raw IO
// is injected. No manufactured WAIT/STOP/completion/numeric results.
#define PIONEER_Q38_RECURRENT_BLOCK_OWNER_ACTUAL_FIXTURE_ONLY
#include "pioneer_q38_recurrent_block_owner_actual_test.cpp"
struct RecurrentOwnerFaultIO:RecurrentOwnerActualIO {
 unsigned failure=0,targetPhase=0,targetWorker=0;bool hit=false;
 using RecurrentOwnerActualIO::RecurrentOwnerActualIO;
 bool At(unsigned kind)const{return execution && failure==kind && execution->phase==targetPhase && current==targetWorker;}
 bool Fail(){assert(!hit);hit=true;observed=touches;return false;}
 bool RecurrentBlockExecutionScope(const PioneerQ38RecurrentBlockOwner& o,const PioneerQ38RecurrentBlockMemory& m,
  const PioneerQ38CanonicalModelPlan& c,const PioneerQ38MixedRecurrentBlockPlan& p,const PioneerQ38GdnIdentity& id,const uint8_t* l,unsigned n){
  if(At(1))return Fail();
  return RecurrentOwnerActualIO::RecurrentBlockExecutionScope(o,m,c,p,id,l,n);
 }
 bool RecurrentBlockMemoryMappingStable(unsigned w){
  if(At(2)){Touch();assert(w==current);return Fail();}
  return RecurrentMemoryIO::RecurrentBlockMemoryMappingStable(w);
 }
 bool RecurrentBlockMemoryWrite(unsigned w,uint32_t a,uint32_t value){
  if(At(3) && a==RB::Scratch){Touch();writes++;return Fail();}
  return RecurrentOwnerActualIO::RecurrentBlockMemoryWrite(w,a,value);
 }
 bool RecurrentBlockMemoryReadChecked(unsigned w,uint32_t a,uint32_t& value){
  bool okay=RecurrentOwnerActualIO::RecurrentBlockMemoryReadChecked(w,a,value);
  if(At(4) && a==M::Status){
   const auto& p=*machine.child[w-1];assert(okay && live[w] && starts==w && (p.pc==p.waitPC || p.pc==p.waitPC+4));return Fail();
  }
  return okay;
 }
 uint32_t SourceRead(unsigned word){
  uint32_t value=RecurrentOwnerActualIO::SourceRead(word);
  if(At(5) && word==11){Fail();return value^1;}
  return value;
 }
};
int main(int argc,char** argv){
 assert(argc==3);setvbuf(stdout,nullptr,_IOLBF,0);
 (void)&RecurrentOwnerUnusedInput;(void)&Prepare;(void)&Upload;(void)&Close;(void)&PreparePanel;(void)&Run;(void)&Job;(void)&ClosePanel;(void)&Finish;
 (void)&Refusals;(void)&LateRefusals;(void)&RunPersistentGdnPair;
 const unsigned faults[][3]={{1,1,119},{2,2,119},{3,1,0},{4,2,1},{5,2,60}};
 for(const auto& fault:faults){
  auto io=std::make_unique<RecurrentOwnerFaultIO>(argv[1],argv[2]);
  auto cohort=std::make_unique<PioneerQ38DispatchLease>();auto memory=std::make_unique<PioneerQ38RecurrentBlockMemory>();auto owner=std::make_unique<PioneerQ38RecurrentBlockOwner>();
  std::vector<std::unique_ptr<PanelOwners>> leases;auto dram=BankProof(*io->core[0]);PioneerResidentBanks banks;assert(banks.Bind(dram));
  for(unsigned w=0;w<120;w++){
   leases.emplace_back(new PanelOwners(*io->core[w]));auto& n=cohort->worker[w];n.master=&leases[w]->store;n.noc=leases[w]->noc;
   if(w)n.noc.version=PioneerWorkerVersion(w);assert(PioneerQ38DispatchPrepareRole(*io,dram,banks,*cohort,w));
  }
  std::vector<uint8_t> ledger(PioneerResidentBanks::Pages,1);PioneerQ38GdnIdentity id={1,2,3,4};
  PioneerQ38RecurrentBlockMemoryResult journal;PioneerQ38RecurrentBlockResult result;io->memoryState=&journal;io->track=true;
  assert(memory->PrepareHeld(*io,dram,banks,*cohort,*io->machine.canonical,id,ledger.data(),unsigned(ledger.size()),journal));
  io->execution=&result;io->failure=fault[0];io->targetPhase=fault[1];io->targetWorker=fault[2];
  auto prepare=[&](){return owner->Prepare(*io,dram,banks,*cohort,*memory,*io->machine.canonical,*io->machine.block,id,ledger.data(),unsigned(ledger.size()),result);};
  assert(!prepare() && io->hit && owner->Unsafe() && memory->Unsafe() && !owner->Closed() && !memory->Closed()
   && cohort->unsafe && banks.Quarantined() && io->retained && io->quarantined && !io->holds && !journal.restoredWords
   && io->observed==io->touches && !io->live[0]);
  for(const auto& n:cohort->worker){const auto& s=n.master?*n.master:n.store;assert(n.image.unsafe && s.recovery_required);}
  unsigned touches=io->touches,reads=io->reads,writes=io->writes,starts=io->starts;
  assert(!prepare() && !owner->Run(*io,result) && !owner->Close(*io,result,journal) && !memory->CloseHeldNoLaunch(*io,journal)
   && io->touches==touches && io->reads==reads && io->writes==writes && io->starts==starts);
  printf("PASS QRB owner actual preparation fault%u/phase%u/worker%u/real virgin boots%u/ALL120 retained/ZERO post-failure IO,hold,restore or retry;NOT native recovery\n",fault[0],fault[1],fault[2],starts);
 }
}
