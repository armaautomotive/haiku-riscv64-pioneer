/* SPDX-License-Identifier: MIT */
// Rechecked selection. A distinct code revision,not a physical/Work grant.
#ifndef PIONEER_Q38_TOKEN_FAULT_PROGRAM_SELECTION_H
#define PIONEER_Q38_TOKEN_FAULT_PROGRAM_SELECTION_H
#ifdef PIONEER_Q38_TOKEN_FAULT_PROGRAMS
enum {PioneerQ38TokenFaultProgramSelection=1};
#else
enum {PioneerQ38TokenFaultProgramSelection=0};
#endif
#endif
#ifdef PIONEER_Q38_TOKEN_FAULT_PROGRAMS
#if PIONEER_Q38_TOKEN_FAULT_PROGRAMS != 1 \
 || !defined(PIONEER_Q38_TOKEN_PUSHED_PROGRAMS) || PIONEER_Q38_TOKEN_PUSHED_PROGRAMS != 1 \
 || defined(PIONEER_Q38_TOKEN_PRECISE_PROGRAMS) || defined(PIONEER_Q38_TOKEN_PUSHED_PROGRAMS_H) \
 || defined(PIONEER_Q38_TOKEN_PROGRAMS_H)
#error Fault-reporting code requires its separate exact G3 program selection
#endif
static_assert(PioneerQ38TokenFaultProgramSelection==1,"late fault program selection");
#ifdef PIONEER_Q38_TOKEN_NATIVE_CONTEXT
#if !defined(PIONEER_Q38_TOKEN_CONTEXT_COMPILE_ONLY) || PIONEER_Q38_TOKEN_CONTEXT_COMPILE_ONLY != 1 \
 || !defined(PIONEER_Q38_TOKEN_PUSHED_NATIVE_CONTEXT) || PIONEER_Q38_TOKEN_PUSHED_NATIVE_CONTEXT != 1 \
 || !defined(PIONEER_Q38_TOKEN_HELD_HIDDEN_DIAGNOSTIC) || PIONEER_Q38_TOKEN_HELD_HIDDEN_DIAGNOSTIC != 1 \
 || !defined(PIONEER_DRIVER_BUILD_TAG) || PIONEER_DRIVER_BUILD_TAG != 0x5154334e4f445256ULL \
 || defined(PIONEER_Q38_TOKEN_VALIDATION_NATIVE) || defined(PIONEER_Q38_TOKEN_HIDDEN_VALIDATION_NATIVE)
#error Fault-reporting native integration is private QT3NODRV COMPILE-ONLY
#endif
#endif
#if defined(PIONEER_Q38_TOKEN_VALIDATION_NATIVE) || defined(PIONEER_Q38_TOKEN_HIDDEN_VALIDATION_NATIVE) \
 || defined(PIONEER_Q38_TOKEN_VALIDATION_CLIENT) || defined(PIONEER_Q38_TOKEN_HIDDEN_VALIDATION_CLIENT) \
 || defined(PIONEER_Q38_TOKEN_BOUNDARY_VALIDATION_CLIENT) || defined(PIONEER_Q38_CPU_VALIDATION) \
 || defined(PIONEER_Q38_WORK_INFERENCE) || defined(TT_PIONEER_Q38_WORK_MODE) \
 || defined(PIONEER_Q38_FULL_MODEL_CPU_REFERENCE)
#error Fault-reporting program selection cannot alias CPU-reference,USER or Work roles
#endif
#else
static_assert(PioneerQ38TokenFaultProgramSelection==0,"late fault program removal");
#endif
