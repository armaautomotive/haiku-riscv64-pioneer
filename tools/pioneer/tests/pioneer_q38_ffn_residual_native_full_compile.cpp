/* SPDX-License-Identifier: MIT */
// Actual SAME native57 claim/boot/FFN+residual/STOP/hold/Close/END compile.
// Old QFG2 grammar still coexists,NEVER re-tagged into a producer grant.
#define PIONEER_Q38_FFN_RESIDUAL_NATIVE 1
#include "pioneer_q38_residual_rms_native_full_compile.cpp"
extern "C" bool pioneer_ffn_residual_run(PioneerStoreContext& c,PioneerQ38ResidentDecoderResult& r){
 return RunQ38ResidentFfn(c,r);
}
