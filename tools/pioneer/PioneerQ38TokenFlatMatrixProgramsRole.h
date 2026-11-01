/* SPDX-License-Identifier: MIT */
// Exact immutable DATA/private/diagnostic binding,rechecked every include.
#ifndef PIONEER_Q38_TOKEN_FLAT_MATRIX_SELECTION_H
#define PIONEER_Q38_TOKEN_FLAT_MATRIX_SELECTION_H
#ifdef PIONEER_Q38_TOKEN_FLAT_MATRIX_BENCHMARK_NATIVE
enum {PioneerQ38TokenFlatMatrixSelection=3};
#elif defined(PIONEER_Q38_TOKEN_FLAT_MATRIX_NATIVE_CONTEXT)
enum {PioneerQ38TokenFlatMatrixSelection=2};
#elif defined(PIONEER_Q38_TOKEN_FLAT_MATRIX_PROGRAMS)
enum {PioneerQ38TokenFlatMatrixSelection=1};
#else
enum {PioneerQ38TokenFlatMatrixSelection=0};
#endif
#endif
#ifdef PIONEER_Q38_TOKEN_FLAT_MATRIX_PROGRAMS
#if PIONEER_Q38_TOKEN_FLAT_MATRIX_PROGRAMS != 1 \
 || !defined(PIONEER_Q38_TOKEN_EMPTY_RECEIPT_POLL_PROGRAMS) || PIONEER_Q38_TOKEN_EMPTY_RECEIPT_POLL_PROGRAMS != 1 \
 || defined(PIONEER_Q38_TOKEN_EMPTY_RECEIPT_POLL_PROGRAMS_H)
#error Exact separate flat QGW3 DATA before old code binding required
#endif
#ifdef PIONEER_Q38_TOKEN_FLAT_MATRIX_BENCHMARK_NATIVE
#include "../../src/add-ons/kernel/drivers/misc/PioneerQ38TokenFlatMatrixBenchmarkRole.h"
static_assert(PioneerQ38TokenFlatMatrixSelection==3,"late flat grammar diagnostic selection");
#elif defined(PIONEER_Q38_TOKEN_FLAT_MATRIX_NATIVE_CONTEXT)
#include "../../src/add-ons/kernel/drivers/misc/PioneerQ38TokenFlatMatrixNativeRole.h"
static_assert(PioneerQ38TokenFlatMatrixSelection==2,"late flat grammar private selection");
#else
#if defined(_KERNEL_MODE) || defined(PIONEER_Q38_TOKEN_NATIVE_CONTEXT) \
 || defined(PIONEER_Q38_TOKEN_EMPTY_RECEIPT_POLL_NATIVE_CONTEXT) || defined(PIONEER_Q38_TOKEN_EMPTY_RECEIPT_POLL_BENCHMARK_NATIVE) \
 || defined(PIONEER_Q38_TOKEN_CONTEXT_COMPILE_ONLY) || defined(PIONEER_Q38_FLAT_MATRIX_GRAMMAR_IMAGE) \
 || defined(PIONEER_Q38_TOKEN_FRONTIER_BENCHMARK_CLIENT) || defined(PIONEER_Q38_TOKEN_FRONTEND_PROFILE_CLIENT) \
 || defined(PIONEER_Q38_TOKEN_DECODE_BENCHMARK_CLIENT) || defined(PIONEER_Q38_TOKEN_HIDDEN_VALIDATION_CLIENT) \
 || defined(PIONEER_Q38_CPU_VALIDATION) || defined(PIONEER_Q38_FULL_MODEL_CPU_REFERENCE) \
 || defined(PIONEER_Q38_WORK_INFERENCE) || defined(TT_PIONEER_Q38_WORK_MODE) \
 || defined(PIONEER_Q38_TOKEN_SELECTED_PROGRAMS_H)
#error Flat grammar DATA alone has NO native,physical,USER,ROOT BUILD,CPU or Work owner
#endif
static_assert(PioneerQ38TokenFlatMatrixSelection==1,"late flat grammar DATA selection");
#endif
#include "PioneerQ38TokenEmptyReceiptPollProgramsRole.h"
#else
#if defined(PIONEER_Q38_TOKEN_FLAT_MATRIX_NATIVE_CONTEXT) || defined(PIONEER_Q38_TOKEN_FLAT_MATRIX_BENCHMARK_NATIVE)
#error Flat grammar context requires its distinct immutable code package
#endif
static_assert(PioneerQ38TokenFlatMatrixSelection==0,"late flat grammar DATA removal");
#endif
