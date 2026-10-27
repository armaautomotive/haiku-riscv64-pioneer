/* SPDX-License-Identifier: MIT */
// Host lifetime/controller tests only. Device records below are EXPLICIT
// synthetic endpoints, NOT instruction execution, native authority or TPS.
#define PIONEER_Q38_MIXED_LIVE_MEMORY_FIXTURE_ONLY
#define Input LiveOwnerUnusedInput
#include "pioneer_q38_mixed_live_memory_test.cpp"
#undef Input
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38MixedLiveOwner.h"
namespace L=PioneerQ38MixedLiveBlockRootLayout;
namespace MW=PioneerQ38MixedWorkerLayout;
struct LiveOwnerIO:MixedLiveMemoryIO {
 const PioneerQ38MixedLiveBlockPlan* operation=nullptr;
 const PioneerQ38MixedLiveResult* execution=nullptr;
 bool executionScope=true;
 unsigned failure=0,failurePhase=0,failureWorker=0,pcObservations=0,holds=0;
 uint32_t initial[120][8]={};
 bool AtExecution()const{return execution && execution->phase==failurePhase && current==failureWorker;}
 bool BadExecution(unsigned kind){if(failure==kind && AtExecution()){observed=touches;return true;}return false;}
 bool MixedLiveExecutionScope(const PioneerQ38MixedLiveOwner&,const PioneerQ38MixedLiveMemory&,
  const PioneerQ38CanonicalModelPlan&,const PioneerQ38MixedLiveBlockPlan&,
  const PioneerQ38GdnIdentity&,const uint8_t*,unsigned){if(BadExecution(12))return false;return executionScope;}
 void Pause(){now++;}
 uint64_t Q38CohortNow(){Touch();if(BadExecution(11))return UINT64_MAX;return now;}
 uint32_t ResetRead(){auto value=MixedLiveMemoryIO::ResetRead();return BadExecution(5)?value^1:value;}
 uint32_t SourceRead(unsigned i){auto value=MixedLiveMemoryIO::SourceRead(i);return i==11 && BadExecution(6)?value^1:value;}
 uint32_t HostPciRead(unsigned p,uint16_t a,uint8_t bytes){
  auto value=MixedLiveMemoryIO::HostPciRead(p,a,bytes);return a==4 && BadExecution(8)?value^1:value;
 }
 uint32_t ResidentNocRead(uint32_t a){
  auto value=MixedLiveMemoryIO::ResidentNocRead(a);
  if((a==BlackholeDramSourceOffset(11) && BadExecution(7)) || (a==PioneerQ38PanelCounter(7) && BadExecution(9))
   || (!a && BadExecution(10)))return value^1;
  return value;
 }
 uint32_t RuntimePcRead(){
  Touch();assert(execution && C().coreReset==0x46800);
  // Speculative values are deliberately NOT faults or completed records.
  if(++pcObservations%4==1)return 0xfffffff1;
  return current?(FullRead(current,MW::Status+16)==2?0x20060:0x2004c):
   (FullRead(0,L::Status+16)==2?0x40060:0x4004c);
 }
 bool MixedLiveMemoryMappingStable(unsigned w){
  Touch();assert(w==current);return !BadExecution(1);
 }
 bool MixedLiveMemoryReadChecked(unsigned w,uint32_t a,uint32_t& value){
  if(!execution)return MixedLiveMemoryIO::MixedLiveMemoryReadChecked(w,a,value);
  Touch();reads++;if(BadExecution(2))return false;
  value=FullRead(w,a);
  if((a==(w?unsigned(MW::Status):unsigned(L::Status))+60 && BadExecution(13))
   || (w && a==MW::Job+252 && BadExecution(14)))value^=1;
  return true;
 }
 bool MixedLiveMemoryWrite(unsigned w,uint32_t a,uint32_t value){
  if(!execution)return MixedLiveMemoryIO::MixedLiveMemoryWrite(w,a,value);
  Touch();writes++;if(BadExecution(3))return false;
  assert(memoryState->savedWords==TT_Q38_MIXED_LIVE_SAVED_WORDS);
  FullWrite(w,a,value);
  if(!w && a==L::Doorbell && value && execution->phase<8)SyntheticBlock(value==2);
  return true;
 }
 void RuntimeResetWrite(uint32_t value){
  Touch();assert(execution && operation);C().resets.push_back(value);
  if(BadExecution(4))return;
  if(value==0x87ffff){
   if(execution->phase==7){
    // Independent endpoint assertions before EVERY hold, including FIRST.
    unsigned selected=current;
    for(unsigned w=0;w<120;w++){
     current=w;assert(FullRead(w,(w?unsigned(MW::Status):unsigned(L::Status))+16)==2);
    }
    current=selected;holds++;
   }
  }else if(value==0x46800){
   released++;for(unsigned i=0;i<16;i++)FullWrite(current,(current?unsigned(MW::Status):unsigned(L::Status))+i*4,i==3?(current?unsigned(MW::Magic):unsigned(L::Magic)):0);
  }else assert(value==0x47800 && holds);
  C().coreReset=value;
 }
 void ResidentNocWrite(uint32_t a,uint32_t value){
  if(!execution){BootstrapIO::ResidentNocWrite(a,value);return;}
  Touch();if(a<40){C().command[a/4]=value;return;}
  assert(!current && a==0x40 && value==1 && C().command[7]==0x212);
  auto& c=C();c.writeCounters[0]++;c.writeCounters[1]+=c.command[8]/64;c.writeCounters[3]++;c.accepted++;
 }
 void Counts(unsigned w,uint32_t* out){
  auto& c=*core[w];out[0]=c.counters[0];out[1]=c.counters[1];out[2]=out[6]=c.accepted;out[3]=c.counters[3];
  out[4]=c.writeCounters[0];out[5]=c.writeCounters[1];out[7]=c.writeCounters[3];
 }
 void AddMatrix(unsigned w,unsigned slot){
  const auto* d=operation->Recipe(slot);if(w-1>=d[7])return;
  const auto* part=d+80+(w-1)*8;unsigned block=d[14]/(d[15]==8?32:256),tile=d[15]==8?576:d[15]==12?2304:3392;
  auto& c=C();unsigned requests=part[2]*block+(d[14]*2+16383)/16384+1;
  c.counters[0]+=requests;c.counters[3]+=requests;c.accepted+=requests;
  c.counters[1]+=part[2]*block*(tile/64)+(d[14]*2+((block*4+63)&~63U))/64;
 }
 void SyntheticBlock(bool stop){
  assert(!current);const auto* o=operation->Outer();
  for(unsigned w=1;w<120;w++){
   current=w;unsigned n=L::RunDelta(w-1)+unsigned(stop);const auto* d=operation->Recipe(6);const auto* p=d+80+(w-1)*8;
   const unsigned head[]={MW::Magic,n,stop?2U:1U,w,MW::Peer(w),d[42],d[40],d[6]+unsigned(stop)};
   const unsigned capsule[]={0x31483851,12,2304,0,0,17408,p[0],p[2],p[4],1};
   for(unsigned i=0;i<64;i++)FullWrite(w,MW::Job+i*4,i<8?head[i]:i<16?d[24+i]:!stop && i<26?capsule[i-16]:!stop && i==26?1U:0);
   FullWrite(w,MW::Doorbell,n);
   const unsigned status[]={n,n,0,MW::Magic,stop?2U:0U,0,o[3],o[4]};
   for(unsigned i=0;i<16;i++)FullWrite(w,MW::Status+i*4,i<8?status[i]:0);
   if(stop)continue;
   for(unsigned slot=0;slot<6;slot++)AddMatrix(w,slot);
   uint32_t before[8],after[8];Counts(w,before);AddMatrix(w,6);Counts(w,after);
   const unsigned cmd[]={p[0]+(p[2]*68-1)*2304,0,p[4],0x90000,0,MW::Peer(w),0,0,2304,0};
   for(unsigned i=0;i<10;i++)C().command[i]=cmd[i];
   for(unsigned i=0;i<64;i++)FullWrite(w,0x83f00+i*4,0);
   for(unsigned i=0;i<8;i++){FullWrite(w,0x83f00+i*4,before[i]);FullWrite(w,0x83f20+i*4,after[i]);}
   const unsigned proof[]={p[2]*17408/16,n,w,0x32504651};
   for(unsigned i=0;i<4;i++)FullWrite(w,0x83f40+i*4,proof[i]);
   for(unsigned i=0;i<10;i++)FullWrite(w,0x83f50+i*4,cmd[i]);
   FullWrite(w,0x83f78,operation->Boot(w)[16]);FullWrite(w,0x83f7c,MW::Peer(w));
   for(unsigned i=0;i<16;i++)FullWrite(w,0x83f80+i*4,operation->Boot(w)[16+i]);
  }
  current=0;
  // Arbitrary synthetic ROOT counts; exact observed receipt equality is
  // tested, never mislabeled as independently derived instruction counts.
  C().counters[0]+=99;C().counters[1]+=199;C().counters[3]+=99;C().accepted+=99;
  const unsigned lastRun[]={L::Scratch,0,129,o[9]+L::FinalOffset+16384,0,o[8],0,0x212,4096,0};
  const unsigned lastStop[]={0xffb20228,0,MW::Peer(119),L::Scratch+8,0,129,0,0,4,0};
  for(unsigned i=0;i<10;i++)C().command[i]=stop?lastStop[i]:lastRun[i];
  unsigned summary=stop?unsigned(L::StopSummary):unsigned(L::JointSummary);uint32_t count[8];Counts(0,count);
  for(unsigned i=0;i<64;i++)FullWrite(0,summary+i*4,0);
  const unsigned head[]={stop?unsigned(L::StopMagic):unsigned(L::Magic),stop?2U:1U,stop?1U:o[3],stop?119U:o[4],stop?o[3]:5120U,stop?o[4]:119U,stop?129U:7U,stop?1U:17408U};
  for(unsigned i=0;i<8;i++){FullWrite(0,summary+i*4,head[i]);FullWrite(0,summary+32+i*4,count[i]);}
  if(stop){
   for(unsigned i=0;i<10;i++)FullWrite(0,summary+64+i*4,C().command[i]);
   for(unsigned i=0;i<8;i++)FullWrite(0,summary+104+i*4,operation->Boot(0)[8+i]);
   for(unsigned i=0;i<16;i++)FullWrite(0,summary+136+i*4,operation->Boot(0)[16+i]);
  }
  const unsigned status[]={stop?2U:1U,stop?2U:1U,0,L::Magic,stop?2U:0U,stop?13U:12U,o[3],o[4]};
  for(unsigned i=0;i<16;i++)FullWrite(0,L::Status+i*4,i<8?status[i]:0);
 }
};
struct LiveOwnerFixture {
 LiveOwnerIO io;tt_dram_samples_result dram;PioneerResidentBanks banks;
 std::unique_ptr<PioneerQ38DispatchLease> cohort{new PioneerQ38DispatchLease};
 std::unique_ptr<PioneerQ38MixedLiveMemory> memory{new PioneerQ38MixedLiveMemory};
 std::unique_ptr<PioneerQ38CanonicalModelPlan> canonical{new PioneerQ38CanonicalModelPlan};
 std::unique_ptr<PioneerQ38MixedLiveBlockPlan> plan{new PioneerQ38MixedLiveBlockPlan};
 std::unique_ptr<PioneerQ38MixedLiveOwner> owner{new PioneerQ38MixedLiveOwner};
 std::vector<std::unique_ptr<PanelOwners>> owners;
 std::vector<uint8_t> ledger=std::vector<uint8_t>(PioneerResidentBanks::Pages,1);
 PioneerQ38GdnIdentity id={1,2,3,4};PioneerQ38MixedLiveMemoryResult journal;PioneerQ38MixedLiveResult result;
 LiveOwnerFixture(){
  dram=BankProof(*io.core[0]);assert(banks.Bind(dram) && canonical->Build());
  for(unsigned w=0;w<120;w++){
   owners.emplace_back(new PanelOwners(*io.core[w]));auto& n=cohort->worker[w];n.master=&owners[w]->store;
   n.noc=owners[w]->noc;if(w)n.noc.version=PioneerWorkerVersion(w);assert(PioneerQ38DispatchPrepareRole(io,dram,banks,*cohort,w));
  }
  assert(plan->Build(*canonical,3,0,0,0,2,1,id,&cohort->authority.source[0][0],0,canonical->Base+canonical->Workspace(0)->offset));
  io.memoryState=&journal;io.track=true;
  assert(memory->PrepareHeld(io,dram,banks,*cohort,*canonical,id,ledger.data(),unsigned(ledger.size()),journal));
  io.execution=&result;io.operation=plan.get();
 }
 bool PrepareOwner(){return owner->Prepare(io,dram,banks,*cohort,*memory,*canonical,*plan,id,ledger.data(),unsigned(ledger.size()),result);}
 bool RunOwner(){return owner->Run(io,result);}
 bool CloseOwner(){return owner->Close(io,result,journal);}
 void Unknown(){
  assert(result.unsafe && owner->Unsafe() && memory->Unsafe() && cohort->unsafe && banks.Quarantined() && io.quarantined && io.retained);
  unsigned touches=io.touches;assert(!PrepareOwner() && !RunOwner() && !CloseOwner() && io.touches==touches);
 }
};
#ifndef PIONEER_Q38_MIXED_LIVE_OWNER_FIXTURE_ONLY
int main(int argc,char** argv){
 (void)&LiveOwnerUnusedInput;(void)&Prepare;(void)&Upload;(void)&Close;(void)&PreparePanel;(void)&Run;(void)&Job;(void)&ClosePanel;(void)&Finish;
 assert(argc==1 || (argc==2 && (!strcmp(argv[1],"--faults") || !strcmp(argv[1],"--preconditions"))));setvbuf(stdout,nullptr,_IOLBF,0);
 if(argc==2 && !strcmp(argv[1],"--preconditions")){
  LiveOwnerFixture f;const PioneerQ38MixedLiveResult pristine;unsigned refused=0;
  auto inert=[&](){unsigned touches=f.io.touches;auto before=f.result;
   assert(!f.PrepareOwner() && f.io.touches==touches && !f.owner->Claimed() && !f.owner->Unsafe()
    && !f.io.quarantined && !memcmp(&before,&f.result,sizeof(before)));refused++;};
  static_assert(sizeof(f.result)==16*sizeof(uint32_t),"all result fields tested");
  for(unsigned i=0;i<16;i++){
   auto* words=reinterpret_cast<uint32_t*>(&f.result);words[i]=i?1U:2U;inert();f.result=pristine;
  }
  f.id.session=0;inert();f.id.session=1;
  f.id.sequence=3;inert();f.id.sequence=2;
  const auto* p=f.canonical->Part(0);unsigned page=0;
  for(unsigned i=0;i<f.canonical->Count();i++)if(f.canonical->Part(i)->kind==f.canonical->WeightKind && f.canonical->Part(i)->layer==3){p=f.canonical->Part(i);break;}
  page=p->bank*unsigned(f.canonical->Capacity/4096)+p->offset/4096;
  for(unsigned bad:{0U,2U,255U}){f.ledger[page]=uint8_t(bad);inert();}f.ledger[page]=1;
  assert(f.PrepareOwner()); // rejected calls never consumed its one real attempt
  printf("PASS actual host owner inert precondition refusals%u/all16resultfields/invalid+foreignidentity/missing+invalidlearnedpage;NO IO/state mutation before valid attempt;synthetic scope ONLY\n",refused);return 0;
 }
 if(argc==1){
  LiveOwnerFixture f;bool prepared=f.PrepareOwner(),ran=prepared && f.RunOwner(),closed=ran && f.CloseOwner();
  if(!closed)fprintf(stderr,"OWNER_FAILURE prepared%u ran%u phase%u worker%u word%u completed%u stopped%u held%u restored%u\n",
   unsigned(prepared),unsigned(ran),f.result.phase,f.result.worker,f.result.word,f.result.completed,f.result.stopped,f.result.held,f.journal.restoredWords);
  assert(closed);
  assert(f.owner->Closed() && f.memory->Closed() && !f.owner->Unsafe() && f.result.completed==119 && f.result.stopped==119
   && f.result.held==120 && f.result.closed && f.io.holds==120 && f.io.released==120
   && f.journal.restoredWords==TT_Q38_MIXED_LIVE_SAVED_WORDS && f.journal.verifiedOriginalWords==2*TT_Q38_MIXED_LIVE_SAVED_WORDS);
  unsigned touched=f.io.touches;assert(!f.PrepareOwner() && !f.RunOwner() && !f.CloseOwner() && f.io.touches==touched);
  puts("PASS actual host owner synthetic-endpoint RUN/119STOP/ALL120terminal-before-FIRSThold/full11398712word restore+secondproof/speculative PC observations;NO actual instructions/native/math/speed qualification");return 0;
 }
 unsigned faults=0;
 for(unsigned phase:{1U,2U,4U,6U,7U,8U,9U})for(unsigned kind:{1U,2U,3U}){
  // Each chosen field is actually reached. Phase7 has only physical
  // guard/reset IO, not L1 reads; phase9 is read-only.
  if(kind==2 && phase==7)continue;
  if(kind==3 && (phase==2 || phase==4 || phase==7 || phase==9))continue;
  LiveOwnerFixture f;f.io.failure=kind;f.io.failurePhase=phase;f.io.failureWorker=phase==2?119:0;
  bool prepared=f.PrepareOwner(),ran=prepared && f.RunOwner(),closed=ran && f.CloseOwner();
  if(closed)fprintf(stderr,"FAULT_NOT_REACHED phase%u kind%u\n",phase,kind);
  assert(!closed && f.io.observed==f.io.touches);f.Unknown();faults++;
 }
 for(unsigned kind=5;kind<=14;kind++){
  LiveOwnerFixture f;f.io.failure=kind;f.io.failurePhase=kind==11?4:kind==13?3:kind==14?6:1;f.io.failureWorker=kind==14?119:0;
  bool prepared=f.PrepareOwner(),ran=prepared && f.RunOwner(),closed=ran && f.CloseOwner();
  if(closed)fprintf(stderr,"SEMANTIC_FAULT_NOT_REACHED kind%u\n",kind);
  assert(!closed && f.io.observed==f.io.touches);f.Unknown();faults++;
 }
 printf("PASS actual host owner synthetic endpoint faults%u/FIRST failed map/read/write retains journal+ALLhosts/NO laterIO/reset/restore/retry;NO native or arithmetic qualification\n",faults);
}
#endif
