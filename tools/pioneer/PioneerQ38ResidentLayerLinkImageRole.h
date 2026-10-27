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
#if defined(PIONEER_Q38_DENSE_PREPARE_IMAGE) || defined(PIONEER_Q38_DENSE_ATTENTION_IMAGE) || defined(PIONEER_Q38_DENSE_PROJECTION_IMAGE) || defined(PIONEER_Q38_DENSE_PARTITION_IMAGE) || defined(PIONEER_Q38_MIXED_LIVE_BLOCK_ROOT_IMAGE) || defined(PIONEER_Q38_MIXED_BLOCK_ROOT_IMAGE) || defined(PIONEER_Q38_MIXED_JOINED_ROOT_IMAGE) || defined(PIONEER_Q38_MIXED_PROJECTION_ROOT_IMAGE) || defined(PIONEER_Q38_MIXED_DENSE_ROOT_IMAGE) || defined(PIONEER_Q38_MIXED_WORKER_IMAGE) || defined(PIONEER_Q38_MIXED_CAPSULE_IMAGE)
#error Local dense-prepare IMAGE is not a layer-link IMAGE
#endif
#ifdef PIONEER_Q38_RESIDENT_CONTINUE_IMAGE
#error Continuation IMAGE has no existing native/CPU/Work/component role
#endif
// No guard: revalidate exact offline role even on late inclusion.
#ifdef PIONEER_Q38_RESIDENT_ADVANCE_IMAGE
#error Link and advance components have separate exact compilation roles
#endif
#if !defined(PIONEER_Q38_RESIDENT_LAYER_LINK_IMAGE) || PIONEER_Q38_RESIDENT_LAYER_LINK_IMAGE != 1
#error Exact separate device-layer-link image required
#endif
#if defined(PIONEER_Q38_RESIDENT_LAYER_NATIVE_COMPONENT) || defined(PIONEER_Q38_RESIDENT_LAYER_VALIDATION_NATIVE) \
 || defined(PIONEER_Q38_RESIDENT_LAYER_VALIDATION_CLIENT) || defined(PIONEER_Q38_RESIDENT_LAYER_NATIVE_CONTEXT) \
 || defined(PIONEER_Q38_RESIDENT_LAYER_CONTEXT_COMPILE_ONLY) \
 || defined(PIONEER_Q38_RESIDENT_LAYER_ROOT_IMAGE) || defined(PIONEER_Q38_RESIDENT_LAYER_NORM_IMAGE) || defined(PIONEER_Q38_RESIDENT_LAYER_FFN_IMAGE) \
 || defined(PIONEER_Q38_RESIDENT_OUTPUT_OWNER) || defined(PIONEER_Q38_RESIDENT_OUTPUT_VALIDATION_NATIVE) \
 || defined(PIONEER_Q38_RESIDENT_OUTPUT_CONTEXT_COMPILE_ONLY) || defined(PIONEER_Q38_RESIDENT_OUTPUT_VALIDATION_CLIENT) \
 || defined(PIONEER_Q38_RESIDENT_LAYER_OWNER) || defined(PIONEER_Q38_RESIDENT_DECODER_NATIVE) \
 || defined(PIONEER_Q38_NEXT_RMS_NATIVE) || defined(PIONEER_Q38_CPU_VALIDATION) || defined(PIONEER_Q38_WORK_INFERENCE) \
 || defined(_KERNEL_MODE) || defined(__FAST_MATH__)
#error Offline layer link never aliases native69, old58, CPU validation or Work
#endif
