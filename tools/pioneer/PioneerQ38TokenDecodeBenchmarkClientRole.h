/* SPDX-License-Identifier: MIT */
// Separate USER lifecycle, never a default/short diagnostic or Work alias.
#if !defined(PIONEER_Q38_TOKEN_DECODE_BENCHMARK_CLIENT) || PIONEER_Q38_TOKEN_DECODE_BENCHMARK_CLIENT != 1 \
 || !defined(PIONEER_Q38_TOKEN_HIDDEN_VALIDATION_CLIENT) || PIONEER_Q38_TOKEN_HIDDEN_VALIDATION_CLIENT != 1 \
 || defined(PIONEER_Q38_TOKEN_DECODE_BENCHMARK_NATIVE) \
 || defined(PIONEER_Q38_TOKEN_GENERATION_HOST_TIMING)
#error Exact separate decode benchmark USER revision required
#endif
#if defined(_KERNEL_MODE) || defined(PIONEER_ATTENTION_FREESTANDING) || defined(__FAST_MATH__) \
 || defined(PIONEER_Q38_TOKEN_NATIVE_CONTEXT) || defined(PIONEER_Q38_TOKEN_VALIDATION_NATIVE) \
 || defined(PIONEER_Q38_TOKEN_HIDDEN_VALIDATION_NATIVE) || defined(PIONEER_Q38_TOKEN_VALIDATION_CLIENT) \
 || defined(PIONEER_Q38_TOKEN_BOUNDARY_VALIDATION_CLIENT) || defined(PIONEER_Q38_CPU_VALIDATION) \
 || defined(PIONEER_Q38_FULL_MODEL_CPU_REFERENCE) || defined(PIONEER_Q38_WORK_INFERENCE) \
 || defined(TT_PIONEER_Q38_WORK_MODE) || defined(PIONEER_Q38_BENCHMARK_HOST) \
 || defined(PIONEER_Q38_TOKEN_POSTED_CANCEL) || defined(PIONEER_Q38_POSTED_CANCEL_TEST)
#error Decode benchmark USER cannot alias kernel,native,old USER,CPU,Work or HOST test roles
#endif
// The shared USER header additionally rejects kernel/native/CPU/Work/test
// contamination. Selecting this header alone grants no device capability.
