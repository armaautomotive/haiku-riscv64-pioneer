/* SPDX-License-Identifier: MIT */
// Kernel ABI canonical DATA factory only;no image entry/driver selection.
#include "pioneer_q38_gdn_output_gather_native_full_compile.cpp"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38GdnOutputBootstrapPlan.h"
extern "C" bool pioneer_gdn_output_bootstrap(PioneerStoreContext& c,PioneerQ38GdnOutputProjectionPlan& input,
 PioneerQ38GdnOutputBootstrapPlan& bootstrap){
 auto* m=c.q38EmbeddingModel;
 return m && m->projectionCohort && bootstrap.Build(input,*m->projectionCohort)
  && bootstrap.Matches(input,*m->projectionCohort) && bootstrap.Manifest() && bootstrap.Sources();
}
extern "C" bool pioneer_gdn_output_matrix_metadata(const uint32_t* descriptor,const uint32_t* header,const uint32_t* banks,
 const uint32_t parts[119][8],const uint32_t* rows){
 return PioneerQ38GdnOutputMatrixShape(descriptor,header,banks,parts,rows);
}
