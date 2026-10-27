/* SPDX-License-Identifier: MIT */
#ifdef PIONEER_Q38_MIXED_RECURRENT_PROJECTION_ROOT_IMAGE
#error Separate recurrent projection image cannot select an existing image role
#endif
// No guard:component roles are revalidated on every inclusion.
#include "PioneerQ38ResidentOutputRootImageRole.h"
#if !defined(PIONEER_Q38_RESIDENT_LAYER_ROOT_IMAGE) || PIONEER_Q38_RESIDENT_LAYER_ROOT_IMAGE != 1
#error Exact separate OFFLINE complete recurrent-layer ROOT required
#endif
#if defined(PIONEER_Q38_RESIDENT_LAYER_FFN_IMAGE) || defined(PIONEER_Q38_RESIDENT_LAYER_NORM_IMAGE) \
 || defined(PIONEER_Q38_RESIDENT_LAYER_NATIVE_COMPONENT) \
 || defined(PIONEER_Q38_RESIDENT_OUTPUT_OWNER) || defined(PIONEER_Q38_RESIDENT_OUTPUT_VALIDATION_NATIVE) \
 || defined(PIONEER_Q38_RESIDENT_OUTPUT_CONTEXT_COMPILE_ONLY) || defined(PIONEER_Q38_RESIDENT_OUTPUT_VALIDATION_CLIENT)
#error New layer ROOT is not a component,native68,compile67 or client alias
#endif
