/* SPDX-License-Identifier: MIT */
#if !defined(PIONEER_Q38_TOKEN_NATIVE_RESTORE_DISPATCH_FIXTURE) || PIONEER_Q38_TOKEN_NATIVE_RESTORE_DISPATCH_FIXTURE != 1 \
 || !defined(PIONEER_Q38_TOKEN_NATIVE_RESTORE_EMPTY_FIXTURE) || PIONEER_Q38_TOKEN_NATIVE_RESTORE_EMPTY_FIXTURE != 1 \
 || !defined(PIONEER_Q38_TOKEN_NATIVE_READOUT_EMPTY_FIXTURE) || PIONEER_Q38_TOKEN_NATIVE_READOUT_EMPTY_FIXTURE != 1 \
 || !defined(PIONEER_Q38_TOKEN_NATIVE_HOLD_LAYER_FIXTURE) || PIONEER_Q38_TOKEN_NATIVE_HOLD_LAYER_FIXTURE != 1 \
 || !defined(PIONEER_Q38_TOKEN_NATIVE_CANCEL_LAYER_FIXTURE) || PIONEER_Q38_TOKEN_NATIVE_CANCEL_LAYER_FIXTURE != 1 \
 || !defined(PIONEER_Q38_TOKEN_NATIVE_FULL_LAYER_FIXTURE) || PIONEER_Q38_TOKEN_NATIVE_FULL_LAYER_FIXTURE != 1 \
 || !defined(PIONEER_Q38_TOKEN_NATIVE_PEER_FIXTURE) || PIONEER_Q38_TOKEN_NATIVE_PEER_FIXTURE != 1 \
 || !defined(PIONEER_Q38_TOKEN_CONTEXT_COMPILE_ONLY) || PIONEER_Q38_TOKEN_CONTEXT_COMPILE_ONLY != 1 \
 || defined(PIONEER_Q38_TOKEN_NATIVE_FULL_FLOW_FIXTURE) || defined(PIONEER_Q38_WORK_INFERENCE)
#error Explicit native earlier-dispatch restoration TEST required
#endif
#ifdef PIONEER_Q38_TOKEN_NATIVE_RESTORE_PEERS_FIXTURE
#include "pioneer_q38_token_native_peer_restore_checks.inc"
#endif
static void CheckNativeDispatchRestore(Fixture& f){
 auto& c=*f.c;auto& io=f.io;auto* state=c.q38TokenNative;
 const unsigned allWords=120U*(TT_Q38_PANEL_VIEW_WORDS+1U),before=io.touches,packets=io.runtimePackets;
 assert(!lockHeld && state->OriginalL1Restored() && !state->DispatchImagesRestored() && !state->Closed()
  && io.beforeDispatchImage.size()==allWords && !io.dispatchFaultSeen);
 std::array<unsigned,120> resets;for(unsigned w=0;w<120;w++)resets[w]=io.Core(w).resets.size();
 PioneerQ38TokenDispatchRestoreResult result;const auto pristine=result;auto* words=reinterpret_cast<uint32*>(&result);
 io.nativeDispatchResult=&result;
 assert(!RestoreQ38TokenNativeDispatchImages(c,result) && io.touches==before);
 {
  PioneerQ38TokenCallScope scope(&f.cookie);assert(scope.Context()==&c);
  for(unsigned i=0;i<16;i++){
   words[i]^=1;assert(!RestoreQ38TokenNativeDispatchImages(c,result) && io.touches==before);result=pristine;
  }
  const bool okay=RestoreQ38TokenNativeDispatchImages(c,result);
  if(io.dispatchRestoreFault){
   assert(!okay && result.started && result.unsafe && !result.ready && io.dispatchFaultSeen==1
    && io.touches==io.dispatchFaultTouches && state->Unsafe() && !state->DispatchImagesRestored());
  }else{
   if(!okay)fprintf(stderr,"DISPATCH_RESTORE_DIAGNOSTIC phase%u worker%u word%u cores%u restored%u verified%u unsafe%u;NO original supplied\n",
    result.phase,result.worker,result.word,result.restoredCores,result.restoredWords,result.verifiedWords,result.unsafe);
   assert(okay && result.ready && !result.unsafe && result.restoredCores==120 && result.savedWords==allWords
    && result.restoredWords==allWords && result.verifiedWords==allWords && result.entryWords==120
    && state->DispatchImagesRestored() && !state->OriginalL1Restored() && !state->Closed()
    && !state->Unsafe() && io.TokenModelSourceScope(false));
  }
  const unsigned touches=io.touches;PioneerQ38TokenDispatchRestoreResult again;PioneerQ38TokenRestoreResult l1;PioneerQ38TokenReadResult read;
  assert(!RestoreQ38TokenNativeDispatchImages(c,again) && !RestoreQ38TokenNativeL1(c,l1)
   && !ReadQ38TokenNativeRecords(c,0,nullptr,read) && io.touches==touches);
 }
 io.nativeDispatchResult=nullptr;
 for(unsigned w=0;w<120;w++){
  assert(io.Core(w).coreReset==0x47800 && io.Core(w).resets.size()==resets[w]);
  if(!io.dispatchRestoreFault){
   const auto& image=c.q38Dispatch->worker[w].image;const unsigned base=w*(TT_Q38_PANEL_VIEW_WORDS+1U);
   assert(image.saved && !image.ready && !image.dirty && !image.running && !image.unsafe && c.q38Dispatch->worker[w].Store().ready);
   assert(io.Raw(w,0x6000)==io.beforeDispatchImage[base]);
   for(unsigned i=0;i<TT_Q38_PANEL_VIEW_WORDS;i++)assert(io.Raw(w,TT_Q38_PANEL_VIEW_BASE+i*4)==io.beforeDispatchImage[base+1+i]);
  }
 }
 assert(io.runtimePackets==packets && !io.pciWrites && !io.HostReadMock::bmeWrites && !io.wideReleased
  && !io.PageMock::releases && !io.OutputBufferMock::releases);
 puts(io.dispatchRestoreFault?
  "EXECUTED native actual earlier-dispatch original write then generation loss/ALL owners retained/no later IO/retry/reset/release;NO clean dispatch/NIU/CSR/END or hardware grant":
  "EXECUTED native SAME original firstlayer/cancel/STOP/HOLD/drain/full-token L1 restored/ALL120 earlier dispatch images proved BEFORE first original/exact120*67585 saved words+entry writes+full second pass/ALL bytes independently match pre-dispatch physical observation;NIU/scratch/TLB/master/clock/maps/hosts STILL OWNED,NO END/physical/Work/TPS grant");
#ifdef PIONEER_Q38_TOKEN_NATIVE_RESTORE_PEERS_FIXTURE
 if(!io.dispatchRestoreFault)CheckNativePeerRestore(f);
#endif
}
