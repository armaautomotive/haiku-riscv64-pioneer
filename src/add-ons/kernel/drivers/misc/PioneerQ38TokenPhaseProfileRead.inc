/* SPDX-License-Identifier: MIT */
// Member of the actual token owner ONLY. No mock-ready state or live reads.
template<class IO> __attribute__((noinline)) bool ReadPhaseProfile(IO& io,uint32_t* output,PioneerQ38TokenPhaseProfileResult& r){
 namespace Q=PioneerQ38TokenPhaseProfileData;
 static_assert(PioneerQ38TokenPrograms::TokenPhaseProfile==1
  && unsigned(PioneerQ38TokenPrograms::PhaseProfileAddress)==unsigned(Q::Address)
  && unsigned(PioneerQ38TokenPrograms::PhaseProfileWords)==unsigned(Q::Words),"actual matching full timing ROOT required");
 const PioneerQ38TokenPhaseProfileResult pristine;
 const auto* a=reinterpret_cast<const uint8_t*>(&r);const auto* b=reinterpret_cast<const uint8_t*>(&pristine);
 for(unsigned i=0;i<sizeof(r);i++)if(a[i]!=b[i])return false;
 if(phaseProfileRead || restoring || l1Restored || readoutPending || !CommittedRecordsRead() || !Scope(io)
  || !output || (reinterpret_cast<uintptr_t>(output)&3))return false;
 const uintptr_t out=reinterpret_cast<uintptr_t>(output),result=reinterpret_cast<uintptr_t>(&r);
 const size_t bytes=64*4;
 if(out>UINTPTR_MAX-bytes || result>UINTPTR_MAX-sizeof(r)
  || (out<result+sizeof(r) && result<out+bytes)
  || !startup->PublishedOutput(output,bytes) || !DispatchOutput(output,bytes))return false;
 const void* owned[]={this,memory,cohort,banks,proof,canonical,startup,ledger};
 const size_t sizes[]={sizeof(*this),sizeof(*memory),sizeof(*cohort),sizeof(*banks),sizeof(*proof),
  sizeof(*canonical),sizeof(*startup),PioneerResidentBanks::Pages};
 for(unsigned i=0;i<8;i++){
  const uintptr_t first=reinterpret_cast<uintptr_t>(owned[i]);
  if(!first || first>UINTPTR_MAX-sizes[i] || (out<first+sizes[i] && first<out+bytes))return false;
 }
 for(unsigned i=0;i<64;i++)if(output[i])return false;
 phaseProfileRead=true;r.started=1;r.phase=1;
 const uint64_t began=io.Q38CohortNow();uint64_t previous=began;
 if(began>UINT64_MAX-UINT64_C(30000000))return Fail(io,r);
 for(unsigned w=0;w<120;w++){
  r.worker=w;if(!BootTime(io,began,previous) || !HoldGuard(io,w,r))return Fail(io,r);
 }
 r.worker=0;uint32_t boot[64],snapshot[Q::Words];PioneerQ38TokenReadResult frontier;
 if(!startup->ReadBoot(0,boot) || !ReadFrontier(io,boot,frontier))return Fail(io,r);
 r.phase=2;
 for(unsigned i=0;i<Q::Words;i++){
  r.word=i;if(!BootTime(io,began,previous) || !HoldGuard(io,0,r)
   || !io.TokenMemoryReadChecked(0,Q::Address+i*4,snapshot[i]))return Fail(io,r);
 }
 r.phase=3;
 for(unsigned i=0;i<Q::Words;i++){
  uint32_t actual=0;r.word=i;
  if(!BootTime(io,began,previous) || !HoldGuard(io,0,r)
   || !io.TokenMemoryReadChecked(0,Q::Address+i*4,actual) || actual!=snapshot[i])return Fail(io,r);
 }
 for(unsigned w=0;w<120;w++){
  r.worker=w;if(!BootTime(io,began,previous) || !HoldGuard(io,w,r))return Fail(io,r);
 }
 r.worker=0;
 if(!ReadFrontier(io,boot,frontier) || !BootTime(io,began,previous))return Fail(io,r);
 // Atomic trusted-buffer publication ONLY after both identical reads and
 // fresh original terminal/boot/counter/source/HOLD seals. Bad metric DATA
 // is NOT a hardware fault or a model result;normal cleanup stays required.
 for(unsigned i=0;i<Q::Words;i++)output[i]=snapshot[i];
 r.committed=observedTerminal[3];r.words=Q::Words;r.metricState=snapshot[Q::State];r.metricError=snapshot[Q::Error];
 r.valid=Q::Closed(snapshot) && snapshot[Q::Committed]==r.committed;
 r.ready=1;r.phase=4;return true;
} // no cursor/terminal/restore/END state change;NO commands,DMA,reset or CPU model reference.
