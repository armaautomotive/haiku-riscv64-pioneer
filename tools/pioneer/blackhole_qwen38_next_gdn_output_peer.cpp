/* SPDX-License-Identifier: MIT */
// SAME persistent Q8_0 matmul template, distinct QOI2 helper image.
// Old QFP3 boot word is NOT proof that this complete image is installed.
#include "PioneerQ38NextGdnOutputImageOnly.h"
#if !defined(PIONEER_Q38_NEXT_GDN_OUTPUT_INPUT_IMAGE) || PIONEER_Q38_NEXT_GDN_OUTPUT_INPUT_IMAGE != 1 || defined(PIONEER_Q38_GDN_OUTPUT_INPUT_IMAGE) || defined(PIONEER_Q38_GDN_OUTPUT_GRAPH_ROOT) || defined(PIONEER_Q38_NEXT_GDN_OUTPUT_GRAPH_ROOT) || defined(PIONEER_Q38_NEXT_GDN_OUTPUT_GATHER_IMAGE)
#error Exact standalone QOI2 persistent peer required
#endif
#define PIONEER_Q38_GDN_OUTPUT_INPUT_IMAGE 1
#include "blackhole_qwen38_int_sfpu_persistent.cpp"
