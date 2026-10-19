/* SPDX-License-Identifier: MIT */
// OFFLINE ONLY: exact INTEGER matrix with resident FP32 SFPU affine state.
// Not the failed BF16 image. A new image pin/native qualification is required.
#define PIONEER_Q38_SFPU_ACCUMULATOR
#define PIONEER_Q38_SFPU_INTEGER
#include "blackhole_qwen38_vector_resident_persistent.cpp"
