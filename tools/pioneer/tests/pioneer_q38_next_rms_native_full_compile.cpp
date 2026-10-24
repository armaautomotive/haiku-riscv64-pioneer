/* SPDX-License-Identifier: MIT */
// SAME actual owner claims genuine FFN hidden then QRN1/holds/Close/END.
#define PIONEER_Q38_NEXT_RMS_NATIVE 1
#include "pioneer_q38_ffn_residual_native_full_compile.cpp"
extern "C" bool pioneer_next_rms_run(PioneerStoreContext& c,PioneerQ38ResidentDecoderResult& r){return RunQ38NextRms(c,r);}
