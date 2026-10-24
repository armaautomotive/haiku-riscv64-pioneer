/* SPDX-License-Identifier: MIT */
// Offline emitted-instruction image roles ONLY. Never a native/Work/DEBUG
// selector. Compiler flags cannot turn descriptor DATA into ownership.
// Role rejection precedes the include guard: reinclusion cannot hide a
// later native/reference/Work definition behind an earlier offline read.
#if defined(_KERNEL_MODE) || defined(PIONEER_Q38_CPU_VALIDATION) || defined(PIONEER_Q38_WORK_INFERENCE) || defined(PIONEER_Q38_NEXT_GDN_OUTPUT_NATIVE) || defined(PIONEER_Q38_NEXT_GDN_NATIVE) || defined(PIONEER_Q38_NEXT_CONV_NATIVE) || defined(PIONEER_Q38_NEXT_PROJECTION_NATIVE) || defined(PIONEER_Q38_NEXT_RMS_NATIVE) || defined(PIONEER_Q38_FFN_RESIDUAL_NATIVE) || defined(PIONEER_Q38_RESIDUAL_RMS_NATIVE)
#error QOP2 family is offline image-only, never native/reference/Work
#endif
#if defined(PIONEER_Q38_RESIDENT_DECODER_NATIVE) || defined(PIONEER_Q38_RESIDENT_DECODER_VALIDATION_NATIVE) || defined(PIONEER_Q38_RESIDENT_DECODER_OWNER) || defined(PIONEER_Q38_PROJECTION_EXECUTION_OWNER) || defined(PIONEER_Q38_GDN_INPUT_OWNER) || defined(PIONEER_Q38_GDN_STATE_INIT_NATIVE) || defined(PIONEER_Q38_GDN_RECURRENT_NATIVE) || defined(PIONEER_Q38_GDN_RECURRENT_VALIDATION_NATIVE) || defined(PIONEER_Q38_GDN_GATED_NATIVE) || defined(PIONEER_Q38_GDN_RENDEZVOUS_NATIVE) || defined(PIONEER_Q38_GDN_RENDEZVOUS_VALIDATION_NATIVE) || defined(PIONEER_Q38_GDN_OUTPUT_PRODUCER_NATIVE) || defined(PIONEER_Q38_GDN_OUTPUT_EXECUTION_NATIVE)
#error QOP2 image never shares native producer or DEBUG ownership
#endif
#if defined(PIONEER_Q38_EMBEDDING_NATIVE) || defined(PIONEER_Q38_EMBEDDING_VALIDATION_NATIVE) || defined(PIONEER_Q38_LEARNED_RMS_NATIVE) || defined(PIONEER_Q38_LEARNED_RMS_VALIDATION_NATIVE) || defined(PIONEER_Q38_PROJECTION_VALIDATION_NATIVE) || defined(PIONEER_Q38_GDN_INPUT_VALIDATION_NATIVE) || defined(PIONEER_Q38_PROJECTION_RESULT_STASH) || defined(PIONEER_Q38_PROJECTION_NEXT_ROOT)
#error QOP2 image cannot alias old/native input or result roles
#endif
#ifndef PIONEER_Q38_NEXT_GDN_OUTPUT_IMAGE_ONLY_H
#define PIONEER_Q38_NEXT_GDN_OUTPUT_IMAGE_ONLY_H
#endif
