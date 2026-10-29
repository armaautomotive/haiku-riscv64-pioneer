/* SPDX-License-Identifier: MIT */
#if !defined(PIONEER_Q38_TOKEN_NATIVE_RESTORE_PEERS_FIXTURE) || PIONEER_Q38_TOKEN_NATIVE_RESTORE_PEERS_FIXTURE != 1 \
 || !defined(PIONEER_Q38_TOKEN_NATIVE_RESTORE_DISPATCH_FIXTURE) || PIONEER_Q38_TOKEN_NATIVE_RESTORE_DISPATCH_FIXTURE != 1 \
 || !defined(PIONEER_Q38_TOKEN_NATIVE_RESTORE_EMPTY_FIXTURE) || PIONEER_Q38_TOKEN_NATIVE_RESTORE_EMPTY_FIXTURE != 1 \
 || !defined(PIONEER_Q38_TOKEN_NATIVE_READOUT_EMPTY_FIXTURE) || PIONEER_Q38_TOKEN_NATIVE_READOUT_EMPTY_FIXTURE != 1 \
 || !defined(PIONEER_Q38_TOKEN_NATIVE_HOLD_LAYER_FIXTURE) || PIONEER_Q38_TOKEN_NATIVE_HOLD_LAYER_FIXTURE != 1 \
 || !defined(PIONEER_Q38_TOKEN_NATIVE_CANCEL_LAYER_FIXTURE) || PIONEER_Q38_TOKEN_NATIVE_CANCEL_LAYER_FIXTURE != 1 \
 || !defined(PIONEER_Q38_TOKEN_NATIVE_FULL_LAYER_FIXTURE) || PIONEER_Q38_TOKEN_NATIVE_FULL_LAYER_FIXTURE != 1 \
 || !defined(PIONEER_Q38_TOKEN_NATIVE_PEER_FIXTURE) || PIONEER_Q38_TOKEN_NATIVE_PEER_FIXTURE != 1 \
 || !defined(PIONEER_Q38_TOKEN_CONTEXT_COMPILE_ONLY) || PIONEER_Q38_TOKEN_CONTEXT_COMPILE_ONLY != 1 \
 || defined(PIONEER_Q38_TOKEN_NATIVE_FULL_FLOW_FIXTURE) || defined(PIONEER_Q38_WORK_INFERENCE)
#error Explicit native119-peer local NIU+scratch restoration TEST required
#endif
static void CheckNativePeerRestore(Fixture& f){
 auto& c=*f.c;auto& io=f.io;auto* state=c.q38TokenNative;
 assert(!lockHeld && state->DispatchImagesRestored() && !state->PeerStoreOriginalsRestored()
  && !state->Closed() && io.beforePeerScratch.size()==119*2048 && !io.peerFaultSeen);
 const unsigned before=io.touches,packets=io.runtimePackets;
 std::array<unsigned,120> resets;for(unsigned w=0;w<120;w++)resets[w]=io.Core(w).resets.size();
 std::array<uint32_t,10> rootCommand;for(unsigned i=0;i<10;i++)rootCommand[i]=io.Core(0).command[i];
 const uint32_t rootConfig=io.Core(0).source[0];
 PioneerQ38TokenPeerRestoreResult result;const auto pristine=result;auto* words=reinterpret_cast<uint32*>(&result);
 io.nativePeerResult=&result;
 assert(!RestoreQ38TokenNativePeerStores(c,result) && io.touches==before);
 {
  PioneerQ38TokenCallScope scope(&f.cookie);assert(scope.Context()==&c);
  for(unsigned i=0;i<16;i++){
   words[i]^=1;assert(!RestoreQ38TokenNativePeerStores(c,result) && io.touches==before);result=pristine;
  }
  const bool okay=RestoreQ38TokenNativePeerStores(c,result);
  if(io.peerRestoreFault){
   assert(!okay && result.started && result.unsafe && !result.ready && io.peerFaultSeen==1
    && io.touches==io.peerFaultTouches && state->Unsafe() && !state->PeerStoreOriginalsRestored());
   if(io.peerRestoreFault==2)assert(io.peerMapWriteSeen==1 && result.phase==1);
  }else{
   if(!okay)fprintf(stderr,"PEER_RESTORE_DIAGNOSTIC phase%u worker%u word%u peers%u commands%u configs%u scratch%u verified%u unsafe%u;NO original supplied\n",
    result.phase,result.worker,result.word,result.peers,result.commandWords,result.configWords,result.scratchWords,result.verifiedWords,result.unsafe);
   assert(okay && result.ready && !result.unsafe && result.peers==119 && result.commandWords==1190 && result.configWords==119
    && result.scratchWords==119*2048 && result.verifiedWords==result.scratchWords && state->PeerStoreOriginalsRestored()
    && !state->DispatchImagesRestored() && !state->Closed() && !state->Unsafe() && io.TokenModelSourceScope(false));
  }
  const unsigned touches=io.touches;PioneerQ38TokenPeerRestoreResult again;PioneerQ38TokenDispatchRestoreResult dispatch;
  assert(!RestoreQ38TokenNativePeerStores(c,again) && !RestoreQ38TokenNativeDispatchImages(c,dispatch) && io.touches==touches);
 }
 io.nativePeerResult=nullptr;
 for(unsigned w=0;w<120;w++)assert(io.Core(w).coreReset==0x47800 && io.Core(w).resets.size()==resets[w]);
 assert(io.Core(0).source[0]==rootConfig && c.lease.ready && !c.q38Dispatch->closed);
 for(unsigned i=0;i<10;i++)assert(io.Core(0).command[i]==rootCommand[i]);
 if(!io.peerRestoreFault)for(unsigned w=1;w<120;w++){
  const auto& store=c.q38Dispatch->worker[w].Store();
  assert(!store.ready && store.niu_restored && store.l1_restored && !store.l1_dirty && !store.tlb_restored
   && store.controls_saved && store.configured && !store.recovery_required && io.Core(w).source[0]==io.beforePeerConfig[w-1]);
  for(unsigned i=0;i<10;i++)assert(!io.Core(w).command[i]);
  for(unsigned i=0;i<2048;i++)assert(io.Raw(w,0x17e000+i*4)==io.beforePeerScratch[(w-1)*2048+i]);
 }
 assert(io.runtimePackets==packets && !io.pciWrites && !io.HostReadMock::bmeWrites && !io.wideReleased
  && !io.PageMock::releases && !io.OutputBufferMock::releases);
 if(io.peerRestoreFault)printf("EXECUTED native119-peer restore fault%u/ACTUAL preceding original layer/cancel/STOP/HOLD/drain/token+dispatch originals/first bad real mapping or NIU/scratch readback or generation loss/NO following IO/retry/reset/release;ALL resources retained,NO clean closure/END/physical/Work/TPS grant\n",io.peerRestoreFault);
 else puts("EXECUTED native SAME full851/original firstlayer+cancel/STOP/HOLD/drain/token+dispatch originals/ALL120 current held preflight/119 exact original NIU configs+1190 zero command words+243712 original scratch words immediate reads+full second pass/independent pre-lease byte+config observations match including UINT32_MAX;ROOT/sharedTLBs/master/clock/maps/hosts STILL OWNED,NO END/physical/Work/TPS grant");
}
