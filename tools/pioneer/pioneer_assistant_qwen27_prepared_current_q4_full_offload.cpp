/* SPDX-License-Identifier: MIT */
// Distinct USER worker entry point; never silently fall back to the older
// tag97 program package when building the current tag102 prepared-Q4 worker.
#if !defined(PIONEER_Q38_FULL_OFFLOAD_PREPARED_Q4_CURRENT_WORKER) \
 || PIONEER_Q38_FULL_OFFLOAD_PREPARED_Q4_CURRENT_WORKER != 1
#error Current prepared-Q4 Assistant worker requires its explicit tag102 role
#endif
#include "pioneer_assistant_qwen27_prepared_q4_full_offload.cpp"
