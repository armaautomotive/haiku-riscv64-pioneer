/* SPDX-License-Identifier: MIT */
#include "PioneerQ38ResidentOperatorImageRole.h"
#if !defined(PIONEER_Q38_RESIDENT_GDN_OPERATOR_IMAGE) || PIONEER_Q38_RESIDENT_GDN_OPERATOR_IMAGE != 1
#error Exact separate OFFLINE returning GDN service IMAGE required
#endif
#if defined(PIONEER_Q38_RESIDENT_COHORT_ROOT_IMAGE) || defined(PIONEER_Q38_RGDN_ROOT_QUALIFICATION) || defined(PIONEER_Q38_RGDN_ROOT_NATIVE) || defined(PIONEER_Q38_NATIVE_Q8_LAYOUT) || defined(PIONEER_Q38_GDN_GATED_NORM_LOCAL)
#error Returning GDN service is separate from root/CPU-reference/alternate-layout diagnostics
#endif
