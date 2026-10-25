/* SPDX-License-Identifier: MIT */
// ACTUAL retained native FD/context/factories/scopes/maps/controllers.
// All initializer/prefix/coordinator/peer programs execute. Endpoints TEST.
#define PIONEER_Q38_PROJECTION_NATIVE_FACTORY_FIXTURE
#define PIONEER_Q38_OUTPUT_BINDING_FIXTURE_ONLY
#include "pioneer_q38_resident_layer_native_binding_test.cpp"
#include <sys/wait.h>
#include <unistd.h>
static bool nativeLayerTerminalChild=false;
static unsigned nativeLayerTerminalFault=0;
static void UnknownNativeOutput(NativeBindingFixture& f,ProjectionNativeTransport& transport,PioneerQ38ResidentLayerResult& output,unsigned fault){
 auto& c=*f.c;auto* state=c.q38ResidentLayerNative;
 assert(state && state->Unsafe() && c.q38EmbeddingModel->Unsafe() && c.dispatchOwnerRetained && sColdRecoveryRequired
  && c.banks.Quarantined() && c.lease.recovery_required && c.bulk.unsafe && f.transfer.quarantined && f.wide.quarantined
  && c.q38Clock.quarantined && f.io.PioneerHostPageIO::quarantined && !state->Closed(c.banks,f.owner.dram) && !state->ClosedLayerBinding());
 if(transport.ResidentLayerFaultFence())assert(transport.Touches()==transport.ResidentLayerFaultFence());
 unsigned touches=f.io.touches,device=transport.Touches(),maps=f.io.mapCalls;
 assert(!PrepareQ38ResidentLayerNative(c,output) && !RunQ38ResidentLayerNative(c,output) && !CloseQ38ResidentLayerNative(c,output)
  && !EndFactory(c) && !EndFactory(c) && f.io.touches==touches && transport.Touches()==device && f.io.mapCalls==maps);
 printf("PASS ACTUAL_NATIVE_LAYER fault%u phase%u worker%u;ALL native owners/maps/journal/clock/hosts retained,ZERO following IO/reset/STOP/restore/retry/END\n",fault,output.phase,output.worker);
}
template<class F> static void NativeOutputChild(F&& action){
 pid_t child=fork();assert(child>=0);if(!child){action();_exit(0);}
 int status=0;assert(waitpid(child,&status,0)==child && WIFEXITED(status) && WEXITSTATUS(status)==0);
}
static void FinishNativeOutput(NativeBindingFixture& f,ProjectionNativeTransport& transport,PioneerQ38ResidentLayerResult& output,unsigned fault){
 auto& c=*f.c;auto& m=*c.q38EmbeddingModel;auto* state=c.q38ResidentLayerNative;
 unsigned touches=f.io.touches,device=transport.Touches();
 if(fault==5)currentTeam=99;
 if(fault==6){const auto* p=m.plan.ProjectionStripe(0,13,0);assert(p);
  unsigned page=p->bank*unsigned(m.plan.Capacity/4096)+p->offset/4096;assert(f.learned[page]==1);f.learned[page]=0;
 }
 transport.ResidentLayerFault(fault,output);
 bool ran=RunQ38ResidentLayerNative(c,output);
 if(nativeLayerTerminalChild)fault=nativeLayerTerminalFault;
 if(fault==5 || fault==6){assert(!ran && !output.rootStarts && !output.rootHolds && !output.memory.restoredWords
  && f.io.touches==touches && transport.Touches()==device);UnknownNativeOutput(f,transport,output,fault);return;}
 if(fault==8 || fault==12 || (fault>=14 && fault<=17)){assert(!ran && transport.ResidentLayerFaultInjected() && !output.rootHolds && !output.memory.restoredWords);
  UnknownNativeOutput(f,transport,output,fault);return;}
 assert(ran && transport.ResidentLayerFrontendSamples());transport.VerifyResidentLayer();
 puts("NEW_NATIVE_COMPLETE_ACTUAL_CONV_GDN_OUTPUT_RMS_FFN_FINAL5120RESIDUAL;BEGIN_CLOSE");
 bool closed=CloseQ38ResidentLayerNative(c,output);
 if((fault>=9 && fault<=11) || fault==13){
  assert(!closed && transport.ResidentLayerFaultInjected());
  if(fault==9)assert(!output.held && !output.memory.restoredWords);
  if(fault==10)assert(output.stopped==119 && output.held==118 && !output.memory.restoredWords);
  if(fault==11)assert(output.held==119 && output.memory.restoredWords==TT_Q38_RESIDENT_LAYER_SAVED_WORDS-1);
  if(fault==13)assert(output.held==119 && output.memory.restoredWords==TT_Q38_RESIDENT_LAYER_SAVED_WORDS
   && output.memory.verifiedOriginalWords==2*TT_Q38_RESIDENT_LAYER_SAVED_WORDS-1);
  UnknownNativeOutput(f,transport,output,fault);return;
 }
 assert(closed && state->Closed(c.banks,f.owner.dram) && state->ClosedLayerBinding()
  && output.stopped==119 && output.held==119 && output.memory.restoredWords==TT_Q38_RESIDENT_LAYER_SAVED_WORDS
  && output.memory.verifiedOriginalWords==2*TT_Q38_RESIDENT_LAYER_SAVED_WORDS && transport.ResidentLayerStops()==119);
 puts("NEW_NATIVE_ACTUAL119STOP_ALLHOLDS_FULLRESTORE_AND_CLOSED_BINDING;BEGIN_END");
 touches=f.io.touches;device=transport.Touches();
 assert(!PrepareQ38ResidentLayerNative(c,output) && !RunQ38ResidentLayerNative(c,output) && !CloseQ38ResidentLayerNative(c,output)
  && f.io.touches==touches && transport.Touches()==device && !f.io.ResidentLayerSourceScope(true));
 if(fault==7){f.io.q38ResidentLayerMemoryView[1]=nullptr;
  assert(!EndFactory(c) && f.io.touches==touches && transport.Touches()==device);UnknownNativeOutput(f,transport,output,fault);return;
 }
 assert(!fault && EndFactory(c) && !c.dispatchOwnerRetained && !state->Unsafe() && !c.Unsafe());
 puts("PASS ACTUAL SAME native FD/team/context:private GDN+history zero/CLOSE->QPE1/QRN1/4QPS1/STOP/fullrestore->distinct maps+44MB journal/new controller/private claim->actual Conv/GDN/output/residual+RMS/357FFNmatrixjobs+88producers/final5120residual->ALL119 STOP/ALL120holds/fulloriginal+secondproof->genuine CLOSED lineage/actual END; OS/learned endpoints TEST,NOT silicon/ORIGINAL-model/full64layer/tokenloop/rate qualification");
}
struct NativeLayerTerminalForks {ProjectionNativeTransport& transport;PioneerQ38ResidentLayerResult& result;};
static void ForkActualNativeLayerTerminal(void* context){
 auto& c=*static_cast<NativeLayerTerminalForks*>(context);unsigned cases=0;
 for(unsigned fault:{0U,7U,8U,9U,10U,11U,12U,13U,14U,15U,16U,17U}){
  pid_t child=fork();assert(child>=0);
  if(!child){nativeLayerTerminalChild=true;nativeLayerTerminalFault=fault;c.transport.ResidentLayerFault(fault,c.result);return;}
  int status=0;assert(waitpid(child,&status,0)==child && WIFEXITED(status) && !WEXITSTATUS(status));cases++;
 }
 assert(cases==12);
 puts("PASS COMPLETE NEW_NATIVE_LAYER actual terminal-stack positive+17 source/map/clock/code/PC/receipt/STOP/restore/END containment cases;forks inherit genuine initialization/QPS1/new private claim/installed119boot/COMPLETED emitted joined-layer instructions,EVERY child still owes actual native ROOT+ALL119 proofs/STOP/restore/END;NO fabricated readiness/products,NO silicon/fullmodel/rate qualification");
 _exit(0); // TEST virtual endpoints only;parent never impersonates a clean native END
}
#ifndef PIONEER_Q38_LAYER_EXECUTION_FIXTURE_ONLY
int main(int argc,char** argv){
 assert(argc==9 && (!strcmp(argv[8],"--all") || !strcmp(argv[8],"--positive")));setvbuf(stdout,nullptr,_IOLBF,0);
 (void)&Input;(void)&Prepare;(void)&Upload;(void)&Close;(void)&PreparePanel;(void)&Run;(void)&Job;(void)&ClosePanel;(void)&Finish;
 NativeBindingFixture f(argv[3],argv[4]);f.io.rmsImage=argv[5];f.c->q38Clock.value.busy_mhz=1400;
 assert(BindQ38ResidentLayerNativeState(*f.c));auto& c=*f.c;auto& m=*c.q38EmbeddingModel;auto* state=c.q38ResidentLayerNative;
 PioneerQ38StateZeroResult gdn;PioneerQ38ConvHistoryZeroResult history;PioneerQ38LearnedRmsResult rms;
 assert(InitializeQ38ResidentLayerRecurrent(c,gdn) && InitializeQ38ResidentLayerHistory(c,history)
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
 transport.ResidentLayerSources(argv[6],argv[7],f.io);
 assert(f.io.ResidentLayerSourceScope(false) && !f.io.ResidentLayerSourceScope(true) && !state->HasLayerMemory());
 PioneerQ38ResidentLayerResult output;uint32 resultWords[28];memcpy(resultWords,&output,sizeof(output));
 unsigned touches=f.io.touches,device=transport.Touches();
 for(unsigned word=0;word<28;word++){
  resultWords[word]^=1;memcpy(&output,resultWords,sizeof(output));
  assert(!PrepareQ38ResidentLayerNative(c,output) && !state->HasLayerMemory() && !f.io.q38ResidentLayerMemoryMapAttempted
   && f.io.touches==touches && transport.Touches()==device);resultWords[word]^=1;
 }memcpy(&output,resultWords,sizeof(output));
 for(unsigned mhz:{0U,1401U}){c.q38Clock.value.busy_mhz=mhz;assert(!PrepareQ38ResidentLayerNative(c,output)
  && f.io.touches==touches && transport.Touches()==device && !state->HasLayerMemory());}c.q38Clock.value.busy_mhz=1400;
 auto refuse=[&](auto& field,auto bad){auto saved=field;field=bad;
  assert(!f.io.ResidentLayerSourceScope(false) && !PrepareQ38ResidentLayerNative(c,output) && !state->HasLayerMemory()
   && !c.dispatchOwnerRetained && !f.io.q38ResidentLayerMemoryMapAttempted && f.io.touches==touches && transport.Touches()==device);
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
   assert(!PrepareQ38ResidentLayerNative(c,output) && !state->HasLayerMemory() && !f.io.q38ResidentLayerMemoryMapAttempted
    && f.io.touches==touches && transport.Touches()==device);f.learned[page]=saved;tested=true;
  }assert(tested);
 }
 puts("PASS ACTUAL_NATIVE_LAYER inert entry refusals50;ZERO new mapping/claim/IO");
 if(!strcmp(argv[8],"--all"))for(unsigned fault=1;fault<=4;fault++)NativeOutputChild([&](){
  unsigned slot=(fault-1)/2;if(fault&1)f.io.layerFailMap=int(slot);else f.io.layerNullMap=int(slot);
  unsigned maps=f.io.mapCalls;assert(!PrepareQ38ResidentLayerNative(c,output) && f.io.q38ResidentLayerMemoryMapAttempted
   && f.io.mapCalls==maps+slot+1 && transport.Touches()==device && !transport.ResidentLayerBoots());
  if(slot)assert(f.io.q38ResidentLayerMemoryArea[0]==40 && f.io.q38ResidentLayerMemoryView[0]);
  UnknownNativeOutput(f,transport,output,fault);
 });
 bool prepared=PrepareQ38ResidentLayerNative(c,output);
 if(!prepared)fprintf(stderr,"NATIVE_OUTPUT_PREPARE_FAIL phase%u worker%u word%u mem%u/%u/%u prepared%u boots%u unsafe%u\n",
  output.phase,output.worker,output.word,output.memory.phase,output.memory.worker,output.memory.word,output.prepared,transport.ResidentLayerBoots(),output.unsafe);
 assert(prepared && state->HasLayerMemory() && !state->Closed(c.banks,f.owner.dram) && !m.projection.ResidentInput()
  && transport.ResidentLayerBoots()==119 && c.Unsafe() && !c.Unsafe(true,false,true) && f.io.ResidentLayerSourceScope(true)
  && !f.io.ResidentLayerPrefixInitializationScope());
 puts("NEW_NATIVE_ACTUAL_MAPS_PREPARE_CLAIM_AND119BOOT;BEGIN_RUN");
 if(!strcmp(argv[8],"--all")){
  for(unsigned fault:{5U,6U})NativeOutputChild([&](){FinishNativeOutput(f,transport,output,fault);});
  NativeLayerTerminalForks checkpoint{transport,output};transport.ResidentLayerRunTerminalCheckpoint(ForkActualNativeLayerTerminal,&checkpoint);
  FinishNativeOutput(f,transport,output,0);
  assert(nativeLayerTerminalChild);_exit(0);
 }else FinishNativeOutput(f,transport,output,0);
}
#endif
