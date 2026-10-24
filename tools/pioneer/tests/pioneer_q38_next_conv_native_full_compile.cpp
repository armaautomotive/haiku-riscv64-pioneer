/* SPDX-License-Identifier: MIT */
#define PIONEER_Q38_NEXT_CONV_NATIVE 1
#include "pioneer_q38_next_projection_native_full_compile.cpp"
extern "C" bool pioneer_next_conv_run(PioneerStoreContext& c,PioneerQ38ResidentDecoderResult& r){return RunQ38NextConv(c,r);}
