/* SPDX-License-Identifier: MIT */
// ACTUAL full zero controllers + zero/QPE1/QRN1/four QPS1 programs. Only
// endpoint bytes/clock/learned weights simulated. NO constructed ready seal.
#define PIONEER_Q38_PROJECTION_EXECUTION_FIXTURE_ONLY
#include "pioneer_q38_projection_execution_test.cpp"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38ResidentOutputPlan.h"
int main(int argc,char** argv){
 assert(argc==6);setvbuf(stdout,nullptr,_IOLBF,0);
 (void)&Input;(void)&Prepare;(void)&Upload;(void)&Close;(void)&PreparePanel;(void)&Run;(void)&Job;(void)&ClosePanel;(void)&Finish;
 ProjectionExecutionProducer producer(argv[3],argv[4],argv[5]);auto& model=producer.Model();assert(producer.Valid() && !model.position);
 auto* g=model.graph.Recurrent(model.identity);auto* h=model.graph.ConvHistory(model.identity);
 assert(g && h && g->InitializationPublished(model.identity) && h->InitializationPublished(model.identity));
 ExecutionIO io;io.producer=&producer;
 auto cohort=std::make_unique<PioneerQ38DispatchLease>();auto memory=std::make_unique<PioneerQ38ProjectionGraphMemory>();
 auto projection=std::make_unique<PioneerQ38ProjectionGraphOwner>();PioneerQ38ProjectionGraphResult result;
 std::vector<std::unique_ptr<PanelOwners>> participants;
 for(unsigned w=0;w<120;w++){
  participants.emplace_back(new PanelOwners(*io.core[w]));auto& n=cohort->worker[w];n.master=&participants[w]->store;
  n.noc=participants[w]->noc;if(w)n.noc.version=PioneerWorkerVersion(w);assert(PioneerQ38DispatchPrepareRole(io,producer.Proof(),producer.Banks(),*cohort,w));
 }
 uint32_t source[16],command[10],counts[8];producer.RootControls(source,command,counts);io.SetControls(0,command,counts);
 io.expectedOwner=projection.get();io.expectedMemory=memory.get();io.result=&result;io.memoryState=&result.memory;io.track=true;
 io.machine.reset(new ProjectionExecutionMachine(argv[1],argv[2],io));
 assert(projection->Prepare(io,producer.Proof(),producer.Banks(),*cohort,*memory,model.graph,model.plan,model.rms,model.identity,
  producer.Ledger(),PioneerResidentBanks::Pages,model.token,model.position,model.bank,10,result));
 for(unsigned role:{10U,11U,5U,6U}){assert(projection->Run(io,role,result));io.machine->VerifyOutput();}
 assert(projection->Close(io,result) && projection->ResidentInput() && projection->ResidentInput()->Current()
  && g->InitializationPublished(model.identity) && h->InitializationPublished(model.identity));
 auto plan=std::make_unique<PioneerQ38ResidentOutputPlan>();uint32_t seq[119]={};
 assert(plan->Build(model.plan,0,0,model.bank,0,model.token,17,model.identity,&cohort->authority.source[0][0],seq)
  && plan->LearnedPagesPresent(producer.Ledger(),PioneerResidentBanks::Pages));
 for(unsigned w=1;w<=96;w++){
  const auto* d=PioneerQ38CohortGdn(plan->Manifest(),w)+64;uint32_t data[8192];
  for(unsigned side=0;side<2;side++){
   assert(producer.ReadResidentState(d[4+side*2],d[5+side*2],data,8192));for(unsigned value:data)assert(!value);
  }
 }
 puts("PASS ACTUAL full73728GDN+1680history page zero/CLEAN_CLOSE->workspace zero/QPE1/QRN1/ALL4QPS1+119peers->STOP/full old restore/private closed GPU input;ALL required learned-page recipe present and BOTH layer0 state slots retain ACTUAL zero bytes. TEST source/clock/learned bytes ONLY,not new native factory/controller Run/silicon speed");
}
