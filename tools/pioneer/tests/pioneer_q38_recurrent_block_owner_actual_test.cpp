/* SPDX-License-Identifier: MIT */
// Production DISTINCT host owner + actual QRB1/119QMW1 machines. Only raw
// learned/hidden/prior-state bytes,MMIO and time are simulated. No fake
// completed status/PC/nonce or arithmetic products enter the controller.
#define PIONEER_Q38_RECURRENT_BLOCK_MEMORY_FIXTURE_ONLY
#define Input RecurrentOwnerUnusedInput
#include "pioneer_q38_recurrent_block_memory_test.cpp"
#undef Input
#define PIONEER_Q38_MIXED_RECURRENT_BLOCK_MACHINE_ONLY
#include "pioneer_q38_mixed_recurrent_block_root_test.cpp"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38RecurrentBlockOwner.h"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38DispatchClose.h"
struct RecurrentOwnerActualIO:RecurrentMemoryIO {
 RecurrentBlockMachine machine;const char* workerPath;
 const PioneerQ38RecurrentBlockResult* execution=nullptr;
 bool live[120]={};unsigned starts=0,holds=0;
 RecurrentOwnerActualIO(const char* root,const char* worker):machine(root,worker,true),workerPath(worker){
  // Staging-only descriptor/source bytes,not processor progress. Replace
  // every staging peer by a NEW never-booted machine on actual release.
  std::array<uint32_t,1920> sources;
  for(unsigned w=0;w<120;w++)for(unsigned i=0;i<16;i++)sources[w*16+i]=core[w]->source[i]|(i?0U:0x10000U);
  auto block=std::make_unique<PioneerQ38MixedRecurrentBlockPlan>();const auto* o=machine.block->Outer();
  assert(block->Build(*machine.canonical,0,0,0,0,0,1001,1,{1,2,3,4},sources.data(),o[68],o[69]));
  machine.block=std::move(block);
  for(unsigned i=0;i<128;i++)machine.outer[i]=machine.block->Outer()[i];
  for(unsigned i=0;i<64;i++)machine.boot[i]=machine.block->Boot(0)[i];
  for(unsigned i=0;i<16;i++)machine.source[i]=sources[i];
 }
 bool RecurrentBlockExecutionScope(const PioneerQ38RecurrentBlockOwner&,const PioneerQ38RecurrentBlockMemory&,
  const PioneerQ38CanonicalModelPlan&,const PioneerQ38MixedRecurrentBlockPlan&,const PioneerQ38GdnIdentity&,const uint8_t*,unsigned){return true;}
 uint32_t SourceRead(unsigned i){Touch();return core[0]->SourceRead(i);} // global PCIe source,NOT selected worker NIU
 void Pause(){now++;} // simulated monotonic clock,NOT silicon throughput
 void Sync(unsigned w){
  if(!live[w] || core[w]->coreReset!=0x46800)return;
  auto& c=*core[w];uint32_t count[8];for(unsigned i=0;i<8;i++)count[i]=w?machine.child[w-1]->Count(i):machine.counts[i];
  c.counters[0]=count[0];c.counters[1]=count[1];c.accepted=count[2];c.counters[3]=count[3];
  c.writeCounters[0]=count[4];c.writeCounters[1]=count[5];c.writeCounters[3]=count[7];
  for(unsigned i=0;i<10;i++)c.command[i]=w?machine.child[w-1]->command[i]:machine.command[i];
  c.pending=w?machine.child[w-1]->pending:machine.pending;
 }
 bool SelectRuntimeCore(unsigned w){bool okay=CodeIO::SelectRuntimeCore(w);if(okay)Sync(w);return okay;}
 void Advance(unsigned w){
  if(!live[w] || core[w]->coreReset!=0x46800)return;
  if(w){
   auto& p=*machine.child[w-1];
   for(unsigned i=0;i<1048576;i++){
    if(p.pc==p.waitPC || p.pc==p.waitPC+4 || p.pc==p.stopPC)break;
    p.StepBlock();
   }
   Sync(w);return;
  }
  for(unsigned i=0;i<1048576;i++){
   if(machine.pc==0x40060 || machine.pc==0x4009c
    || ((machine.pc==0x4004c || machine.pc==0x40050) && machine.memory.at(RB::Doorbell)==machine.memory.at(RB::Status+4)))break;
   machine.Step(); // includes actual paired-GDN progress,NEVER synthesized readiness
  }
  Sync(0);
 }
 uint32_t PhysicalRead(unsigned w,unsigned a){
  if(!live[w] || PioneerQ38RecurrentBlockCodeContains(w,a))return FullRead(w,a);
  const auto& memory=w?machine.child[w-1]->memory:machine.memory;
  auto at=memory.find(a);return at==memory.end()?FullRead(w,a):at->second;
 }
 void PhysicalWrite(unsigned w,unsigned a,uint32_t value){
  FullWrite(w,a,value);
  if(live[w] && !PioneerQ38RecurrentBlockCodeContains(w,a)){
   if(w)machine.child[w-1]->memory[a]=value;else machine.memory[a]=value;
  }
 }
 bool RecurrentBlockMemoryReadChecked(unsigned w,uint32_t a,uint32_t& output){
  if(!execution)return RecurrentMemoryIO::RecurrentBlockMemoryReadChecked(w,a,output);
  Touch();reads++;if(a==(w?unsigned(M::Status):unsigned(RB::Status)))Advance(w);
  output=PhysicalRead(w,a);return true;
 }
 bool RecurrentBlockMemoryWrite(unsigned w,uint32_t a,uint32_t value){
  if(!execution)return RecurrentMemoryIO::RecurrentBlockMemoryWrite(w,a,value);
  Touch();writes++;PhysicalWrite(w,a,value);return true;
 }
 uint32_t RuntimePcRead(){Touch();assert(live[current] && C().coreReset==0x46800);return current?machine.child[current-1]->pc:machine.pc;}
 void ResidentNocWrite(uint32_t a,uint32_t value){
  if(!execution || execution->closed){BootstrapIO::ResidentNocWrite(a,value);return;}
  Touch();if(a<40){C().command[a/4]=value;return;}
  // Actual host metadata DMA only,NOT graph execution or host products.
  assert(!current && !live[0] && a==0x40 && value==1);auto& c=C();
  assert(c.command[7]==0x212 && c.command[2]==129 && !c.command[1] && !c.command[4] && !c.command[6] && !c.command[9]);
  for(unsigned i=0;i<c.command[8]/4;i++)machine.Put(c.command[5],c.command[3]+i*4,PhysicalRead(0,c.command[0]+i*4));
  c.writeCounters[0]++;c.writeCounters[1]+=c.command[8]/64;c.writeCounters[3]++;c.accepted++;
 }
 void RuntimeResetWrite(uint32_t value){
  Touch();assert(execution);C().resets.push_back(value);
  if(value==0x87ffff && execution->phase==7){
   assert(machine.pc==0x40060 && machine.memory.at(RB::Status+16)==2);
   for(const auto& peer:machine.child)assert(peer->pc==peer->stopPC && peer->memory.at(M::Status+16)==2 && !peer->pending);
   holds++;
  }
  if(value==0x46800){
   assert(!live[current]);starts++;
   if(current){
    auto peer=std::make_unique<NewCohortPeer>(workerPath,current,8,5120);peer->bytes=&machine;
    for(auto& entry:peer->memory)if(PioneerQ38RecurrentBlockSnapshotContains(current,entry.first))entry.second=FullRead(current,entry.first);
    for(unsigned i=0;i<64;i++){peer->boot[i]=FullRead(current,M::Boot+i*4);peer->memory[M::Boot+i*4]=peer->boot[i];peer->memory[M::Job+i*4]=FullRead(current,M::Job+i*4);}
    peer->memory[M::Doorbell]=FullRead(current,M::Doorbell);
    for(unsigned i=0;i<16;i++){peer->source[i]=BootstrapIO::ResidentNocRead(BlackholeDramSourceOffset(i));peer->memory[M::Status+i*4]=FullRead(current,M::Status+i*4);}
    for(unsigned i=0;i<8;i++)peer->countBase[i]=machine.peerCounterBaseline[current-1][i]=BootstrapIO::ResidentNocRead(PioneerQ38PanelCounter(i));
    for(unsigned i=0;i<10;i++)peer->command[i]=C().command[i];
    assert(peer->pc==0x6000 && !peer->sequence);machine.child[current-1]=std::move(peer);
    for(unsigned w=0;w<96;w++)machine.child[w]->peer=machine.child[w^1].get();
   }else{
    assert(machine.pc==0x6000);
    for(auto& entry:machine.memory)if(PioneerQ38RecurrentBlockSnapshotContains(0,entry.first))entry.second=FullRead(0,entry.first);
    for(unsigned i=0;i<64;i++)machine.boot[i]=FullRead(0,RB::Boot+i*4);
    for(unsigned i=0;i<8;i++)machine.counts[i]=BootstrapIO::ResidentNocRead(PioneerQ38PanelCounter(i));
    for(unsigned i=0;i<10;i++)machine.command[i]=C().command[i];
   }
   for(unsigned i=0;i<PioneerQ38RecurrentBlockCodeWords(current);i++){
    uint32_t a=PioneerQ38RecurrentBlockCodeAddress(current,i),expected;
    assert(PioneerQ38RecurrentBlockProgramWord(current,a,expected) && FullRead(current,a)==expected);
    if(a==0x6000)assert((current?machine.child[current-1]->l1[0]:machine.l1[0])==expected);
    else if(current)assert(machine.child[current-1]->Load(a)==expected);else assert(machine.Load(a)==expected);
   }
   live[current]=true;
  }else assert(value==0x87ffff || (value==0x47800 && holds));
  C().coreReset=value;
 }
};
#ifndef PIONEER_Q38_RECURRENT_BLOCK_OWNER_ACTUAL_FIXTURE_ONLY
int main(int argc,char** argv){
 assert(argc==3);setvbuf(stdout,nullptr,_IOLBF,0);
 (void)&RecurrentOwnerUnusedInput;(void)&Prepare;(void)&Upload;(void)&Close;(void)&PreparePanel;(void)&Run;(void)&Job;(void)&ClosePanel;(void)&Finish;
 (void)&Refusals;(void)&LateRefusals;(void)&RunPersistentGdnPair;
 auto io=std::make_unique<RecurrentOwnerActualIO>(argv[1],argv[2]);
 auto cohort=std::make_unique<PioneerQ38DispatchLease>();auto memory=std::make_unique<PioneerQ38RecurrentBlockMemory>();auto owner=std::make_unique<PioneerQ38RecurrentBlockOwner>();
 std::vector<std::unique_ptr<PanelOwners>> leases;auto dram=BankProof(*io->core[0]);PioneerResidentBanks banks;assert(banks.Bind(dram));
 for(unsigned w=0;w<120;w++){
  leases.emplace_back(new PanelOwners(*io->core[w]));auto& n=cohort->worker[w];n.master=&leases[w]->store;n.noc=leases[w]->noc;if(w)n.noc.version=PioneerWorkerVersion(w);
  assert(PioneerQ38DispatchPrepareRole(*io,dram,banks,*cohort,w));
  for(unsigned i=0;i<16;i++)assert(cohort->authority.source[w][i]==io->machine.block->Boot(w)[16+i]);
 }
 std::vector<uint8_t> ledger(PioneerResidentBanks::Pages,1);PioneerQ38GdnIdentity id={1,2,3,4};PioneerQ38RecurrentBlockMemoryResult journal;PioneerQ38RecurrentBlockResult result;
 io->memoryState=&journal;io->track=true;
 auto original=io->data;
 assert(memory->PrepareHeld(*io,dram,banks,*cohort,*io->machine.canonical,id,ledger.data(),unsigned(ledger.size()),journal));
 io->execution=&result;
 bool prepared=owner->Prepare(*io,dram,banks,*cohort,*memory,*io->machine.canonical,*io->machine.block,id,ledger.data(),unsigned(ledger.size()),result);
 if(!prepared)fprintf(stderr,"QRB_ACTUAL_PREPARE_FAILED phase%u worker%u word%u starts%u\n",result.phase,result.worker,result.word,io->starts);
 assert(prepared && io->starts==119 && !io->live[0]);
 puts("PASS QRB actual owner released119 NEW virgin QMW1 boots/ROOT held/ALL installed artifact words/current sources/counters;NO synthetic ready");
 bool ran=owner->Run(*io,result);
 if(!ran)fprintf(stderr,"QRB_ACTUAL_RUN_FAILED phase%u worker%u word%u rootPC%08x peerPC%08x\n",result.phase,result.worker,result.word,io->machine.pc,io->machine.child[result.worker?result.worker-1:0]->pc);
 assert(ran);io->machine.VerifyBlock();
 puts("PASS QRB actual owner+ROOT8matrices/720jobs+640Conv+96GDN/RMS/residual/FFN/final5120/ALL119 packet-counter proofs;NO host products");
 bool closed=owner->Close(*io,result,journal);
 if(!closed)fprintf(stderr,"QRB_ACTUAL_CLOSE_FAILED phase%u worker%u word%u stopped%u held%u restored%u rootPC%08x\n",result.phase,result.worker,result.word,result.stopped,result.held,journal.restoredWords,io->machine.pc);
 assert(closed && owner->Closed() && memory->Closed() && owner->MatchesClosedNativeOwner(*memory,*io->machine.block)
  && io->starts==120 && io->holds==120 && result.completed==119 && result.stopped==119 && result.held==120
  && journal.restoredWords==TT_Q38_RECURRENT_BLOCK_SAVED_WORDS && journal.verifiedOriginalWords==2*TT_Q38_RECURRENT_BLOCK_SAVED_WORDS && io->data==original);
 unsigned touches=io->touches;
 assert(!owner->Run(*io,result) && !owner->Close(*io,result,journal) && !memory->CloseHeldNoLaunch(*io,journal) && io->touches==touches);
 puts("PASS QRB actual owner/real119STOP+ROOT/ALL120terminals BEFORE firsthold/ALL120holds BEFORE firstrestore/full11406904word restore+secondproof;synthetic raw inputs/MMIO/time ONLY,NOT native or silicon TPS");
 bool dispatched=PioneerQ38DispatchClose(*io,dram,banks,*cohort,true);
 if(!dispatched)fprintf(stderr,"QRB_LEGACY_END_FAILED phase%u worker%u detail%08x/%08x/%08x\n",cohort->closePhase,cohort->closeWorker,
  cohort->closeDetail[0],cohort->closeDetail[1],cohort->closeDetail[2]);
 assert(dispatched && cohort->closed && !cohort->unsafe && owner->MatchesClosedNativeOwner(*memory,*io->machine.block));
 puts("PASS QRB actual execution closure survives ACTUAL known-clean legacy dispatch CLOSE/ALL120 saved image+peer lease restoration;NOT native final clock/VM/PCI/host disposal");
}
#endif
