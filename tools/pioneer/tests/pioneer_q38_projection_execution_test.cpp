/* SPDX-License-Identifier: MIT */
// Actual private producer+new execution owner+installed QPR1/ALL119 QFP3.
// Hardware/clock/transport/learned weights simulated,NOT a native adapter.
#define PIONEER_Q38_PROJECTION_MEMORY_FIXTURE_ONLY
#include "pioneer_q38_projection_memory_test.cpp"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38ProjectionGraphOwner.h"
#include "pioneer_q38_projection_execution_producer.h"
#include "pioneer_q38_projection_execution_machine.h"
#include "pioneer_q38_projection_stash_observer.h"
struct ExecutionIO:ProjectionMemoryIO,ProjectionExecutionFabric {
 ProjectionExecutionProducer* producer=nullptr;
 PioneerQ38ProjectionGraphOwner* expectedOwner=nullptr;PioneerQ38ProjectionGraphMemory* expectedMemory=nullptr;
 PioneerQ38ProjectionGraphResult* result=nullptr;std::unique_ptr<ProjectionExecutionMachine> machine;
 bool scope=true,retainedExecution=false,damaged=false;unsigned starts=0,boots=0,stops=0,receiptFault=UINT32_MAX,postFault=0,restoreFault=0,clockFault=0;
 void Pause(){now++;} // TEST clock,not the legacy held-only fixture's10ms-per-instruction-quantum clock
 uint64_t Q38CohortNow(){return starts && clockFault?(clockFault==1?now+30000001:0):now;}
 bool ProjectionGraphMemoryScope(const PioneerQ38ProjectionGraphMemory& memory,const PioneerQ38CanonicalModelPlan& plan,
  const PioneerQ38GdnIdentity& id,const uint8_t* ledger,unsigned pages){
  return producer && producer->Valid() && scope && &memory==expectedMemory && &plan==&producer->Model().plan
   && id==producer->Model().identity && ledger==producer->Ledger() && pages==PioneerResidentBanks::Pages;
 }
 bool ProjectionGraphExecutionScope(const PioneerQ38ProjectionGraphOwner& owner,const PioneerQ38ProjectionGraphMemory& memory,
  const PioneerQ38ProjectionMutableBanks& graph,const PioneerQ38CanonicalModelPlan& plan,const PioneerQ38LearnedRmsController& input,
  const PioneerQ38ProjectionIdentity& id,const uint8_t* ledger,unsigned pages){
  return &owner==expectedOwner && &graph==&producer->Model().graph && &input==&producer->Model().rms
   && ProjectionGraphMemoryScope(memory,plan,id,ledger,pages);
 }
 void ProjectionGraphRetainHostOwners(){retainedExecution=retained=true;producer->Retain();}
 bool ProjectionGraphMemoryWrite(unsigned w,uint32_t a,uint32_t v){Touch();writes++;assert(w==current);
  if(restoreFault && result && result->phase==9 && w==119 && result->word==PioneerQ38ProjectionGraphSnapshotWords(119)-1)return false;
  FullWrite(w,a,v);return true;}
 uint32_t Q38PanelRead(uint32_t a){Touch();uint32_t v=PioneerQ38ProjectionGraphSnapshotContains(current,a)?FullRead(current,a):BootstrapIO::Q38PanelRead(a);
  if(result && result->phase==7 && !current && a==TT_Q38_PROJECTION_GRAPH_STATS+receiptFault*4)v^=0x80000000;
  return v;}
 void Q38PanelWrite(uint32_t a,uint32_t v){Touch();if(PioneerQ38ProjectionGraphSnapshotContains(current,a))FullWrite(current,a,v);else BootstrapIO::Q38PanelWrite(a,v);}
 void ResidentNocWrite(uint32_t a,uint32_t v){Touch();if(track)assert(a<40 && !(a&3));BootstrapIO::ResidentNocWrite(a,v);}
 uint32_t RuntimeStatusRead(unsigned i){Touch();if(track && !current && C().coreReset==0x46800)machine->Advance(0);
  if(postFault && !damaged && result && result->phase==7 && starts && C().coreReset==0x46800 && machine->Pc(0)==TT_Q38_DISPATCH_PARK){damaged=true;
   if(postFault==1)C().source[11]^=1;if(postFault==2)C().coreReset=0x47800;if(postFault==3)C().command[9]^=1;
   if(postFault==4)C().counters[0]++;if(postFault==5)data.at(Index(0,0x64000))^=1;if(postFault==6)C().mapping=false;
  }
  uint32_t v=FullRead(current,0x17ffc0+i*4);return v;}
 void RuntimeStatusWrite(unsigned i,uint32_t v){Touch();FullWrite(current,0x17ffc0+i*4,v);}
 uint32_t RuntimePcRead(){Touch();if(C().coreReset==0x46800)machine->Advance(current);return machine->Pc(current);}
 void RuntimeResetWrite(uint32_t value){Touch();C().coreReset=value;
  if(value==0x46800){assert(track && machine);machine->Start(current);if(current)boots++;else starts++;}
  if(value==0x47800)stops++;
 }
 bool ProjectionGraphMemoryReadChecked(unsigned w,uint32_t a,uint32_t& out){
  Touch();reads++;assert(w==current);out=FullRead(w,a);
  return true;
 }
 uint32_t ReadLocal(unsigned w,uint32_t a)override{unsigned old=current;current=w;uint32_t v=FullRead(w,a);current=old;return v;}
 void WriteLocal(unsigned w,uint32_t a,uint32_t v)override{
  // DEVICE stores through the SAME aliases,not host writes while running.
  assert(PioneerQ38ProjectionGraphSnapshotContains(w,a));
  if(a==0x6000)core[w]->entry=v;
  else if(a>=0x70000 && a<0x74000)core[w]->view[(a-TT_Q38_PANEL_VIEW_BASE)/4]=v;
  else data.at(Index(w,a))=v;
 }
 uint32_t NoC(unsigned w,unsigned a)override{return core[w]->ResidentNocRead(a);}
 void SetControls(unsigned w,const uint32_t* c,const uint32_t* n)override{
  auto& p=*core[w];for(unsigned i=0;i<10;i++)p.command[i]=c[i];
  p.counters[0]=n[0];p.counters[1]=n[1];p.accepted=n[2];p.counters[3]=n[3];p.writeCounters[0]=n[4];p.writeCounters[1]=n[5];p.writeCounters[3]=n[7];
 }
 uint32_t Reset(unsigned w)override{return core[w]->coreReset;}
 std::vector<uint32_t>& Gddr()override{return producer->DeviceWords();}
};
int main(int argc,char** argv){
 assert(argc==6 || argc==7);setvbuf(stdout,nullptr,_IOLBF,0);bool wide=argc==7 && !strcmp(argv[6],"--wide");int fault=argc==7 && !wide?atoi(argv[6]):-1;
 (void)&Input;(void)&Prepare;(void)&Upload;(void)&Close;(void)&PreparePanel;(void)&Run;(void)&Job;(void)&ClosePanel;(void)&Finish;
 ProjectionExecutionProducer producer(argv[3],argv[4],argv[5]);auto& model=producer.Model();assert(producer.Valid());
 ExecutionIO io;io.producer=&producer;
 auto cohort=std::make_unique<PioneerQ38DispatchLease>();auto memory=std::make_unique<PioneerQ38ProjectionGraphMemory>();
 auto owner=std::make_unique<PioneerQ38ProjectionGraphOwner>();PioneerQ38ProjectionGraphResult result;
 std::vector<std::unique_ptr<PanelOwners>> participants;
 for(unsigned w=0;w<120;w++){participants.emplace_back(new PanelOwners(*io.core[w]));auto& n=cohort->worker[w];n.master=&participants[w]->store;
  n.noc=participants[w]->noc;if(w)n.noc.version=PioneerWorkerVersion(w);assert(PioneerQ38DispatchPrepareRole(io,producer.Proof(),producer.Banks(),*cohort,w));}
 uint32_t source[16],command[10],counts[8];producer.RootControls(source,command,counts);
 for(unsigned i=0;i<16;i++)assert(source[i]==cohort->authority.source[0][i]);
 io.SetControls(0,command,counts); // SAME device controls after actual RMS,not fresh synthetic counters
 auto original=io.data;io.expectedOwner=owner.get();io.expectedMemory=memory.get();io.result=&result;io.memoryState=&result.memory;io.track=true;
 io.machine.reset(new ProjectionExecutionMachine(argv[1],argv[2],io));
 unsigned refusals=0;
 auto attempt=[&](const PioneerQ38LearnedRmsController& input,const PioneerQ38CanonicalModelPlan& plan,const PioneerQ38ProjectionIdentity& id,
  unsigned token,unsigned position,unsigned bank,unsigned role){return owner->Prepare(io,producer.Proof(),producer.Banks(),*cohort,*memory,model.graph,plan,input,id,
   producer.Ledger(),PioneerResidentBanks::Pages,token,position,bank,role,result);};
 auto refuse=[&](const PioneerQ38LearnedRmsController& input,const PioneerQ38CanonicalModelPlan& plan,const PioneerQ38ProjectionIdentity& id,
  unsigned token,unsigned position,unsigned bank,unsigned role){unsigned n=io.touches;
  assert(!attempt(input,plan,id,token,position,bank,role) && io.touches==n && !owner->Unsafe() && !memory->Unsafe() && !cohort->unsafe);refusals++;};
 auto pristineRefuse=[&](){refuse(model.rms,model.plan,model.identity,model.token,model.position,model.bank,10);};
 for(unsigned role:{0U,2U,4U,13U})refuse(model.rms,model.plan,model.identity,model.token,model.position,model.bank,role);
 refuse(model.rms,model.plan,model.identity,model.token^1,model.position,model.bank,10);
 refuse(model.rms,model.plan,model.identity,model.token,model.position^1,model.bank,10);
 refuse(model.rms,model.plan,model.identity,model.token,model.position,(model.bank+1)%7,10);
 auto otherId=model.identity;otherId.modelEpoch++;refuse(model.rms,model.plan,otherId,model.token,model.position,model.bank,10);
 auto planCopy=std::make_unique<PioneerQ38CanonicalModelPlan>();assert(planCopy->Build());
 refuse(model.rms,*planCopy,model.identity,model.token,model.position,model.bank,10);
 auto fake=std::make_unique<PioneerQ38LearnedRmsController>();refuse(*fake,model.plan,model.identity,model.token,model.position,model.bank,10);
 result.version=2;pristineRefuse();result.version=1;
 result.memory.phase=1;pristineRefuse();result.memory.phase=0;
 cohort->authority.source[0][11]^=1;pristineRefuse();cohort->authority.source[0][11]^=1;
 const auto& part=*model.plan.ProjectionStripe(0,10,0);unsigned page=part.bank*unsigned(model.plan.Capacity/4096)+part.offset/4096;
 auto* ledger=const_cast<uint8_t*>(producer.Ledger());assert(ledger[page]==1);ledger[page]=0;pristineRefuse();ledger[page]=1;
 printf("PROJECTION_EXECUTION_PRECONDITION_REFUSALS%u no I/O/claims\n",refusals);
#ifdef PIONEER_Q38_PROJECTION_RESULT_STASH
 unsigned produced=0,proofChecks=0;
 auto checkProof=[&](bool closed=false){
  unsigned n=io.touches;
  for(unsigned role:{10U,11U,5U,6U,0U,2U,4U,13U,UINT32_MAX}){
   unsigned slot=PioneerQ38ProjectionResultSlot(role);
   bool expected=!closed && slot<4 && bool(produced&(1U<<slot));
   assert(PioneerQ38ResidentDecoderOwner::Completed(*owner,model.graph,model.plan,model.identity,model.token,model.position,model.bank,role)==expected);
   assert(!PioneerQ38ResidentDecoderOwner::Completed(*owner,model.graph,*planCopy,model.identity,model.token,model.position,model.bank,role));
   assert(!PioneerQ38ResidentDecoderOwner::Completed(*owner,model.graph,model.plan,otherId,model.token,model.position,model.bank,role));
   assert(!PioneerQ38ResidentDecoderOwner::Completed(*owner,model.graph,model.plan,model.identity,model.token^1,model.position,model.bank,role));
   assert(!PioneerQ38ResidentDecoderOwner::Completed(*owner,model.graph,model.plan,model.identity,model.token,model.position^1,model.bank,role));
   assert(!PioneerQ38ResidentDecoderOwner::Completed(*owner,model.graph,model.plan,model.identity,model.token,model.position,(model.bank+1)%7,role));
   proofChecks+=6;
  }
  assert(io.touches==n);
 };
 checkProof();
#endif
 puts("PROJECTION_EXECUTION_PREPARE_ACTUAL_PRIVATE_RMS_PRODUCER");
 assert(owner->Prepare(io,producer.Proof(),producer.Banks(),*cohort,*memory,model.graph,model.plan,model.rms,model.identity,
  producer.Ledger(),PioneerResidentBanks::Pages,model.token,model.position,model.bank,10,result));
 assert(io.boots==119 && result.memory.savedWords==8134072 && result.memory.installedWords==989304 && !result.memory.restoredWords);
#ifdef PIONEER_Q38_PROJECTION_RESULT_STASH
 checkProof();
#endif
 unsigned touched=io.touches;assert(!memory->CloseHeldNoLaunch(io,result.memory) && io.touches==touched); // no held-only promotion
 auto unknown=[&](){
  assert(owner->Unsafe() && !owner->Ready() && !owner->Closed() && memory->Unsafe() && !memory->Closed() && model.graph.Unsafe()
   && producer.Banks().Quarantined() && cohort->unsafe && io.quarantined && io.retainedExecution && !producer.Valid());
  unsigned n=io.touches,reads=io.reads,writes=io.writes;
#ifdef PIONEER_Q38_PROJECTION_RESULT_STASH
  checkProof(true);
#endif
  assert(!owner->Run(io,10,result) && !owner->Close(io,result) && !memory->CloseHeldNoLaunch(io,result.memory)
   && io.touches==n && io.reads==reads && io.writes==writes);
  printf("PASS projection execution fault%d ALL retained,NO subsequent I/O/STOP/END/restore\n",fault);
 };
 if((fault>=0 && fault<=8) || fault==11 || fault==12){
  if(fault==0)io.scope=false;if(fault==1)io.receiptFault=56;if(fault==2)io.receiptFault=6;
  if(fault>=3 && fault<=8)io.postFault=unsigned(fault-2);
  if(fault>=11)io.clockFault=unsigned(fault-10);
  assert(!owner->Run(io,10,result) && !result.rootHolds && !result.memory.restoredWords);unknown();return 0;
 }
 auto input=producer.DeviceWords();std::array<uint32_t,48> first={};
#ifdef PIONEER_Q38_PROJECTION_RESULT_STASH
 (void)first;
 std::vector<uint32_t> preserved(input.begin()+TT_Q38_PROJECTION_RESULT_BASE/4,
  input.begin()+(TT_Q38_PROJECTION_RESULT_BASE+TT_Q38_PROJECTION_RESULT_BYTES)/4);
#endif
 std::vector<unsigned> roles={10,11};if(wide){roles.push_back(5);roles.push_back(6);}
 for(unsigned role:roles){
  printf("PROJECTION_EXECUTION_RUN_ROLE%u\n",role);
  bool okay=owner->Run(io,role,result);
  if(!okay){fprintf(stderr,"PROJECTION_EXECUTION_FAIL phase%u worker%u word%u roots%u;captured CPU state only\n",result.phase,result.worker,result.word,result.rootStarts);io.machine->Diagnostic();}
  assert(okay && io.boots==119 && owner->Ready() && !result.memory.restoredWords);io.machine->VerifyOutput();
#ifdef PIONEER_Q38_PROJECTION_RESULT_STASH
  unsigned at=(PioneerQ38ProjectionResultOffset(role)-TT_Q38_PROJECTION_RESULT_BASE)/4,rows=PioneerQ38ProjectionResultRows(role);
  // New slot alone may change. ALL prior/future slots and every padding
  // word must survive actual emitted packets;never copy refs TO device.
  for(unsigned i=0;i<preserved.size();i++)if(i<at || i>=at+rows)
   assert(producer.DeviceWords()[TT_Q38_PROJECTION_RESULT_BASE/4+i]==preserved[i]);
  for(unsigned i=0;i<rows;i++)preserved[at+i]=producer.DeviceWords()[TT_Q38_PROJECTION_RESULT_BASE/4+at+i];
  produced|=1U<<PioneerQ38ProjectionResultSlot(role);checkProof();
#else
  if(role==10)for(unsigned i=0;i<48;i++)first[i]=producer.DeviceWords()[TT_Q38_PROJECTION_OUTPUT_OFFSET/4+i];
  else if(role==11){bool changed=false;for(unsigned i=0;i<48;i++)changed|=first[i]!=producer.DeviceWords()[TT_Q38_PROJECTION_OUTPUT_OFFSET/4+i];assert(changed);}
#endif
  for(unsigned i=0;i<5120;i++)assert(producer.DeviceWords()[i]==input[i]);
 }
 if(fault==9)io.receiptFault=56;
 if(fault==10)io.restoreFault=1;
 puts("PROJECTION_EXECUTION_STOP_RESTORE");bool closed=owner->Close(io,result);
 if(fault>=9){assert(!closed);unknown();return 0;}
 assert(closed && owner->Closed() && memory->Closed());
#ifdef PIONEER_Q38_PROJECTION_RESULT_STASH
 checkProof(true);
 for(unsigned i=0;i<preserved.size();i++)assert(producer.DeviceWords()[TT_Q38_PROJECTION_RESULT_BASE/4+i]==preserved[i]);
 printf("PASS QPS1 fixed distinct result stashes/ALL%u private identity-bound proof checks/no fabricated ready proof/18432words including untouched padding preserved after ALL later roles and complete restore;NOT_NATIVE_OR_SPEED\n",proofChecks);
#endif
 assert(result.stopped==119 && result.held==119 && result.memory.restoredWords==8134072 && result.memory.verifiedOriginalWords==16268144
  && io.boots==119 && io.starts==roles.size()+1 && io.machine->Matrices()==(wide?244U:6U) && io.machine->Producers()==(wide?80U:40U)
  && io.machine->Stops()==119 && io.data==original);
 touched=io.touches;assert(!owner->Run(io,10,result) && !owner->Close(io,result) && io.touches==touched);
 printf("PASS actual zero/QPE1/QRN1 private producer->complete projection OWNER/QPR1/ALL119 QFP3,projections%zu/matrices%u/producers%u,SAME119boots,NO CPU vectors/reference/fallback,8134072-word snapshots restored ONLY after STOP/ALL physical terminals;SIMULATED HARDWARE/WEIGHTS_NOT_NATIVE_OR_MODEL_SPEED\n",roles.size(),io.machine->Matrices(),io.machine->Producers());
}
