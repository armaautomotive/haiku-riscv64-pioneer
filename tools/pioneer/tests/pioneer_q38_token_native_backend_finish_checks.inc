/* SPDX-License-Identifier: MIT */
#include "pioneer_q38_token_native_backend_finish_role.h"
#ifdef PIONEER_Q38_TOKEN_NATIVE_CONTROL_FINISH_FIXTURE
#include "pioneer_q38_token_native_control_finish_checks.inc"
#endif
static void CheckNativeBackendFinishUndrained(Fixture& f){
 auto& c=*f.c;auto& io=f.io;auto* state=c.q38TokenNative;
 const unsigned before=io.touches,pci=io.nativePci.events,arc=c.q38ClockIO.rawEvents;
 assert(!lockHeld && state->TerminalHeld() && !state->OriginalL1Restored());
#ifdef PIONEER_Q38_TOKEN_NATIVE_CONTROL_FINISH_FIXTURE
 status_t status;const auto result=CheckNativeControlFinishCall(f,false,status);assert(status==B_NOT_ALLOWED);
#else
 const auto result=FinishQ38TokenNativeBackend(&f.cookie);
#endif
 assert(result.started==1 && result.stage==1 && !result.ready && !result.complete && !result.unsafe
  && io.touches==before && io.nativePci.events==pci && c.q38ClockIO.rawEvents==arc
  && state->TerminalHeld() && !state->OriginalL1Restored() && !state->Unsafe() && !state->Closed());
 puts("PASS composite Finish refuses actual terminal/HOLD without a drained frontier;NO IO/restore/release/phase change or fabricated drain");
}
static void CheckNativeBackendFinish(Fixture& f){
 namespace L=PioneerQ38TokenL1;
 auto& c=*f.c;auto& io=f.io;auto* state=c.q38TokenNative;
 // The parent just executed the real typed ReadCommitted and asserted its
 // drained result. Finish independently checks its private drain; no friend
 // or new private-state getter is granted to this const TEST observer.
 assert(!lockHeld && !io.nativeEndObserved && state->TerminalHeld()
  && !state->OriginalL1Restored() && !state->Unsafe() && !state->Closed()
  && io.executionCpu->fullLayerRunning && !io.executionCpu->fullLayerComplete && io.executionCpu->pc==0x60004 && io.executionCancelWrites==1
  && io.Raw(0,PioneerQ38TokenLayout::TerminalLocal+6*4)==1
  && io.beforeNativeImage.size()==L::SavedWords && io.beforeDispatchImage.size()==120U*(TT_Q38_PANEL_VIEW_WORDS+1U)
  && io.beforePeerScratch.size()==119U*2048 && io.nativeViewDeletes.empty() && io.nativeHostDeletes.empty());
 const unsigned packets=io.runtimePackets,rootPackets=io.executionCpu->layerPackets,steps=io.executionCpu->steps;
 std::array<unsigned,120> resets;for(unsigned w=0;w<120;w++)resets[w]=io.Core(w).resets.size();
 const PioneerQ38TokenBackendResult pristine;
 f.cookie.magic^=1;const auto denied=FinishQ38TokenNativeBackend(&f.cookie);f.cookie.magic^=1;
 assert(!memcmp(&denied,&pristine,sizeof(denied)) && !io.nativeUnreserveCalls);
 io.nativeEndObserved=true;
#ifdef PIONEER_Q38_TOKEN_NATIVE_CONTROL_OPERATIONS_FIXTURE
 assert(!tokenControlOperationCounts[1] && tokenControlOperationCounts[2]==2 && tokenControlOperationCounts[3]==1
  && tokenControlOperationCounts[4]==1 && tokenControlOperationCounts[5]==1 && !tokenControlOperationCounts[6]);
#endif
#ifdef PIONEER_Q38_TOKEN_NATIVE_CONTROL_FINISH_FIXTURE
 status_t status;const bool lostReply=tokenControlFinishLoseReply;
 const auto result=CheckNativeControlFinishCall(f,lostReply,status);assert(status==(lostReply?-6:B_OK));
#else
 const bool lostReply=false;
 const auto result=FinishQ38TokenNativeBackend(&f.cookie);
#endif
 // Successful Finish actually deletes c+state. No dereference after this call.
 if(!f.cookie.context)(void)f.c.release();
 if(!lostReply && !result.complete)fprintf(stderr,"BACKEND_FINISH_DIAGNOSTIC stage%u started%u ready%u unsafe%u detail%x:%x:%x:%x;NO completion supplied\n",
  result.stage,result.started,result.ready,result.unsafe,result.detail[0],result.detail[1],result.detail[2],result.detail[3]);
 assert(lostReply || (result.started==1 && result.stage==11 && result.ready==1 && result.complete==1 && !result.unsafe));
 assert(!f.c && !f.cookie.context && !sRuntimeOwner && !sBankStorageOnly && !io.q38MutableContext && !sColdRecoveryRequired
  && !lockHeld && lockAcquires==lockReleases && io.nativeUnreserveCalls==1 && io.nativeViewDeletes.size()==29
  && (io.nativeHostDeletes==std::vector<unsigned>{2,1,0}) && io.nativePci.writes==22
  && io.runtimePackets==packets && io.executionCpu->layerPackets==rootPackets && io.executionCpu->steps==steps
  && !io.HostReadMock::bmeWrites);
 // Compare earliest observed original bytes, accounting for the real ordered
 // overlapping journals. Expectations are read-only and never seed a restore.
 for(unsigned w=0;w<120;w++){
  assert(io.Core(w).coreReset==0x47800 && io.Core(w).resets.size()==resets[w]);
  const unsigned dispatch=w*(TT_Q38_PANEL_VIEW_WORDS+1U);
  for(unsigned i=0;i<L::SnapshotWords(w);i++){
   const unsigned address=L::SnapshotAddress(w,i);uint32 expected=io.beforeNativeImage[L::SnapshotBase(w)+i];
   if(address==0x6000)expected=io.beforeDispatchImage[dispatch];
   else if(address>=TT_Q38_PANEL_VIEW_BASE && address-TT_Q38_PANEL_VIEW_BASE<TT_Q38_PANEL_VIEW_WORDS*4U)
    expected=io.beforeDispatchImage[dispatch+1+(address-TT_Q38_PANEL_VIEW_BASE)/4];
   else if(address>=0x17e000 && address<0x180000)
    expected=w?io.beforePeerScratch[(w-1)*2048+(address-0x17e000)/4]:io.beforeRootScratch[(address-0x17e000)/4];
   assert(io.Raw(w,address)==expected);
  }
  assert(io.Core(w).config==(w?io.beforePeerConfig[w-1]:io.beforeRootConfig));
  for(unsigned i=0;i<10;i++)assert(!io.Core(w).command[i]);
  for(unsigned i=0;i<2048;i++)assert(io.Raw(w,0x17e000+i*4)==(w?io.beforePeerScratch[(w-1)*2048+i]:io.beforeRootScratch[i]));
 }
 for(unsigned s=0;s<2;s++)for(unsigned word=0;word<3;word++)assert(io.Core(0).tlb[s][word]==io.beforeSharedTlb[s][word]);
 const unsigned touches=io.touches,pci=io.nativePci.events;
 const auto again=FinishQ38TokenNativeBackend(&f.cookie);
 assert(!memcmp(&again,&pristine,sizeof(again)) && io.touches==touches && io.nativePci.events==pci && io.nativeUnreserveCalls==1);
#ifdef PIONEER_Q38_TOKEN_NATIVE_CONTROL_FINISH_FIXTURE
 printf("EXECUTED private control Finish/%s/actual11 cleanup stages and locked whole-context deletion independently verified/NO post-delete dereference,IO,retention,retry or double release;NO supplied user completion/positive token/full-model/physical/Work/TPS grant\n",
  lostReply?"REPLY COPY FAILED AFTER context deletion;user reply unchanged":"genuine completed reply copied under SAME real scope lock");
#endif
 puts("EXECUTED composite Finish after SAME genuine original firstlayer/cancel/ROOT+119STOP/120HOLD/zero-commit drain:ALL11 real restoration/clock/hardware/29VM/PCI/3host/unreserve/context-delete stages;earliest originals independently exact;NO reset,retry,UAF,CPU products or supplied cleanup authority;SIMULATED endpoints,NOT positive token/full-model/physical/Work/TPS qualification");
}
