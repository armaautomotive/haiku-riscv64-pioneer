/* SPDX-License-Identifier: MIT */
#define PIONEER_Q38_NEXT_RESIDUAL_RMS_NATIVE 1
#include "pioneer_q38_next_gdn_output_native_full_compile.cpp"
extern "C" bool pioneer_next_residual_rms_run(PioneerStoreContext& c,PioneerQ38ResidentDecoderResult& r){return RunQ38NextResidualRms(c,r);}
