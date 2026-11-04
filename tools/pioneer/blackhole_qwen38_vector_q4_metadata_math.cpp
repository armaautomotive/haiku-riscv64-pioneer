/* SPDX-License-Identifier: MIT */
// Separate shader BUILD only. This changes the actual Work-Q4 calculation
// path inside the full returning worker;it grants no native/physical use.
#if !defined(PIONEER_Q38_VECTOR_Q4_METADATA_IMAGE) || PIONEER_Q38_VECTOR_Q4_METADATA_IMAGE != 1 \
 || !defined(PIONEER_Q38_VECTOR_Q4_METADATA) || PIONEER_Q38_VECTOR_Q4_METADATA != 1
#error Exact distinct vector metadata candidate required
#endif
#include "blackhole_qwen38_twin_batch_prefetch_math.cpp"
