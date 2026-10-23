/* SPDX-License-Identifier: MIT */
// COMPILE ONLY exact tag45,NO public selector/driver install/hardware grant.
#define PIONEER_Q38_GDN_INPUT_OWNER 1
#include "pioneer_q38_resident_decoder_native_full_compile.cpp"
extern "C" bool pioneer_gdn_input_native_prepare(PioneerStoreContext& c,PioneerQ38ResidentDecoderResult& r){return PrepareQ38ResidentGdnInputs(c,r);}
