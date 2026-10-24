/* SPDX-License-Identifier: MIT */
// Actual SAME native51 -> gated producer/complete ssm_out geometry ONLY53.
#define PIONEER_Q38_GDN_OUTPUT_PRODUCER_NATIVE 1
#include "pioneer_q38_gdn_rendezvous_native_full_compile.cpp"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38GdnOutputProjectionPlan.h"
extern "C" bool pioneer_gdn_output_producer_plan(PioneerStoreContext& c,PioneerQ38GdnOutputProjectionPlan& plan){
 if(!c.q38EmbeddingModel)return false;
 const auto* input=c.q38EmbeddingModel->decoder.GatedOutput();
 return input && plan.Build(*input) && plan.Count()==119 && plan.InputAddress() && plan.OutputAddress();
}
