/* SPDX-License-Identifier: MIT */
// Deliberately rechecked on every include,including after a layout bind.
#include "PioneerQ38ResidentGdnOperatorImageRole.h"
#if !defined(PIONEER_Q38_WIDE_GDN_RETURNING_IMAGE) || PIONEER_Q38_WIDE_GDN_RETURNING_IMAGE != 1 || !defined(PIONEER_Q38_RGDN_COMBINED_STAGE) || PIONEER_Q38_RGDN_COMBINED_STAGE != 1 || !defined(PIONEER_Q38_MIXED_CAPSULE_IMAGE) || PIONEER_Q38_MIXED_CAPSULE_IMAGE != 1
#error Exact separately selected returning wide-GDN IMAGE and combined-stage/capsule parents required
#endif
#if defined(PIONEER_Q38_WIDE_DELTA_INSTRUCTION_TEST) || defined(PIONEER_Q38_WIDE_DELTA_EXPECTED_OBSERVER_TEST) || defined(PIONEER_Q38_LAYER29_WIDE_EMITTED_DIAGNOSTIC) || defined(PIONEER_Q38_LAYER29_EMITTED_DELTA_DIAGNOSTIC) || defined(PIONEER_Q38_LAYER29_WIDE_REDUCTION_DIAGNOSTIC) || defined(PIONEER_Q38_TOKEN_NATIVE_CONTEXT) || defined(PIONEER_Q38_TOKEN_TWIN_BATCH_PREFETCH_NATIVE_CONTEXT) || defined(PIONEER_Q38_TOKEN_BATCHED_OUTPUT_CLOSURE_NATIVE_CONTEXT)
#error Returning wide-GDN IMAGE is NOT a HOST/numerical/native/old-work context
#endif
#if defined(PIONEER_Q38_TOKEN_WIDE_GDN_PROGRAMS) || defined(PIONEER_Q38_TOKEN_WIDE_GDN_NATIVE_CONTEXT) || defined(PIONEER_Q38_TOKEN_WIDE_GDN_BENCHMARK_NATIVE)
#error Returning shader BUILD cannot alias its separate token DATA/native family
#endif
