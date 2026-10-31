/* SPDX-License-Identifier: MIT */
// Rechecked selection boundary, including late/defined-zero changes.
#ifndef PIONEER_Q38_TOKEN_PRECISE_BETA_SELECTION_H
#define PIONEER_Q38_TOKEN_PRECISE_BETA_SELECTION_H
#ifdef PIONEER_Q38_TOKEN_PRECISE_BETA_VALIDATION_NATIVE
enum {PioneerQ38TokenPreciseBetaSelection=2};
#elif defined(PIONEER_Q38_TOKEN_PRECISE_BETA_PROGRAMS)
enum {PioneerQ38TokenPreciseBetaSelection=1};
#else
enum {PioneerQ38TokenPreciseBetaSelection=0};
#endif
#endif
#ifdef PIONEER_Q38_TOKEN_PRECISE_BETA_PROGRAMS
#if PIONEER_Q38_TOKEN_PRECISE_BETA_PROGRAMS != 1 \
 || !defined(PIONEER_Q38_TOKEN_PUSHED_PROGRAMS) || PIONEER_Q38_TOKEN_PUSHED_PROGRAMS != 1 \
 || !defined(PIONEER_Q38_TOKEN_FAULT_PROGRAMS) || PIONEER_Q38_TOKEN_FAULT_PROGRAMS != 1 \
 || defined(PIONEER_Q38_TOKEN_PUSHED_PROGRAMS_H) || defined(PIONEER_Q38_TOKEN_FAULT_PROGRAMS_H) \
 || defined(PIONEER_Q38_TOKEN_PROGRAMS_H) || defined(PIONEER_Q38_TOKEN_PRECISE_PROGRAMS)
#error Precise-beta DATA requires its separate exact fault-ROOT/G3 selection before any old package
#endif
#ifdef PIONEER_Q38_TOKEN_PRECISE_BETA_VALIDATION_NATIVE
static_assert(PioneerQ38TokenPreciseBetaSelection==2,"late physical precise-beta selection");
#include "../../src/add-ons/kernel/drivers/misc/PioneerQ38TokenPreciseBetaValidationRole.h"
#else
static_assert(PioneerQ38TokenPreciseBetaSelection==1,"late precise-beta program selection");
#if defined(PIONEER_Q38_TOKEN_FAULT_VALIDATION_NATIVE) || defined(PIONEER_Q38_TOKEN_HIDDEN_VALIDATION_NATIVE)
#error Precise-beta programs require their separate explicit physical revision,not an old physical owner
#endif
#endif
#if defined(PIONEER_Q38_TOKEN_VALIDATION_NATIVE) || defined(PIONEER_Q38_TOKEN_VALIDATION_CLIENT) \
 || defined(PIONEER_Q38_TOKEN_HIDDEN_VALIDATION_CLIENT) || defined(PIONEER_Q38_TOKEN_BOUNDARY_VALIDATION_CLIENT) \
 || defined(PIONEER_Q38_CPU_VALIDATION) || defined(PIONEER_Q38_FULL_MODEL_CPU_REFERENCE) \
 || defined(PIONEER_Q38_WORK_INFERENCE) || defined(TT_PIONEER_Q38_WORK_MODE)
#error Precise-beta programs are not an old physical, USER, CPU-reference or Work route
#endif
#ifdef PIONEER_Q38_TOKEN_NATIVE_CONTEXT
#ifndef PIONEER_Q38_TOKEN_PRECISE_BETA_VALIDATION_NATIVE
#if !defined(PIONEER_Q38_TOKEN_CONTEXT_COMPILE_ONLY) || PIONEER_Q38_TOKEN_CONTEXT_COMPILE_ONLY != 1 \
 || !defined(PIONEER_Q38_TOKEN_PUSHED_NATIVE_CONTEXT) || PIONEER_Q38_TOKEN_PUSHED_NATIVE_CONTEXT != 1 \
 || !defined(PIONEER_Q38_TOKEN_HELD_HIDDEN_DIAGNOSTIC) || PIONEER_Q38_TOKEN_HELD_HIDDEN_DIAGNOSTIC != 1 \
 || !defined(PIONEER_DRIVER_BUILD_TAG) || PIONEER_DRIVER_BUILD_TAG != 0x5154334e4f445256ULL
#error Precise-beta native integration is private QT3NODRV COMPILE-ONLY
#endif
#endif
#elif defined(_KERNEL_MODE)
#error Precise-beta DATA has no standalone kernel or physical owner
#endif
#else
#ifdef PIONEER_Q38_TOKEN_PRECISE_BETA_VALIDATION_NATIVE
#error Physical precise-beta revision requires its actual separate program selection,including defined0
#endif
static_assert(PioneerQ38TokenPreciseBetaSelection==0,"late precise-beta program removal");
#endif
