/* SPDX-License-Identifier: MIT */
// ACTUAL complete native context/factories/mapping/scope/retention/END,
// compilation ONLY;not a selector,linked role or device qualification.
#define PIONEER_Q38_PROJECTION_EXECUTION_OWNER 1
#include "pioneer_q38_learned_rms_native_full_compile.cpp"
extern "C" bool pioneer_projection_native_kernel_prepare(PioneerStoreContext& c,PioneerQ38ProjectionGraphResult& result){
 return PrepareQ38ProjectionExecution(c,result);
}
extern "C" bool pioneer_projection_native_kernel_run(PioneerStoreContext& c,unsigned role,PioneerQ38ProjectionGraphResult& result){
 return RunQ38ProjectionExecution(c,role,result);
}
extern "C" bool pioneer_projection_native_kernel_close(PioneerStoreContext& c,PioneerQ38ProjectionGraphResult& result){
 return CloseQ38ProjectionExecution(c,result);
}
