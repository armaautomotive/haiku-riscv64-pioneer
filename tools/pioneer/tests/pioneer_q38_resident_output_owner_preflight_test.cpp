/* SPDX-License-Identifier: MIT */
// Actual source/zero/QPE1/QRN1/four QPS1 roles, NOT a manufactured input.
#define PIONEER_Q38_PROJECTION_EXECUTION_FIXTURE_ONLY
#include "pioneer_q38_projection_execution_test.cpp"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38ResidentOutputOwner.h"
struct OutputPreflightIO:ExecutionIO {
 uint32_t RuntimePcRead(){assert(C().coreReset==0x46800);return ExecutionIO::RuntimePcRead();}
 void ResidentOutputRetainHostOwners(){assert(false && "inert refusal must not retain a claim");}
 bool ResidentOutputExecutionScope(const PioneerQ38ResidentOutputOwner&,const PioneerQ38ProjectionGraphOwner&,const PioneerQ38ResidentOutputMemory&,
  const PioneerQ38ProjectionMutableBanks&,const PioneerQ38CanonicalModelPlan&,const PioneerQ38GdnIdentity&,const uint8_t*,unsigned){assert(false);return false;}
 bool ResidentOutputMemoryScope(const PioneerQ38ResidentOutputMemory&,const PioneerQ38CanonicalModelPlan&,const PioneerQ38GdnIdentity&,const uint8_t*,unsigned){assert(false);return false;}
 bool ResidentOutputMemoryMappingStable(unsigned){assert(false);return false;}
 bool ResidentOutputMemoryReadChecked(unsigned,uint32_t,uint32_t&){assert(false);return false;}
 bool ResidentOutputMemoryWrite(unsigned,uint32_t,uint32_t){assert(false);return false;}
};
int main(int argc,char** argv){
 assert(argc==6);setvbuf(stdout,nullptr,_IOLBF,0);
 (void)&Input;(void)&Prepare;(void)&Upload;(void)&Close;(void)&PreparePanel;(void)&Run;(void)&Job;(void)&ClosePanel;(void)&Finish;
 ProjectionExecutionProducer producer(argv[3],argv[4],argv[5]);auto& model=producer.Model();assert(producer.Valid() && !model.position);
 OutputPreflightIO io;io.producer=&producer;
 auto cohort=std::make_unique<PioneerQ38DispatchLease>();auto oldMemory=std::make_unique<PioneerQ38ProjectionGraphMemory>();
 auto projection=std::make_unique<PioneerQ38ProjectionGraphOwner>();PioneerQ38ProjectionGraphResult old;
 std::vector<std::unique_ptr<PanelOwners>> participants;
 for(unsigned w=0;w<120;w++){
  participants.emplace_back(new PanelOwners(*io.core[w]));auto& n=cohort->worker[w];n.master=&participants[w]->store;
  n.noc=participants[w]->noc;if(w)n.noc.version=PioneerWorkerVersion(w);assert(PioneerQ38DispatchPrepareRole(io,producer.Proof(),producer.Banks(),*cohort,w));
 }
 uint32_t source[16],command[10],counts[8];producer.RootControls(source,command,counts);io.SetControls(0,command,counts);
 io.expectedOwner=projection.get();io.expectedMemory=oldMemory.get();io.result=&old;io.memoryState=&old.memory;io.track=true;
 io.machine.reset(new ProjectionExecutionMachine(argv[1],argv[2],io));
 assert(projection->Prepare(io,producer.Proof(),producer.Banks(),*cohort,*oldMemory,model.graph,model.plan,model.rms,model.identity,
  producer.Ledger(),PioneerResidentBanks::Pages,model.token,model.position,model.bank,10,old));
 for(unsigned role:{10U,11U,5U,6U}){assert(projection->Run(io,role,old));io.machine->VerifyOutput();}
 assert(projection->Close(io,old));const auto* input=projection->ResidentInput();assert(input && input->Current());
 auto* history=model.graph.ConvHistory(model.identity);auto* recurrent=model.graph.Recurrent(model.identity);
 assert(history && recurrent && !history->InitializationPublished(model.identity) && !recurrent->InitializationPublished(model.identity));
 auto controller=std::make_unique<PioneerQ38ResidentOutputOwner>();auto memory=std::make_unique<PioneerQ38ResidentOutputMemory>();
 PioneerQ38ResidentOutputResult result;const unsigned touches=io.touches,reads=io.reads,writes=io.writes;
 auto refuse=[&](){assert(!controller->Prepare(io,*projection,*memory,result) && !controller->Run(io,result) && !controller->Close(io,result)
  && io.touches==touches && io.reads==reads && io.writes==writes && input->Current() && projection->ResidentInput()==input
  && !controller->Unsafe() && !controller->Closed() && !controller->Completed() && !memory->Installed() && !memory->Unsafe()
  && !producer.Banks().Quarantined() && !cohort->unsafe);};
 refuse();unsigned refused=1;
 for(unsigned kind=0;kind<15;kind++){
  result=PioneerQ38ResidentOutputResult();
  if(kind==0)result.version=2;if(kind==1)result.started=1;if(kind==2)result.prepared=1;if(kind==3)result.completed=119;
  if(kind==4)result.closed=1;if(kind==5)result.unsafe=1;if(kind==6)result.armed=119;if(kind==7)result.released=119;
  if(kind==8)result.waiting=119;if(kind==9)result.stopped=119;if(kind==10)result.held=119;if(kind==11)result.rootStarts=1;
  if(kind==12)result.rootHolds=1;if(kind==13)result.memory.installed=1;if(kind==14)result.memory.verifiedOriginalWords=TT_Q38_RESIDENT_OUTPUT_SAVED_WORDS;
  refuse();refused++;
 }
 printf("PASS actual four-QPS1/119-peer private closed input CANNOT replace genuine history+GDN initialization;complete new controller inertRefusals%u/ZERO IO/claim/consume/quarantine,forged public result grants NOTHING. Actual new Run/native factory/initialization integration still UNQUALIFIED;NOT throughput\n",refused);
}
