/* SPDX-License-Identifier: MIT */
#include "PioneerQ38DenseProjectionImageRole.h"
#if !defined(PIONEER_Q38_PROJECTION_MATRIX_ROOT) || PIONEER_Q38_PROJECTION_MATRIX_ROOT != 1
#error Dense projection uses ONLY the separately scoped QDM1 matrix root
#endif
#include "blackhole_qwen38_dispatch.cpp"
