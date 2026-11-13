/* SPDX-License-Identifier: MIT */
#if !defined(PIONEER_Q38_TOKEN_NATIVE_LAYER_FIXTURE) || PIONEER_Q38_TOKEN_NATIVE_LAYER_FIXTURE != 1
#error Explicit offline native layer test required
#endif
// Members of the SAME virgin ROOT,not another CPU or a completed cohort.
bool layerActive=false,layerHeld=false;
#ifdef PIONEER_Q38_TOKEN_NATIVE_RECURRENT_PRODUCERS_FIXTURE
bool recurrentProducersRunning=false;unsigned recurrentProducerDoorbells=0;
#endif
#ifdef PIONEER_Q38_TOKEN_NATIVE_RECURRENT_MATRIX_COHORT_FIXTURE
bool recurrentMatrixRunning=false;unsigned recurrentMatrixDoorbells=0;
#ifdef PIONEER_Q38_TOKEN_NATIVE_RECURRENT_MATRIX_REUSE_FIXTURE
bool recurrentMatrixReuseRunning=false;
#ifdef PIONEER_Q38_TOKEN_NATIVE_GATE_COHORT_FIXTURE
bool recurrentGateRunning=false;unsigned recurrentGateDoorbells=0,recurrentGateFirst[119]={},recurrentGateBands[119]={};
#ifdef PIONEER_Q38_TOKEN_NATIVE_QKV_COHORT_FIXTURE
bool recurrentQkvRunning=false;unsigned recurrentQkvDoorbells=0,recurrentQkvFirst[119]={},recurrentQkvBands[119]={};
#ifdef PIONEER_Q38_TOKEN_NATIVE_CONV_COHORT_FIXTURE
bool recurrentConvRunning=false;unsigned recurrentConvDoorbells=0;
#ifdef PIONEER_Q38_TOKEN_NATIVE_GDN_COHORT_FIXTURE
bool recurrentGdnRunning=false;unsigned recurrentGdnDoorbells=0;
#ifdef PIONEER_Q38_TOKEN_NATIVE_OUTPUT_PRODUCERS_FIXTURE
bool recurrentOutputRunning=false;unsigned recurrentOutputDoorbells=0;
#endif
#endif
#endif
#ifdef PIONEER_Q38_TOKEN_NATIVE_QKV_CONV_RECEIPT_FAULT_FIXTURE
bool qkvConvFaultInjected=false;unsigned qkvConvWrites=0;
#endif
#endif
#endif
#endif
#endif
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
#elif defined(PIONEER_Q38_TOKEN_NATIVE_CONV_COHORT_FIXTURE)
 const bool convScratch=ConvScratchPacket(recurrentConvRunning,pc,local,bytes);
 if(!regularScratch && !convScratch)fprintf(stderr,"CONV_SCRATCH_REFUSAL pc=%x local=%x remote=%x bytes=%u\n",pc,local,remote,bytes);
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
#ifdef PIONEER_Q38_RECURRENT_KERNEL_COMPILE_ONLY
#ifdef PIONEER_Q38_TOKEN_NATIVE_RECURRENT_PRODUCERS_FIXTURE
#ifdef PIONEER_Q38_TOKEN_NATIVE_RECURRENT_MATRIX_COHORT_FIXTURE
  if(!recurrentMatrixRunning || recurrentMatrixDoorbells<2
#ifdef PIONEER_Q38_TOKEN_NATIVE_RECURRENT_MATRIX_REUSE_FIXTURE
   || (recurrentMatrixReuseRunning && recurrentMatrixDoorbells<4)
#ifdef PIONEER_Q38_TOKEN_NATIVE_GATE_COHORT_FIXTURE
   || (recurrentGateRunning && recurrentGateDoorbells<119)
#ifdef PIONEER_Q38_TOKEN_NATIVE_QKV_COHORT_FIXTURE
   || (recurrentQkvRunning && recurrentQkvDoorbells<119)
#endif
#endif
#endif
  )
#endif
  assert(io.Raw(worker,W::Job+8)==(recurrentProducersRunning && recurrentProducerDoorbells==20?1U:5U));
#ifdef PIONEER_Q38_TOKEN_NATIVE_CONV_COHORT_FIXTURE
  if(recurrentConvRunning && recurrentConvDoorbells<119)assert(io.Raw(worker,W::Job+8)==7);
#ifdef PIONEER_Q38_TOKEN_NATIVE_GDN_COHORT_FIXTURE
  if(recurrentGdnRunning && recurrentGdnDoorbells<96)assert(io.Raw(worker,W::Job+8)==8);
#ifdef PIONEER_Q38_TOKEN_NATIVE_OUTPUT_PRODUCERS_FIXTURE
  if(recurrentOutputRunning && recurrentOutputDoorbells<24)assert(io.Raw(worker,W::Job+8)==5);
#endif
#endif
#endif
  if(recurrentProducersRunning && recurrentProducerDoorbells<20){layerHeld=false;}
#ifdef PIONEER_Q38_TOKEN_NATIVE_RECURRENT_MATRIX_COHORT_FIXTURE
  else if(recurrentMatrixRunning && recurrentMatrixDoorbells<2){layerHeld=false;}
#ifdef PIONEER_Q38_TOKEN_NATIVE_RECURRENT_MATRIX_REUSE_FIXTURE
  else if(recurrentMatrixReuseRunning && recurrentMatrixDoorbells<4){layerHeld=false;}
#ifdef PIONEER_Q38_TOKEN_NATIVE_GATE_COHORT_FIXTURE
  else if(recurrentGateRunning && recurrentGateDoorbells<119){layerHeld=false;}
#ifdef PIONEER_Q38_TOKEN_NATIVE_QKV_COHORT_FIXTURE
  else if(recurrentQkvRunning && recurrentQkvDoorbells<119){layerHeld=false;}
#ifdef PIONEER_Q38_TOKEN_NATIVE_CONV_COHORT_FIXTURE
  else if(recurrentConvRunning && recurrentConvDoorbells<119){layerHeld=false;}
#ifdef PIONEER_Q38_TOKEN_NATIVE_GDN_COHORT_FIXTURE
  else if(recurrentGdnRunning && recurrentGdnDoorbells<96){layerHeld=false;}
#ifdef PIONEER_Q38_TOKEN_NATIVE_OUTPUT_PRODUCERS_FIXTURE
  else if(recurrentOutputRunning && recurrentOutputDoorbells<24){layerHeld=false;}
#endif
#endif
#endif
#endif
#endif
#endif
#endif
  else
#else
  assert(io.Raw(worker,W::Job+8)==5); // QTPM stages the input-reuse producer first.
#endif
#else
  assert(io.Raw(worker,W::Job+8)==1);
#endif
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
#if defined(PIONEER_Q38_TOKEN_NATIVE_FULL_LAYER_FIXTURE) || defined(PIONEER_Q38_TOKEN_NATIVE_GDN_COHORT_FIXTURE)
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
#ifdef PIONEER_Q38_TOKEN_NATIVE_CONV_COHORT_FIXTURE
   if(write){assert(original);for(unsigned i=0;i<bytes/4;i++)assert(original->PutSyntheticConv(route,remote+i*4,io.Raw(0,local+i*4)));}
   else
#else
   assert(!write);
#endif
   {assert(bytes<=16384);std::array<uint8_t,16384> raw;
   assert(original && original->Read(route,remote,raw.data(),bytes));
   for(unsigned i=0;i<bytes/4;i++)io.Raw(0,local+i*4)=uint32_t(raw[i*4])|uint32_t(raw[i*4+1])<<8|uint32_t(raw[i*4+2])<<16|uint32_t(raw[i*4+3])<<24;
   layerRawReads++;}
  }
#endif
 }else{
  unsigned worker=1;while(worker<120 && W::Peer(worker)!=route)worker++;assert(worker<120 && io.Core(worker).coreReset==0x46800);
#ifdef PIONEER_Q38_TOKEN_NATIVE_QKV_CONV_RECEIPT_FAULT_FIXTURE
  const bool convPhase=recurrentQkvDoorbells==119 && io.Raw(0,C::R::Status+20)==6;
  if(convPhase && write && (remote==W::Job || remote==0x74800))qkvConvWrites++;
#endif
#ifdef PIONEER_Q38_TOKEN_NATIVE_FULL_FLOW_FIXTURE
  if(fullFlowRunning && !write && remote==W::Partial && bytes==512){assert(io.observeVocabulary);io.observeVocabulary(io,worker);}
#endif
  if(remote==0xffb121b0 || remote==0xffb13140){
   assert(!write && bytes==4);io.Raw(0,local)=remote==0xffb121b0?io.Core(worker).coreReset:livePeerPC(io,worker);
  }else if(remote>=0xffb20000 && remote<0xffb22000){
   assert(!write && bytes==4);uint32_t value=io.Core(worker).ResidentNocRead(remote-0xffb20000);
#if defined(PIONEER_Q38_TOKEN_NATIVE_QKV_CONV_RECEIPT_FAULT_FIXTURE) && PIONEER_Q38_TOKEN_NATIVE_QKV_CONV_FAULT_KIND == 3
   if(convPhase && worker==1 && remote==0xffb20208 && !qkvConvFaultInjected){value^=1;qkvConvFaultInjected=true;}
#endif
   io.Raw(0,local)=value;
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
#elif defined(PIONEER_Q38_TOKEN_NATIVE_RECURRENT_PRODUCER_FIXTURE)
    if(at==W::Doorbell){
#ifdef PIONEER_Q38_TOKEN_NATIVE_RECURRENT_PRODUCERS_FIXTURE
     if(io.Raw(worker,W::Job+8)==5
#ifdef PIONEER_Q38_TOKEN_NATIVE_OUTPUT_PRODUCERS_FIXTURE
       && !(recurrentOutputRunning && recurrentOutputDoorbells<24)
#endif
     ){
      assert(recurrentProducersRunning && worker==recurrentProducerDoorbells+1 && worker<=20 && io.Raw(0,local+i*4)==1);recurrentProducerDoorbells++;
     }
#ifdef PIONEER_Q38_TOKEN_NATIVE_RECURRENT_MATRIX_FIXTURE
#ifdef PIONEER_Q38_TOKEN_NATIVE_RECURRENT_MATRIX_COHORT_FIXTURE
#ifdef PIONEER_Q38_TOKEN_NATIVE_GATE_COHORT_FIXTURE
#ifdef PIONEER_Q38_TOKEN_NATIVE_QKV_COHORT_FIXTURE
#ifdef PIONEER_Q38_TOKEN_NATIVE_CONV_COHORT_FIXTURE
#ifdef PIONEER_Q38_TOKEN_NATIVE_GDN_COHORT_FIXTURE
#ifdef PIONEER_Q38_TOKEN_NATIVE_OUTPUT_PRODUCERS_FIXTURE
     else if(recurrentOutputRunning && recurrentOutputDoorbells<24){
      const unsigned w=recurrentOutputDoorbells,seq=w<2?8:w<20?6:5;
      assert(worker==w+1 && io.Raw(0,local+i*4)==seq && io.Raw(worker,W::Job+8)==5);recurrentOutputDoorbells++;
     }
#endif
     else if(recurrentGdnRunning && recurrentGdnDoorbells<96){
      assert(worker==recurrentGdnDoorbells+1 && io.Raw(worker,W::Job+8)==8);recurrentGdnDoorbells++;
     }
#endif
     else if(recurrentConvRunning && recurrentConvDoorbells<119){
      const unsigned w=recurrentConvDoorbells,seq=w<2?6:w<20?4:3;
      assert(worker==w+1 && io.Raw(0,local+i*4)==seq && io.Raw(worker,W::Job+8)==7);
      recurrentConvDoorbells++;
     }
#endif
     else if(recurrentQkvRunning && recurrentQkvDoorbells<119){
      const unsigned w=recurrentQkvDoorbells,seq=w<2?5:w<20?3:2;
      assert(worker==w+1 && io.Raw(0,local+i*4)==seq && io.Raw(worker,W::Job+8)==1
       && io.Raw(worker,W::Job+34*4)==6 && io.Raw(worker,W::Job+33*4)==10240);
      recurrentQkvFirst[w]=io.Raw(worker,W::Job+32*4);recurrentQkvBands[w]=io.Raw(worker,W::Job+20*4);
      assert(recurrentQkvBands[w] && recurrentQkvBands[w]<=8 && recurrentQkvFirst[w]<10240
       && recurrentQkvBands[w]*32<=10240-recurrentQkvFirst[w]);recurrentQkvDoorbells++;
     }
#endif
     else if(recurrentGateRunning && recurrentGateDoorbells<119){
      const unsigned w=recurrentGateDoorbells,seq=w<2?4:w<20?2:1;
      assert(worker==w+1 && io.Raw(0,local+i*4)==seq && io.Raw(worker,W::Job+8)==1
       && io.Raw(worker,W::Job+34*4)==5 && io.Raw(worker,W::Job+33*4)==6144);
      recurrentGateFirst[w]=io.Raw(worker,W::Job+32*4);recurrentGateBands[w]=io.Raw(worker,W::Job+20*4);
      assert(recurrentGateBands[w] && recurrentGateBands[w]<=8 && recurrentGateFirst[w]<6144
       && recurrentGateBands[w]*32<=6144-recurrentGateFirst[w]);recurrentGateDoorbells++;
     }
#endif
     else {assert(recurrentMatrixRunning && recurrentProducerDoorbells==20
#ifdef PIONEER_Q38_TOKEN_NATIVE_RECURRENT_MATRIX_REUSE_FIXTURE
      && (recurrentMatrixDoorbells<2 || (recurrentMatrixReuseRunning && recurrentMatrixDoorbells<4))
#else
      && recurrentMatrixDoorbells<2
#endif
      && worker==recurrentMatrixDoorbells%2+1 && io.Raw(0,local+i*4)==2+recurrentMatrixDoorbells/2 && io.Raw(worker,W::Job+8)==1);recurrentMatrixDoorbells++;}
#else
     else assert(recurrentProducersRunning && recurrentProducerDoorbells==20 && worker==1
      && io.Raw(0,local+i*4)==2 && io.Raw(worker,W::Job+8)==1);
#endif
#else
     else assert(false && "matrix doorbell must remain pending");
#endif
#else
     assert(worker==1 && io.Raw(0,local+i*4)==1 && io.Raw(worker,W::Job+8)==5);
#endif
    }
#else
    assert(at!=W::Doorbell);
#endif
    io.Raw(worker,at)=io.Raw(0,local+i*4);
   }
   else {
    uint32_t value=io.Raw(worker,at);
#ifdef PIONEER_Q38_TOKEN_NATIVE_QKV_CONV_RECEIPT_FAULT_FIXTURE
    if(convPhase && worker==1 && !qkvConvFaultInjected
     && ((PIONEER_Q38_TOKEN_NATIVE_QKV_CONV_FAULT_KIND==1 && remote==0x83f00 && at==0x83f00+54*4)
      || (PIONEER_Q38_TOKEN_NATIVE_QKV_CONV_FAULT_KIND==2 && remote==W::Job && at==W::Job+7*4))){
     value^=PIONEER_Q38_TOKEN_NATIVE_QKV_CONV_FAULT_KIND==1?32U:1U;
     qkvConvFaultInjected=true; // corrupt only this test transport reply
    }
#endif
    io.Raw(0,local+i*4)=value;
   }
  }
  if(write)layerPeerWrites++;else layerPeerReads++;
 }
 if(write){core.writeCounters[0]++;core.writeCounters[1]+=(bytes+63)/64;core.writeCounters[3]++;}
 else{core.counters[0]++;core.counters[1]+=(bytes+63)/64;core.counters[3]++;}
 core.pending=false;core.done=true;completed++;
} // byte-only hardware endpoints;no RMS,quantized activation,matrix or ready-state producer
