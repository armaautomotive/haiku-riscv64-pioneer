/* SPDX-License-Identifier: MIT */
#ifdef PIONEER_Q38_TOKEN_NATIVE_CONTEXT
if(q38TokenNative && q38TokenNative->ControlsCohort(q38Dispatch)){
 if(!controlledToken)return true;
}else
#endif
#ifdef PIONEER_Q38_RECURRENT_PREFIX_NATIVE_CONTEXT
if(q38RecurrentPrefixNative && q38RecurrentPrefixNative->ControlsCohort(q38Dispatch)){
 if(!controlledEmbedding)return true;
}else
#endif
#ifdef PIONEER_Q38_RECURRENT_BLOCK_NATIVE_CONTEXT
if(q38RecurrentBlockNative && q38RecurrentBlockNative->ControlsCohort(q38Dispatch)){
 if(!controlledEmbedding)return true;
}else
#endif
#ifdef PIONEER_Q38_MIXED_LIVE_NATIVE_CONTEXT
if(q38MixedLiveNative && q38MixedLiveNative->ControlsCohort(q38Dispatch)){
 if(!controlledEmbedding)return true;
}else
#endif
if(q38Dispatch && PioneerQ38DispatchUnsafe(*q38Dispatch))return true;
