/* SPDX-License-Identifier: MIT */
#if !defined(PIONEER_Q38_TOKEN_NATIVE_RESTORE_ROOT_STORE_FIXTURE) || PIONEER_Q38_TOKEN_NATIVE_RESTORE_ROOT_STORE_FIXTURE != 1 \
 || !defined(PIONEER_Q38_TOKEN_NATIVE_RESTORE_PEERS_FIXTURE) || PIONEER_Q38_TOKEN_NATIVE_RESTORE_PEERS_FIXTURE != 1 \
 || !defined(PIONEER_Q38_TOKEN_NATIVE_RESTORE_DISPATCH_FIXTURE) || PIONEER_Q38_TOKEN_NATIVE_RESTORE_DISPATCH_FIXTURE != 1 \
 || !defined(PIONEER_Q38_TOKEN_NATIVE_RESTORE_EMPTY_FIXTURE) || PIONEER_Q38_TOKEN_NATIVE_RESTORE_EMPTY_FIXTURE != 1 \
 || !defined(PIONEER_Q38_TOKEN_NATIVE_READOUT_EMPTY_FIXTURE) || PIONEER_Q38_TOKEN_NATIVE_READOUT_EMPTY_FIXTURE != 1 \
 || !defined(PIONEER_Q38_TOKEN_NATIVE_HOLD_LAYER_FIXTURE) || PIONEER_Q38_TOKEN_NATIVE_HOLD_LAYER_FIXTURE != 1 \
 || !defined(PIONEER_Q38_TOKEN_NATIVE_CANCEL_LAYER_FIXTURE) || PIONEER_Q38_TOKEN_NATIVE_CANCEL_LAYER_FIXTURE != 1 \
 || !defined(PIONEER_Q38_TOKEN_NATIVE_FULL_LAYER_FIXTURE) || PIONEER_Q38_TOKEN_NATIVE_FULL_LAYER_FIXTURE != 1 \
 || !defined(PIONEER_Q38_TOKEN_NATIVE_PEER_FIXTURE) || PIONEER_Q38_TOKEN_NATIVE_PEER_FIXTURE != 1 \
 || !defined(PIONEER_Q38_TOKEN_CONTEXT_COMPILE_ONLY) || PIONEER_Q38_TOKEN_CONTEXT_COMPILE_ONLY != 1 \
 || defined(PIONEER_Q38_TOKEN_NATIVE_FULL_FLOW_FIXTURE) || defined(PIONEER_Q38_WORK_INFERENCE)
#error Explicit native119-peer-preceded ROOT-local restoration TEST required
#endif
static void CheckNativeRootStoreRestore(Fixture& f){
 auto& c=*f.c;auto& io=f.io;auto* state=c.q38TokenNative;
 assert(!lockHeld && state->PeerStoreOriginalsRestored() && !state->RootStoreOriginalsRestored()
  && !state->Closed() && !io.rootStoreFaultSeen && c.lease.ready && !c.lease.tlb_restored);
 const unsigned before=io.touches,packets=io.runtimePackets;
 std::array<unsigned,120> resets;for(unsigned w=0;w<120;w++)resets[w]=io.Core(w).resets.size();
 PioneerQ38TokenRootStoreRestoreResult result;const auto pristine=result;auto* words=reinterpret_cast<uint32*>(&result);
 io.nativeRootStoreResult=&result;
 assert(!RestoreQ38TokenNativeRootStore(c,result) && io.touches==before);
 {
  PioneerQ38TokenCallScope scope(&f.cookie);assert(scope.Context()==&c);
  uint32 actual=0xa5a55a5a;
  for(unsigned address:{0U,4U,36U,40U,0x40U,0x64U,0x100U,0x174U,0x204U,UINT32_MAX}){
   assert(!io.TokenRootNiuReadChecked(address,actual) && actual==0xa5a55a5a && !io.TokenRootNiuWrite(address,0));
  }
  for(unsigned word:{0U,2047U,2048U,UINT32_MAX}){
   assert(!io.TokenRootScratchReadChecked(word,actual) && actual==0xa5a55a5a && !io.TokenRootScratchWrite(word,0));
  }
  assert(io.touches==before);
  for(unsigned i=0;i<16;i++){
   words[i]^=1;assert(!RestoreQ38TokenNativeRootStore(c,result) && io.touches==before);result=pristine;
  }
  const bool okay=RestoreQ38TokenNativeRootStore(c,result);
  if(io.rootStoreFault){
   assert(!okay && result.started && result.unsafe && !result.ready && io.rootStoreFaultSeen==1
    && io.touches==io.rootStoreFaultTouches && state->Unsafe() && !state->RootStoreOriginalsRestored());
   if(io.rootStoreFault==2)assert(io.rootStoreMapWrites==1 && result.phase==1);
  }else{
   if(!okay)fprintf(stderr,"ROOT_STORE_RESTORE_DIAGNOSTIC phase%u worker%u word%u commands%u configs%u scratch%u verified%u unsafe%u;NO original supplied\n",
    result.phase,result.worker,result.word,result.commandWords,result.configWords,result.scratchWords,result.verifiedWords,result.unsafe);
   assert(okay && result.ready && !result.unsafe && result.roots==1 && result.commandWords==10 && result.configWords==1
    && result.scratchWords==2048 && result.verifiedWords==2048 && state->RootStoreOriginalsRestored()
    && !state->PeerStoreOriginalsRestored() && !state->Closed() && !state->Unsafe() && io.TokenModelSourceScope(false));
  }
  const unsigned touches=io.touches;PioneerQ38TokenRootStoreRestoreResult again;PioneerQ38TokenPeerRestoreResult peers;
  assert(!RestoreQ38TokenNativeRootStore(c,again) && !RestoreQ38TokenNativePeerStores(c,peers) && io.touches==touches);
 }
 io.nativeRootStoreResult=nullptr;
 for(unsigned w=0;w<120;w++)assert(io.Core(w).coreReset==0x47800 && io.Core(w).resets.size()==resets[w]);
 assert(c.lease.ready && !c.lease.tlb_restored && !c.q38Dispatch->closed);
 if(!io.rootStoreFault){
  assert(c.lease.niu_restored && c.lease.l1_restored && !c.lease.l1_dirty && io.Core(0).config==io.beforeRootConfig);
  for(unsigned i=0;i<10;i++)assert(!io.Core(0).command[i]);
  for(unsigned i=0;i<2048;i++)assert(io.Raw(0,0x17e000+i*4)==io.beforeRootScratch[i]);
 }
 for(unsigned w=1;w<120;w++){
  const auto& store=c.q38Dispatch->worker[w].Store();
  assert(!store.ready && store.niu_restored && store.l1_restored && !store.l1_dirty && !store.tlb_restored
   && io.Core(w).config==io.beforePeerConfig[w-1]);
  for(unsigned i=0;i<10;i++)assert(!io.Core(w).command[i]);
  for(unsigned i=0;i<2048;i++)assert(io.Raw(w,0x17e000+i*4)==io.beforePeerScratch[(w-1)*2048+i]);
 }
 assert(io.runtimePackets==packets && !io.pciWrites && !io.HostReadMock::bmeWrites && !io.wideReleased
  && !io.PageMock::releases && !io.OutputBufferMock::releases);
 if(io.rootStoreFault)printf("EXECUTED native ROOT-local restore fault%u/actual SAME original firstlayer+cancel/STOP/HOLD/drain/token+dispatch+119peer originals/first uncertainty/NO following IO/retry/reset/release;shared/master owners retained,NO END/physical/Work/TPS grant\n",io.rootStoreFault);
 else puts("EXECUTED native SAME full851/original firstlayer+cancel/STOP/HOLD/drain/token+dispatch+119peer originals/ROOT10 zero commands+exact original NIU config+2048 scratch originals/immediate reads+full secondpass/independent pre-master-lease observations match including UINT32_MAX;master.ready/sharedTLB/clock/maps/hosts STILL OWNED,NO END/physical/Work/TPS grant");
}
