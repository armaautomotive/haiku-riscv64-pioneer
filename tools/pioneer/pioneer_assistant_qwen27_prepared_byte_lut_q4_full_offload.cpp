/* SPDX-License-Identifier: MIT */
// Distinct USER worker entry point for tag104's byte-index activation LUT.
#if !defined(PIONEER_Q38_FULL_OFFLOAD_PREPARED_Q4_ACTIVATION_BYTE_LUT_WORKER) \
 || PIONEER_Q38_FULL_OFFLOAD_PREPARED_Q4_ACTIVATION_BYTE_LUT_WORKER != 1 \
 || defined(PIONEER_Q38_FULL_OFFLOAD_PREPARED_Q4_CURRENT_WORKER) \
 || defined(PIONEER_Q38_FULL_OFFLOAD_PREPARED_Q4_ACTIVATION_LUT_WORKER)
#error Prepared-Q4 byte-index activation-LUT Assistant worker requires its separate tag104 role
#endif
#include "pioneer_assistant_qwen27_prepared_q4_full_offload.cpp"
