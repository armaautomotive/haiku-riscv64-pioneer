/* SPDX-License-Identifier: MIT */
#if defined(PIONEER_Q38_MIXED_RECURRENT_COHORT_ROOT_IMAGE) || defined(PIONEER_Q38_MIXED_ROOT_CAPSULE_IMAGE)
#error New recurrent ROOT/cohort capsule cannot select an existing image role
#endif
#ifdef PIONEER_Q38_MIXED_RECURRENT_PROJECTION_ROOT_IMAGE
#error Separate recurrent projection image cannot select an existing image role
#endif
// Deliberately no include guard: late role changes must still fail.
#include "PioneerQ38ResidentGdnOperatorImageRole.h"
#if !defined(PIONEER_Q38_RESIDENT_GDN_OUTPUT_OPERATOR_IMAGE) || PIONEER_Q38_RESIDENT_GDN_OUTPUT_OPERATOR_IMAGE != 1
#error Exact separate OFFLINE all-layer GDN output service IMAGE required
#endif
#if defined(PIONEER_Q38_GDN_OUTPUT_INPUT_IMAGE) || defined(PIONEER_Q38_PROJECTION_MATRIX_ROOT) || defined(PIONEER_Q38_PROJECTION_GRAPH_ROOT) || defined(PIONEER_Q38_FFN_GRAPH_ROOT)
#error Returning all-layer GDN output peer is not an old QOI1 or ROOT image
#endif
