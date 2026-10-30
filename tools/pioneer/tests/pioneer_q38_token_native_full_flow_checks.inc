/* SPDX-License-Identifier: MIT */
#if !defined(PIONEER_Q38_TOKEN_NATIVE_FULL_FLOW_FIXTURE) || PIONEER_Q38_TOKEN_NATIVE_FULL_FLOW_FIXTURE != 1 \
 || !defined(PIONEER_Q38_TOKEN_NATIVE_PEER_FIXTURE) || PIONEER_Q38_TOKEN_NATIVE_PEER_FIXTURE != 1 \
 || !defined(PIONEER_Q38_TOKEN_CONTEXT_COMPILE_ONLY) || PIONEER_Q38_TOKEN_CONTEXT_COMPILE_ONLY != 1 \
 || defined(PIONEER_Q38_WORK_INFERENCE)
#error Explicit native full-flow TEST required
#endif
#ifdef PIONEER_Q38_TOKEN_NATIVE_READOUT_FLOW_FIXTURE
#include "pioneer_q38_token_native_readout_flow_checks.inc"
#endif
static const char* tokenNativeTraceDirectory=nullptr;
static void CheckNativeFullFlow(Fixture& f){
 namespace C=PioneerQ38GlobalChainLayout;namespace R=C::R;namespace T=PioneerQ38TokenLayout;namespace W=PioneerQ38SelectedGlobalWorker;
 auto& c=*f.c;auto& io=f.io;auto& cpu=*io.executionCpu;auto* state=c.q38TokenNative;
 assert(!lockHeld && !state->Unsafe() && tokenNativeTraceDirectory && io.nativeTrace && io.kvWords.empty());
 auto embedding=[&](unsigned position,unsigned token){
  std::array<uint32,5120> actual;for(unsigned i=0;i<5120;i++)actual[i]=io.runtimeDram.at(PioneerQ38GlobalFeedbackLayout::Offset/4+i);
  assert(io.nativeTrace->Embedding(position,token,actual.data(),actual.size()));
 };
 auto commit=[&](unsigned position){
  std::array<uint32,64> actual;for(unsigned i=0;i<64;i++)actual[i]=io.runtimeDram.at(T::OutputOffset/4+position*64+i);
  assert(io.nativeTrace->Commit(position,actual.data(),actual.size()));
 }; // actual immutable published GDDR bytes,not L1 RecordLocal or a host completion
 embedding(0,io.runtimeDram.at(T::SessionOffset/4+7));cpu.ResumeFullLayer();cpu.fullFlowRunning=true;
 uint64 rootBucket=cpu.ObservedRootInstructions()/20000000;
 for(unsigned event=0;event<196 && cpu.pc!=0x60000 && cpu.pc!=0x60004;event++){
  {
   PioneerQ38TokenCallScope scope(&f.cookie);assert(scope.Context()==&c);
   for(unsigned round=0;round<12000000 && !cpu.fullLayerComplete && !cpu.flowBoundary && cpu.pc!=0x60000 && cpu.pc!=0x60004;round++){
    io.Pause();if(cpu.ObservedRootInstructions()/20000000!=rootBucket){rootBucket=cpu.ObservedRootInstructions()/20000000;
     printf("OBSERVED native full-flow position%u layers%u ROOTinstructions%llu pc%x packets%u stateW%llu KVwords%zu;NOT whole-model/token/STOP qualification\n",
      cpu.flowPosition,cpu.flowLayers,(unsigned long long)cpu.ObservedRootInstructions(),cpu.pc,cpu.layerPackets,
      (unsigned long long)io.mutableWriteWords[0],io.kvWords.size());}
   }
   if(cpu.pc==0x60000)fprintf(stderr,"NATIVE_FULL_FLOW_FAULT position%u layers%u ROOTinstructions%llu pc%x phase%x;NO completion supplied\n",
    cpu.flowPosition,cpu.flowLayers,(unsigned long long)cpu.ObservedRootInstructions(),cpu.pc,io.Raw(0,R::Status+20));
   assert(cpu.pc!=0x60000 && !state->Unsafe() && !state->Closed());
   PioneerQ38TokenExecutionResult observed;assert(ObserveQ38TokenNativeRoot(c,observed) && observed.acknowledged && !observed.unsafe);
  }
  if(cpu.fullLayerComplete){
   const unsigned position=cpu.flowLayers/64,layer=cpu.flowLayers%64;
   assert(position<3 && position==cpu.flowPosition && io.Raw(0,C::Progress)==T::Magic
    && io.Raw(0,C::Progress+4)==position && io.Raw(0,C::Progress+8)==layer+1 && !io.Core(0).pending);
   std::array<uint32,5120> actual;for(unsigned i=0;i<5120;i++)actual[i]=io.runtimeDram.at(R::FinalOffset/4+i);
   assert(io.nativeTrace->Layer(position,layer,actual.data(),actual.size()));
   const uint64 kvExpected=uint64(position*16+(layer+1)/4)*1024;
   assert(io.kvWords.size()==kvExpected && io.kvWriteWords==kvExpected); // only genuine first appends,never prior-row rewrites
   for(unsigned w=1;w<120;w++){
    auto& p=*io.bootstrapPeer[w-1];assert(p.Idle() && p.cpu->ObserveLayerBoundary());p.Observe();
   }
   if(!position && !layer){
    assert(io.nativeTrace->SnapshotFirstLayerDiagnostic(tokenNativeTraceDirectory)
     && !io.nativeTrace->Complete() && !io.nativeTrace->SavedComplete());
    puts("SNAPSHOT native first ACTUAL layer+all119 verified boundaries/incomplete prefix-0.bin;SAME full flow continues,NOT token/STOP/END/physical/TPS qualification");
   }
   cpu.ObserveRootLayerWindow();
   printf("OBSERVED native original position%u/layer%u ACTUAL return+all119 down-matrix arithmetic/DONE/WAIT/5120 final bytes recorded/ROOTinstructions%llu;CPU trace comparison and full terminal STILL REQUIRED\n",
    position,layer,(unsigned long long)cpu.ObservedRootInstructions());
  }else if(cpu.flowBoundary){
   assert(cpu.flowPosition<2 && cpu.flowLayers==(cpu.flowPosition+1)*64 && !io.Core(0).pending
    && cpu.ObserveParent(16)==cpu.flowPosition+1 && cpu.ObserveParent(18)==cpu.flowPosition+1
    && cpu.ObserveParent(19)==((cpu.flowPosition+1)&1));
   commit(cpu.flowPosition);cpu.flowPosition++;
   embedding(cpu.flowPosition,cpu.ObserveParent(17));cpu.flowBoundary=false;
   printf("OBSERVED native actual immutable token record%u/greedy-or-prompt next embedding/committed frontier from device stack;NOT terminal/END or independent numerical qualification\n",cpu.flowPosition-1);
  }else{assert(cpu.pc==0x60004);break;}
 }
 assert(cpu.pc==0x60004 && cpu.flowLayers==192 && cpu.flowPosition==2 && !cpu.fullLayerComplete && !cpu.flowBoundary
  && !io.Core(0).pending && io.kvReadWords && io.kvWriteWords==49152 && io.kvWords.size()==49152 && !state->Unsafe() && !state->Closed());
 const uint32 terminal[]={T::TerminalMagic,1,1,3,2,119,64,973,1,1};
 for(unsigned i=0;i<10;i++)assert(io.Raw(0,T::TerminalLocal+i*4)==terminal[i]);
 for(unsigned i=10;i<12;i++)assert(!io.Raw(0,T::TerminalLocal+i*4));
 for(unsigned i=0;i<8;i++){
  assert(io.Raw(0,T::TerminalLocal+(12+i)*4)==io.Raw(0,R::Boot+(8+i)*4));
  assert(io.Raw(0,T::TerminalLocal+(36+i)*4)==io.Core(0).ResidentNocRead(i==0?0x208:i==1?0x20c:(i==2 || i==6)?0x210:i==3?0x214:i==4?0x204:i==5?0x220:0x228));
 }
 for(unsigned i=0;i<16;i++)assert(io.Raw(0,T::TerminalLocal+(20+i)*4)==io.Core(0).source[i]);
 for(unsigned i=44;i<64;i++)assert(!io.Raw(0,T::TerminalLocal+i*4));
 const unsigned packets=cpu.layerPackets,submitted=cpu.submitted,completed=cpu.completed;
 const uint64 instructionsBefore=cpu.ObservedRootInstructions();
 for(unsigned i=0;i<2048;i++){cpu.StepScalar();assert(cpu.pc==0x60004);}
 assert(cpu.ObservedRootInstructions()==instructionsBefore+2048 && cpu.layerPackets==packets && cpu.submitted==submitted && cpu.completed==completed);
 uint64 peerInstructions=0;
 for(unsigned w=1;w<120;w++){
  auto& p=*io.bootstrapPeer[w-1];const auto s=p.cpu->Stats();assert(p.cpu->ObserveStopped() && !s.active && !s.pending
   && io.Raw(w,W::Job+5*4)==64 && io.Raw(w,W::Job+6*4)==2 && io.Raw(w,W::Job+7*4)==9999);
  p.cpu->AdvanceStopped(2048);const auto after=p.cpu->Stats();
  assert(after.totalSteps==s.totalSteps+2048 && after.requests==s.requests && after.responses==s.responses && p.cpu->ObserveStopped());
  peerInstructions+=after.totalSteps;
 }
 {
  PioneerQ38TokenCallScope scope(&f.cookie);assert(scope.Context()==&c);PioneerQ38TokenTerminalResult observed;
  const bool okay=ObserveQ38TokenNativeTerminal(c,observed);
  if(!okay)fprintf(stderr,"FULL_FLOW_TERMINAL_DIAGNOSTIC phase%u worker%u word%u firstPC%x secondPC%x unsafe%u\n",
   observed.phase,observed.worker,observed.word,observed.firstPC,observed.secondPC,observed.unsafe);
  assert(okay && observed.ready && !observed.cancelled && observed.committed==3 && observed.position==2
   && observed.completedLayers==64 && !observed.stage && state->TerminalObserved() && !state->Closed());
 }
 commit(2);assert(io.nativeTrace->Complete() && io.nativeTrace->Save(tokenNativeTraceDirectory));
 assert(!io.runtimeDram.at(T::OutputOffset/4+3*64) && !io.pciWrites && !io.HostReadMock::bmeWrites
  && !io.wideReleased && !io.PageMock::releases && !io.OutputBufferMock::releases);
 printf("EXECUTED native SAME full851 owner+1010 genuinezero+119virgin full engines/3positions*64layers/actual GDN+Conv+KV+vocabulary+device committed records/genuine ROOT terminal+all119 actual STOP+native terminal observation/ROOTinstructions%llu peerInstructions%llu/complete RAW QOD1 traces saved;independent CPU numerical comparison STILL REQUIRED;NO restoration/END/physical/Work/TPS grant\n",
  (unsigned long long)cpu.ObservedRootInstructions(),(unsigned long long)peerInstructions);
#ifdef PIONEER_Q38_TOKEN_NATIVE_READOUT_FLOW_FIXTURE
 CheckNativeReadoutFlow(f);
#endif
}
