/* SPDX-License-Identifier: MIT */
// Actual owner templates and the new private getter, kernel COMPILE ONLY.
#define PIONEER_Q38_RESIDENT_OUTPUT_OWNER 1
#define PIONEER_Q38_PROJECTION_RESULT_STASH 1
#include "pioneer_q38_projection_execution_kernel_compile.cpp"
extern "C" bool pioneer_resident_projection_input_lineage(const PioneerQ38ProjectionGraphOwner& owner){
 const auto* input=owner.ResidentInput();return input && input->Current();
}
