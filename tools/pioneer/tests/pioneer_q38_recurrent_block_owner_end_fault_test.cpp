/* SPDX-License-Identifier: MIT */
// One genuine whole instruction run,then fork its exact completed state.
// No PC/nonce/ready/STOP result is fabricated to reach cleanup phases.
#define PIONEER_Q38_RECURRENT_BLOCK_OWNER_ACTUAL_FIXTURE_ONLY
#include "pioneer_q38_recurrent_block_owner_actual_test.cpp"
struct RecurrentOwnerEndFaultIO:RecurrentOwnerActualIO {
 unsigned failure=0,targetPhase=0,targetWorker=0;bool hit=false;
 using RecurrentOwnerActualIO::RecurrentOwnerActualIO;
 bool At(unsigned kind)const{return execution && failure==kind && execution->phase==targetPhase && current==targetWorker;}
 bool Fail(){assert(!hit);hit=true;observed=touches;return false;}
 bool RecurrentBlockMemoryWrite(unsigned w,uint32_t a,uint32_t value){
  if((At(6) && a==RB::Outer) || (At(10) && a==0x64000)){Touch();writes++;return Fail();}
  return RecurrentOwnerActualIO::RecurrentBlockMemoryWrite(w,a,value);
 }
 bool RecurrentBlockMemoryReadChecked(unsigned w,uint32_t a,uint32_t& value){
  bool okay=RecurrentOwnerActualIO::RecurrentBlockMemoryReadChecked(w,a,value);
  if((At(7) && a==RB::Status) || (At(8) && a==M::Status)
   || (At(11) && a==PioneerQ38RecurrentBlockSnapshotAddress(w,PioneerQ38RecurrentBlockSnapshotWords(w)-1))){assert(okay);return Fail();}
  return okay;
 }
 uint32_t ResetRead(){auto value=RecurrentMemoryIO::ResetRead();if(At(9)){Fail();return value^1;}return value;}
 uint32_t ResidentNocRead(uint32_t a){auto value=RecurrentMemoryIO::ResidentNocRead(a);
  if(At(12) && a==PioneerQ38PanelCounter(7)){Fail();return value^1;}return value;
 }
};
int main(int argc,char** argv){
 assert(argc==3);setvbuf(stdout,nullptr,_IOLBF,0);
 (void)&RecurrentOwnerUnusedInput;(void)&Prepare;(void)&Upload;(void)&Close;(void)&PreparePanel;(void)&Run;(void)&Job;(void)&ClosePanel;(void)&Finish;
 (void)&Refusals;(void)&LateRefusals;(void)&RunPersistentGdnPair;
 auto io=std::make_unique<RecurrentOwnerEndFaultIO>(argv[1],argv[2]);
 auto cohort=std::make_unique<PioneerQ38DispatchLease>();auto memory=std::make_unique<PioneerQ38RecurrentBlockMemory>();auto owner=std::make_unique<PioneerQ38RecurrentBlockOwner>();
 std::vector<std::unique_ptr<PanelOwners>> leases;auto dram=BankProof(*io->core[0]);PioneerResidentBanks banks;assert(banks.Bind(dram));
 for(unsigned w=0;w<120;w++){
  leases.emplace_back(new PanelOwners(*io->core[w]));auto& n=cohort->worker[w];n.master=&leases[w]->store;n.noc=leases[w]->noc;
  if(w)n.noc.version=PioneerWorkerVersion(w);assert(PioneerQ38DispatchPrepareRole(*io,dram,banks,*cohort,w));
 }
 std::vector<uint8_t> ledger(PioneerResidentBanks::Pages,1);PioneerQ38GdnIdentity id={1,2,3,4};
 PioneerQ38RecurrentBlockMemoryResult journal;PioneerQ38RecurrentBlockResult result;io->memoryState=&journal;io->track=true;
 assert(memory->PrepareHeld(*io,dram,banks,*cohort,*io->machine.canonical,id,ledger.data(),unsigned(ledger.size()),journal));
 io->execution=&result;
 assert(owner->Prepare(*io,dram,banks,*cohort,*memory,*io->machine.canonical,*io->machine.block,id,ledger.data(),unsigned(ledger.size()),result));
 assert(owner->Run(*io,result));io->machine.VerifyBlock();
 puts("PASS QRB actual owner reached full real RUN/counter proofs before ANY cleanup fault injection");fflush(nullptr);
 const unsigned faults[][3]={{6,6,0},{7,6,0},{8,6,119},{9,7,60},{10,8,0},{11,9,119},{12,6,60}};
 for(const auto& fault:faults){
  pid_t child=fork();assert(child>=0);
  if(!child){
   io->failure=fault[0];io->targetPhase=fault[1];io->targetWorker=fault[2];
   assert(!owner->Close(*io,result,journal) && io->hit && owner->Unsafe() && memory->Unsafe() && !owner->Closed() && !memory->Closed()
    && banks.Quarantined() && cohort->unsafe && io->quarantined && io->retained && io->observed==io->touches);
   if(fault[1]==6)assert(!io->holds && !journal.restoredWords);
   if(fault[1]==7)assert(io->holds==60 && !journal.restoredWords);
   if(fault[1]>=8)assert(io->holds==120 && result.held==120);
   if(fault[1]==9)assert(journal.restoredWords==TT_Q38_RECURRENT_BLOCK_SAVED_WORDS);
   unsigned touches=io->touches,reads=io->reads,writes=io->writes,holds=io->holds;
   assert(!owner->Run(*io,result) && !owner->Close(*io,result,journal) && !memory->CloseHeldNoLaunch(*io,journal)
    && touches==io->touches && reads==io->reads && writes==io->writes && holds==io->holds);
   printf("PASS QRB owner genuinely reached close fault%u/phase%u/worker%u/holds%u/restored%u/ALL retained ZERO later IO/reset/restore/retry\n",fault[0],fault[1],fault[2],holds,journal.restoredWords);_exit(0);
  }
  int status=0;assert(waitpid(child,&status,0)==child && WIFEXITED(status) && !WEXITSTATUS(status));
 }
 assert(owner->Close(*io,result,journal) && owner->MatchesClosedNativeOwner(*memory,*io->machine.block)
  && result.stopped==119 && result.held==120 && journal.restoredWords==TT_Q38_RECURRENT_BLOCK_SAVED_WORDS
  && journal.verifiedOriginalWords==2*TT_Q38_RECURRENT_BLOCK_SAVED_WORDS);
 puts("PASS QRB owner 7genuine post-RUN close faults + actual unmodified parent119STOP/120holds/full restore+secondproof;SIMULATED physical endpoints/NOT native or TPS");
}
