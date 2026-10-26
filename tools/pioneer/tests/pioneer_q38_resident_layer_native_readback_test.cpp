/* SPDX-License-Identifier: MIT */
// Actual SAME new native context+complete emitted layer+guarded QLV1
// observer. Simulated OS/byte endpoints ONLY,not original learned weights,
// a physical card, complete model, Work-mode inference or a speed claim.
#define PIONEER_Q38_LAYER_EXECUTION_FIXTURE_ONLY
#include "pioneer_q38_resident_layer_native_execution_test.cpp"
static void FinishLayerReadback(NativeBindingFixture& f,ProjectionNativeTransport& transport,PioneerQ38ResidentLayerResult& output,
 unsigned fault,unsigned page,unsigned word,std::vector<uint32_t>* observed=nullptr){
 auto& c=*f.c;auto* state=c.q38ResidentLayerNative;
 std::vector<uint32_t> values(PioneerQ38ResidentLayerReadbackPlan::Words,0xdeadbeef);PioneerQ38ResidentLayerReadbackResult readback;
 unsigned touches=f.io.touches,device=transport.Touches();
 for(unsigned bad:{0U,1629120U,PioneerQ38ResidentLayerReadbackPlan::Words-1U,PioneerQ38ResidentLayerReadbackPlan::Words+1U,UINT32_MAX})
  assert(!CloseQ38ResidentLayerNativeForValidation(c,output,values.data(),bad,readback));
 assert(!CloseQ38ResidentLayerNativeForValidation(c,output,nullptr,unsigned(values.size()),readback)
  && !CloseQ38ResidentLayerNativeForValidation(c,output,reinterpret_cast<uint32_t*>(reinterpret_cast<uintptr_t>(values.data())+1),unsigned(values.size()),readback)
  && !CloseQ38ResidentLayerNativeForValidation(c,output,reinterpret_cast<uint32_t*>(UINTPTR_MAX-3),unsigned(values.size()),readback));
 uint32_t fields[5];memcpy(fields,&readback,sizeof(readback));
 for(unsigned i=0;i<5;i++){fields[i]^=1;memcpy(&readback,fields,sizeof(readback));
  assert(!CloseQ38ResidentLayerNativeForValidation(c,output,values.data(),unsigned(values.size()),readback));fields[i]^=1;
 }memcpy(&readback,fields,sizeof(readback));
 assert(!state->Unsafe() && !c.dispatchOwnerRetained && transport.Touches()==device && f.io.touches==touches
  && !f.io.ResidentOutputSourceScope(true));
 transport.ObserveResidentLayerReadback(fault,page,word);
 bool closed=CloseQ38ResidentLayerNativeForValidation(c,output,values.data(),unsigned(values.size()),readback);
 if(fault){
  const bool contained=!closed && state->Unsafe() && c.dispatchOwnerRetained && sColdRecoveryRequired && c.banks.Quarantined()
   && f.transfer.quarantined && f.wide.quarantined && c.q38Clock.quarantined && output.unsafe && !output.closed
   && output.stopped==119 && output.held==119 && !output.memory.restoredWords && !readback.completed
   && transport.ResidentLayerReadbackFaultFence() && transport.Touches()==transport.ResidentLayerReadbackFaultFence();
  if(!contained)fprintf(stderr,"LAYER_READBACK_CONTAINMENT_FAIL fault%u/page%u/word%u closed%u unsafe%u retained%u cold%u stopped%u held%u restored%u readback%u/%u/%u transfers%u fence%u touches%u\n",
   fault,page,word,unsigned(closed),unsigned(state->Unsafe()),unsigned(c.dispatchOwnerRetained),unsigned(sColdRecoveryRequired),
   output.stopped,output.held,output.memory.restoredWords,readback.pages,readback.words,readback.completed,
   transport.ResidentLayerReadbackRequests(),transport.ResidentLayerReadbackFaultFence(),transport.Touches());
  assert(contained && transport.ResidentLayerReadbackRequests()==page+1);
  if(fault>=4)assert(readback.pages==page && readback.words==page*1024+word);
  touches=f.io.touches;device=transport.Touches();
  assert(!CloseQ38ResidentLayerNativeForValidation(c,output,values.data(),unsigned(values.size()),readback)
   && !CloseQ38ResidentLayerNative(c,output) && !EndFactory(c) && !EndFactory(c)
   && f.io.touches==touches && transport.Touches()==device);
  printf("PASS NEW_NATIVE_QLV1 fault%u/page%u/word%u;ALL119 actual STOP/ALL120holds precede reads,ZERO restore/following IO/retry/END,ALL owners retained\n",fault,page,word);return;
 }
 assert(closed && readback.started && readback.completed && readback.pages==1592 && readback.words==1630208
  && state->Closed(c.banks,f.owner.dram) && state->ClosedLayerBinding() && output.stopped==119 && output.held==119
  && output.memory.restoredWords==TT_Q38_RESIDENT_LAYER_SAVED_WORDS
  && output.memory.verifiedOriginalWords==2*TT_Q38_RESIDENT_LAYER_SAVED_WORDS);
 transport.VerifyResidentLayerReadback(values.data(),unsigned(values.size()));
 touches=f.io.touches;device=transport.Touches();
 assert(!CloseQ38ResidentLayerNativeForValidation(c,output,values.data(),unsigned(values.size()),readback)
  && !CloseQ38ResidentLayerNative(c,output) && f.io.touches==touches && transport.Touches()==device);
 assert(EndFactory(c) && !state->Unsafe() && !c.dispatchOwnerRetained && !c.Unsafe());
 if(observed)*observed=std::move(values); // TEST artifact only,AFTER genuine native END;NEVER an input/ready setter
 puts("PASS ACTUAL NEW native zero/QPE1/QRN1/4QPS1->oneROOT Conv/GDN/output/norm/FFN/final5120residual->ALL119STOP/ALL120holds->1592 canonical fixed4KiB transfers/ALL1630208 genuine emitted VM workspace+BOTHslots words/exact counter ancestry->full44003552B snapshot restore/secondproof/genuine closed native lineage/actual END;NO old-prefix payload alias,NO physical/ORIGINAL-model/full64layer/Work/rate qualification");
}
#ifndef PIONEER_Q38_LAYER_READBACK_FIXTURE_ONLY
int main(int argc,char** argv){
 assert(argc==9 && (!strcmp(argv[8],"--all") || !strcmp(argv[8],"--positive")));setvbuf(stdout,nullptr,_IOLBF,0);
 (void)&Input;(void)&Prepare;(void)&Upload;(void)&Close;(void)&PreparePanel;(void)&Run;(void)&Job;(void)&ClosePanel;(void)&Finish;
 (void)&ForkActualNativeLayerTerminal;(void)&FinishNativeOutput;
 NativeBindingFixture f(argv[3],argv[4]);f.io.rmsImage=argv[5];auto& c=*f.c;auto& m=*c.q38EmbeddingModel;c.q38Clock.value.busy_mhz=1400;
 assert(BindQ38ResidentLayerNativeState(c));PioneerQ38StateZeroResult gdn;PioneerQ38ConvHistoryZeroResult history;PioneerQ38LearnedRmsResult rms;
 assert(InitializeQ38ResidentLayerRecurrent(c,gdn) && InitializeQ38ResidentLayerHistory(c,history)
  && f.Initialize() && f.Run() && RunQ38LearnedRmsModel(c,rms));
 std::vector<uint32> addresses(73737),values(73737);uint32 sources[16],commands[10],counts[8];
 for(unsigned i=0;i<73737;i++){addresses[i]=PioneerQ38EmbeddingSnapshotAddress(i);values[i]=f.io.RawSlot(addresses[i]);}
 for(unsigned i=0;i<16;i++)sources[i]=i?f.io.source[i]:f.io.config;
 for(unsigned i=0;i<10;i++)commands[i]=f.io.command[i];for(unsigned i=0;i<8;i++)counts[i]=f.io.PanelIO::ResidentNocRead(PioneerQ38SpanCounter(i));
 ProjectionNativeTransport transport(argv[1],argv[2],f.io.wholeWorkspace);transport.SeedRoot(addresses.data(),values.data(),unsigned(values.size()),sources,commands,counts);
 f.io.projectionTransport=&transport;PioneerQ38ProjectionGraphResult projection;transport.Observe(projection);
 assert(PrepareQ38ProjectionExecution(c,projection));
 for(unsigned role:{10U,11U,5U,6U}){assert(RunQ38ProjectionExecution(c,role,projection));transport.VerifyOutput();}
 assert(CloseQ38ProjectionExecution(c,projection) && m.Closed() && m.projection.ResidentInput());
 transport.ResidentLayerSources(argv[6],argv[7],f.io);PioneerQ38ResidentLayerResult output;
 assert(PrepareQ38ResidentLayerNative(c,output));transport.ResidentLayerFault(0,output);
 assert(RunQ38ResidentLayerNative(c,output));transport.VerifyResidentLayer();
 puts("ACTUAL_NEW_NATIVE_COMPLETE_LAYER_COMPUTATION_PROOFS_ROOTHELD;BEGIN_SAME_OWNER_QLV1");
 auto child=[&](unsigned fault,unsigned page,unsigned word){pid_t pid=fork();assert(pid>=0);
  if(!pid){FinishLayerReadback(f,transport,output,fault,page,word);_exit(0);}int status=0;
  assert(waitpid(pid,&status,0)==pid && WIFEXITED(status) && !WEXITSTATUS(status));
 };
 if(!strcmp(argv[8],"--all")){
  for(unsigned fault=1;fault<=3;fault++)child(fault,0,0);
  for(unsigned page:{0U,5U,15U,45U,51U,56U,824U,1591U})child(4,page,0);
  child(4,1591,1023);
  for(unsigned fault:{5U,6U})child(fault,56,0);
  child(7,0,1023);child(8,0,0);
 }
 FinishLayerReadback(f,transport,output,0,0,0);
}
#endif
