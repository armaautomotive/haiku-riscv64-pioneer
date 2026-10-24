/* SPDX-License-Identifier: MIT */
// Actual SAME native54 claim/images/boot/ROOT run+STOP/restore/END code.
// COMPILE ONLY,not a public selection or physical qualification.
#define PIONEER_Q38_GDN_OUTPUT_EXECUTION_NATIVE 1
#include "pioneer_q38_gdn_output_bootstrap_native_full_compile.cpp"
extern "C" bool pioneer_gdn_output_execution_run(PioneerStoreContext& c,PioneerQ38ResidentDecoderResult& r){
 return RunQ38ResidentGdnOutput(c,r);
}
