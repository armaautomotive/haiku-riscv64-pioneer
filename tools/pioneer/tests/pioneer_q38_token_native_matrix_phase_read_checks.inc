/* SPDX-License-Identifier: MIT */
#if !defined(PIONEER_Q38_TOKEN_NATIVE_MATRIX_PHASE_READ_FIXTURE) || PIONEER_Q38_TOKEN_NATIVE_MATRIX_PHASE_READ_FIXTURE != 1 \
 || !defined(PIONEER_Q38_TOKEN_NATIVE_MATRIX_PHASE_FIXTURE) || PIONEER_Q38_TOKEN_NATIVE_MATRIX_PHASE_FIXTURE != 1 \
 || !defined(PIONEER_Q38_TOKEN_MATRIX_PHASE_READ_NATIVE) || PIONEER_Q38_TOKEN_MATRIX_PHASE_READ_NATIVE != 1 \
 || !defined(PIONEER_Q38_TOKEN_NATIVE_READOUT_EMPTY_FIXTURE) || PIONEER_Q38_TOKEN_NATIVE_READOUT_EMPTY_FIXTURE != 1 \
 || defined(PIONEER_Q38_TOKEN_NATIVE_FULL_FLOW_FIXTURE)
#error Exact fresh-slot reader TEST requires SAME original-model first-layer/cancel/HOLD/drain/full-END predecessor
#endif
#include "../PioneerQ38TokenMatrixPhaseReadRole.h"
#include <cerrno>
#include <sys/wait.h>
#include <unistd.h>
static void CheckNativeMatrixPhaseFaults(Fixture& f){
 namespace Q=PioneerQ38TokenMatrixPhaseData;
 using IO=TokenHeldEndpoints;
 auto& c=*f.c;auto& io=f.io;auto* state=c.q38TokenNative;
 assert(!lockHeld && state->TerminalHeld()
  && !state->Unsafe() && !io.matrixFaultResult && !io.matrixFaultSeen);
 const unsigned originalTouches=io.touches,packets=io.runtimePackets,resets=io.resets.size();
 const auto generation=sReloadGeneration;unsigned faults=0,dataCases=0;
 auto run=[&](IO::MatrixFault fault,unsigned phase,unsigned worker,unsigned address,unsigned dataCase){
  // fork is a negative-test-only COW branch of this GENUINELY earned HOLD
  // history. No copied object is transplanted or accepted as a positive
  // native/physical run. The parent alone continues normal full END.
  fflush(nullptr);const pid_t child=fork();assert(child>=0);
  if(!child){
   PioneerQ38TokenMatrixPhaseResult result;uint32_t output[Q::Words]={};
   io.matrixFaultResult=&result;io.matrixFault=fault;io.matrixFaultPhase=phase;
   io.matrixFaultWorker=worker;io.matrixFaultAddress=address;
   if(dataCase){
    // Deliberately fabricated NEGATIVE metric DATA,never a model product,
    // owner receipt or a claimed authentic current-worker publication.
    if(dataCase==1)for(unsigned i=0;i<Q::Words;i++)io.Raw(worker,Q::Address+i*4)=0;
    else if(dataCase==2){io.Raw(worker,Q::Address+7*4)=0;for(unsigned i=8;i<14;i++)io.Raw(worker,Q::Address+i*4)=0;}
    else if(dataCase==3)io.Raw(worker,Q::Address)^=1;
    else{assert(dataCase==4);for(unsigned i=8;i<14;i++)io.Raw(worker,Q::Address+i*4)=UINT32_MAX;}
   }
   bool okay;
   {PioneerQ38TokenCallScope scope(&f.cookie);assert(scope.Context()==&c);
    okay=ReadQ38TokenNativeMatrixPhase(c,worker,output,result);}
   if(dataCase){
    const unsigned expected=dataCase==1?Q::Missing:dataCase==2?Q::InvalidClock:Q::Malformed;
    assert(okay && result.started && result.ready && !result.unsafe && !result.valid
     && result.metricState==expected && state->TerminalHeld() && !state->Unsafe() && !sColdRecoveryRequired);
    for(unsigned i=0;i<Q::Words;i++)assert(output[i]==io.Raw(worker,Q::Address+i*4));
    assert(!io.matrixFaultSeen);
   }else{
    if(okay || !io.matrixFaultSeen || io.touches!=io.matrixFaultTouches)
     fprintf(stderr,"MATRIX_READER_FAULT failure kind%u phase%u requestedWorker%u address%x observed%u touches%u/%u ready%u unsafe%u\n",
      unsigned(fault),phase,worker,address,io.matrixFaultSeen,io.touches,io.matrixFaultTouches,result.ready,result.unsafe);
    assert(!okay && result.started && result.unsafe && !result.ready && !result.valid
     && io.matrixFaultSeen==1 && io.touches==io.matrixFaultTouches);
    for(auto word:output)assert(!word);
    f.Unknown(); // actual owner+memory+bank+host+clock retention;NO following IO
    const unsigned after=io.touches;
    {PioneerQ38TokenCallScope scope(&f.cookie);assert(!scope.Context());
     PioneerQ38TokenMatrixPhaseResult again;PioneerQ38TokenRestoreResult restore;
     assert(!ReadQ38TokenNativeMatrixPhase(c,worker,output,again)
      && !ReadQ38TokenNativeMatrixPhase(c,worker==1?119:1,output,again)
      && !RestoreQ38TokenNativeL1(c,restore) && io.touches==after);}
   }
   assert(io.runtimePackets==packets && io.resets.size()==resets);
   _exit(0); // deliberate negative snapshot disposal,NOT driver Close/END proof
  }
  int status=0;pid_t got;do{got=waitpid(child,&status,0);}while(got<0 && errno==EINTR);
  assert(got==child && WIFEXITED(status) && WEXITSTATUS(status)==0);
  assert(io.touches==originalTouches && io.runtimePackets==packets && io.resets.size()==resets
   && sReloadGeneration==generation && state->TerminalHeld() && !state->Unsafe());
  if(dataCase)dataCases++;else faults++;
 };
 for(unsigned phase:{2U,3U})for(unsigned word=0;word<Q::Words;word++)
  run(IO::MatrixGeneration,phase,1,Q::Address+word*4,0);
 for(unsigned word=0;word<Q::Words;word++)run(IO::MatrixChangedWord,3,1,Q::Address+word*4,0);
 for(unsigned worker:{60U,119U})for(unsigned word:{0U,Q::Words-1U}){
  for(unsigned phase:{2U,3U})run(IO::MatrixGeneration,phase,worker,Q::Address+word*4,0);
  run(IO::MatrixChangedWord,3,worker,Q::Address+word*4,0);
 }
 for(unsigned phase:{1U,3U})for(unsigned word:{0U,63U})
  run(IO::MatrixChangedWord,phase,119,PioneerQ38SelectedGlobalWorker::Boot+word*4,0);
 for(unsigned phase:{1U,3U})for(unsigned word:{0U,PioneerQ38TokenPrograms::CodeWords(119)-1U})
  run(IO::MatrixChangedWord,phase,119,PioneerQ38TokenPrograms::CodeAddress(119,word),0);
 run(IO::MatrixOverflow,1,1,0,0);
 for(unsigned phase:{1U,2U,3U})for(auto fault:{IO::MatrixBackwards,IO::MatrixDeadline})run(fault,phase,1,0,0);
 for(unsigned phase:{2U,3U})for(auto fault:{IO::MatrixSource,IO::MatrixCounter,IO::MatrixFifo,IO::MatrixFifoOverflow,IO::MatrixMapping})
  run(fault,phase,119,0,0);
 for(unsigned dataCase=1;dataCase<=4;dataCase++)run(IO::MatrixNone,0,119,0,dataCase);
 printf("EXECUTED %u negative HOST-only COW branches AFTER genuine original-model/cancel/119STOP/120HOLD/drain: generation loss at ALL16 words/BOTH passes,changed second reads,first/middle/last peers,boot+code changes,backwards/deadline/overflow clocks,source+counter+FIFO+mapping failures retain actual owners/output unpublished/NO following IO,reset,packet,retry,restore;plus%u deliberately invalid metric DATA cases remain nonfatal;parent unmodified and full END follows;NOT physical fault,authentic metrics,whole-model,Work or TPS qualification\n",faults,dataCases);
}
static void CheckNativeMatrixPhaseUndrained(Fixture& f){
 auto& c=*f.c;auto& io=f.io;const unsigned before=io.touches;
 PioneerQ38TokenMatrixPhaseResult r;uint32_t output[16]={};
 PioneerQ38TokenCallScope scope(&f.cookie);assert(scope.Context()==&c);
 assert(!ReadQ38TokenNativeMatrixPhase(c,1,output,r) && io.touches==before && !r.started && !r.unsafe);
}
static void CheckNativeMatrixPhaseRead(Fixture& f){
 namespace Q=PioneerQ38TokenMatrixPhaseData;
 auto& c=*f.c;auto& io=f.io;auto* state=c.q38TokenNative;
 assert(!lockHeld && state->TerminalHeld() && !state->Closed() && !state->Unsafe());
 CheckNativeMatrixPhaseFaults(f);
 const unsigned packets=io.runtimePackets,resets=io.resets.size();unsigned intervals=0,missing=0,invalid=0;
 uint32_t output[Q::Words]={};PioneerQ38TokenMatrixPhaseResult r;unsigned before=io.touches;
 assert(!ReadQ38TokenNativeMatrixPhase(c,1,output,r) && io.touches==before);
 {
  PioneerQ38TokenCallScope scope(&f.cookie);assert(scope.Context()==&c);
  const auto pristine=r;auto* fields=reinterpret_cast<uint32_t*>(&r);
  for(unsigned i=0;i<16;i++){
   fields[i]^=1;assert(!ReadQ38TokenNativeMatrixPhase(c,1,output,r) && io.touches==before);r=pristine;
  }
  for(unsigned worker:{0U,120U,UINT32_MAX})assert(!ReadQ38TokenNativeMatrixPhase(c,worker,output,r) && io.touches==before);
  assert(!ReadQ38TokenNativeMatrixPhase(c,1,nullptr,r) && io.touches==before);
  for(void* alias:{static_cast<void*>(&c),static_cast<void*>(&io),static_cast<void*>(state),static_cast<void*>(&r)})
   assert(!ReadQ38TokenNativeMatrixPhase(c,1,static_cast<uint32_t*>(alias),r) && io.touches==before);
  auto* misaligned=reinterpret_cast<uint32_t*>(reinterpret_cast<uint8_t*>(output)+1);
  assert(!ReadQ38TokenNativeMatrixPhase(c,1,misaligned,r) && io.touches==before);
  output[7]=1;assert(!ReadQ38TokenNativeMatrixPhase(c,1,output,r) && io.touches==before);output[7]=0;
  for(unsigned worker=1;worker<120;worker++){
   uint32_t observed[Q::Words];for(unsigned i=0;i<Q::Words;i++)observed[i]=io.Raw(worker,Q::Address+i*4);
   const unsigned stop=io.Raw(0,PioneerQ38GlobalChainLayout::R::Nonces+(worker-1)*4);
   const unsigned expected=Q::Classify(observed,worker,stop);
   assert(expected!=Q::Malformed); // actual known publisher,not supplied metric DATA
   const bool okay=ReadQ38TokenNativeMatrixPhase(c,worker,output,r);
   if(!okay)fprintf(stderr,"NATIVE_MATRIX_PHASE_READ_REFUSAL requested%u phase%u worker%u word%u unsafe%u;NO ready or metric supplied\n",
    worker,r.phase,r.worker,r.word,r.unsafe);
   assert(okay && r.started && r.ready && !r.unsafe && r.phase==4 && r.worker==worker
    && r.metricState==expected && r.valid==(expected==Q::Intervals)
    && r.nonce==observed[2] && r.inputs==observed[5] && r.groups==observed[6]);
   for(unsigned i=0;i<Q::Words;i++)assert(output[i]==observed[i]);
   intervals+=expected==Q::Intervals;missing+=expected==Q::Missing;invalid+=expected==Q::InvalidClock;
   const unsigned after=io.touches;PioneerQ38TokenMatrixPhaseResult again;uint32_t untouched[Q::Words]={};
   assert(!ReadQ38TokenNativeMatrixPhase(c,worker,untouched,again) && io.touches==after);
   for(auto& word:output)word=0;r=pristine;
  }
 }
 assert(intervals && intervals+missing+invalid==119 && state->TerminalHeld() && !state->Closed() && !state->Unsafe()
  && io.runtimePackets==packets && io.resets.size()==resets);
 printf("EXECUTED SAME native original851/first-layer/cancel/119STOP/120HOLD/real zero-record drain/fresh prelaunch slots/ALL119 real held+doublechecked16word samples+same complete code+boot/source/counter seals/intervals%u missing%u invalidClock%u/NO native lifecycle transition,NoC packet,reset or invented latest-job identity;normal full-END follows;clock+OS endpoints FABRICATED,NOT physical timing,whole-model,Work or TPS\n",intervals,missing,invalid);
}
