/* SPDX-License-Identifier: MIT */
#include "pioneer_q38_token_native_full_end_role.h"
void PioneerSessionIO::NativeSharedAliasRefusals(){
 if(!nativeSharedTlbResult || nativeSharedTlbResult->phase!=3 || sharedTlbWrites!=1 || sharedAliasRefusals)return;
 auto* c=q38MutableContext;auto* state=c?c->q38TokenNative:nullptr;
 assert(c && state && state->runtime && state->runtime->SharedMappingsChanged());
 const unsigned before=touches;uint32 value=0x7531a9c2;
 assert(!TokenMemoryReadChecked(0,0x40000,value) && !TokenMemoryWrite(0,0x40000,0)
  && !TokenFifoReadChecked(0,false,value) && !TokenPeerStoreSelect(0)
  && !TokenRootNiuReadChecked(0,value) && !TokenRootNiuWrite(0,0)
  && !TokenRootScratchReadChecked(0,value) && !TokenRootScratchWrite(0,0)
  && !TokenSharedRootCommandReadChecked(0,value) && value==0x7531a9c2 && touches==before);
 sharedAliasRefusals=9;
} // observations after a real mapping write,not a caller-supplied veto or completion
