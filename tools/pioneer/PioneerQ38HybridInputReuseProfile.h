/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_HYBRID_INPUT_REUSE_PROFILE_H
#define PIONEER_Q38_HYBRID_INPUT_REUSE_PROFILE_H
#if !defined(PIONEER_Q38_HYBRID_INPUT_REUSE_INSPECTION) || PIONEER_Q38_HYBRID_INPUT_REUSE_INSPECTION != 1 \
 || defined(_KERNEL_MODE) || defined(PIONEER_DRIVER_BUILD_TAG) || defined(PIONEER_Q38_WORK_INFERENCE) \
 || defined(TT_PIONEER_Q38_WORK_MODE) || defined(PIONEER_Q38_FULL_OFFLOAD_NATIVE) \
 || defined(PIONEER_Q38_FULL_OFFLOAD_CLIENT) || defined(PIONEER_Q38_TOKEN_NATIVE_CONTEXT) \
 || defined(PIONEER_Q38_BF16_FULL_NATIVE) || defined(PIONEER_Q38_BF16_FULL_CLIENT) \
 || defined(PIONEER_Q38_CPU_VALIDATION) || defined(PIONEER_Q38_FULL_MODEL_CPU_REFERENCE)
#error Hybrid input-reuse accounting is for the isolated inspection image only
#endif

// QGW5 inspection accounting, NOT an upload, lifetime or launch authority.
// BFP4 reads the input once per job and weights once per band. BFP8 retains
// its existing two operands per band. A BFP4 multi-band job finishes with a
// WEIGHT read, not an input read, and the ROOT receipt must check that command.
struct PioneerQ38HybridInputReuseProfile {
 static bool Describe(unsigned width,unsigned bands,bool eight,unsigned& reads,unsigned& flits,bool& lastWeight){
  if((width!=5120 && width!=17408) || !bands || bands>8)return false;
  const unsigned tiles=width/32;unsigned nextReads=eight?0:tiles,nextFlits=eight?0:tiles;
  const unsigned perRead=eight?2*tiles:tiles;
  const unsigned perFlit=eight?((tiles<<4)+(tiles<<1)):((tiles<<3)+tiles);
  for(unsigned band=0;band<bands;band++){nextReads+=perRead;nextFlits+=perFlit;}
  reads=nextReads;flits=nextFlits;lastWeight=!eight && bands>1;return true;
 }
};
#endif
