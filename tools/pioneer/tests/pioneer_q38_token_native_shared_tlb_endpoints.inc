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
#error Explicit native shared-mapping restoration TEST endpoint required
#endif
void PioneerSessionIO::NativeSharedAliasRefusals(){
 if(!nativeSharedTlbResult || nativeSharedTlbResult->phase!=3 || sharedTlbWrites!=1 || sharedAliasRefusals || sharedTlbFaultSeen)return;
 auto* c=q38MutableContext;auto* state=c?c->q38TokenNative:nullptr;
 assert(c && state && state->runtime && state->runtime->SharedMappingsChanged());
 const unsigned before=touches;uint32 value=0x7531a9c2;
 assert(!TokenMemoryReadChecked(0,0x40000,value) && !TokenMemoryWrite(0,0x40000,0)
  && !TokenFifoReadChecked(0,false,value) && !TokenPeerStoreSelect(0)
  && !TokenRootNiuReadChecked(0,value) && !TokenRootNiuWrite(0,0)
  && !TokenRootScratchReadChecked(0,value) && !TokenRootScratchWrite(0,0)
  && !TokenSharedRootCommandReadChecked(0,value) && value==0x7531a9c2 && touches==before);
 sharedAliasRefusals=9;
} // real post-write callback observes the private alias veto;never supplies one
