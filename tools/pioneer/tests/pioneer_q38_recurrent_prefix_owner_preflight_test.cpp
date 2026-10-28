/* SPDX-License-Identifier: MIT */
// Request boundary/first-scope-loss only. Simulated endpoints, no native grant.
#define PIONEER_Q38_RECURRENT_PREFIX_MEMORY_FIXTURE_ONLY
#define Input PrefixPreflightUnusedInput
#include "pioneer_q38_recurrent_prefix_memory_test.cpp"
#undef Input
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38RecurrentPrefixOwner.h"
struct PrefixPreflightIO:PrefixMemoryIO {
 bool RecurrentPrefixExecutionScope(const PioneerQ38RecurrentPrefixOwner&,const PioneerQ38RecurrentPrefixMemory&,
  const PioneerQ38CanonicalModelPlan&,const PioneerQ38RecurrentPrefixPlan&,const PioneerQ38GdnIdentity&,const uint8_t*,unsigned){return false;}
 uint64_t Q38CohortNow(){assert(false && "preflight must not reach a hardware wait");return 0;}
 void Pause(){assert(false && "preflight must not start or wait");}
 void RuntimeResetWrite(uint32_t){assert(false && "preflight must not reset");}
 void ResidentNocWrite(uint32_t,uint32_t){assert(false && "preflight must not submit");}
};
int main(){
 (void)&PrefixPreflightUnusedInput;(void)&Prepare;(void)&Upload;(void)&Close;(void)&PreparePanel;(void)&Run;(void)&Job;(void)&ClosePanel;(void)&Finish;
 auto io=std::make_unique<PrefixPreflightIO>();auto cohort=std::make_unique<PioneerQ38DispatchLease>();
 auto c=std::make_unique<PioneerQ38CanonicalModelPlan>();auto p=std::make_unique<PioneerQ38RecurrentPrefixPlan>();assert(c->Build());
 std::vector<std::unique_ptr<PanelOwners>> leases;auto dram=BankProof(*io->core[0]);PioneerResidentBanks banks;assert(banks.Bind(dram));
 for(unsigned w=0;w<120;w++){
  leases.emplace_back(new PanelOwners(*io->core[w]));auto& node=cohort->worker[w];node.master=&leases[w]->store;
  node.noc=leases[w]->noc;if(w)node.noc.version=PioneerWorkerVersion(w);assert(PioneerQ38DispatchPrepareRole(*io,dram,banks,*cohort,w));
 }
 PioneerQ38GdnIdentity id={1,2,3,4};assert(p->Build(*c,0,id,&cohort->authority.source[0][0]));
 auto memory=std::make_unique<PioneerQ38RecurrentPrefixMemory>();PioneerQ38RecurrentPrefixMemoryResult journal;
 std::vector<uint8_t> ledger(PioneerResidentBanks::Pages,1);io->memoryState=&journal;io->track=true;
 assert(memory->PrepareHeld(*io,dram,banks,*cohort,*c,id,ledger.data(),unsigned(ledger.size()),journal));
 const unsigned touches=io->touches,reads=io->reads,writes=io->writes;unsigned refused=0;
 auto deny=[&](PioneerQ38RecurrentPrefixOwner& owner,PioneerQ38RecurrentPrefixResult& result){
  auto previous=result;
  assert(!owner.Prepare(*io,dram,banks,*cohort,*memory,*c,*p,id,ledger.data(),unsigned(ledger.size()),result)
   && !owner.Claimed() && !owner.Unsafe() && io->touches==touches && io->reads==reads && io->writes==writes
   && !std::memcmp(&previous,&result,sizeof(result)));refused++;
 };
 for(unsigned word=0;word<16;word++){
  PioneerQ38RecurrentPrefixOwner owner;PioneerQ38RecurrentPrefixResult result;
  reinterpret_cast<uint32_t*>(&result)[word]^=1;deny(owner,result);
 }
 for(unsigned w=0;w<120;w++)for(unsigned i=0;i<16;i++){
  PioneerQ38RecurrentPrefixOwner owner;PioneerQ38RecurrentPrefixResult result;
  cohort->authority.source[w][i]^=1;deny(owner,result);cohort->authority.source[w][i]^=1;
 }
 // Once a valid claim encounters unknown native scope, retain ALL resources
 // and prove subsequent operations are inert. No ready/native setter exists.
 auto owner=std::make_unique<PioneerQ38RecurrentPrefixOwner>();PioneerQ38RecurrentPrefixResult result;
 assert(!owner->Prepare(*io,dram,banks,*cohort,*memory,*c,*p,id,ledger.data(),unsigned(ledger.size()),result)
  && result.started && result.unsafe && owner->Claimed() && owner->Unsafe() && memory->Unsafe() && !memory->Closed()
  && cohort->unsafe && banks.Quarantined() && io->quarantined && io->retained && io->touches==touches && io->reads==reads && io->writes==writes);
 assert(!owner->Prepare(*io,dram,banks,*cohort,*memory,*c,*p,id,ledger.data(),unsigned(ledger.size()),result)
  && !owner->Run(*io,result) && !owner->Close(*io,result,journal) && !memory->CloseHeldNoLaunch(*io,journal)
  && io->touches==touches && io->reads==reads && io->writes==writes);
 printf("PASS prefix owner request refusals%u/ALL16 result words+ALL1920 source fields/invalid request unchanged+zeroIO/first genuine-scope absence retains all/no subsequent IO,reset,start,retry or restore;SIMULATED,NOT native or execution proof\n",refused);
}
