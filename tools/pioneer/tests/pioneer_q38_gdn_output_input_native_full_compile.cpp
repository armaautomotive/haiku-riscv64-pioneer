/* SPDX-License-Identifier: MIT */
// Force actual kernel-ABI DATA-only descriptor method;NO image install/IO.
#include "pioneer_q38_gdn_output_producer_native_full_compile.cpp"
extern "C" bool pioneer_gdn_output_input_descriptor(PioneerStoreContext& c,PioneerQ38GdnOutputProjectionPlan& plan,unsigned chunk,uint32_t (&out)[64]){
 auto* m=c.q38EmbeddingModel;
 return m && m->projectionCohort && plan.InputDescriptor(chunk,*m->projectionCohort,out);
}
