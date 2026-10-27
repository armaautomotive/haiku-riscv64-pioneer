/* SPDX-License-Identifier: MIT */
#ifdef PIONEER_Q38_MIXED_LIVE_NATIVE_CONTEXT
if(io.q38MixedLiveMemoryMapAttempted && (!q38MixedLiveNative || !q38MixedLiveNative->memory))return true;
if(q38MixedLiveNative && (q38MixedLiveNative->Unsafe() || (!controlledEmbedding && !q38MixedLiveNative->Closed())))return true;
#endif
