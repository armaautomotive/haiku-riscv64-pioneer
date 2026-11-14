/* SPDX-License-Identifier: MIT */
// Rechecked on every inclusion. This role exists only for the private offline
// returning-GDN image variant; it cannot select Work or native execution.
#if !defined(PIONEER_Q38_GDN_INPLACE_STATE_EXPERIMENT) || PIONEER_Q38_GDN_INPLACE_STATE_EXPERIMENT != 1 \
 || !defined(PIONEER_Q38_RESIDENT_GDN_OPERATOR_IMAGE) || PIONEER_Q38_RESIDENT_GDN_OPERATOR_IMAGE != 1 \
 || defined(PIONEER_Q38_WORK_INFERENCE) || defined(PIONEER_Q38_CPU_VALIDATION) \
 || defined(PIONEER_Q38_FULL_MODEL_CPU_REFERENCE) || defined(PIONEER_Q38_FULL_OFFLOAD_NATIVE) \
 || defined(PIONEER_Q38_HYBRID_FULL_NATIVE) || defined(_KERNEL_MODE)
#error In-place GDN L1 alias is a separate offline returning-image experiment only
#endif
