/* SPDX-License-Identifier: MIT */
#ifdef PIONEER_Q38_NEXT_FFN_RESIDUAL_NATIVE
#error Native65 never compiles LOCAL arithmetic diagnostics
#endif
#if defined(PIONEER_Q38_CONV_SILU_PRECISION_NATIVE) || defined(PIONEER_Q38_CONV_SILU_PRECISION_VALIDATION_CLIENT)
#error Separate native/CPU precision roles never compile LOCAL arithmetic helpers
#endif
// Intentionally outside any include guard: reject role changes on reinclude.
#if (defined(PIONEER_Q38_SILU_PRECISION_IMAGE) + defined(PIONEER_Q38_SILU_LEGACY_CONTROL_IMAGE)) != 1
#error Exact single LOCAL SiLU diagnostic role required
#endif
#if defined(PIONEER_Q38_SILU_PRECISION_IMAGE) && PIONEER_Q38_SILU_PRECISION_IMAGE != 1
#error Exact LOCAL precision image required
#endif
#if defined(PIONEER_Q38_SILU_LEGACY_CONTROL_IMAGE) && PIONEER_Q38_SILU_LEGACY_CONTROL_IMAGE != 1
#error Exact LOCAL legacy control required
#endif
#if defined(_KERNEL_MODE) || defined(__FAST_MATH__) || defined(TT_PIONEER_DRIVER_RELOAD)
#error LOCAL SiLU diagnostic has no native,CPU-reference,kernel or Work context
#endif
#if defined(PIONEER_DRIVER_BUILD_TAG) || defined(PIONEER_Q38_CPU_VALIDATION) || defined(PIONEER_Q38_WORK_INFERENCE)
#error LOCAL SiLU diagnostic has no native,CPU-reference,kernel or Work context
#endif
#if defined(PIONEER_Q38_NEXT_RESIDUAL_RMS_NATIVE) || defined(PIONEER_Q38_NEXT_GDN_OUTPUT_NATIVE) || defined(PIONEER_Q38_NEXT_GDN_NATIVE)
#error LOCAL SiLU diagnostic has no native,CPU-reference,kernel or Work context
#endif
#if defined(PIONEER_Q38_NEXT_CONV_NATIVE) || defined(PIONEER_Q38_NEXT_PROJECTION_NATIVE) || defined(PIONEER_Q38_NEXT_RMS_NATIVE)
#error LOCAL SiLU diagnostic has no native,CPU-reference,kernel or Work context
#endif
#if defined(PIONEER_Q38_FFN_RESIDUAL_NATIVE) || defined(PIONEER_Q38_RESIDUAL_RMS_NATIVE) || defined(PIONEER_Q38_GDN_OUTPUT_EXECUTION_NATIVE)
#error LOCAL SiLU diagnostic has no native,CPU-reference,kernel or Work context
#endif
#if defined(PIONEER_Q38_GDN_OUTPUT_PRODUCER_NATIVE) || defined(PIONEER_Q38_GDN_RENDEZVOUS_NATIVE) || defined(PIONEER_Q38_GDN_GATED_NATIVE)
#error LOCAL SiLU diagnostic has no native,CPU-reference,kernel or Work context
#endif
#if defined(PIONEER_Q38_GDN_RECURRENT_NATIVE) || defined(PIONEER_Q38_GDN_STATE_INIT_NATIVE) || defined(PIONEER_Q38_GDN_INPUT_OWNER)
#error LOCAL SiLU diagnostic has no native,CPU-reference,kernel or Work context
#endif
#if defined(PIONEER_Q38_RESIDENT_DECODER_NATIVE) || defined(PIONEER_Q38_RESIDENT_DECODER_OWNER) || defined(PIONEER_Q38_DECODER_RUNTIME)
#error LOCAL SiLU diagnostic has no native,CPU-reference,kernel or Work context
#endif
#if defined(PIONEER_Q38_PROJECTION_EXECUTION_OWNER) || defined(PIONEER_Q38_PROJECTION_RESULT_STASH) || defined(PIONEER_Q38_LEARNED_RMS_NATIVE)
#error LOCAL SiLU diagnostic has no native,CPU-reference,kernel or Work context
#endif
#if defined(PIONEER_Q38_EMBEDDING_NATIVE) || defined(PIONEER_Q38_GDN_OUTPUT_VALIDATION_NATIVE) || defined(PIONEER_Q38_GDN_RENDEZVOUS_VALIDATION_NATIVE)
#error LOCAL SiLU diagnostic has no native,CPU-reference,kernel or Work context
#endif
#if defined(PIONEER_Q38_GDN_RECURRENT_VALIDATION_NATIVE) || defined(PIONEER_Q38_GDN_INPUT_VALIDATION_NATIVE) || defined(PIONEER_Q38_RESIDENT_DECODER_VALIDATION_NATIVE)
#error LOCAL SiLU diagnostic has no native,CPU-reference,kernel or Work context
#endif
#if defined(PIONEER_Q38_PROJECTION_VALIDATION_NATIVE) || defined(PIONEER_Q38_LEARNED_RMS_VALIDATION_NATIVE) || defined(PIONEER_Q38_EMBEDDING_VALIDATION_NATIVE)
#error LOCAL SiLU diagnostic has no native,CPU-reference,kernel or Work context
#endif
#if defined(PIONEER_Q38_RESIDENT_DECODER_VALIDATION_CLIENT) || defined(PIONEER_Q38_GDN_INPUT_VALIDATION_CLIENT) || defined(PIONEER_Q38_GDN_RECURRENT_VALIDATION_CLIENT)
#error LOCAL SiLU diagnostic has no native,CPU-reference,kernel or Work context
#endif
#if defined(PIONEER_Q38_GDN_RENDEZVOUS_VALIDATION_CLIENT) || defined(PIONEER_Q38_GDN_OUTPUT_VALIDATION_CLIENT) || defined(PIONEER_Q38_LEARNED_RMS_VALIDATION_REFERENCE)
#error LOCAL SiLU diagnostic has no native,CPU-reference,kernel or Work context
#endif
#if defined(PIONEER_Q38_NEXT_GDN_OUTPUT_VALIDATION_REFERENCE)
#error LOCAL SiLU diagnostic has no native,CPU-reference,kernel or Work context
#endif
