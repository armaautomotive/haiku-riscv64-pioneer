/* SPDX-License-Identifier: MIT */
#ifdef PIONEER_Q38_MIXED_LIVE_NATIVE_CONTEXT
if(q38MixedLiveNative && q38MixedLiveNative->ControlsCohort(q38Dispatch)){
 if(!controlledEmbedding)return true;
}else
#endif
if(q38Dispatch && PioneerQ38DispatchUnsafe(*q38Dispatch))return true;
