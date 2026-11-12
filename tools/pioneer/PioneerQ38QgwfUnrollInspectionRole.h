/* SPDX-License-Identifier: MIT */
// Fake-tag native inspection only; not a loadable driver or Work authority.
#include "PioneerQ38QgwfFrontierNativeInspectionRole.h"
#if !defined(PIONEER_Q38_QGWF_UNROLL_NATIVE_INSPECTION) || PIONEER_Q38_QGWF_UNROLL_NATIVE_INSPECTION != 1 \
 || defined(PIONEER_Q38_QGWF_UNROLL_NATIVE_145) || defined(PIONEER_Q38_QGWF_UNROLL_CLIENT_145) \
 || defined(PIONEER_Q38_WORK_INFERENCE) || defined(TT_PIONEER_Q38_WORK_MODE) \
 || defined(PIONEER_Q38_CPU_VALIDATION) || defined(PIONEER_Q38_FULL_MODEL_CPU_REFERENCE)
#error Unrolled QGWF fake-tag inspection cannot be promoted to a native candidate
#endif
