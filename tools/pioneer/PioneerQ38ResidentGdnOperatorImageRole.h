/* SPDX-License-Identifier: MIT */
#ifdef PIONEER_Q38_MIXED_RECURRENT_BLOCK_ROOT_IMAGE
#error A separate recurrent BLOCK parent cannot select an existing image role
#endif
#if defined(PIONEER_Q38_MIXED_RECURRENT_COHORT_ROOT_IMAGE) || defined(PIONEER_Q38_MIXED_ROOT_CAPSULE_IMAGE)
#error New recurrent ROOT/cohort capsule cannot select an existing image role
#endif
#ifdef PIONEER_Q38_MIXED_RECURRENT_PROJECTION_ROOT_IMAGE
#error Separate recurrent projection image cannot select an existing image role
#endif
#include "PioneerQ38ResidentOperatorImageRole.h"
#if !defined(PIONEER_Q38_RESIDENT_GDN_OPERATOR_IMAGE) || PIONEER_Q38_RESIDENT_GDN_OPERATOR_IMAGE != 1
#error Exact separate OFFLINE returning GDN service IMAGE required
#endif
#if defined(PIONEER_Q38_RESIDENT_COHORT_ROOT_IMAGE) || defined(PIONEER_Q38_RESIDENT_OUTPUT_ROOT_IMAGE) || defined(PIONEER_Q38_RGDN_ROOT_QUALIFICATION) || defined(PIONEER_Q38_RGDN_ROOT_NATIVE) || defined(PIONEER_Q38_NATIVE_Q8_LAYOUT) || defined(PIONEER_Q38_GDN_GATED_NORM_LOCAL)
#error Returning GDN service is separate from root/CPU-reference/alternate-layout diagnostics
#endif
