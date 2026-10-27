/* SPDX-License-Identifier: MIT */
// Production preconditions and first source-scope failure;NO instruction
// execution or synthetic success records. Physical/native scope is DENIED.
#define PIONEER_Q38_RECURRENT_BLOCK_MEMORY_FIXTURE_ONLY
#define Input RecurrentRefusalUnusedInput
#include "pioneer_q38_recurrent_block_memory_test.cpp"
#undef Input
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38RecurrentBlockOwner.h"
struct RecurrentOwnerDeniedIO:RecurrentMemoryIO {
 unsigned scopeCalls=0;
 bool RecurrentBlockExecutionScope(const PioneerQ38RecurrentBlockOwner&,const PioneerQ38RecurrentBlockMemory&,
  const PioneerQ38CanonicalModelPlan&,const PioneerQ38MixedRecurrentBlockPlan&,const PioneerQ38GdnIdentity&,const uint8_t*,unsigned){
  scopeCalls++;observed=touches;return false;
 }
 void RuntimeResetWrite(uint32_t){assert(false && "denied source scope must never reset/start/hold");}
 void ResidentNocWrite(uint32_t,uint32_t){assert(false && "denied source scope must never issue a packet");}
};
int main(){
 (void)&RecurrentRefusalUnusedInput;(void)&Prepare;(void)&Upload;(void)&Close;(void)&PreparePanel;(void)&Run;(void)&Job;(void)&ClosePanel;(void)&Finish;
 auto io=std::make_unique<RecurrentOwnerDeniedIO>();auto cohort=std::make_unique<PioneerQ38DispatchLease>();
 auto memory=std::make_unique<PioneerQ38RecurrentBlockMemory>();auto owner=std::make_unique<PioneerQ38RecurrentBlockOwner>();
 auto canonical=std::make_unique<PioneerQ38CanonicalModelPlan>();auto operation=std::make_unique<PioneerQ38MixedRecurrentBlockPlan>();
 assert(canonical->Build());auto dram=BankProof(*io->core[0]);PioneerResidentBanks banks;assert(banks.Bind(dram));
 std::vector<std::unique_ptr<PanelOwners>> leases;
 for(unsigned w=0;w<120;w++){
  leases.emplace_back(new PanelOwners(*io->core[w]));auto& n=cohort->worker[w];n.master=&leases[w]->store;n.noc=leases[w]->noc;
  if(w)n.noc.version=PioneerWorkerVersion(w);assert(PioneerQ38DispatchPrepareRole(*io,dram,banks,*cohort,w));
 }
 PioneerQ38GdnIdentity id={1,2,3,4};std::vector<uint8_t> ledger(PioneerResidentBanks::Pages,1);
 unsigned input=canonical->Base+canonical->Workspace(0)->offset;
 assert(operation->Build(*canonical,0,0,0,0,0,1001,1,id,&cohort->authority.source[0][0],0,input));
 PioneerQ38RecurrentBlockMemoryResult journal;PioneerQ38RecurrentBlockResult result;io->memoryState=&journal;io->track=true;
 assert(!owner->Run(*io,result) && !owner->Close(*io,result,journal) && !owner->Claimed() && !owner->Unsafe());
 assert(memory->PrepareHeld(*io,dram,banks,*cohort,*canonical,id,ledger.data(),unsigned(ledger.size()),journal));
 unsigned touches=io->touches,reads=io->reads,writes=io->writes,refused=2;
 auto prepare=[&](){return owner->Prepare(*io,dram,banks,*cohort,*memory,*canonical,*operation,id,ledger.data(),unsigned(ledger.size()),result);};
 auto inert=[&](){assert(!prepare() && !owner->Claimed() && !owner->Unsafe() && !memory->Unsafe()
  && io->touches==touches && io->reads==reads && io->writes==writes && !io->scopeCalls);refused++;};
 const auto initial=result;auto* words=reinterpret_cast<uint32_t*>(&result);
 static_assert(sizeof(result)==64,"all result fields have explicit pristine values");
 for(unsigned i=0;i<16;i++){words[i]^=1;inert();result=initial;}
 id.session++;inert();id.session--;
 cohort->prepared--;inert();cohort->prepared++;
 const PioneerQ38CanonicalPart* learned=nullptr;
 for(unsigned i=0;i<canonical->Count();i++){
  const auto* p=canonical->Part(i);if(p->kind==canonical->WeightKind && !p->layer && !p->role)learned=p;
 }
 assert(learned);unsigned page=learned->bank*unsigned(canonical->Capacity/4096)+learned->offset/4096;
 auto saved=ledger[page];ledger[page]=0;inert();ledger[page]=saved;
 assert(!prepare() && owner->Claimed() && owner->Unsafe() && memory->Unsafe() && !memory->Closed()
  && cohort->unsafe && banks.Quarantined() && io->quarantined && io->retained && io->scopeCalls==1
  && io->touches==touches && io->reads==reads && io->writes==writes && !io->armed && !io->released);
 for(const auto& n:cohort->worker){const auto& s=n.master?*n.master:n.store;assert(n.image.unsafe && s.recovery_required);}
 assert(!prepare() && !owner->Run(*io,result) && !owner->Close(*io,result,journal) && !memory->CloseHeldNoLaunch(*io,journal)
  && io->touches==touches && io->reads==reads && io->writes==writes && io->scopeCalls==1);
 printf("PASS DISTINCT QRB owner inert preconditions%u/first genuine-scope denial retains journal+ALL120/NO reset,start,packet,restore,retry or post-failure IO;NOT a native/compute/speed grant\n",refused);
}
