/* SPDX-License-Identifier: MIT */
// Actual SAME-owner native56 QRR1 claim/image/execution/Close/END compile.
#define PIONEER_Q38_RESIDUAL_RMS_NATIVE 1
#include "pioneer_q38_gdn_output_execution_native_full_compile.cpp"
extern "C" bool pioneer_residual_rms_run(PioneerStoreContext& c,PioneerQ38ResidentDecoderResult& r){
 return RunQ38ResidentResidualRms(c,r);
}
