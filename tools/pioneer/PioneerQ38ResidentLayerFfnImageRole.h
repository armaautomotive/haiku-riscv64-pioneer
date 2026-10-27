/* SPDX-License-Identifier: MIT */
#ifdef PIONEER_Q38_MIXED_RECURRENT_PROJECTION_ROOT_IMAGE
#error Separate recurrent projection image cannot select an existing image role
#endif
// No guard:late role changes are rechecked on every inclusion.
#if !defined(PIONEER_Q38_RESIDENT_LAYER_FFN_IMAGE) || PIONEER_Q38_RESIDENT_LAYER_FFN_IMAGE != 1 \
 || !defined(PIONEER_Q38_FFN_GRAPH_ROOT) || PIONEER_Q38_FFN_GRAPH_ROOT != 1 \
 || !defined(PIONEER_Q38_FFN_GRAPH_RESIDUAL_ROOT) || PIONEER_Q38_FFN_GRAPH_RESIDUAL_ROOT != 1
#error Exact separate OFFLINE layer FFN component required
#endif
#if defined(PIONEER_Q38_RESIDENT_LAYER_ROOT_IMAGE) || defined(PIONEER_Q38_RESIDENT_LAYER_NORM_IMAGE) \
 || defined(PIONEER_Q38_RESIDENT_LAYER_NATIVE_COMPONENT) \
 || defined(PIONEER_Q38_RESIDENT_OUTPUT_ROOT_IMAGE) || defined(PIONEER_Q38_RESIDENT_OUTPUT_OWNER) \
 || defined(PIONEER_Q38_RESIDENT_OUTPUT_VALIDATION_NATIVE) || defined(PIONEER_Q38_RESIDENT_OUTPUT_CONTEXT_COMPILE_ONLY) \
 || defined(PIONEER_Q38_RESIDENT_OUTPUT_VALIDATION_CLIENT) || defined(PIONEER_Q38_WORK_INFERENCE) \
 || defined(PIONEER_Q38_CPU_VALIDATION) || defined(_KERNEL_MODE) || defined(__FAST_MATH__)
#error Layer FFN component never aliases ROOT,native,image,CPU or Work capabilities
#endif
