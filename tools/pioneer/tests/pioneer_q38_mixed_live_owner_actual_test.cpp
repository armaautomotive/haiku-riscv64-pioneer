/* SPDX-License-Identifier: MIT */
// SAME production host owner + actual QBL1/119QMW1 instruction machines.
// Only raw bytes, physical MMIO and time are simulated. No synthetic
// completion, nonce, receipt, numeric result or ready flag is supplied.
#define PIONEER_Q38_MIXED_LIVE_MEMORY_FIXTURE_ONLY
#define Input LiveActualUnusedInput
#include "pioneer_q38_mixed_live_memory_test.cpp"
#undef Input
#define PIONEER_Q38_MIXED_LIVE_CANONICAL_MACHINE_ONLY
#include "pioneer_q38_mixed_live_canonical_joined_test.cpp"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38MixedLiveOwner.h"
struct LiveActualIO:MixedLiveMemoryIO {
 CanonicalLiveMachine machine;
 const char* workerPath;
 const PioneerQ38MixedLiveResult* execution=nullptr;
 bool live[120]={};unsigned starts=0,holds=0;
#ifdef PIONEER_Q38_MIXED_LIVE_VALIDATION_READBACK
 unsigned readbackPackets=0;
#endif
 LiveActualIO(const char* root,const char* worker,const char* qdp,const char* qda)
  :machine(root,worker,qdp,qda),workerPath(worker){
  // Canonical staging uses the MMIO fabric's valid DRAM-routing table,
  // not the arithmetic fixture's arbitrary source-cookie words. These
  // staging-only peers are replaced by NEW unbooted machines on release.
  machine.boot[4]=core[0]->source[0]|0x10000;machine.memory[T::Boot+16]=machine.boot[4];
  for(unsigned i=0;i<16;i++)machine.memory[T::Boot+64+i*4]=machine.boot[16+i]=machine.source[i]=core[0]->source[i]|(i?0U:0x10000U);
  for(unsigned w=1;w<120;w++){
   auto& peer=*machine.child[w-1];peer.boot[5]=core[w]->source[0]|0x10000;peer.memory[M::Boot+20]=peer.boot[5];
   for(unsigned i=0;i<16;i++)peer.memory[M::Boot+64+i*4]=peer.boot[16+i]=peer.source[i]=core[w]->source[i]|(i?0U:0x10000U);
  }
  machine.StageCanonical(1,0,3);
 }
 bool MixedLiveExecutionScope(const PioneerQ38MixedLiveOwner&,const PioneerQ38MixedLiveMemory&,
  const PioneerQ38CanonicalModelPlan&,const PioneerQ38MixedLiveBlockPlan&,const PioneerQ38GdnIdentity&,const uint8_t*,unsigned){return true;}
 uint32_t SourceRead(unsigned i){Touch();return core[0]->SourceRead(i);} // fixed global PCIe source, not the selected worker
 void Pause(){now++;} // synthetic monotonic clock;NOT silicon time
 void Sync(unsigned w){
  // Only a running instruction machine can change these physical NoC
  // registers. Once held, host restore writes are authoritative; stale
  // emulator fields must never overwrite them on a later alias selection.
  if(!live[w] || core[w]->coreReset!=0x46800)return;
  auto& c=*core[w];uint32_t count[8];
  for(unsigned i=0;i<8;i++)count[i]=w?machine.child[w-1]->Count(i):machine.counts[i];
  c.counters[0]=count[0];c.counters[1]=count[1];c.accepted=count[2];c.counters[3]=count[3];
  c.writeCounters[0]=count[4];c.writeCounters[1]=count[5];c.writeCounters[3]=count[7];
  for(unsigned i=0;i<10;i++)c.command[i]=w?machine.child[w-1]->command[i]:machine.command[i];
  c.pending=w?machine.child[w-1]->pending:machine.pending;
 }
 bool SelectRuntimeCore(unsigned w){bool okay=CodeIO::SelectRuntimeCore(w);if(okay)Sync(w);return okay;}
 void Advance(unsigned w){
  if(!live[w] || core[w]->coreReset!=0x46800)return;
  if(w){machine.Advance(w-1);Sync(w);return;}
  for(unsigned i=0;i<1048576;i++){
   if(machine.pc==0x40060 || machine.pc==0x4009c
    || ((machine.pc==0x4004c || machine.pc==0x40050) && machine.memory.at(T::Doorbell)==machine.memory.at(T::Status+4)))break;
   machine.StepScalar();
  }
  Sync(0);
 }
 uint32_t PhysicalRead(unsigned w,unsigned a){
  if(!live[w] || PioneerQ38MixedLiveCodeContains(w,a))return FullRead(w,a);
  const auto& memory=w?machine.child[w-1]->memory:machine.memory;
  auto found=memory.find(a);return found==memory.end()?FullRead(w,a):found->second;
 }
 void PhysicalWrite(unsigned w,unsigned a,uint32_t value){
  FullWrite(w,a,value);
  if(live[w] && !PioneerQ38MixedLiveCodeContains(w,a)){
   if(w)machine.child[w-1]->memory[a]=value;else machine.memory[a]=value;
  }
 }
 bool MixedLiveMemoryReadChecked(unsigned w,uint32_t a,uint32_t& output){
  if(!execution)return MixedLiveMemoryIO::MixedLiveMemoryReadChecked(w,a,output);
  Touch();reads++;if(a==(w?unsigned(M::Status):unsigned(T::Status)))Advance(w);
  output=PhysicalRead(w,a);return true;
 }
 bool MixedLiveMemoryWrite(unsigned w,uint32_t a,uint32_t value){
  if(!execution)return MixedLiveMemoryIO::MixedLiveMemoryWrite(w,a,value);
  Touch();writes++;PhysicalWrite(w,a,value);return true;
 }
 uint32_t RuntimePcRead(){Touch();assert(live[current] && C().coreReset==0x46800);return current?machine.child[current-1]->pc:machine.pc;}
 void ResidentNocWrite(uint32_t a,uint32_t value){
  if(!execution){BootstrapIO::ResidentNocWrite(a,value);return;}
  Touch();if(a<40){C().command[a/4]=value;return;}
#ifdef PIONEER_Q38_MIXED_LIVE_VALIDATION_READBACK
  if(!current && live[0] && !C().command[7]){
   assert(a==0x40 && value==1 && holds==120 && execution->phase==7 && execution->stopped==119 && execution->held==120 && readbackPackets<5);
   assert(machine.pc==0x40060 && machine.memory.at(T::Status+16)==2 && !machine.pending);
   for(unsigned w=0;w<120;w++)assert(core[w]->coreReset==0x47800);
   for(const auto& peer:machine.child)assert(peer->pc==0x20060 && peer->memory.at(M::Status+16)==2 && !peer->pending);
   auto& c=C();const auto& o=machine.blockOuter;
   const unsigned next[]={o[9]+T::FinalOffset+readbackPackets*4096,0,o[8],0x80000,0,129,0,0,4096,0};
   for(unsigned i=0;i<10;i++)assert(c.command[i]==next[i]);
   // ONLY actual GDDR bytes written by genuine graph instructions. No
   // oracle products or synthesized completion/status/nonce/ready record.
   for(unsigned i=0;i<1024;i++)PhysicalWrite(0,0x80000+i*4,machine.dram.at(CanonicalLiveMachine::Key(next[2],next[0]+i*4)));
   c.counters[0]++;c.counters[1]+=64;c.counters[3]++;c.accepted++;readbackPackets++;return;
  }
#endif
  // Real host setup transfer, not a graph instruction or a CPU product.
  assert(!current && !live[0] && a==0x40 && value==1);auto& c=C();
  assert(c.command[7]==0x212 && c.command[2]==129 && !c.command[1] && !c.command[4] && !c.command[6] && !c.command[9]);
  for(unsigned i=0;i<c.command[8]/4;i++)machine.dram[CanonicalLiveMachine::Key(c.command[5],c.command[3]+i*4)]=PhysicalRead(0,c.command[0]+i*4);
  c.writeCounters[0]++;c.writeCounters[1]+=c.command[8]/64;c.writeCounters[3]++;c.accepted++;
 }
 void RuntimeResetWrite(uint32_t value){
  Touch();assert(execution);C().resets.push_back(value);
  if(value==0x87ffff && execution->phase==7){
   assert(machine.pc==0x40060 && machine.memory.at(T::Status+16)==2);
   for(const auto& peer:machine.child)assert(peer->pc==0x20060 && peer->memory.at(M::Status+16)==2 && !peer->pending);
   holds++;
  }
  if(value==0x46800){
   assert(!live[current]);starts++;
   if(current){
    // NEW machine starts at its real boot instruction, never a caller PC
    // reset or fabricated WAIT. Import staged L1 and actual counter base.
    auto peer=std::make_unique<RetainedPeer>(workerPath,current);
    for(auto& entry:peer->memory)if(PioneerQ38MixedLiveSnapshotContains(current,entry.first))entry.second=FullRead(current,entry.first);
    for(unsigned i=0;i<64;i++){peer->boot[i]=FullRead(current,M::Boot+i*4);peer->memory[M::Boot+i*4]=peer->boot[i];peer->memory[M::Job+i*4]=FullRead(current,M::Job+i*4);}
    peer->memory[M::Doorbell]=FullRead(current,M::Doorbell);
    for(unsigned i=0;i<16;i++){peer->source[i]=BootstrapIO::ResidentNocRead(BlackholeDramSourceOffset(i));peer->memory[M::Status+i*4]=FullRead(current,M::Status+i*4);}
    for(unsigned i=0;i<8;i++)peer->countBase[i]=BootstrapIO::ResidentNocRead(PioneerQ38PanelCounter(i));
    for(unsigned i=0;i<10;i++)peer->command[i]=C().command[i];
    assert(peer->pc==0x6000);machine.child[current-1]=std::move(peer);
   }else{
    assert(machine.pc==0x6000);
    for(auto& entry:machine.memory)if(PioneerQ38MixedLiveSnapshotContains(0,entry.first))entry.second=FullRead(0,entry.first);
    for(unsigned i=0;i<64;i++)machine.boot[i]=FullRead(0,T::Boot+i*4);
    for(unsigned i=0;i<8;i++)machine.counts[i]=BootstrapIO::ResidentNocRead(PioneerQ38PanelCounter(i));
    for(unsigned i=0;i<10;i++)machine.command[i]=C().command[i];
   }
   // All immutable bytes were installed/read back by the real journal;
   // assert artifact execution fetches exactly that installed registry.
   for(unsigned i=0;i<PioneerQ38MixedLiveCodeWords(current);i++){
    uint32_t a=PioneerQ38MixedLiveCodeAddress(current,i),expected;
    assert(PioneerQ38MixedLiveProgramWord(current,a,expected) && FullRead(current,a)==expected);
    if(a==0x6000)assert((current?machine.child[current-1]->l1[0]:machine.l1[0])==expected);
    else if(current)assert(machine.child[current-1]->Load(a)==expected);else assert(machine.Load(a)==expected);
   }
   live[current]=true;
  }else assert(value==0x87ffff || (value==0x47800 && holds));
  C().coreReset=value;
 }
};
#ifndef PIONEER_Q38_MIXED_LIVE_OWNER_ACTUAL_FIXTURE_ONLY
int main(int argc,char** argv){
 assert(argc==5);setvbuf(stdout,nullptr,_IOLBF,0);
 (void)&LiveActualUnusedInput;(void)&Prepare;(void)&Upload;(void)&Close;(void)&PreparePanel;(void)&Run;(void)&Job;(void)&ClosePanel;(void)&Finish;
 (void)&BlockInitialRefusals;(void)&BlockLaterRefusals;(void)&JoinedRefusals;(void)&ProjectionRefusals;(void)&Refusals;(void)&Group;(void)&Geometry;(void)&Faults;(void)&D::Run;
 auto io=std::make_unique<LiveActualIO>(argv[1],argv[2],argv[3],argv[4]);
 auto cohort=std::make_unique<PioneerQ38DispatchLease>();auto memory=std::make_unique<PioneerQ38MixedLiveMemory>();auto owner=std::make_unique<PioneerQ38MixedLiveOwner>();
 std::vector<std::unique_ptr<PanelOwners>> leases;auto dram=BankProof(*io->core[0]);PioneerResidentBanks banks;assert(banks.Bind(dram));
 for(unsigned w=0;w<120;w++){
  leases.emplace_back(new PanelOwners(*io->core[w]));auto& n=cohort->worker[w];n.master=&leases[w]->store;n.noc=leases[w]->noc;if(w)n.noc.version=PioneerWorkerVersion(w);
  assert(PioneerQ38DispatchPrepareRole(*io,dram,banks,*cohort,w));
  for(unsigned i=0;i<16;i++)assert(cohort->authority.source[w][i]==io->machine.plan->Boot(w)[16+i]);
 }
 std::vector<uint8_t> ledger(PioneerResidentBanks::Pages,1);PioneerQ38GdnIdentity id={1,2,3,4};PioneerQ38MixedLiveMemoryResult journal;PioneerQ38MixedLiveResult result;
 io->memoryState=&journal;io->track=true;
 assert(memory->PrepareHeld(*io,dram,banks,*cohort,*io->machine.canonical,id,ledger.data(),unsigned(ledger.size()),journal));
 io->execution=&result;
 assert(owner->Prepare(*io,dram,banks,*cohort,*memory,*io->machine.canonical,*io->machine.plan,id,ledger.data(),unsigned(ledger.size()),result));
 puts("PASS actual owner released119 genuinely fresh QMW1 boots/ROOT remains held/actual installed code and current sources/counters;NO synthetic ready");
 bool ran=owner->Run(*io,result);
 if(!ran)fprintf(stderr,"ACTUAL_OWNER_RUN_FAILURE phase%u worker%u word%u rootPC%08x peerPC%08x\n",result.phase,result.worker,result.word,io->machine.pc,io->machine.child[result.worker?result.worker-1:0]->pc);
 assert(ran);io->machine.VerifyLive();
 puts("PASS actual owner RUN + actual complete dense block ORIGINAL-input/RMS/7matrices/dense attention/SiLU/residual/SAME119;NO host numeric products");
 bool closed=owner->Close(*io,result,journal);
 if(!closed)fprintf(stderr,"ACTUAL_OWNER_CLOSE_FAILURE phase%u worker%u word%u stopped%u held%u restored%u rootPC%08x\n",
  result.phase,result.worker,result.word,result.stopped,result.held,journal.restoredWords,io->machine.pc);
 assert(closed);
 assert(owner->Closed() && memory->Closed() && io->starts==120 && io->holds==120 && result.completed==119 && result.stopped==119 && result.held==120
  && journal.restoredWords==TT_Q38_MIXED_LIVE_SAVED_WORDS && journal.verifiedOriginalWords==2*TT_Q38_MIXED_LIVE_SAVED_WORDS);
 puts("PASS actual owner+QBL1/QMW1 RUN/real119STOP/ALL120actualterminals BEFORE FIRSThold/complete11398712word restore+secondproof;RAW synthetic weights/MMIO/time only,NOT native original-model or silicon TPS");
}
#endif
