/* SPDX-License-Identifier: MIT */
#define PIONEER_Q38_NEXT_PROJECTION_NATIVE 1
#include "pioneer_q38_next_rms_native_full_compile.cpp"
extern "C" bool pioneer_next_projection_run(PioneerStoreContext& c,PioneerQ38ResidentDecoderResult& r){return RunQ38NextProjections(c,r);}
