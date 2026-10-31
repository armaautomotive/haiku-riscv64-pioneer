/* SPDX-License-Identifier: MIT */
// New immutable DATA only. A native reader must be separately implemented
// and qualified; no existing physical driver may silently adopt this ROM.
#ifndef PIONEER_Q38_TOKEN_FRONTIER_PROGRAM_SELECTION_H
#define PIONEER_Q38_TOKEN_FRONTIER_PROGRAM_SELECTION_H
#ifdef PIONEER_Q38_TOKEN_FRONTIER_PROGRAMS
enum {PioneerQ38TokenFrontierProgramSelection=1};
#else
enum {PioneerQ38TokenFrontierProgramSelection=0};
#endif
#endif
#ifdef PIONEER_Q38_TOKEN_FRONTIER_PROGRAMS
#if PIONEER_Q38_TOKEN_FRONTIER_PROGRAMS != 1 \
 || !defined(PIONEER_Q38_TOKEN_JOB_BOOT_FOLD_PROGRAMS) || PIONEER_Q38_TOKEN_JOB_BOOT_FOLD_PROGRAMS != 1 \
 || defined(PIONEER_Q38_TOKEN_JOB_BOOT_FOLD_PROGRAMS_H) \
 || defined(PIONEER_Q38_TOKEN_PRECISE_BETA_PROGRAMS_H) || defined(PIONEER_Q38_TOKEN_FAULT_PROGRAMS_H) \
 || defined(PIONEER_Q38_TOKEN_PUSHED_PROGRAMS_H) || defined(PIONEER_Q38_TOKEN_PROGRAMS_H)
#error Token frontier DATA requires its distinct package before every old package
#endif
#if defined(_KERNEL_MODE) || defined(PIONEER_Q38_TOKEN_NATIVE_CONTEXT) \
 || defined(PIONEER_Q38_TOKEN_PUSHED_NATIVE_CONTEXT) || defined(PIONEER_Q38_TOKEN_FRONTIER_NATIVE_CONTEXT) \
 || defined(PIONEER_Q38_TOKEN_JOB_BOOT_FOLD_NATIVE_CONTEXT) \
 || defined(PIONEER_Q38_TOKEN_JOB_BOOT_FOLD_VALIDATION_NATIVE) \
 || defined(PIONEER_Q38_TOKEN_PRECISE_BETA_VALIDATION_NATIVE) || defined(PIONEER_Q38_TOKEN_FAULT_VALIDATION_NATIVE) \
 || defined(PIONEER_Q38_TOKEN_DECODE_BENCHMARK_NATIVE) || defined(PIONEER_Q38_TOKEN_DECODE_BENCHMARK_CLIENT) \
 || defined(PIONEER_Q38_TOKEN_FRONTIER_IMAGE)
#error Frontier program DATA has NO qualified native,physical,USER or image owner
#endif
#include "PioneerQ38TokenJobBootFoldProgramsRole.h"
static_assert(PioneerQ38TokenFrontierProgramSelection==1,"late frontier DATA selection");
#else
static_assert(PioneerQ38TokenFrontierProgramSelection==0,"late frontier DATA removal");
#endif
