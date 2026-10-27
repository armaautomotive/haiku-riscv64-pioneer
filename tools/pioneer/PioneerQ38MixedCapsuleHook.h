/* SPDX-License-Identifier: MIT */
// Recheck the NEW root role before the guard: a prior no-op/worker include
// must never be promoted to a root seal by a late flag change.
#if defined(PIONEER_Q38_MIXED_ROOT_SEAL_BOUND) && !defined(PIONEER_Q38_MIXED_CAPSULE_HOOK_H)
#error The root hook witness is private to this header
#endif
#if defined(PIONEER_Q38_MIXED_ROOT_CAPSULE_IMAGE)
#if PIONEER_Q38_MIXED_ROOT_CAPSULE_IMAGE != 1 || defined(PIONEER_Q38_MIXED_CAPSULE_IMAGE) || defined(_KERNEL_MODE) || defined(PIONEER_Q38_WORK_INFERENCE) || defined(PIONEER_Q38_CPU_VALIDATION) || defined(__FAST_MATH__)
#error Exact separate recurrent ROOT numeric capsule only
#endif
#if defined(PIONEER_Q38_MIXED_RECURRENT_COHORT_ROOT_IMAGE) && defined(PIONEER_Q38_MIXED_RECURRENT_BLOCK_ROOT_IMAGE)
#error Exactly one private ROOT parent is required
#elif defined(PIONEER_Q38_MIXED_RECURRENT_COHORT_ROOT_IMAGE)
#include "PioneerQ38MixedRecurrentCohortRootImageRole.h"
#if PIONEER_Q38_MIXED_RECURRENT_COHORT_ROOT_IMAGE != 1
#error Exact cohort ROOT parent required
#endif
#elif defined(PIONEER_Q38_MIXED_RECURRENT_BLOCK_ROOT_IMAGE)
#include "PioneerQ38MixedRecurrentBlockRootImageRole.h"
#if PIONEER_Q38_MIXED_RECURRENT_BLOCK_ROOT_IMAGE != 1
#error Exact recurrent BLOCK ROOT parent required
#endif
#else
#error A numeric capsule has no implicit ROOT parent
#endif
#if defined(PIONEER_Q38_MIXED_CAPSULE_HOOK_H) && !defined(PIONEER_Q38_MIXED_ROOT_SEAL_BOUND)
#error A previously selected no-op or worker hook is not a root seal
#endif
#elif defined(PIONEER_Q38_MIXED_ROOT_SEAL_BOUND) || defined(PIONEER_Q38_MIXED_RECURRENT_COHORT_ROOT_IMAGE) || defined(PIONEER_Q38_MIXED_RECURRENT_BLOCK_ROOT_IMAGE)
#error The recurrent ROOT capsule requires its explicit unchanged root hook
#endif
#ifndef PIONEER_Q38_MIXED_CAPSULE_HOOK_H
#define PIONEER_Q38_MIXED_CAPSULE_HOOK_H
#ifdef PIONEER_Q38_MIXED_ROOT_CAPSULE_IMAGE
#define PIONEER_Q38_MIXED_ROOT_SEAL_BOUND 1
extern "C" void q38_mixed_root_seal();
#define PIONEER_Q38_MIXED_SEAL() q38_mixed_root_seal()
#elif defined(PIONEER_Q38_MIXED_CAPSULE_IMAGE)
#if PIONEER_Q38_MIXED_CAPSULE_IMAGE != 1 || defined(_KERNEL_MODE) || defined(PIONEER_Q38_WORK_INFERENCE) || defined(PIONEER_Q38_CPU_VALIDATION) || defined(__FAST_MATH__)
#error Internal OFFLINE capsule only, not native/CPU/Work
#endif
// ALL capsules reserve s11. Only the NEW front end sets it to its live
// private stack snapshot. No mutable global/mailbox manufactures a seal.
extern "C" void q38_mixed_seal();
#define PIONEER_Q38_MIXED_SEAL() q38_mixed_seal()
#else
#define PIONEER_Q38_MIXED_SEAL() ((void)0)
#endif
#endif
