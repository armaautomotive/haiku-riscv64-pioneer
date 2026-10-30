/* SPDX-License-Identifier: MIT */
#if !defined(PIONEER_Q38_TOKEN_NATIVE_FULL_LAYER_FIXTURE) || PIONEER_Q38_TOKEN_NATIVE_FULL_LAYER_FIXTURE != 1 \
 || !defined(PIONEER_Q38_TOKEN_NATIVE_PEER_FIXTURE) || PIONEER_Q38_TOKEN_NATIVE_PEER_FIXTURE != 1 \
 || !defined(PIONEER_Q38_TOKEN_CONTEXT_COMPILE_ONLY) || PIONEER_Q38_TOKEN_CONTEXT_COMPILE_ONLY != 1 \
 || defined(PIONEER_Q38_WORK_INFERENCE)
#error Explicit native full-layer TEST required
#endif
static const char* tokenNativePrefixFile=nullptr;
static void CheckNativeFullLayer(Fixture& f){
 namespace C=PioneerQ38GlobalChainLayout;namespace R=C::R;namespace W=PioneerQ38SelectedGlobalWorker;
 auto& c=*f.c;auto& io=f.io;auto& cpu=*io.executionCpu;auto* state=c.q38TokenNative;
 assert(!lockHeld && !state->Unsafe() && tokenNativePrefixFile && io.mutableBytes.size()==1009);
 cpu.ResumeFullLayer();unsigned observedBucket=cpu.steps/10000000;
 {
  PioneerQ38TokenCallScope scope(&f.cookie);assert(scope.Context()==&c);
  for(unsigned round=0;round<12000000 && !cpu.fullLayerComplete && cpu.pc!=0x60000 && cpu.pc!=0x60004;round++){
   io.Pause();if(cpu.steps/10000000!=observedBucket){observedBucket=cpu.steps/10000000;
    printf("OBSERVED native layer0 ROOTsteps%u pc%x packets%u newDoors%u phase%x stateR%llu stateW%llu historyR%llu;NOT layer/token qualification\n",
     cpu.steps,cpu.pc,cpu.layerPackets,cpu.fullLayerDoorbells,io.Raw(0,R::Status+20),
     (unsigned long long)io.mutableReadWords[0],(unsigned long long)io.mutableWriteWords[0],(unsigned long long)io.mutableReadWords[1]);
   }
  }
  if(!cpu.fullLayerComplete)fprintf(stderr,"FULL_LAYER_DIAGNOSTIC ROOTpc%x steps%u packets%u doors%u phase%x;NO result supplied\n",
   cpu.pc,cpu.steps,cpu.layerPackets,cpu.fullLayerDoorbells,io.Raw(0,R::Status+20));
  assert(cpu.fullLayerComplete && !cpu.layerHeld && !io.Core(0).pending && cpu.submitted==cpu.completed
   && cpu.pc!=0x60000 && cpu.pc!=0x60004 && !state->Unsafe() && !state->Closed()
   && io.Raw(0,C::Progress)==PioneerQ38TokenLayout::Magic && !io.Raw(0,C::Progress+4) && io.Raw(0,C::Progress+8)==1);
  PioneerQ38TokenExecutionResult observed;assert(ObserveQ38TokenNativeRoot(c,observed) && observed.acknowledged && !observed.unsafe);
 }
 uint64 instructions=0;unsigned activeWorkers=0,nextNonzero=0,minPeerSp=0x70000,minPeerAccessed=0x70000;
 for(unsigned w=1;w<120;w++){
  auto& p=*io.bootstrapPeer[w-1];const auto s=p.cpu->Stats();
  assert(p.Idle() && !s.pending && !s.active && !io.Raw(w,W::Status+8) && io.Raw(w,W::Status+4)==s.sequence);
  instructions+=s.steps;activeWorkers+=s.sequence!=0;
  const auto stack=p.cpu->Stack();assert(stack.instructions==s.totalSteps && stack.minimumSp>=0x6e000);
  minPeerSp=std::min(minPeerSp,stack.minimumSp);minPeerAccessed=std::min(minPeerAccessed,stack.minimumAccessed);
 }
 assert(activeWorkers==119 && io.mutableReadWords[0] && io.mutableWriteWords[0] && io.mutableReadWords[1]);
 for(unsigned i=0;i<672;i++){const auto& r=io.recurrent[i];const auto* words=io.recurrentWords.data()+io.recurrentFirst[i]*1024;
  for(unsigned j=0;j<r.heads*16384;j++){
   if(r.layer || !r.slot)assert(!words[j]);
   else{assert(isfinite(Float(words[j])));nextNonzero+=(words[j]&0x7fffffff)!=0;}
  }
 }
 assert(nextNonzero); // actual next-slot products,NOT a committed current-slot or token frontier
 FILE* out=fopen(tokenNativePrefixFile,"wbx");assert(out);
 auto emit=[&](uint32 word){unsigned char raw[4];for(unsigned b=0;b<4;b++)raw[b]=uint8(word>>(b*8));assert(fwrite(raw,1,4,out)==4);};
 const uint32 header[16]={0x31504f51,1,0,248319,2,5120,1,0,0,0,0,0,0,0,0,0};
 for(auto word:header)emit(word);
 for(unsigned offset:{unsigned(PioneerQ38GlobalFeedbackLayout::Offset),unsigned(R::FinalOffset)})for(unsigned i=0;i<5120;i++){
  const uint32 word=io.runtimeDram.at(offset/4+i);assert(isfinite(Float(word)));emit(word);
 }
 emit(0x454e4450);assert(!fclose(out));
 assert(cpu.instructionStack.Valid() && cpu.instructionStack.Initialized() && cpu.instructionStack.Instructions()==cpu.steps);
 printf("OBSERVED actual first-layer combined accelerator ROOT reserved stack%u/accessed%u;MAX119peer reserved%u/accessed%u;8192 bytes EACH/separate HOST kernel stack unqualified\n",
  0x70000-cpu.instructionStack.MinimumSp(),0x70000-cpu.minStack,0x70000-minPeerSp,0x70000-minPeerAccessed);
 printf("EXECUTED native ORIGINAL layer0/SAME full851+1010zero+119virgin full engines/actual matrix+Conv+GDN+FFN/initialized native recurrent/history/workspace byte aliases/next-slot nonzero%u/all119 actual DONE+WAIT/workerInstructions%llu ROOTinstructions%u/raw prefix OBSERVATION emitted;independent numerical comparison STILL REQUIRED;NOT token/committed state/KV/STOP/END/physical/Work/TPS\n",
  nextNonzero,(unsigned long long)instructions,cpu.steps);
}
