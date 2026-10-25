/* SPDX-License-Identifier: MIT */
// ACTUAL retained native FD/context/factories/scopes/maps/controllers.
// All initializer/prefix/coordinator/peer programs execute. Endpoints TEST.
#define PIONEER_Q38_PROJECTION_NATIVE_FACTORY_FIXTURE
#define PIONEER_Q38_OUTPUT_BINDING_FIXTURE_ONLY
#include "pioneer_q38_resident_output_native_binding_test.cpp"
#include <sys/wait.h>
#include <unistd.h>
static void UnknownNativeOutput(NativeBindingFixture& f,ProjectionNativeTransport& transport,PioneerQ38ResidentOutputResult& output,unsigned fault){
 auto& c=*f.c;auto* state=c.q38ResidentOutputNative;
 assert(state && state->Unsafe() && c.q38EmbeddingModel->Unsafe() && c.dispatchOwnerRetained && sColdRecoveryRequired
  && c.banks.Quarantined() && c.lease.recovery_required && c.bulk.unsafe && f.transfer.quarantined && f.wide.quarantined
  && c.q38Clock.quarantined && f.io.PioneerHostPageIO::quarantined && !state->Closed(c.banks,f.owner.dram) && !state->ClosedOutputBinding());
 if(transport.ResidentOutputFaultFence())assert(transport.Touches()==transport.ResidentOutputFaultFence());
 unsigned touches=f.io.touches,device=transport.Touches(),maps=f.io.mapCalls;
 assert(!PrepareQ38ResidentOutputNative(c,output) && !RunQ38ResidentOutputNative(c,output) && !CloseQ38ResidentOutputNative(c,output)
  && !EndFactory(c) && !EndFactory(c) && f.io.touches==touches && transport.Touches()==device && f.io.mapCalls==maps);
 printf("PASS ACTUAL_NATIVE_OUTPUT fault%u phase%u worker%u;ALL native owners/maps/journal/clock/hosts retained,ZERO following IO/reset/STOP/restore/retry/END\n",fault,output.phase,output.worker);
}
template<class F> static void NativeOutputChild(F&& action){
 pid_t child=fork();assert(child>=0);if(!child){action();_exit(0);}
 int status=0;assert(waitpid(child,&status,0)==child && WIFEXITED(status) && WEXITSTATUS(status)==0);
}
static void FinishNativeOutput(NativeBindingFixture& f,ProjectionNativeTransport& transport,PioneerQ38ResidentOutputResult& output,unsigned fault){
 auto& c=*f.c;auto& m=*c.q38EmbeddingModel;auto* state=c.q38ResidentOutputNative;
 unsigned touches=f.io.touches,device=transport.Touches();
 if(fault==5)currentTeam=99;
 if(fault==6){const auto* p=m.plan.ProjectionStripe(0,13,0);assert(p);
  unsigned page=p->bank*unsigned(m.plan.Capacity/4096)+p->offset/4096;assert(f.learned[page]==1);f.learned[page]=0;
 }
 transport.ResidentOutputFault(fault,output);
 bool ran=RunQ38ResidentOutputNative(c,output);
 if(fault==5 || fault==6){assert(!ran && !output.rootStarts && !output.rootHolds && !output.memory.restoredWords
  && f.io.touches==touches && transport.Touches()==device);UnknownNativeOutput(f,transport,output,fault);return;}
 if(fault==8 || fault==12 || (fault>=14 && fault<=17)){assert(!ran && transport.ResidentOutputFaultInjected() && !output.rootHolds && !output.memory.restoredWords);
  UnknownNativeOutput(f,transport,output,fault);return;}
 assert(ran && transport.ResidentOutputFrontendSamples());transport.VerifyResidentOutput();
 puts("NEW_NATIVE_COMPLETE_ACTUAL_CONV_GDN_QOL1_ALL5120_OUTPUTS;BEGIN_CLOSE");
 bool closed=CloseQ38ResidentOutputNative(c,output);
 if((fault>=9 && fault<=11) || fault==13){
  assert(!closed && transport.ResidentOutputFaultInjected());
  if(fault==9)assert(!output.held && !output.memory.restoredWords);
  if(fault==10)assert(output.stopped==119 && output.held==118 && !output.memory.restoredWords);
  if(fault==11)assert(output.held==119 && output.memory.restoredWords==TT_Q38_RESIDENT_OUTPUT_SAVED_WORDS-1);
  if(fault==13)assert(output.held==119 && output.memory.restoredWords==TT_Q38_RESIDENT_OUTPUT_SAVED_WORDS
   && output.memory.verifiedOriginalWords==2*TT_Q38_RESIDENT_OUTPUT_SAVED_WORDS-1);
  UnknownNativeOutput(f,transport,output,fault);return;
 }
 assert(closed && state->Closed(c.banks,f.owner.dram) && state->ClosedOutputBinding()
  && output.stopped==119 && output.held==119 && output.memory.restoredWords==TT_Q38_RESIDENT_OUTPUT_SAVED_WORDS
  && output.memory.verifiedOriginalWords==2*TT_Q38_RESIDENT_OUTPUT_SAVED_WORDS && transport.ResidentOutputStops()==119);
 puts("NEW_NATIVE_ACTUAL119STOP_ALLHOLDS_FULLRESTORE_AND_CLOSED_BINDING;BEGIN_END");
 touches=f.io.touches;device=transport.Touches();
 assert(!PrepareQ38ResidentOutputNative(c,output) && !RunQ38ResidentOutputNative(c,output) && !CloseQ38ResidentOutputNative(c,output)
  && f.io.touches==touches && transport.Touches()==device && !f.io.ResidentOutputSourceScope(true));
 if(fault==7){f.io.q38ResidentOutputMemoryView[1]=nullptr;
  assert(!EndFactory(c) && f.io.touches==touches && transport.Touches()==device);UnknownNativeOutput(f,transport,output,fault);return;
 }
 assert(!fault && EndFactory(c) && !c.dispatchOwnerRetained && !state->Unsafe() && !c.Unsafe());
 puts("PASS ACTUAL SAME native FD/team/context:private GDN+history zero/CLOSE->QPE1/QRN1/4QPS1/STOP/fullrestore->distinct maps+43.8MB journal/new controller/private claim->actual Conv/GDN/QOL1/ALL5120outputs->ALL119 STOP/ALL120holds/fulloriginal+secondproof->genuine CLOSED lineage/actual END; OS/learned endpoints TEST,NOT silicon/full64layer/tokenloop/rate qualification");
}
int main(int argc,char** argv){
 assert(argc==9 && (!strcmp(argv[8],"--all") || !strcmp(argv[8],"--positive")));setvbuf(stdout,nullptr,_IOLBF,0);
 (void)&Input;(void)&Prepare;(void)&Upload;(void)&Close;(void)&PreparePanel;(void)&Run;(void)&Job;(void)&ClosePanel;(void)&Finish;
 NativeBindingFixture f(argv[3],argv[4]);f.io.rmsImage=argv[5];f.c->q38Clock.value.busy_mhz=1400;
 assert(BindQ38ResidentOutputNativeState(*f.c));auto& c=*f.c;auto& m=*c.q38EmbeddingModel;auto* state=c.q38ResidentOutputNative;
 PioneerQ38StateZeroResult gdn;PioneerQ38ConvHistoryZeroResult history;PioneerQ38LearnedRmsResult rms;
 assert(InitializeQ38ResidentOutputRecurrent(c,gdn) && InitializeQ38ResidentOutputHistory(c,history)
  && state->InitializationsClosed(c.banks,f.owner.dram) && f.Initialize() && f.Run() && RunQ38LearnedRmsModel(c,rms));
 std::vector<uint32> addresses(73737),values(73737);uint32 sources[16],commands[10],counts[8];
 for(unsigned i=0;i<73737;i++){addresses[i]=PioneerQ38EmbeddingSnapshotAddress(i);values[i]=f.io.RawSlot(addresses[i]);}
 for(unsigned i=0;i<16;i++)sources[i]=i?f.io.source[i]:f.io.config;
 for(unsigned i=0;i<10;i++)commands[i]=f.io.command[i];for(unsigned i=0;i<8;i++)counts[i]=f.io.PanelIO::ResidentNocRead(PioneerQ38SpanCounter(i));
 ProjectionNativeTransport transport(argv[1],argv[2],f.io.wholeWorkspace);
 transport.SeedRoot(addresses.data(),values.data(),unsigned(values.size()),sources,commands,counts);
 f.io.projectionTransport=&transport;PioneerQ38ProjectionGraphResult projection;transport.Observe(projection);
 assert(PrepareQ38ProjectionExecution(c,projection));
 for(unsigned role:{10U,11U,5U,6U}){assert(RunQ38ProjectionExecution(c,role,projection));transport.VerifyOutput();}
 assert(CloseQ38ProjectionExecution(c,projection) && m.Closed() && m.projection.ResidentInput() && m.projection.ResidentInput()->Current());
 puts("NEW_NATIVE_GENUINE_GDN_HISTORY_ZERO_QPE1_QRN1_ALL4QPS1_STOP_RESTORE_PRIVATE_INPUT;BEGIN_NEW_FACTORY");
 transport.ResidentOutputSources(argv[6],argv[7],f.io);
 assert(f.io.ResidentOutputSourceScope(false) && !f.io.ResidentOutputSourceScope(true) && !state->HasOutputMemory());
 PioneerQ38ResidentOutputResult output;uint32 resultWords[28];memcpy(resultWords,&output,sizeof(output));
 unsigned touches=f.io.touches,device=transport.Touches();
 for(unsigned word=0;word<28;word++){
  resultWords[word]^=1;memcpy(&output,resultWords,sizeof(output));
  assert(!PrepareQ38ResidentOutputNative(c,output) && !state->HasOutputMemory() && !f.io.q38ResidentOutputMemoryMapAttempted
   && f.io.touches==touches && transport.Touches()==device);resultWords[word]^=1;
 }memcpy(&output,resultWords,sizeof(output));
 for(unsigned mhz:{0U,1401U}){c.q38Clock.value.busy_mhz=mhz;assert(!PrepareQ38ResidentOutputNative(c,output)
  && f.io.touches==touches && transport.Touches()==device && !state->HasOutputMemory());}c.q38Clock.value.busy_mhz=1400;
 auto refuse=[&](auto& field,auto bad){auto saved=field;field=bad;
  assert(!f.io.ResidentOutputSourceScope(false) && !PrepareQ38ResidentOutputNative(c,output) && !state->HasOutputMemory()
   && !c.dispatchOwnerRetained && !f.io.q38ResidentOutputMemoryMapAttempted && f.io.touches==touches && transport.Touches()==device);
  field=saved;
 };
 refuse(currentTeam,99);refuse(failThreadInfo,true);refuse(currentThread,99);refuse(f.cookie.magic,0U);refuse(f.cookie.context,nullptr);
 refuse(sRuntimeOwner,nullptr);refuse(sReloadGeneration,sReloadGeneration+1);refuse(c.initialized,nullptr);refuse(c.hashes,nullptr);
 refuse(m.identity.modelEpoch,m.identity.modelEpoch+1);refuse(c.q38GdnCallThread,-1);refuse(c.bulk.dirty,true);
 refuse(c.bulkHost.ready,false);refuse(c.wideHost.ready,false);refuse(c.q38Clock.value.busy_acknowledged,0U);
 for(unsigned role:{7U,8U,9U,12U,13U}){bool tested=false;
  for(unsigned i=0;i<m.plan.Count() && !tested;i++){const auto* p=m.plan.Part(i);
   if(p->kind!=m.plan.WeightKind || p->layer || p->role!=role)continue;
   unsigned page=p->bank*unsigned(m.plan.Capacity/4096)+p->offset/4096;auto saved=f.learned[page];assert(saved==1);f.learned[page]=0;
   assert(!PrepareQ38ResidentOutputNative(c,output) && !state->HasOutputMemory() && !f.io.q38ResidentOutputMemoryMapAttempted
    && f.io.touches==touches && transport.Touches()==device);f.learned[page]=saved;tested=true;
  }assert(tested);
 }
 puts("PASS ACTUAL_NATIVE_OUTPUT inert entry refusals50;ZERO new mapping/claim/IO");
 if(!strcmp(argv[8],"--all"))for(unsigned fault=1;fault<=4;fault++)NativeOutputChild([&](){
  unsigned slot=(fault-1)/2;if(fault&1)f.io.outputFailMap=int(slot);else f.io.outputNullMap=int(slot);
  unsigned maps=f.io.mapCalls;assert(!PrepareQ38ResidentOutputNative(c,output) && f.io.q38ResidentOutputMemoryMapAttempted
   && f.io.mapCalls==maps+slot+1 && transport.Touches()==device && !transport.ResidentOutputBoots());
  if(slot)assert(f.io.q38ResidentOutputMemoryArea[0]==30 && f.io.q38ResidentOutputMemoryView[0]);
  UnknownNativeOutput(f,transport,output,fault);
 });
 bool prepared=PrepareQ38ResidentOutputNative(c,output);
 if(!prepared)fprintf(stderr,"NATIVE_OUTPUT_PREPARE_FAIL phase%u worker%u word%u mem%u/%u/%u prepared%u boots%u unsafe%u\n",
  output.phase,output.worker,output.word,output.memory.phase,output.memory.worker,output.memory.word,output.prepared,transport.ResidentOutputBoots(),output.unsafe);
 assert(prepared && state->HasOutputMemory() && !state->Closed(c.banks,f.owner.dram) && !m.projection.ResidentInput()
  && transport.ResidentOutputBoots()==119 && c.Unsafe() && !c.Unsafe(true,false,true) && f.io.ResidentOutputSourceScope(true)
  && !f.io.ResidentOutputPrefixInitializationScope());
 puts("NEW_NATIVE_ACTUAL_MAPS_PREPARE_CLAIM_AND119BOOT;BEGIN_RUN");
 if(!strcmp(argv[8],"--all")){
  for(unsigned fault:{0U,5U,6U,7U,8U,9U,10U,11U,12U,13U,14U,15U,16U,17U})NativeOutputChild([&](){FinishNativeOutput(f,transport,output,fault);});
  puts("PASS COMPLETE ACTUAL_NATIVE_OUTPUT NEW dual-park positive plus17 native containment failures including fault/padding/fault-loop PC+old aliased fault instruction;forks inherit ACTUAL private prefix/claim/installed119boot,NEVER fabricated owner or completion;TEST byte endpoints,NOT silicon/fullmodel/rate");
 }else FinishNativeOutput(f,transport,output,0);
}
