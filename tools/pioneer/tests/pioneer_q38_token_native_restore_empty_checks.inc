/* SPDX-License-Identifier: MIT */
#if !defined(PIONEER_Q38_TOKEN_NATIVE_RESTORE_EMPTY_FIXTURE) || PIONEER_Q38_TOKEN_NATIVE_RESTORE_EMPTY_FIXTURE != 1 \
 || !defined(PIONEER_Q38_TOKEN_NATIVE_READOUT_EMPTY_FIXTURE) || PIONEER_Q38_TOKEN_NATIVE_READOUT_EMPTY_FIXTURE != 1 \
 || !defined(PIONEER_Q38_TOKEN_NATIVE_HOLD_LAYER_FIXTURE) || PIONEER_Q38_TOKEN_NATIVE_HOLD_LAYER_FIXTURE != 1 \
 || !defined(PIONEER_Q38_TOKEN_NATIVE_CANCEL_LAYER_FIXTURE) || PIONEER_Q38_TOKEN_NATIVE_CANCEL_LAYER_FIXTURE != 1 \
 || !defined(PIONEER_Q38_TOKEN_NATIVE_FULL_LAYER_FIXTURE) || PIONEER_Q38_TOKEN_NATIVE_FULL_LAYER_FIXTURE != 1 \
 || !defined(PIONEER_Q38_TOKEN_NATIVE_PEER_FIXTURE) || PIONEER_Q38_TOKEN_NATIVE_PEER_FIXTURE != 1 \
 || !defined(PIONEER_Q38_TOKEN_CONTEXT_COMPILE_ONLY) || PIONEER_Q38_TOKEN_CONTEXT_COMPILE_ONLY != 1 \
 || defined(PIONEER_Q38_TOKEN_NATIVE_FULL_FLOW_FIXTURE) || defined(PIONEER_Q38_WORK_INFERENCE)
#error Explicit native original-L1 restoration TEST required
#endif
static void CheckNativeRestoreEmpty(Fixture& f){
 namespace L=PioneerQ38TokenL1;auto& c=*f.c;auto& io=f.io;auto* state=c.q38TokenNative;
 assert(!lockHeld && state->TerminalHeld() && !state->Closed() && !state->OriginalL1Restored()
  && io.beforeNativeImage.size()==L::SavedWords && !io.restoreFaultSeen);
 const unsigned packets=io.runtimePackets,before=io.touches;std::array<unsigned,120> resets;
 for(unsigned w=0;w<120;w++)resets[w]=io.Core(w).resets.size();
 PioneerQ38TokenRestoreResult result;io.nativeRestoreResult=&result;const auto pristine=result;
 auto* words=reinterpret_cast<uint32*>(&result);
 assert(!RestoreQ38TokenNativeL1(c,result) && io.touches==before);
 {
  PioneerQ38TokenCallScope scope(&f.cookie);assert(scope.Context()==&c);
  for(unsigned i=0;i<16;i++){
   words[i]^=1;assert(!RestoreQ38TokenNativeL1(c,result) && io.touches==before);result=pristine;
  }
  const bool okay=RestoreQ38TokenNativeL1(c,result);
  if(io.restoreFault){
   assert(!okay && result.started && result.unsafe && !result.ready && io.restoreFaultSeen==1
    && io.touches==io.restoreFaultTouches && state->Unsafe() && !state->OriginalL1Restored());
   const unsigned touches=io.touches;PioneerQ38TokenRestoreResult again;PioneerQ38TokenReadResult read;
   assert(!RestoreQ38TokenNativeL1(c,again) && !ReadQ38TokenNativeRecords(c,0,nullptr,read) && io.touches==touches);
  }else{
   if(!okay)fprintf(stderr,"L1_RESTORE_DIAGNOSTIC phase%u worker%u word%u cores%u restored%u verified%u unsafe%u;NO original supplied\n",
    result.phase,result.worker,result.word,result.restoredCores,result.restoredWords,result.verifiedWords,result.unsafe);
   assert(okay && result.ready && !result.unsafe && result.restoredCores==120 && result.savedWords==L::SavedWords
    && result.restoredWords==L::SavedWords && result.verifiedWords==L::SavedWords && state->OriginalL1Restored()
    && !state->Closed() && !state->Unsafe() && io.TokenModelSourceScope(false));
   const unsigned touches=io.touches;PioneerQ38TokenRestoreResult again;PioneerQ38TokenReadResult read;
   assert(!RestoreQ38TokenNativeL1(c,again) && !ReadQ38TokenNativeRecords(c,0,nullptr,read) && io.touches==touches);
  }
 }
 io.nativeRestoreResult=nullptr;
 for(unsigned w=0;w<120;w++){
  assert(io.Core(w).coreReset==0x47800 && io.Core(w).resets.size()==resets[w]);
  if(!io.restoreFault)for(unsigned i=0;i<L::SnapshotWords(w);i++)
   assert(io.Raw(w,L::SnapshotAddress(w,i))==io.beforeNativeImage[L::SnapshotBase(w)+i]);
 }
 assert(io.runtimePackets==packets && !io.pciWrites && !io.HostReadMock::bmeWrites && !io.wideReleased
  && !io.PageMock::releases && !io.OutputBufferMock::releases);
 puts(io.restoreFault?
  "EXECUTED native actual original-L1 write then generation loss/ALL owners retained/no later IO/retry/restore/reset/release;NO clean L1/CSR/END or hardware grant":
  "EXECUTED native SAME full851/original firstlayer+cancel/ROOT+119STOP/current terminal+120HOLD/zero-commit drain/ALL120 frozen images BEFORE first restore/exact original-L1 writes+full second pass/ALL observed original words including UINT32_MAX independently match/no reset or NIU command;dispatch/CSR/clock/maps/hosts STILL OWNED,NO END/physical/Work/TPS grant");
}
