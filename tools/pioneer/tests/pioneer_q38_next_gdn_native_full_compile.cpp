/* SPDX-License-Identifier: MIT */
#define PIONEER_Q38_NEXT_GDN_NATIVE 1
#include "pioneer_q38_next_conv_native_full_compile.cpp"
extern "C" bool pioneer_next_gdn_run(PioneerStoreContext& c,PioneerQ38ResidentDecoderResult& r){return RunQ38NextGdn(c,r);}
