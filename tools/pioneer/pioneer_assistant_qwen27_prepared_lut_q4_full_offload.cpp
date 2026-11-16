/* SPDX-License-Identifier: MIT */
// Distinct USER worker entry point for the separately tagged tag103
// activation-LUT Q4 package. Never alias tag102 or the default worker.
#if !defined(PIONEER_Q38_FULL_OFFLOAD_PREPARED_Q4_ACTIVATION_LUT_WORKER) \
 || PIONEER_Q38_FULL_OFFLOAD_PREPARED_Q4_ACTIVATION_LUT_WORKER != 1 \
 || defined(PIONEER_Q38_FULL_OFFLOAD_PREPARED_Q4_CURRENT_WORKER)
#error Prepared-Q4 activation-LUT Assistant worker requires its separate tag103 role
#endif
#include "pioneer_assistant_qwen27_prepared_q4_full_offload.cpp"
