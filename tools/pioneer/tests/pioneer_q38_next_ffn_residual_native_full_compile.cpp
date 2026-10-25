/* SPDX-License-Identifier: MIT */
#define PIONEER_Q38_NEXT_FFN_RESIDUAL_NATIVE 1
#include "pioneer_q38_next_residual_rms_native_full_compile.cpp"
extern "C" bool pioneer_next_ffn_residual_run(PioneerStoreContext& c,PioneerQ38ResidentDecoderResult& r){return RunQ38NextFfnResidual(c,r);}
