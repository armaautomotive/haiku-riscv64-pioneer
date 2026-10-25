/* SPDX-License-Identifier: MIT */
// Genuine complete native lifetime+actual programs;byte/OS endpoints TEST.
#define PIONEER_Q38_PROJECTION_NATIVE_FACTORY_FIXTURE
#define PIONEER_Q38_OUTPUT_BINDING_FIXTURE_ONLY
#include "pioneer_q38_resident_output_native_binding_test.cpp"
#include <sys/wait.h>
#include <unistd.h>
static void FinishReadback(NativeBindingFixture& f,ProjectionNativeTransport& transport,PioneerQ38ResidentOutputResult& output,
 unsigned fault,unsigned page,unsigned word){
 auto& c=*f.c;auto* state=c.q38ResidentOutputNative;
 std::vector<uint32_t> values(PioneerQ38ResidentOutputReadbackPlan::Words,0xdeadbeef);PioneerQ38ResidentOutputReadbackResult readback;
 unsigned touches=f.io.touches,device=transport.Touches();
 for(unsigned bad:{0U,PioneerQ38ResidentOutputReadbackPlan::Words-1U,PioneerQ38ResidentOutputReadbackPlan::Words+1U,UINT32_MAX})
  assert(!CloseQ38ResidentOutputNativeForValidation(c,output,values.data(),bad,readback));
 assert(!CloseQ38ResidentOutputNativeForValidation(c,output,nullptr,unsigned(values.size()),readback)
  && !CloseQ38ResidentOutputNativeForValidation(c,output,reinterpret_cast<uint32_t*>(reinterpret_cast<uintptr_t>(values.data())+1),unsigned(values.size()),readback)
  && !CloseQ38ResidentOutputNativeForValidation(c,output,reinterpret_cast<uint32_t*>(UINTPTR_MAX-3),unsigned(values.size()),readback));
 uint32_t fields[5];memcpy(fields,&readback,sizeof(readback));
 for(unsigned i=0;i<5;i++){fields[i]^=1;memcpy(&readback,fields,sizeof(readback));
  assert(!CloseQ38ResidentOutputNativeForValidation(c,output,values.data(),unsigned(values.size()),readback));fields[i]^=1;
 }memcpy(&readback,fields,sizeof(readback));
 assert(!state->Unsafe() && !c.dispatchOwnerRetained && transport.Touches()==device && f.io.touches==touches);
 transport.ObserveResidentReadback(fault,page,word);
 bool closed=CloseQ38ResidentOutputNativeForValidation(c,output,values.data(),unsigned(values.size()),readback);
 if(fault){
  const bool contained=!closed && state->Unsafe() && c.dispatchOwnerRetained && sColdRecoveryRequired && c.banks.Quarantined()
   && f.transfer.quarantined && f.wide.quarantined && c.q38Clock.quarantined && output.unsafe && !output.closed
   && output.stopped==119 && output.held==119 && !output.memory.restoredWords && !readback.completed
   && transport.ResidentReadbackFaultFence() && transport.Touches()==transport.ResidentReadbackFaultFence();
  if(!contained)fprintf(stderr,"READBACK_CONTAINMENT_FAIL fault%u/page%u/word%u closed%u unsafe%u retained%u cold%u stopped%u held%u restored%u readback%u/%u/%u transfers%u fence%u touches%u\n",
   fault,page,word,unsigned(closed),unsigned(state->Unsafe()),unsigned(c.dispatchOwnerRetained),unsigned(sColdRecoveryRequired),
   output.stopped,output.held,output.memory.restoredWords,readback.pages,readback.words,readback.completed,
   transport.ResidentReadbackRequests(),transport.ResidentReadbackFaultFence(),transport.Touches());
  assert(contained && transport.ResidentReadbackRequests()==page+1);
  touches=f.io.touches;device=transport.Touches();
  assert(!CloseQ38ResidentOutputNativeForValidation(c,output,values.data(),unsigned(values.size()),readback)
   && !CloseQ38ResidentOutputNative(c,output) && !EndFactory(c) && !EndFactory(c)
   && f.io.touches==touches && transport.Touches()==device);
  printf("PASS actual native combined readback fault%u/page%u/word%u;ALL actual STOP/holds before reads,ZERO restore/following IO/retry/END,all owners retained\n",fault,page,word);return;
 }
 assert(closed && readback.started && readback.completed && readback.pages==1592 && readback.words==1629120
  && state->Closed(c.banks,f.owner.dram) && state->ClosedOutputBinding() && output.stopped==119 && output.held==119
  && output.memory.restoredWords==TT_Q38_RESIDENT_OUTPUT_SAVED_WORDS
  && output.memory.verifiedOriginalWords==2*TT_Q38_RESIDENT_OUTPUT_SAVED_WORDS);
 transport.VerifyResidentReadback(values.data(),unsigned(values.size()));
 assert(EndFactory(c) && !state->Unsafe() && !c.dispatchOwnerRetained && !c.Unsafe());
 puts("PASS ACTUAL native zero/prefix/new Conv/GDN/QOL1/full5120 matrix->ALL119STOP/ALL120holds->1592fixed transfers/ALL1629120 actual VM state+workspace words/BOTH slots/exact transfer counter ancestry->full snapshot restore/second proof/actual END;TEST byte endpoints,NOT silicon/wholemodel/Work/rate");
}
int main(int argc,char** argv){
 assert(argc==9 && (!strcmp(argv[8],"--all") || !strcmp(argv[8],"--positive")));setvbuf(stdout,nullptr,_IOLBF,0);
 (void)&Input;(void)&Prepare;(void)&Upload;(void)&Close;(void)&PreparePanel;(void)&Run;(void)&Job;(void)&ClosePanel;(void)&Finish;
 NativeBindingFixture f(argv[3],argv[4]);f.io.rmsImage=argv[5];auto& c=*f.c;auto& m=*c.q38EmbeddingModel;c.q38Clock.value.busy_mhz=1400;
 assert(BindQ38ResidentOutputNativeState(c));PioneerQ38StateZeroResult gdn;PioneerQ38ConvHistoryZeroResult history;PioneerQ38LearnedRmsResult rms;
 assert(InitializeQ38ResidentOutputRecurrent(c,gdn) && InitializeQ38ResidentOutputHistory(c,history)
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
 transport.ResidentOutputSources(argv[6],argv[7],f.io);PioneerQ38ResidentOutputResult output;
 assert(PrepareQ38ResidentOutputNative(c,output));transport.ResidentOutputFault(0,output);
 assert(RunQ38ResidentOutputNative(c,output));transport.VerifyResidentOutput();
 puts("ACTUAL complete combined execution reached terminal;BEGIN SAME-OWNER READBACK");
 auto child=[&](unsigned fault,unsigned page,unsigned word){pid_t pid=fork();assert(pid>=0);
  if(!pid){FinishReadback(f,transport,output,fault,page,word);_exit(0);}int status=0;
  assert(waitpid(pid,&status,0)==pid && WIFEXITED(status) && !WEXITSTATUS(status));
 };
 if(!strcmp(argv[8],"--all")){
  for(unsigned fault=1;fault<=3;fault++)child(fault,0,0);
  for(unsigned page:{0U,10U,40U,46U,49U,50U,51U,56U,824U,1591U})child(4,page,0);
  child(4,49,8);child(4,49,767);child(4,1591,1023);
 }
 FinishReadback(f,transport,output,0,0,0);
}
