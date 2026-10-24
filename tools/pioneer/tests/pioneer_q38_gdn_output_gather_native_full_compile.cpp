/* SPDX-License-Identifier: MIT */
// Kernel ABI metadata methods only;no ROOT/peer image selection or IO.
#include "pioneer_q38_gdn_output_input_native_full_compile.cpp"
extern "C" bool pioneer_gdn_output_gather_descriptors(PioneerStoreContext& c,PioneerQ38GdnOutputProjectionPlan& plan,
 uint32_t (&descriptor)[64],uint32_t (&sources)[24][16]){
 auto* m=c.q38EmbeddingModel;
 return m && m->projectionCohort && plan.GatherDescriptor(*m->projectionCohort,descriptor) && plan.GatherSources(*m->projectionCohort,sources);
}
