/* SPDX-License-Identifier: MIT */
#if !defined(PIONEER_Q38_TOKEN_NATIVE_RESTORE_SHARED_TLB_FIXTURE) || PIONEER_Q38_TOKEN_NATIVE_RESTORE_SHARED_TLB_FIXTURE != 1 \
 || !defined(PIONEER_Q38_TOKEN_NATIVE_RESTORE_ROOT_STORE_FIXTURE) || PIONEER_Q38_TOKEN_NATIVE_RESTORE_ROOT_STORE_FIXTURE != 1 \
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
#error Explicit native ROOT-preceded shared-mapping restoration TEST required
#endif
static void CheckNativeSharedTlbRestore(Fixture& f){
 auto& c=*f.c;auto& io=f.io;auto* state=c.q38TokenNative;
 assert(!lockHeld && state->RootStoreOriginalsRestored() && !state->SharedMappingsRestored()
  && !state->Closed() && c.lease.ready && !c.lease.tlb_restored && !io.sharedTlbWrites && !io.sharedTlbFaultSeen);
 const unsigned before=io.touches,packets=io.runtimePackets;
 std::array<unsigned,120> resets;for(unsigned w=0;w<120;w++)resets[w]=io.Core(w).resets.size();
 PioneerQ38TokenSharedTlbRestoreResult result;const auto pristine=result;auto* words=reinterpret_cast<uint32*>(&result);
 io.nativeSharedTlbResult=&result;
 assert(!RestoreQ38TokenNativeSharedMappings(c,result) && io.touches==before);
 {
  PioneerQ38TokenCallScope scope(&f.cookie);assert(scope.Context()==&c);
  for(unsigned i=0;i<16;i++){
   words[i]^=1;assert(!RestoreQ38TokenNativeSharedMappings(c,result) && io.touches==before);result=pristine;
  }
  for(unsigned offset:{0U,1U,unsigned(TT_BULK_UPLOAD_SAVED_WORDS-1)})
   assert(!Q38TokenExecutionOutput(c,io.BulkSavedWords()+offset,sizeof(result)) && io.touches==before);
  const bool okay=RestoreQ38TokenNativeSharedMappings(c,result);
  if(io.sharedTlbFault){
   assert(!okay && result.started && result.unsafe && !result.ready && io.sharedTlbFaultSeen==1
    && io.touches==io.sharedTlbFaultTouches && state->Unsafe() && !state->SharedMappingsRestored()
    && !c.lease.tlb_restored && io.sharedTlbWrites==(io.sharedTlbFault==3?6U:1U));
   assert(io.sharedAliasRefusals==(io.sharedTlbFault==1?0U:9U));
  }else{
   if(!okay)fprintf(stderr,"SHARED_TLB_RESTORE_DIAGNOSTIC phase%u worker%u word%u restored%u verified%u bulk%u unsafe%u;NO original supplied\n",
    result.phase,result.worker,result.word,result.restoredWords,result.verifiedWords,result.bulkWords,result.unsafe);
   assert(okay && result.ready && !result.unsafe && result.restoredWords==6 && result.verifiedWords==6 && result.bulkWords==8192
    && io.sharedTlbWrites==6 && io.sharedAliasRefusals==9 && c.lease.tlb_restored && state->SharedMappingsRestored()
    && !state->RootStoreOriginalsRestored() && !state->Closed() && !state->Unsafe() && io.TokenModelSourceScope(false)
    && !io.TokenMemoryAccessScope());
   for(unsigned s=0;s<2;s++)for(unsigned i=0;i<3;i++)assert(io.Core(0).tlb[s][i]==io.beforeSharedTlb[s][i]);
  }
  const unsigned touches=io.touches;PioneerQ38TokenSharedTlbRestoreResult again;PioneerQ38TokenRootStoreRestoreResult root;
  assert(!RestoreQ38TokenNativeSharedMappings(c,again) && !RestoreQ38TokenNativeRootStore(c,root) && io.touches==touches);
 }
 io.nativeSharedTlbResult=nullptr;
 for(unsigned w=0;w<120;w++)assert(io.Core(w).coreReset==0x47800 && io.Core(w).resets.size()==resets[w]);
 assert(c.lease.ready && c.bulk.ready && !c.bulk.dirty && !c.q38Dispatch->closed && c.q38Clock.RequiresExplicitEnd()
  && io.runtimePackets==packets && !io.pciWrites && !io.HostReadMock::bmeWrites && !io.wideReleased
  && !io.PageMock::releases && !io.OutputBufferMock::releases);
 if(io.sharedTlbFault)printf("EXECUTED native shared200/201 restore fault%u/actual full predecessor through ROOT originals/first real mapping-write or readback uncertainty/NO following IO/NO mapped alias after first attempted change;ALL owners retained,NO END/physical/Work/TPS grant\n",io.sharedTlbFault);
 else puts("EXECUTED native SAME full851/original firstlayer+cancel/STOP/HOLD/drain/token+dispatch+119peer+ROOT originals/ALL120 current held proofs/8192 bulk originals verified/6 exact pre-master-lease shared control originals+immediate reads+secondpass/9 actual post-first-write alias API refusals/NO subsequent mapped-source alias access;clock/master.ready/bulk/VM/hosts STILL OWNED,NO END/physical/Work/TPS grant");
}
