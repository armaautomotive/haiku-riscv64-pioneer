/* SPDX-License-Identifier: MIT */
#include "PioneerDramCohortNativeRole.h"
#ifndef PIONEER_DRAM_READ_SWEEP_NATIVE_ROLE_H
#define PIONEER_DRAM_READ_SWEEP_NATIVE_ROLE_H
#if !defined(PIONEER_DRAM_READ_SWEEP_NATIVE_CONTEXT) || PIONEER_DRAM_READ_SWEEP_NATIVE_CONTEXT != 1 \
 || !defined(TT_PIONEER_DRIVER_RELOAD) || !defined(TT_PIONEER_EXPERIMENTAL_Q38_DISPATCH)
#error Separate DRAM native context and reload perimeter required
#endif
#ifdef PIONEER_DRAM_SWEEP_DIAGNOSTIC
#if PIONEER_DRAM_SWEEP_DIAGNOSTIC != 1 || defined(PIONEER_DRAM_SWEEP_CONTEXT_COMPILE_ONLY) \
 || !defined(PIONEER_DRAM_SWEEP_QUEUED) || PIONEER_DRAM_SWEEP_QUEUED != 1 \
 || !defined(PIONEER_DRAM_SWEEP_WIDE) || PIONEER_DRAM_SWEEP_WIDE != 1 \
 || PIONEER_DRIVER_BUILD_TAG != 0x2026110200050076ULL
#error DRAM diagnostic76 requires its separate physical role;never expose DRSNODRV
#endif
#else
#ifdef PIONEER_DRAM_COHORT_NATIVE_CONTEXT
// Its separately rechecked role above selects either inert DRCNODRV or
// bounded physical77. Neither instantiates the old single-worker algorithm.
#else
#if !defined(PIONEER_DRAM_SWEEP_CONTEXT_COMPILE_ONLY) || PIONEER_DRAM_SWEEP_CONTEXT_COMPILE_ONLY != 1 \
 || PIONEER_DRIVER_BUILD_TAG != 0x4452534e4f445256ULL
#error DRSNODRV requires its renamed inert compile-only perimeter
#endif
#endif
#endif
#if defined(PIONEER_DRAM_SWEEP_QUEUED) && PIONEER_DRAM_SWEEP_QUEUED != 1
#error Queued diagnostic must be selected explicitly,not inherited as zero
#endif
#if defined(PIONEER_DRAM_SWEEP_WIDE) && (PIONEER_DRAM_SWEEP_WIDE != 1 \
 || !defined(PIONEER_DRAM_SWEEP_QUEUED) || PIONEER_DRAM_SWEEP_QUEUED != 1)
#error Wide diagnostic requires its explicit queued perimeter
#endif
#if defined(PIONEER_Q38_TOKEN_NATIVE_CONTEXT) || defined(PIONEER_Q38_RECURRENT_PREFIX_NATIVE_CONTEXT) \
 || defined(PIONEER_Q38_RECURRENT_BLOCK_NATIVE_CONTEXT) || defined(PIONEER_Q38_RESIDENT_OUTPUT_OWNER) \
 || defined(PIONEER_Q38_RESIDENT_LAYER_NATIVE_CONTEXT) || defined(TT_PIONEER_Q38_WORK_MODE) \
 || defined(PIONEER_Q38_CPU_VALIDATION) || defined(PIONEER_Q38_WORK_INFERENCE) \
 || defined(PIONEER_Q38_WORK_MODE) || defined(PIONEER_Q38_NORMAL_CLOCK) \
 || defined(PIONEER_Q38_GDN_MUTABLE_STATE) || defined(PIONEER_Q38_DECODER_RUNTIME)
#error DRAM diagnostic cannot inherit model/CPU-reference/Work native roles
#endif
#endif
