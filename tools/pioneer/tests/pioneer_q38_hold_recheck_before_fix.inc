/* SPDX-License-Identifier: MIT */
// Frozen f4908fa428 DATA for isolated regression,not native authority.
struct PioneerQ38TokenTerminalResult {
 uint32_t version=1,started=0,phase=0,worker=0,word=0,unsafe=0,ready=0,cancelled=0;
 uint32_t committed=0,position=0,completedLayers=0,stage=0,firstPC=0,secondPC=0,reserved[2]={};
};
struct PioneerQ38TokenHoldResult {
 uint32_t version=1,started=0,phase=0,worker=0,word=0,unsafe=0,ready=0,held=0;
 uint32_t firstPC=0,secondPC=0,reserved[6]={};
};
template<class IO> bool BootTime(IO& io,uint64_t began,uint64_t& previous)const{
  if(!Scope(io))return false;
  const uint64_t now=io.Q38CohortNow();
  if(now<previous || now<began || now-began>=UINT64_C(30000000))return false;
  previous=now;return true;
 }
template<class IO> bool HoldStopped(IO& io,PioneerQ38TokenHoldResult& r){
  const PioneerQ38TokenHoldResult pristine;const auto* a=reinterpret_cast<const uint8_t*>(&r);
  const auto* b=reinterpret_cast<const uint8_t*>(&pristine);
  for(unsigned i=0;i<sizeof(r);i++)if(a[i]!=b[i])return false;
  if(holding || held || !TerminalObserved() || !ExecutionStarted() || !Scope(io))return false;
  r.started=1;r.phase=1;PioneerQ38TokenTerminalResult fresh;
  // Re-prove the SAME terminal and every worker in THIS call BEFORE the
  // first HOLD. Cached ready/counters alone can never authorize a reset.
  if(!InspectTerminal(io,fresh,true) || !fresh.ready)return Fail(io,r);
  const uint64_t began=io.Q38CohortNow();uint64_t previous=began;
  if(began>UINT64_MAX-UINT64_C(30000000))return Fail(io,r);
  holding=true;r.phase=2;
  for(unsigned w=0;w<120;w++){
   r.worker=w;
   if(!BootTime(io,began,previous) || !HoldGuard(io,w,r))return Fail(io,r);
   // ROOT first;one known HOLD per core. Mark before uncertain writes so
   // any failure retains the entire cohort without retry/repair/release.
   running[w]=cohort->worker[w].image.running=false;reset[w]=0x47800;
   if(!w)cohort->rootRunning=false;
   io.RuntimeResetWrite(reset[w]);
   if(!BootTime(io,began,previous) || !HoldGuard(io,w,r))return Fail(io,r);
   io.Pause();if(!BootTime(io,began,previous) || !HoldGuard(io,w,r))return Fail(io,r);
   r.held++;
  }
  r.phase=3;
  for(unsigned w=0;w<120;w++){
   r.worker=w;if(running[w] || !BootTime(io,began,previous) || !HoldGuard(io,w,r))return Fail(io,r);
  }
  r.worker=0;
  if(r.held!=120 || !Sources(io,0))return Fail(io,r);
  for(unsigned i=0;i<64;i++){uint32_t actual=0;r.word=i;
   if(!io.TokenMemoryReadChecked(0,PioneerQ38TokenLayout::TerminalLocal+i*4,actual) || actual!=observedTerminal[i])return Fail(io,r);
  }
  if(!HoldGuard(io,0,r))return Fail(io,r);
  held=true;holding=false;r.ready=1;r.phase=4;return true;
 }
