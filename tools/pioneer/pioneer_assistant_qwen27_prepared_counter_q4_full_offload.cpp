/* SPDX-License-Identifier: MIT */
#if !defined(PIONEER_Q38_FULL_OFFLOAD_COUNTER_UNROLL) || PIONEER_Q38_FULL_OFFLOAD_COUNTER_UNROLL != 1
#error Full-card prepared counter Q4 requires its exact separate USER worker revision
#endif
#include "pioneer_assistant_qwen27_prepared_q4_full_offload.cpp"
