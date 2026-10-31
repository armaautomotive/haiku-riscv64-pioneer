/* SPDX-License-Identifier: MIT */
// Separate immutable revision; recheck every include,including defined0.
#ifndef PIONEER_Q38_TOKEN_JOB_BOOT_FOLD_SELECTION_H
#define PIONEER_Q38_TOKEN_JOB_BOOT_FOLD_SELECTION_H
#ifdef PIONEER_Q38_TOKEN_JOB_BOOT_FOLD_VALIDATION_NATIVE
enum {PioneerQ38TokenJobBootFoldSelection=3};
#elif defined(PIONEER_Q38_TOKEN_JOB_BOOT_FOLD_NATIVE_CONTEXT)
enum {PioneerQ38TokenJobBootFoldSelection=2};
#elif defined(PIONEER_Q38_TOKEN_JOB_BOOT_FOLD_PROGRAMS)
enum {PioneerQ38TokenJobBootFoldSelection=1};
#else
enum {PioneerQ38TokenJobBootFoldSelection=0};
#endif
#endif
#ifdef PIONEER_Q38_TOKEN_JOB_BOOT_FOLD_PROGRAMS
#if PIONEER_Q38_TOKEN_JOB_BOOT_FOLD_PROGRAMS != 1 \
 || !defined(PIONEER_Q38_TOKEN_PRECISE_BETA_PROGRAMS) || PIONEER_Q38_TOKEN_PRECISE_BETA_PROGRAMS != 1 \
 || !defined(PIONEER_Q38_TOKEN_FAULT_PROGRAMS) || PIONEER_Q38_TOKEN_FAULT_PROGRAMS != 1 \
 || !defined(PIONEER_Q38_TOKEN_PUSHED_PROGRAMS) || PIONEER_Q38_TOKEN_PUSHED_PROGRAMS != 1 \
 || defined(PIONEER_Q38_TOKEN_PRECISE_BETA_PROGRAMS_H) || defined(PIONEER_Q38_TOKEN_FAULT_PROGRAMS_H) \
 || defined(PIONEER_Q38_TOKEN_PUSHED_PROGRAMS_H) || defined(PIONEER_Q38_TOKEN_PROGRAMS_H) \
 || defined(PIONEER_Q38_TOKEN_PRECISE_PROGRAMS)
#error Job boot fold DATA requires its exact separate beta/fault/G3 revision before any old package
#endif
#if defined(PIONEER_Q38_TOKEN_PRECISE_BETA_VALIDATION_NATIVE) || defined(PIONEER_Q38_TOKEN_FAULT_VALIDATION_NATIVE) \
 || defined(PIONEER_Q38_TOKEN_HIDDEN_VALIDATION_NATIVE)
#ifndef PIONEER_Q38_TOKEN_JOB_BOOT_FOLD_VALIDATION_NATIVE
#error Job boot fold code requires its explicit physical revision,not an old physical owner
#endif
#endif
#if defined(PIONEER_Q38_TOKEN_VALIDATION_NATIVE) \
 || defined(PIONEER_Q38_TOKEN_VALIDATION_CLIENT) || defined(PIONEER_Q38_TOKEN_HIDDEN_VALIDATION_CLIENT) \
 || defined(PIONEER_Q38_TOKEN_BOUNDARY_VALIDATION_CLIENT) || defined(PIONEER_Q38_CPU_VALIDATION) \
 || defined(PIONEER_Q38_FULL_MODEL_CPU_REFERENCE) || defined(PIONEER_Q38_WORK_INFERENCE) \
 || defined(TT_PIONEER_Q38_WORK_MODE) || defined(PIONEER_Q38_JOB_BOOT_FOLD_IMAGE) \
 || defined(PIONEER_Q38_GLOBAL_TOKEN_ROOT_IMAGE) || defined(PIONEER_Q38_GLOBAL_CHAIN_COMPONENT_IMAGE) \
 || defined(PIONEER_Q38_GLOBAL_CHAIN_PUSHED_COMPLETION) || defined(PIONEER_Q38_GLOBAL_WORKER_IMAGE) \
 || defined(__FAST_MATH__)
#error Job boot fold DATA cannot alias an old physical,USER,CPU,image or Work role
#endif
#ifdef PIONEER_Q38_TOKEN_JOB_BOOT_FOLD_VALIDATION_NATIVE
static_assert(PioneerQ38TokenJobBootFoldSelection==3,"late physical job boot fold selection");
#include "../../src/add-ons/kernel/drivers/misc/PioneerQ38TokenJobBootFoldValidationRole.h"
#elif defined(PIONEER_Q38_TOKEN_JOB_BOOT_FOLD_NATIVE_CONTEXT)
#if PIONEER_Q38_TOKEN_JOB_BOOT_FOLD_NATIVE_CONTEXT != 1 \
 || !defined(_KERNEL_MODE) || _KERNEL_MODE != 1 \
 || !defined(PIONEER_Q38_TOKEN_NATIVE_CONTEXT) || PIONEER_Q38_TOKEN_NATIVE_CONTEXT != 1 \
 || !defined(PIONEER_Q38_TOKEN_CONTEXT_COMPILE_ONLY) || PIONEER_Q38_TOKEN_CONTEXT_COMPILE_ONLY != 1 \
 || !defined(PIONEER_Q38_TOKEN_PUSHED_NATIVE_CONTEXT) || PIONEER_Q38_TOKEN_PUSHED_NATIVE_CONTEXT != 1 \
 || !defined(PIONEER_Q38_TOKEN_HELD_HIDDEN_DIAGNOSTIC) || PIONEER_Q38_TOKEN_HELD_HIDDEN_DIAGNOSTIC != 1 \
 || !defined(PIONEER_DRIVER_BUILD_TAG) || PIONEER_DRIVER_BUILD_TAG != 0x5154334e4f445256ULL
#error Job boot fold native revision is explicit private QT3NODRV COMPILE-ONLY
#endif
static_assert(PioneerQ38TokenJobBootFoldSelection==2,"late private job boot fold selection");
#include "../../src/add-ons/kernel/drivers/misc/PioneerQ38TokenNativeRole.h"
#else
static_assert(PioneerQ38TokenJobBootFoldSelection==1,"late job boot fold DATA selection");
#if defined(_KERNEL_MODE) || defined(PIONEER_Q38_TOKEN_NATIVE_CONTEXT) || defined(PIONEER_Q38_TOKEN_PUSHED_NATIVE_CONTEXT)
#error Job boot fold programs require their explicit new native revision,not an old owner
#endif
#endif
#else
#if defined(PIONEER_Q38_TOKEN_JOB_BOOT_FOLD_NATIVE_CONTEXT) || defined(PIONEER_Q38_TOKEN_JOB_BOOT_FOLD_VALIDATION_NATIVE)
#error Job boot fold native revision requires its own actual code package
#endif
static_assert(PioneerQ38TokenJobBootFoldSelection==0,"late job boot fold program removal");
#endif
