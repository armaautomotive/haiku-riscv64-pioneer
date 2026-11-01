/* SPDX-License-Identifier: MIT */
// Re-evaluate outside the binding guard;separate ROOT BUILD,not native DATA.
#include "PioneerQ38JobBootFoldRole.h"
#ifdef PIONEER_Q38_FLAT_MATRIX_GRAMMAR_IMAGE
#if PIONEER_Q38_FLAT_MATRIX_GRAMMAR_IMAGE != 1 \
 || !defined(PIONEER_Q38_JOB_BOOT_FOLD_IMAGE) || PIONEER_Q38_JOB_BOOT_FOLD_IMAGE != 1 \
 || !defined(PIONEER_Q38_GLOBAL_CHAIN_PUSHED_COMPLETION) || PIONEER_Q38_GLOBAL_CHAIN_PUSHED_COMPLETION != 1 \
 || !defined(PIONEER_Q38_GLOBAL_CHAIN_PRECISE_ACTIVATION) || PIONEER_Q38_GLOBAL_CHAIN_PRECISE_ACTIVATION != 1 \
 || !defined(__riscv) || __riscv != 1 || !defined(__riscv_xlen) || __riscv_xlen != 32 \
 || (defined(PIONEER_Q38_GLOBAL_TOKEN_ROOT_IMAGE) == defined(PIONEER_Q38_GLOBAL_CHAIN_COMPONENT_IMAGE))
#error Flat QGW3 grammar requires the exact job-boot-fold returning or token ROOT BUILD
#endif
#if defined(_KERNEL_MODE) || defined(PIONEER_Q38_CPU_VALIDATION) || defined(PIONEER_Q38_FULL_MODEL_CPU_REFERENCE) \
 || defined(PIONEER_Q38_WORK_INFERENCE) || defined(TT_PIONEER_Q38_WORK_MODE) \
 || defined(PIONEER_Q38_GLOBAL_WORKER_IMAGE) || defined(PIONEER_Q38_TOKEN_NATIVE_CONTEXT) \
 || defined(PIONEER_Q38_TOKEN_FLAT_MATRIX_PROGRAMS) || defined(PIONEER_Q38_TOKEN_FLAT_MATRIX_NATIVE_CONTEXT) \
 || defined(PIONEER_Q38_TOKEN_FLAT_MATRIX_BENCHMARK_NATIVE) || defined(__FAST_MATH__)
#error Flat grammar BUILD cannot alias worker,native,CPU or Work authority
#endif
#endif
#ifndef PIONEER_Q38_FLAT_MATRIX_GRAMMAR_BINDING_H
#define PIONEER_Q38_FLAT_MATRIX_GRAMMAR_BINDING_H
#ifdef PIONEER_Q38_FLAT_MATRIX_GRAMMAR_IMAGE
enum {PioneerQ38FlatMatrixGrammarSelection=1};
#else
enum {PioneerQ38FlatMatrixGrammarSelection=0};
#endif
#endif
#ifdef PIONEER_Q38_FLAT_MATRIX_GRAMMAR_IMAGE
static_assert(PioneerQ38FlatMatrixGrammarSelection==1,"late flat grammar selection");
#else
static_assert(PioneerQ38FlatMatrixGrammarSelection==0,"late flat grammar removal");
#endif
