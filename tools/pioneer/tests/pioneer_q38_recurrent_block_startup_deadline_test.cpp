/* SPDX-License-Identifier: MIT */
// Production owner + actual virgin-worker boot instructions. Inject only
// elapsed CPU/clock endpoints, NEVER readiness/status/PC/nonce or products.
#define PIONEER_Q38_RECURRENT_BLOCK_OWNER_ACTUAL_FIXTURE_ONLY
#include "pioneer_q38_recurrent_block_owner_actual_test.cpp"
struct RecurrentStartupClockIO:RecurrentOwnerActualIO {
 bool charged[120]={};unsigned cpuCharges=0;
 unsigned startupFault=0,targetWorker=0;bool clockArmed=false,faultObserved=false;
 uint64_t waitStart=0;
 using RecurrentOwnerActualIO::RecurrentOwnerActualIO;
 bool AtTarget()const{return execution && execution->phase==2 && current==targetWorker && startupFault;}
 uint64_t Q38CohortNow(){
  if(AtTarget()){
   if(C().coreReset==0x87ffff){
    assert(!clockArmed);clockArmed=true;waitStart=now;
    if(startupFault==1){faultObserved=true;now=UINT64_MAX-UINT64_C(30000000)+1;}
   }else if(clockArmed && live[current] && !faultObserved){
    if(startupFault==2){faultObserved=true;now=waitStart-1;}
    if(startupFault==3){faultObserved=true;now=waitStart+30000000;}
   }
  }
  return now;
 }
 void Pause(){
  if(AtTarget() && startupFault==4 && live[current])now+=1000000;else RecurrentOwnerActualIO::Pause();
 }
 bool RecurrentBlockExecutionScope(const PioneerQ38RecurrentBlockOwner& o,const PioneerQ38RecurrentBlockMemory& m,
  const PioneerQ38CanonicalModelPlan& c,const PioneerQ38MixedRecurrentBlockPlan& p,const PioneerQ38GdnIdentity& id,const uint8_t* l,unsigned n){
  if(AtTarget() && startupFault==6 && live[current]){faultObserved=true;observed=touches;return false;}
  return RecurrentOwnerActualIO::RecurrentBlockExecutionScope(o,m,c,p,id,l,n);
 }
 bool RecurrentBlockMemoryReadChecked(unsigned w,uint32_t a,uint32_t& value){
  if(AtTarget() && startupFault==4 && live[w] && a==M::Status){
   // A genuinely released virgin worker does not progress. Its actual
   // initial all-zero status is read; no ready/terminal word is invented.
   Touch();reads++;faultObserved=true;value=PhysicalRead(w,a);assert(!value);return true;
  }
  bool okay=RecurrentOwnerActualIO::RecurrentBlockMemoryReadChecked(w,a,value);
  if(okay && AtTarget() && startupFault==5 && live[w] && a==M::Status+12){faultObserved=true;value^=1;}
  if(okay && execution && execution->phase==2 && w && a==0x6000 && !charged[w]){
   charged[w]=true;cpuCharges++;now+=300000; // 0.3s CPU image validation, NOT a slow boot
  }
  return okay;
 }
};
int main(int argc,char** argv){
 assert(argc==3 || (argc==4 && !strcmp(argv[3],"--expect-shared-timeout")));setvbuf(stdout,nullptr,_IOLBF,0);
 (void)&RecurrentOwnerUnusedInput;(void)&Prepare;(void)&Upload;(void)&Close;(void)&PreparePanel;(void)&Run;(void)&Job;(void)&ClosePanel;(void)&Finish;
 (void)&Refusals;(void)&LateRefusals;(void)&RunPersistentGdnPair;
 auto io=std::make_unique<RecurrentStartupClockIO>(argv[1],argv[2]);
 auto cohort=std::make_unique<PioneerQ38DispatchLease>();auto memory=std::make_unique<PioneerQ38RecurrentBlockMemory>();
 auto owner=std::make_unique<PioneerQ38RecurrentBlockOwner>();
 std::vector<std::unique_ptr<PanelOwners>> leases;auto dram=BankProof(*io->core[0]);PioneerResidentBanks banks;assert(banks.Bind(dram));
 for(unsigned w=0;w<120;w++){
  leases.emplace_back(new PanelOwners(*io->core[w]));auto& n=cohort->worker[w];n.master=&leases[w]->store;n.noc=leases[w]->noc;
  if(w)n.noc.version=PioneerWorkerVersion(w);assert(PioneerQ38DispatchPrepareRole(*io,dram,banks,*cohort,w));
 }
 std::vector<uint8_t> ledger(PioneerResidentBanks::Pages,1);PioneerQ38GdnIdentity id={1,2,3,4};
 PioneerQ38RecurrentBlockMemoryResult journal;PioneerQ38RecurrentBlockResult result;io->memoryState=&journal;io->track=true;
 assert(memory->PrepareHeld(*io,dram,banks,*cohort,*io->machine.canonical,id,ledger.data(),unsigned(ledger.size()),journal));
 io->execution=&result;const auto before=io->now;
 auto prepare=[&]{return owner->Prepare(*io,dram,banks,*cohort,*memory,*io->machine.canonical,*io->machine.block,id,ledger.data(),unsigned(ledger.size()),result);};
 if(argc==3){
  const unsigned reasons[]={0,TT_Q38_RECURRENT_WAIT_CLOCK_OVERFLOW,TT_Q38_RECURRENT_WAIT_CLOCK_BACKWARDS,
   TT_Q38_RECURRENT_WAIT_DEADLINE,TT_Q38_RECURRENT_WAIT_DEADLINE,TT_Q38_RECURRENT_WAIT_STATUS,TT_Q38_RECURRENT_WAIT_GUARD};
  for(unsigned target:{1U,119U})for(unsigned fault=1;fault<=6;fault++){
   fflush(nullptr);pid_t child=fork();assert(child>=0);
   if(!child){
    io->startupFault=fault;io->targetWorker=target;
    assert(!prepare() && io->faultObserved && owner->Unsafe() && memory->Unsafe() && banks.Quarantined() && cohort->unsafe
     && io->quarantined && io->retained && result.phase==2 && result.worker==target && result.word==reasons[fault]
     && io->starts==target-unsigned(fault==1) && result.armed==target && result.released==io->starts
     && !io->live[0] && !io->holds && !journal.restoredWords);
    for(const auto& n:cohort->worker){const auto& s=n.master?*n.master:n.store;assert(n.image.unsafe && s.recovery_required);}
    auto touches=io->touches,reads=io->reads,writes=io->writes,starts=io->starts;
    assert(!prepare() && !owner->Run(*io,result) && !owner->Close(*io,result,journal) && !memory->CloseHeldNoLaunch(*io,journal)
     && touches==io->touches && reads==io->reads && writes==io->writes && starts==io->starts && result.word==reasons[fault]);
    printf("PASS startup fault%u/worker%u/reason%08x/real boots%u/ALL120 retained/no post-failure IO,hold,restore or retry\n",fault,target,result.word,io->starts);
    fflush(nullptr);_exit(0);
   }
   int status=0;assert(waitpid(child,&status,0)==child && WIFEXITED(status) && !WEXITSTATUS(status));
  }
 }
 bool okay=prepare();
 if(argc==4){
  assert(!okay && owner->Unsafe() && memory->Unsafe() && banks.Quarantined() && cohort->unsafe && io->quarantined && io->retained
   && result.phase==2 && result.worker>=99 && result.worker<=100 && io->starts==result.worker && !io->live[0] && !io->holds && !journal.restoredWords);
  auto touches=io->touches,starts=io->starts;assert(!prepare() && !owner->Run(*io,result) && !owner->Close(*io,result,journal)
   && touches==io->touches && starts==io->starts);
  printf("REPRODUCED old shared30s startup deadline/worker%u/cpuCharges%u/elapsed%llu/real virgin boots only/retained/no later IO;NOT physical diagnosis\n",
   result.worker,io->cpuCharges,(unsigned long long)(io->now-before));return 0;
 }
 assert(okay && !owner->Unsafe() && !memory->Unsafe() && !banks.Quarantined() && io->starts==119 && io->cpuCharges==119
  && result.prepared==1 && result.armed==119 && result.released==119 && io->now-before>30000000 && !io->live[0]);
 for(unsigned w=1;w<120;w++){
  const auto& p=*io->machine.child[w-1];assert(io->live[w] && (p.pc==p.waitPC || p.pc==p.waitPC+4)
   && p.memory.at(M::Status+12)==M::Magic && !p.memory.at(M::Status+8));
 }
 printf("PASS per-worker startup deadlines/119 real virgin boots/119 CPU charges/elapsed%llu greater than30s/ROOT held/no manufactured readiness;NOT RUN/STOP/native/physical/TPS\n",
  (unsigned long long)(io->now-before));
}
