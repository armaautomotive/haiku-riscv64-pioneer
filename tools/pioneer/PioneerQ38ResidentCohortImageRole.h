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
// Rechecked on EVERY include. A separate OFFLINE ROOT,not a peer image.
#include "PioneerQ38ResidentOperatorImageRole.h"
#if !defined(PIONEER_Q38_RESIDENT_COHORT_ROOT_IMAGE) || PIONEER_Q38_RESIDENT_COHORT_ROOT_IMAGE != 1
#error Exact separate OFFLINE resident cohort ROOT IMAGE required
#endif
#if defined(PIONEER_Q38_RESIDENT_GDN_OPERATOR_IMAGE) || defined(PIONEER_Q38_RESIDENT_GDN_OUTPUT_OPERATOR_IMAGE) || defined(PIONEER_Q38_RGDN_ROOT_QUALIFICATION) || defined(PIONEER_Q38_RGDN_ROOT_NATIVE) || defined(PIONEER_Q38_PROJECTION_GRAPH_ROOT) || defined(PIONEER_Q38_FFN_GRAPH_ROOT) || defined(PIONEER_Q38_NATIVE_Q8_LAYOUT)
#error Cohort ROOT is not a peer/legacy ROOT/native/CPU arithmetic image
#endif
