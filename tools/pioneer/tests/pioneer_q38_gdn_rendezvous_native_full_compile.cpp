/* SPDX-License-Identifier: MIT */
// Actual timed peer rendezvous/ENTIRE release deadline,COMPILE ONLY51.
#define PIONEER_Q38_GDN_RENDEZVOUS_NATIVE 1
#include "pioneer_q38_gdn_gated_native_full_compile.cpp"
extern "C" bool pioneer_gdn_rendezvous_native_run(PioneerStoreContext& c,PioneerQ38ResidentDecoderResult& r){return RunQ38ResidentGdnRecurrent(c,r);}
