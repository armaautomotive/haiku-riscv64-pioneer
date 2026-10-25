/* SPDX-License-Identifier: MIT */
// Genuine private input/initialization controllers and actual installed
// complete coordinator+ALL119 peers. TEST endpoints are NOT native scope.
#define PIONEER_Q38_PROJECTION_EXECUTION_FIXTURE_ONLY
#include "pioneer_q38_projection_execution_test.cpp"
#include "pioneer_q38_resident_output_execution_machine.h"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38ResidentOutputOwner.h"
#include <sys/wait.h>
#include <unistd.h>
struct ResidentExecutionIO:ExecutionIO,ResidentOutputExecutionFabric {
 bool outputMode=false,outputScope=true,retainedOutput=false;
 PioneerQ38ResidentOutputOwner* outputOwner=nullptr;PioneerQ38ResidentOutputMemory* outputMemory=nullptr;
 PioneerQ38ResidentOutputResult* outputResult=nullptr;std::unique_ptr<ResidentOutputExecutionMachine> outputMachine;
 std::vector<uint32_t> extra=std::vector<uint32_t>(TT_Q38_RESIDENT_OUTPUT_SAVED_WORDS);
 bool startedNew[120]={};unsigned newBoots=0,newStarts=0,newHolds=0;
 unsigned fault=0;bool injected=false,droppedReset=false;unsigned fence=0;
 void Inject(bool terminal=true){injected=true;if(terminal)fence=touches;}
 bool At(unsigned phase,unsigned w)const{return outputMode && outputResult && outputResult->phase==phase && current==w;}
 uint64_t Q38CohortNow(){
  if(outputMode && newStarts && (fault==14 || fault==15)){Inject();return fault==14?now+30000001:0;}
  return ExecutionIO::Q38CohortNow();
 }
 uint32_t ResetRead(){auto v=ProjectionMemoryIO::ResetRead();if(droppedReset)Inject();return v;}
 uint32_t ResidentNocRead(uint32_t a){auto v=ProjectionMemoryIO::ResidentNocRead(a);
  if(fault==6 && At(6,119) && a==BlackholeDramSourceOffset(11)){Inject();return v^1;}
  if(fault==18 && At(10,119) && outputResult->memory.restoredWords==TT_Q38_RESIDENT_OUTPUT_SAVED_WORDS && a==36){Inject();return v^1;}
  return v;
 }
 ResidentExecutionIO(){for(unsigned i=0;i<extra.size();i++)extra[i]=0x3c5a7091U^(i*0x9e3779b9U);}
 static unsigned ExtraIndex(unsigned w,uint32_t a){
  unsigned offset=0;for(unsigned span=0;span<PioneerQ38ResidentOutputSpanCount(w);span++){
   auto s=PioneerQ38ResidentOutputSnapshotSpan(w,span);if(a>=s.address && a-s.address<s.words*4)return PioneerQ38ResidentOutputSnapshotBase(w)+offset+(a-s.address)/4;offset+=s.words;
  }assert(false);return 0;
 }
 uint32_t FullNewRead(unsigned w,uint32_t a){
  assert(w==current);
  if(PioneerQ38ProjectionGraphSnapshotContains(w,a))return ExecutionIO::FullRead(w,a);
  assert(PioneerQ38ResidentOutputSnapshotContains(w,a));return extra.at(ExtraIndex(w,a));
 }
 void FullNewWrite(unsigned w,uint32_t a,uint32_t value){
  assert(w==current);
  if(PioneerQ38ProjectionGraphSnapshotContains(w,a))ExecutionIO::FullWrite(w,a,value);
  else {assert(PioneerQ38ResidentOutputSnapshotContains(w,a));extra.at(ExtraIndex(w,a))=value;}
 }
 uint32_t ReadLocal(unsigned w,uint32_t a)override{unsigned old=current;current=w;auto v=FullNewRead(w,a);current=old;return v;}
 void WriteLocal(unsigned w,uint32_t a,uint32_t v)override{
  // DEVICE stores use raw aliases. Routing them through a held-only HOST
  // panel writer would incorrectly require the executing core to be held.
  assert(outputMode?PioneerQ38ResidentOutputSnapshotContains(w,a):PioneerQ38ProjectionGraphSnapshotContains(w,a));
  if(a==0x6000)core[w]->entry=v;
  else if(a>=0x70000 && a<0x74000)core[w]->view[(a-TT_Q38_PANEL_VIEW_BASE)/4]=v;
  else if(PioneerQ38ProjectionGraphSnapshotContains(w,a))data.at(Index(w,a))=v;
  else extra.at(ExtraIndex(w,a))=v;
 }
 uint32_t NoC(unsigned w,unsigned a)override{return ExecutionIO::NoC(w,a);}
 void SetControls(unsigned w,const uint32_t* c,const uint32_t* n)override{ExecutionIO::SetControls(w,c,n);}
 uint32_t Reset(unsigned w)override{return ExecutionIO::Reset(w);}
 std::vector<uint32_t>& Gddr()override{return producer->DeviceWords();}
 bool ReadState(unsigned b,uint32_t a,uint32_t* out,unsigned n)override{return producer->ReadResidentState(b,a,out,n);}
 bool WriteState(unsigned b,uint32_t a,const uint32_t* in,unsigned n)override{return producer->WriteResidentState(b,a,in,n);}
 bool ResidentOutputMemoryScope(const PioneerQ38ResidentOutputMemory& code,const PioneerQ38CanonicalModelPlan& plan,
  const PioneerQ38GdnIdentity& id,const uint8_t* ledger,unsigned pages){
  if(!outputScope){Inject();return false;}
  return outputMode && outputScope && producer && producer->Valid() && outputOwner && outputMemory==&code
   && &plan==&producer->Model().plan && id==producer->Model().identity && ledger==producer->Ledger() && pages==PioneerResidentBanks::Pages
   && outputOwner->MatchesNativeOwner(*expectedOwner,code);
 }
 bool ResidentOutputExecutionScope(const PioneerQ38ResidentOutputOwner& owner,const PioneerQ38ProjectionGraphOwner& input,
  const PioneerQ38ResidentOutputMemory& code,const PioneerQ38ProjectionMutableBanks& graph,const PioneerQ38CanonicalModelPlan& plan,
  const PioneerQ38GdnIdentity& id,const uint8_t* ledger,unsigned pages){
  return &owner==outputOwner && &input==expectedOwner && &graph==&producer->Model().graph && ResidentOutputMemoryScope(code,plan,id,ledger,pages);
 }
 void ResidentOutputRetainHostOwners(){retainedOutput=true;ProjectionGraphRetainHostOwners();}
 bool ResidentOutputMemoryMappingStable(unsigned w){Touch();
  if((fault==7 && At(6,119)) || (fault==19 && At(11,0))){Inject();return false;}
  return outputMode && w==current && C().mapping;}
 bool ResidentOutputMemoryReadChecked(unsigned w,uint32_t a,uint32_t& out){
  Touch();reads++;assert(w==current && PioneerQ38ResidentOutputSnapshotContains(w,a));
  if((fault==2 && outputResult->memory.phase==2 && w==119 && a==PioneerQ38ResidentOutputSnapshotAddress(w,PioneerQ38ResidentOutputSnapshotWords(w)-1))
   || (fault==12 && At(11,119) && a==PioneerQ38ResidentOutputSnapshotAddress(w,PioneerQ38ResidentOutputSnapshotWords(w)-1))){Inject();return false;}
  if(startedNew[w] && w && a==0x74140 && C().coreReset==0x46800)outputMachine->Advance(w);
  out=FullNewRead(w,a);
  if((fault==4 && At(5,0) && a==TT_Q38_COHORT_STATS+63*4) || (fault==5 && At(6,119) && a==0x83f00+63*4)
   || (fault==8 && At(8,119) && a==0x74150 && outputMachine->Stops())){Inject(false);out^=1;}
  return true;
 }
 bool ResidentOutputMemoryWrite(unsigned w,uint32_t a,uint32_t v){Touch();writes++;assert(w==current && PioneerQ38ResidentOutputSnapshotContains(w,a));
  if((fault==3 && outputResult->memory.phase==3 && w==119 && a==PioneerQ38ResidentOutputCodeAddress(w,PioneerQ38ResidentOutputCodeWords(w)-1))
   || (fault==9 && At(8,119) && a==0x74100)
   || (fault==11 && At(10,119) && a==PioneerQ38ResidentOutputSnapshotAddress(w,PioneerQ38ResidentOutputSnapshotWords(w)-1))){Inject();return false;}
  FullNewWrite(w,a,v);return true;}
 uint32_t Q38PanelRead(uint32_t a){if(!outputMode)return ExecutionIO::Q38PanelRead(a);Touch();return FullNewRead(current,a);}
 void Q38PanelWrite(uint32_t a,uint32_t v){if(!outputMode){ExecutionIO::Q38PanelWrite(a,v);return;}Touch();FullNewWrite(current,a,v);}
 uint32_t RuntimeStatusRead(unsigned i){
  if(!outputMode)return ExecutionIO::RuntimeStatusRead(i);Touch();
  if(!current && startedNew[0] && C().coreReset==0x46800)outputMachine->Advance(0);return FullNewRead(current,0x17ffc0+i*4);
 }
 void RuntimeStatusWrite(unsigned i,uint32_t v){if(!outputMode){ExecutionIO::RuntimeStatusWrite(i,v);return;}Touch();FullNewWrite(current,0x17ffc0+i*4,v);}
 uint32_t RuntimePcRead(){
  assert(C().coreReset==0x46800 && "NEVER sample a held/reset program counter");
  if(!outputMode)return ExecutionIO::RuntimePcRead();Touch();outputMachine->Advance(current);
  if(fault==10 && At(9,119)){Inject();return TT_Q38_PEER_FAULT_PARK;}
  return outputMachine->Pc(current);
 }
 void RuntimeResetWrite(uint32_t v){
  if(!outputMode){ExecutionIO::RuntimeResetWrite(v);return;}Touch();
  if(v==0x87ffff && ((fault==16 && At(6,0)) || (fault==17 && At(9,119)))){injected=droppedReset=true;return;}
  C().coreReset=v;
  if(v==0x46800){assert(!startedNew[current]);startedNew[current]=true;outputMachine->Start(current);if(current)newBoots++;else newStarts++;}
  if(v==0x47800)newHolds++;
 }
};
static void ExecuteOutput(ResidentExecutionIO& io,ProjectionExecutionProducer& producer,PioneerQ38ProjectionGraphOwner& projection,
 const char* root,const char* peer,unsigned fault){
 const auto* input=projection.ResidentInput();assert(input && input->Current());
 std::vector<uint32_t> originals;originals.reserve(TT_Q38_RESIDENT_OUTPUT_SAVED_WORDS);
 for(unsigned w=0;w<120;w++)for(unsigned i=0;i<PioneerQ38ResidentOutputSnapshotWords(w);i++)originals.push_back(io.ReadLocal(w,PioneerQ38ResidentOutputSnapshotAddress(w,i)));
 auto controller=std::make_unique<PioneerQ38ResidentOutputOwner>();auto memory=std::make_unique<PioneerQ38ResidentOutputMemory>();PioneerQ38ResidentOutputResult result;
 io.outputOwner=controller.get();io.outputMemory=memory.get();io.outputResult=&result;io.outputMode=true;io.fault=fault;if(fault==1)io.outputScope=false;
 io.outputMachine.reset(new ResidentOutputExecutionMachine(root,peer,static_cast<ResidentOutputExecutionFabric&>(io)));
 auto unknown=[&](){
  assert(fault && io.injected && controller->Unsafe() && !controller->Closed() && !controller->Completed() && memory->Unsafe() && !memory->Closed()
   && producer.Banks().Quarantined() && io.retainedOutput && io.retainedExecution && !producer.Valid() && !input->Current());
  if(io.fence)assert(io.touches==io.fence);
  unsigned n=io.touches,reads=io.reads,writes=io.writes;
  assert(!controller->Prepare(io,projection,*memory,result) && !controller->Run(io,result) && !controller->Close(io,result)
   && !memory->CloseHeldNoLaunch(io,result.memory) && io.touches==n && io.reads==reads && io.writes==writes);
  printf("PASS actual new output controller fault%u phase%u/worker%u;ALL owners and43.8MB journal retained,ZERO following IO/reset/STOP/restore/retry;TEST endpoints NOT native\n",fault,result.phase,result.worker);
 };
 bool prepared=controller->Prepare(io,projection,*memory,result);
 if(fault>=1 && fault<=3){assert(!prepared);if(fault==2)assert(!result.memory.installedWords);unknown();return;}
 if(!prepared)fprintf(stderr,"OUTPUT_PREPARE_FAIL phase%u worker%u word%u memphase%u memworker%u memword%u boots%u\n",result.phase,result.worker,result.word,result.memory.phase,result.memory.worker,result.memory.word,io.newBoots);
 assert(prepared && !input->Current() && !projection.ResidentInput() && io.newBoots==119);
 puts("OUTPUT_NEW_CONTROLLER_ALL119_ACTUAL_BOOT_AND_WAIT;BEGIN_FULL_RUN");
 if(fault==13)io.outputScope=false;
 if(fault==20){const auto& model=producer.Model();const auto* p=model.plan.ProjectionStripe(0,13,0);assert(p);
  const unsigned page=p->bank*unsigned(model.plan.Capacity/4096)+p->offset/4096;auto* ledger=const_cast<uint8_t*>(producer.Ledger());assert(ledger[page]==1);
  ledger[page]=0;io.Inject();
 }
 bool ran=controller->Run(io,result);
 if((fault>=4 && fault<=7) || (fault>=13 && fault<=16) || fault==20){assert(!ran && !result.rootHolds && !result.memory.restoredWords);unknown();return;}
 if(!ran){fprintf(stderr,"OUTPUT_RUN_FAIL phase%u worker%u word%u completed%u rootHolds%u\n",result.phase,result.worker,result.word,result.completed,result.rootHolds);io.outputMachine->Diagnostic();}
 assert(ran && controller->Completed() && !controller->Unsafe() && result.completed==119 && result.rootStarts==1 && result.rootHolds==1);
 io.outputMachine->VerifyOutput();puts("OUTPUT_NEW_CONTROLLER_FULL_ACTUAL_ARITHMETIC_COMPLETE;BEGIN_STOP_AND_RESTORE");
 bool closed=controller->Close(io,result);
 if(fault){assert(!closed);if(fault==8 || fault==9)assert(!result.held);if(fault==17)assert(result.held==118);unknown();return;}
 assert(closed && controller->Closed() && memory->Closed() && !controller->Unsafe() && !memory->Unsafe()
  && io.newBoots==119 && io.newStarts==1 && io.outputMachine->Boots()==119 && io.outputMachine->Stops()==119
  && result.stopped==119 && result.held==119 && result.memory.restoredWords==TT_Q38_RESIDENT_OUTPUT_SAVED_WORDS
  && result.memory.verifiedOriginalWords==2*TT_Q38_RESIDENT_OUTPUT_SAVED_WORDS);
 unsigned at=0;for(unsigned w=0;w<120;w++)for(unsigned i=0;i<PioneerQ38ResidentOutputSnapshotWords(w);i++)assert(io.ReadLocal(w,PioneerQ38ResidentOutputSnapshotAddress(w,i))==originals[at++]);
 unsigned n=io.touches;assert(!controller->Run(io,result) && !controller->Close(io,result) && io.touches==n && !input->Current());
 puts("PASS GENUINE full state+history zero/QPE1/QRN1/4QPS1->PRIVATE claimed new controller->ACTUAL same119 coordinator Conv/GDN/QOL1/ALL5120 outputs->ALL119 actual STOP/unheld terminals->ALL120 holds/full10951352-word original restore+second proof;NO held PC reads/CPU products/injected success. Synthetic learned byte/clock endpoints;NOT native factory/card/full64layer model or token-rate measurement");
}
int main(int argc,char** argv){
 assert(argc==9);setvbuf(stdout,nullptr,_IOLBF,0);
 (void)&Input;(void)&Prepare;(void)&Upload;(void)&Close;(void)&PreparePanel;(void)&Run;(void)&Job;(void)&ClosePanel;(void)&Finish;
 ProjectionExecutionProducer producer(argv[3],argv[4],argv[5]);auto& model=producer.Model();assert(producer.Valid() && !model.position);
 ResidentExecutionIO io;io.producer=&producer;
 auto cohort=std::make_unique<PioneerQ38DispatchLease>();auto oldMemory=std::make_unique<PioneerQ38ProjectionGraphMemory>();
 auto projection=std::make_unique<PioneerQ38ProjectionGraphOwner>();PioneerQ38ProjectionGraphResult old;
 std::vector<std::unique_ptr<PanelOwners>> participants;
 for(unsigned w=0;w<120;w++){
  participants.emplace_back(new PanelOwners(*io.core[w]));auto& n=cohort->worker[w];n.master=&participants[w]->store;
  n.noc=participants[w]->noc;if(w)n.noc.version=PioneerWorkerVersion(w);assert(PioneerQ38DispatchPrepareRole(io,producer.Proof(),producer.Banks(),*cohort,w));
 }
 uint32_t source[16],command[10],counts[8];producer.RootControls(source,command,counts);io.SetControls(0,command,counts);
 io.expectedOwner=projection.get();io.expectedMemory=oldMemory.get();io.result=&old;io.memoryState=&old.memory;io.track=true;
 io.machine.reset(new ProjectionExecutionMachine(argv[1],argv[2],static_cast<ExecutionIO&>(io)));
 assert(projection->Prepare(io,producer.Proof(),producer.Banks(),*cohort,*oldMemory,model.graph,model.plan,model.rms,model.identity,
  producer.Ledger(),PioneerResidentBanks::Pages,model.token,model.position,model.bank,10,old));
 for(unsigned role:{10U,11U,5U,6U}){assert(projection->Run(io,role,old));io.machine->VerifyOutput();}
 assert(projection->Close(io,old) && projection->ResidentInput() && projection->ResidentInput()->Current());
 puts("OUTPUT_GENUINE_INITIALIZED_PRIVATE_GPU_INPUT_COMPLETE;BEGIN_NEW_CONTROLLER");
 if(!strcmp(argv[8],"--all")){
  for(unsigned fault=0;fault<=20;fault++){
   pid_t pid=fork();assert(pid>=0);if(!pid){ExecuteOutput(io,producer,*projection,argv[6],argv[7],fault);_exit(0);}
   int status=0;assert(waitpid(pid,&status,0)==pid && WIFEXITED(status) && WEXITSTATUS(status)==0);
  }
  puts("PASS complete new controller ACTUAL successful pipeline/STOP/43.8MB restore plus20 failure-containment cases;NOT native or rate qualification");
 }else {unsigned fault=unsigned(atoi(argv[8]));assert(fault<=20);ExecuteOutput(io,producer,*projection,argv[6],argv[7],fault);}
}
