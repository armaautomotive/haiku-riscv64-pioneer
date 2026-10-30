/* SPDX-License-Identifier: MIT */
#if !defined(PIONEER_Q38_TOKEN_NATIVE_LAYER_FIXTURE) || PIONEER_Q38_TOKEN_NATIVE_LAYER_FIXTURE != 1
#error Explicit offline native layer test required
#endif
// Members of the SAME virgin ROOT,not another CPU or a completed cohort.
bool layerActive=false,layerHeld=false;
#ifdef PIONEER_Q38_TOKEN_NATIVE_FIRST_MATRIX_FIXTURE
bool firstMatrixRunning=false;unsigned firstMatrixDoorbells=0;
void ResumeFirstMatrix(){assert(layerHeld && dispatchWorker==1 && io.Core(0).pending);firstMatrixRunning=true;layerHeld=false;dispatchWorker=0;}
#ifdef PIONEER_Q38_TOKEN_NATIVE_FULL_LAYER_FIXTURE
bool fullLayerRunning=false,fullLayerComplete=false;unsigned fullLayerDoorbells=0;
void ResumeFullLayer(){assert(firstMatrixRunning && layerHeld && dispatchWorker==1 && io.Core(0).pending);
 fullLayerRunning=true;layerHeld=false;dispatchWorker=0;}
#ifdef PIONEER_Q38_TOKEN_NATIVE_CANCEL_LAYER_FIXTURE
#if PIONEER_Q38_TOKEN_NATIVE_CANCEL_LAYER_FIXTURE != 1 || defined(PIONEER_Q38_TOKEN_NATIVE_FULL_FLOW_FIXTURE)
#error Explicit first-layer cancellation TEST only,not whole-flow
#endif
void ResumeFirstLayerCancellation(){
 assert(fullLayerRunning && fullLayerComplete && !io.Core(0).pending && submitted==completed
  && io.Raw(0,PioneerQ38TokenLayout::CancelLocal)==PioneerQ38TokenLayout::CancelMagic);
 fullLayerComplete=false; // release HOST pause only;no PC/register/input/receipt change
}
#endif
#ifdef PIONEER_Q38_TOKEN_NATIVE_FULL_FLOW_FIXTURE
bool fullFlowRunning=false,flowBoundary=false;unsigned flowPosition=0,flowLayers=0;uint64_t retiredRootInstructions=0;
uint64_t ObservedRootInstructions()const{return retiredRootInstructions+steps;}
void ObserveRootLayerWindow(){
 namespace C=PioneerQ38GlobalChainLayout;
 assert(fullFlowRunning && fullLayerComplete && flowLayers<192 && !io.Core(0).pending && steps<4000000000U
  && io.Raw(0,C::Progress+4)==flowLayers/64 && io.Raw(0,C::Progress+8)==flowLayers%64+1);
 const unsigned low=steps&255U;retiredRootInstructions+=steps-low;steps=low;flowLayers++;fullLayerComplete=false;
} // HOST watchdog/scheduler bookkeeping ONLY;all low bits,PC/registers/NoC/tensor/private model state preserved
uint32_t ObserveParent(unsigned field){
 assert(fullFlowRunning && field<64);const unsigned parent=regs[27];
 assert(parent>=0x6e000 && parent<=0x70000-1028);return io.Raw(0,parent+(192+field)*4);
} // read the actual current token parent's stack;never supply/replace a private owner
#endif
#endif
#endif
#include "pioneer_q38_token_native_scratch_scope.inc"
unsigned layerPackets=0,layerRawReads=0,layerPeerReads=0,layerPeerWrites=0,dispatchWorker=0;
uint32_t (*livePeerPC)(TokenHeldEndpoints&,unsigned)=nullptr;
void ResumeLayer(){
 assert(embedding && !layerActive && original && original->Ready() && submitted==28 && completed==27 && io.Core(0).pending);
 assert(livePeerPC);layerActive=true;
}
void SubmitLayer(){
 namespace C=PioneerQ38GlobalChainLayout;namespace W=PioneerQ38SelectedGlobalWorker;
 auto& core=io.Core(0);assert(layerActive && !layerHeld && !core.pending);
 for(unsigned i=0;i<10;i++)request[i]=core.command[i];
 const bool write=request[7]==0x212;const unsigned remote=request[write?3:0],local=request[write?0:3],route=request[write?5:2],bytes=request[8];
 assert(bytes && bytes<=16384 && !((remote|local|bytes)&3) && !request[1] && !request[4] && !request[6] && !request[9]
  && request[write?2:5]==129 && (write || !request[7]) && bytes<=16384-(remote&31));
 const bool regularScratch=local==C::R::Scratch+(remote&31);
#ifdef PIONEER_Q38_TOKEN_NATIVE_FULL_LAYER_FIXTURE
 const bool convScratch=ConvScratchPacket(fullLayerRunning,pc,local,bytes);
 if(!regularScratch && !convScratch)fprintf(stderr,"NATIVE_TEST_SCRATCH_REFUSAL pc%08x/local%08x/remote%08x/bytes%u/family%08x;NO packet submitted\n",pc,local,remote,bytes,unsigned(PioneerQ38TokenPrograms::Family));
 assert(regularScratch || convScratch);
#else
 assert(regularScratch);
#endif
 unsigned worker=1;while(worker<120 && W::Peer(worker)!=route)worker++;
 if(worker<120 && write && remote==W::Doorbell){
  assert(bytes==4);
#ifdef PIONEER_Q38_TOKEN_NATIVE_FULL_LAYER_FIXTURE
  if(fullLayerRunning){layerHeld=false;}
  else
#endif
  {
  assert(io.Raw(worker,W::Job+8)==1);
#ifdef PIONEER_Q38_TOKEN_NATIVE_FIRST_MATRIX_FIXTURE
  if(firstMatrixRunning && io.Raw(worker,W::Job+7*4)==1001){layerHeld=false;}
  else
#endif
  {
  assert(!dispatchWorker);
  dispatchWorker=worker;layerHeld=true; // PAUSE AFTER actual submit,BEFORE delivery;NO worker completion supplied
  }
  }
 }
 core.doorbells++;core.doorbellRead=false;core.requests++;core.pending=true;core.accepted++;submitted++;busyReads=0;layerPackets++;
}
void RespondLayer(){
 namespace C=PioneerQ38GlobalChainLayout;namespace W=PioneerQ38SelectedGlobalWorker;
 auto& core=io.Core(0);assert(layerActive && !layerHeld && core.pending && submitted==completed+1);
 const bool write=request[7]==0x212;const unsigned remote=request[write?3:0],local=request[write?0:3],route=request[write?5:2],bytes=request[8];
 unsigned bank=0;while(bank<7 && PioneerQ38ProjectionRoute(bank)!=route)bank++;
 if(bank<7){
#ifdef PIONEER_Q38_TOKEN_NATIVE_FULL_LAYER_FIXTURE
  assert(io.executionDramRead && io.executionDramPut);
  if(write)for(unsigned i=0;i<bytes/4;i++)assert(io.executionDramPut(io,route,remote+i*4,io.Raw(0,local+i*4)));
  else{std::array<uint8,16384> raw;assert(io.executionDramRead(io,route,remote,raw.data(),bytes));
   for(unsigned i=0;i<bytes/4;i++)io.Raw(0,local+i*4)=uint32(raw[i*4])|uint32(raw[i*4+1])<<8|uint32(raw[i*4+2])<<16|uint32(raw[i*4+3])<<24;
   if(!(bank==0 && remote>=io.runtimeWorkspace && remote-io.runtimeWorkspace<=67108864-bytes))layerRawReads++;
  }
#else
  if(bank==0 && remote>=io.runtimeWorkspace && bytes<=C::WorkspaceBytes && remote-io.runtimeWorkspace<=C::WorkspaceBytes-bytes){
   for(unsigned i=0;i<bytes/4;i++){
    auto& physical=io.runtimeDram.at((remote-io.runtimeWorkspace)/4+i);
    if(write)physical=io.Raw(0,local+i*4);else io.Raw(0,local+i*4)=physical;
   }
  }else{
   assert(!write && bytes<=16384);std::array<uint8_t,16384> raw;
   assert(original && original->Read(route,remote,raw.data(),bytes));
   for(unsigned i=0;i<bytes/4;i++)io.Raw(0,local+i*4)=uint32_t(raw[i*4])|uint32_t(raw[i*4+1])<<8|uint32_t(raw[i*4+2])<<16|uint32_t(raw[i*4+3])<<24;
   layerRawReads++;
  }
#endif
 }else{
  unsigned worker=1;while(worker<120 && W::Peer(worker)!=route)worker++;assert(worker<120 && io.Core(worker).coreReset==0x46800);
#ifdef PIONEER_Q38_TOKEN_NATIVE_FULL_FLOW_FIXTURE
  if(fullFlowRunning && !write && remote==W::Partial && bytes==512){assert(io.observeVocabulary);io.observeVocabulary(io,worker);}
#endif
  if(remote==0xffb121b0 || remote==0xffb13140){
   assert(!write && bytes==4);io.Raw(0,local)=remote==0xffb121b0?io.Core(worker).coreReset:livePeerPC(io,worker);
  }else if(remote>=0xffb20000 && remote<0xffb22000){
   assert(!write && bytes==4);io.Raw(0,local)=io.Core(worker).ResidentNocRead(remote-0xffb20000);
  }else for(unsigned i=0;i<bytes/4;i++){
   const unsigned at=remote+i*4;assert(PioneerQ38TokenL1::SnapshotContains(worker,at));uint32_t word;
   if(write){
    assert(!PioneerQ38TokenPrograms::ProgramWord(worker,at,word));
#ifdef PIONEER_Q38_TOKEN_NATIVE_FIRST_MATRIX_FIXTURE
    if(at==W::Doorbell){
#ifdef PIONEER_Q38_TOKEN_NATIVE_FULL_LAYER_FIXTURE
     if(fullLayerRunning){fullLayerDoorbells++;}
     else
#endif
     {assert(firstMatrixRunning && io.Raw(worker,W::Job+7*4)==1001 && io.Raw(0,local+i*4)==1);firstMatrixDoorbells++;}
    }
#else
    assert(at!=W::Doorbell);
#endif
    io.Raw(worker,at)=io.Raw(0,local+i*4);
   }
   else io.Raw(0,local+i*4)=io.Raw(worker,at);
  }
  if(write)layerPeerWrites++;else layerPeerReads++;
 }
 if(write){core.writeCounters[0]++;core.writeCounters[1]+=(bytes+63)/64;core.writeCounters[3]++;}
 else{core.counters[0]++;core.counters[1]+=(bytes+63)/64;core.counters[3]++;}
 core.pending=false;core.done=true;completed++;
} // byte-only hardware endpoints;no RMS,quantized activation,matrix or ready-state producer
