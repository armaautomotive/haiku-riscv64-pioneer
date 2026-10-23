/* SPDX-License-Identifier: MIT */
// ACTUAL source/owners/zero/QPE1/QRN1/QPS1/ALL119 then fixed DEBUG
// readback BEFORE restore. Simulated endpoints,never a physical-card claim.
#define PIONEER_Q38_PROJECTION_NATIVE_FACTORY_ONLY
#include "pioneer_q38_projection_execution_native_factory_test.cpp"
int main(int argc,char** argv){
 assert(argc==6 || argc==7);setvbuf(stdout,nullptr,_IOLBF,0);int fault=argc==7?atoi(argv[6]):-1;
 (void)&Input;(void)&Prepare;(void)&Upload;(void)&Close;(void)&PreparePanel;(void)&Run;(void)&Job;(void)&ClosePanel;(void)&Finish;
 ProjectionNativeFixture f(argv[3],argv[4],argv[5],248319,6);f.Produce();f.Attach(argv[1],argv[2]);
 printf("QPS1_DEBUG_PRECONDITION_REFUSALS%u/%u\n",f.ResultRefusals(),f.ScopeRefusals());
 auto values=std::make_unique<std::array<uint32,18432>>();
 assert(PrepareQ38ProjectionExecution(*f.c,f.projection));
 for(unsigned role:{10U,11U,5U,6U}){
  printf("QPS1_DEBUG_ACTUAL_NATIVE_RUN_ROLE%u\n",role);assert(RunQ38ProjectionExecution(*f.c,role,f.projection));f.transport->VerifyOutput();
  if(role==10){unsigned n=f.io.touches,d=f.transport->Touches();
   auto& m=*f.c->q38EmbeddingModel;
   assert(!m.projection.Close(f.io,f.projection,values->data()) && f.io.touches==n && f.transport->Touches()==d); // incomplete private produced mask
  }
 }
 auto& m=*f.c->q38EmbeddingModel;
 unsigned refusals=1;
 for(unsigned kind=0;kind<5;kind++){
  uint32* output=values->data();if(kind==0)output=nullptr;if(kind==1)output=reinterpret_cast<uint32*>(uintptr_t(1));
  if(kind==2)output=reinterpret_cast<uint32*>(UINTPTR_MAX-3);if(kind==3)f.projection.validationWords=1;if(kind==4)f.projection.validationPages=1;
  unsigned n=f.io.touches,d=f.transport->Touches();
  assert(!m.projection.Close(f.io,f.projection,output) && !m.Unsafe() && f.io.touches==n && f.transport->Touches()==d);
  f.projection.validationWords=f.projection.validationPages=0;refusals++;
 }
 if(fault>=0){assert(fault<8);f.transport->ValidationFault(unsigned(fault/2+1),fault&1?18:1);}
 puts("QPS1_DEBUG_STOP_ALL_TERMINALS_HOLDS_FIXED18_READS_BEFORE_RESTORE");
 bool okay=CloseQ38ProjectionExecution(*f.c,f.projection,values->data());
 if(fault>=0){
  assert(!okay && f.projection.unsafe && !f.projection.closed && !f.projection.memory.restoredWords
   && f.transport->ValidationReads()==unsigned(fault&1?18:1));f.Unknown();
  printf("PASS QPS1 DEBUG endpoint fault%d,ALL retained/no following I/O/END/restore/retry\n",fault);return 0;
 }
 assert(okay && m.Closed() && f.projection.validationPages==18 && f.projection.validationWords==18432 && f.transport->ValidationReads()==18
  && f.projection.memory.savedWords==8134072 && f.projection.memory.restoredWords==8134072 && f.projection.memory.verifiedOriginalWords==16268144
  && f.projection.stopped==119 && f.projection.held==119 && f.transport->Boots()==119 && f.transport->Matrices()==244 && f.transport->Producers()==80);
 for(unsigned i=0;i<values->size();i++)assert((*values)[i]==f.io.wholeWorkspace[TT_Q38_PROJECTION_RESULT_BASE/4+i]);
 assert(!f.io.pciWrites && !f.io.HostReadMock::bmeWrites && EndFactory(*f.c) && !f.c->Unsafe());
 printf("PASS QPS1 actual native fixed DEBUG readback18fresh poisoned pages/18432words/4projections/244matrices/80producers/119boots once/refusals%u/full restore/actual END;SIMULATED_NOT_PHYSICAL_CARD_OR_SPEED\n",refusals);
}
