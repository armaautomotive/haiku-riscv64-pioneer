/* SPDX-License-Identifier: MIT */
// Actual native owner ONLY. One fixed peer slot,no live/arbitrary reads.
template<class IO> __attribute__((noinline)) bool ReadMatrixPhase(IO& io,unsigned worker,uint32_t* output,PioneerQ38TokenMatrixPhaseResult& r){
 namespace Q=PioneerQ38TokenMatrixPhaseData;
 static_assert(PioneerQ38TokenPrograms::MatrixPhase==1
  && unsigned(PioneerQ38TokenPrograms::MatrixPhaseAddress)==unsigned(Q::Address)
  && unsigned(PioneerQ38TokenPrograms::MatrixPhaseWords)==unsigned(Q::Words)
  && unsigned(PioneerQ38TokenPrograms::MatrixPhaseVersion)==unsigned(Q::Version),"matching complete sampler DATA required");
 const PioneerQ38TokenMatrixPhaseResult pristine;
 const auto* a=reinterpret_cast<const uint8_t*>(&r);const auto* b=reinterpret_cast<const uint8_t*>(&pristine);
 for(unsigned i=0;i<sizeof(r);i++)if(a[i]!=b[i])return false;
 if(!worker || worker>=120 || !matrixSlotsInitialized || matrixPhaseRead[worker] || restoring || l1Restored
  || readoutPending || !CommittedRecordsRead() || !Scope(io) || !output || (reinterpret_cast<uintptr_t>(output)&3))return false;
 const uintptr_t out=reinterpret_cast<uintptr_t>(output),result=reinterpret_cast<uintptr_t>(&r);
 const size_t bytes=Q::Words*4;
 if(out>UINTPTR_MAX-bytes || result>UINTPTR_MAX-sizeof(r) || (out<result+sizeof(r) && result<out+bytes)
  || !startup->PublishedOutput(output,bytes) || !DispatchOutput(output,bytes))return false;
 const void* owned[]={this,memory,cohort,banks,proof,canonical,startup,ledger};
 const size_t sizes[]={sizeof(*this),sizeof(*memory),sizeof(*cohort),sizeof(*banks),sizeof(*proof),
  sizeof(*canonical),sizeof(*startup),PioneerResidentBanks::Pages};
 for(unsigned i=0;i<8;i++){
  const uintptr_t first=reinterpret_cast<uintptr_t>(owned[i]);
  if(!first || first>UINTPTR_MAX-sizes[i] || (out<first+sizes[i] && first<out+bytes))return false;
 }
 for(unsigned i=0;i<Q::Words;i++)if(output[i])return false;
 matrixPhaseRead[worker]=true;r.started=1;r.phase=1;
 const uint64_t began=io.Q38CohortNow();uint64_t previous=began;
 if(began>UINT64_MAX-UINT64_C(30000000))return Fail(io,r);
 for(unsigned w=0;w<120;w++){
  r.worker=w;if(!BootTime(io,began,previous) || !HoldGuard(io,w,r))return Fail(io,r);
 }
 uint32_t rootBoot[64],peerBoot[64],snapshot[Q::Words];PioneerQ38TokenReadResult frontier;
 r.worker=worker;PioneerQ38TokenMemoryResult image;
 if(!startup->ReadBoot(0,rootBoot) || !ReadFrontier(io,rootBoot,frontier)
  || !startup->ReadBoot(worker,peerBoot) || !HoldGuard(io,worker,r)
  || !memory->VerifyImage(io,worker,image) || !HoldGuard(io,worker,r))return Fail(io,r);
 for(unsigned i=0;i<64;i++){
  uint32_t actual=0;r.word=i;
  if(!BootTime(io,began,previous) || !io.TokenMemoryReadChecked(worker,PioneerQ38SelectedGlobalWorker::Boot+i*4,actual)
   || actual!=peerBoot[i])return Fail(io,r);
 }
 r.phase=2;
 for(unsigned i=0;i<Q::Words;i++){
  r.word=i;if(!BootTime(io,began,previous) || !HoldGuard(io,worker,r)
   || !io.TokenMemoryReadChecked(worker,Q::Address+i*4,snapshot[i]))return Fail(io,r);
 }
 r.phase=3;
 for(unsigned i=0;i<Q::Words;i++){
  uint32_t actual=0;r.word=i;
  if(!BootTime(io,began,previous) || !HoldGuard(io,worker,r)
   || !io.TokenMemoryReadChecked(worker,Q::Address+i*4,actual) || actual!=snapshot[i])return Fail(io,r);
 }
 if(!memory->VerifyImage(io,worker,image) || !HoldGuard(io,worker,r))return Fail(io,r);
 for(unsigned i=0;i<64;i++){
  uint32_t actual=0;r.word=i;
  if(!BootTime(io,began,previous) || !io.TokenMemoryReadChecked(worker,PioneerQ38SelectedGlobalWorker::Boot+i*4,actual)
   || actual!=peerBoot[i])return Fail(io,r);
 }
 for(unsigned w=0;w<120;w++){
  r.worker=w;if(!BootTime(io,began,previous) || !HoldGuard(io,w,r))return Fail(io,r);
 }
 if(!ReadFrontier(io,rootBoot,frontier) || !BootTime(io,began,previous))return Fail(io,r);
 // Only immutable same-run DATA is published. The STOP nonce is obtained
 // independently from the genuine closed terminal/peer journal. It bounds
 // the reported job nonce;it does NOT predict that exact job. Fresh clearing
 // and the authenticated sole publisher bind the slot to this run. Invalid
 // or absent metric DATA is NOT a model/hardware fault;normal END follows.
 for(unsigned i=0;i<Q::Words;i++)output[i]=snapshot[i];
 r.worker=worker;r.metricState=Q::Classify(snapshot,worker,terminalNonces[worker]);
 r.valid=r.metricState==Q::Intervals;r.nonce=snapshot[2];r.inputs=snapshot[5];r.groups=snapshot[6];
 r.ready=1;r.phase=4;return true;
} // no terminal,cursor,restore,END or Work transition;NO commands,DMA,reset or CPU reference.
