/* SPDX-License-Identifier: MIT */
// Re-evaluate late/defined0 selections;this component is NOT a native grant.
#if !defined(PIONEER_Q38_RECURRENT_BLOCK_VALIDATION_READBACK) || PIONEER_Q38_RECURRENT_BLOCK_VALIDATION_READBACK != 1
#error Exact separate recurrent validation-readback component required
#endif
#if defined(PIONEER_Q38_CPU_VALIDATION) || defined(PIONEER_Q38_WORK_INFERENCE) || defined(__FAST_MATH__) \
 || defined(PIONEER_Q38_MIXED_LIVE_NATIVE_CONTEXT) || defined(PIONEER_Q38_MIXED_LIVE_VALIDATION_READBACK) \
 || defined(PIONEER_Q38_MIXED_LIVE_FINAL_RELEASE) || defined(PIONEER_Q38_MIXED_LIVE_SIGMOID_PROGRAMS) \
 || defined(PIONEER_Q38_RESIDENT_LAYER_NATIVE_CONTEXT) || defined(PIONEER_Q38_RESIDENT_OUTPUT_OWNER) \
 || defined(PIONEER_Q38_PROJECTION_EXECUTION_OWNER) || defined(PIONEER_Q38_LEARNED_RMS_NATIVE) \
 || defined(PIONEER_Q38_RESIDENT_DECODER_NATIVE) || defined(PIONEER_Q38_GDN_STATE_INIT_NATIVE) \
 || defined(PIONEER_Q38_MIXED_RECURRENT_BLOCK_ROOT_IMAGE) || defined(PIONEER_Q38_MIXED_RECURRENT_COHORT_ROOT_IMAGE) \
 || defined(PIONEER_Q38_MIXED_WORKER_IMAGE)
#error Recurrent readback is not a CPU oracle,Work,image or older native-role alias,including defined0
#endif
#ifdef PIONEER_Q38_RECURRENT_BLOCK_VALIDATION_NATIVE
#include "PioneerQ38RecurrentBlockValidationRole.h"
#elif defined(PIONEER_Q38_RECURRENT_BLOCK_NATIVE_CONTEXT)
#include "PioneerQ38RecurrentBlockNativeRole.h"
#endif
