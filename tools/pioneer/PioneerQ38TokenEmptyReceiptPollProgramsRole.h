/* SPDX-License-Identifier: MIT */
// Exact immutable DATA/private/diagnostic binding,rechecked every include.
#ifndef PIONEER_Q38_TOKEN_EMPTY_RECEIPT_POLL_SELECTION_H
#define PIONEER_Q38_TOKEN_EMPTY_RECEIPT_POLL_SELECTION_H
#ifdef PIONEER_Q38_TOKEN_EMPTY_RECEIPT_POLL_BENCHMARK_NATIVE
enum {PioneerQ38TokenEmptyReceiptPollSelection=3};
#elif defined(PIONEER_Q38_TOKEN_EMPTY_RECEIPT_POLL_NATIVE_CONTEXT)
enum {PioneerQ38TokenEmptyReceiptPollSelection=2};
#elif defined(PIONEER_Q38_TOKEN_EMPTY_RECEIPT_POLL_PROGRAMS)
enum {PioneerQ38TokenEmptyReceiptPollSelection=1};
#else
enum {PioneerQ38TokenEmptyReceiptPollSelection=0};
#endif
#endif
#ifdef PIONEER_Q38_TOKEN_EMPTY_RECEIPT_POLL_PROGRAMS
#if PIONEER_Q38_TOKEN_EMPTY_RECEIPT_POLL_PROGRAMS != 1 \
 || !defined(PIONEER_Q38_TOKEN_COUNTER_GATHER_PROGRAMS) || PIONEER_Q38_TOKEN_COUNTER_GATHER_PROGRAMS != 1 \
 || defined(PIONEER_Q38_TOKEN_COUNTER_GATHER_PROGRAMS_H)
#error Exact separate empty-receipt-poll DATA before old code binding required
#endif
#ifdef PIONEER_Q38_TOKEN_EMPTY_RECEIPT_POLL_BENCHMARK_NATIVE
#include "../../src/add-ons/kernel/drivers/misc/PioneerQ38TokenEmptyReceiptPollBenchmarkRole.h"
static_assert(PioneerQ38TokenEmptyReceiptPollSelection==3,"late empty-receipt-poll diagnostic selection");
#elif defined(PIONEER_Q38_TOKEN_EMPTY_RECEIPT_POLL_NATIVE_CONTEXT)
#include "../../src/add-ons/kernel/drivers/misc/PioneerQ38TokenEmptyReceiptPollNativeRole.h"
static_assert(PioneerQ38TokenEmptyReceiptPollSelection==2,"late empty-receipt-poll private selection");
#else
#if defined(_KERNEL_MODE) || defined(PIONEER_Q38_TOKEN_NATIVE_CONTEXT) \
 || defined(PIONEER_Q38_TOKEN_COUNTER_GATHER_NATIVE_CONTEXT) || defined(PIONEER_Q38_TOKEN_COUNTER_GATHER_BENCHMARK_NATIVE) \
 || defined(PIONEER_Q38_TOKEN_PACKET_IDENTITY_BOUNDARY_NATIVE_CONTEXT) || defined(PIONEER_Q38_TOKEN_PACKET_IDENTITY_BOUNDARY_BENCHMARK_NATIVE) \
 || defined(PIONEER_Q38_TOKEN_COMMIT_BOUNDARY_NATIVE_CONTEXT) || defined(PIONEER_Q38_TOKEN_COMMIT_BOUNDARY_BENCHMARK_NATIVE) \
 || defined(PIONEER_Q38_TOKEN_RETAINED_COHORT_NATIVE_CONTEXT) || defined(PIONEER_Q38_TOKEN_RETAINED_COHORT_BENCHMARK_NATIVE) \
 || defined(PIONEER_Q38_TOKEN_FRONTIER_NATIVE_CONTEXT) || defined(PIONEER_Q38_TOKEN_FRONTIER_BENCHMARK_NATIVE) \
 || defined(PIONEER_Q38_TOKEN_CONTEXT_COMPILE_ONLY) || defined(PIONEER_Q38_EMPTY_RECEIPT_POLL_IMAGE) \
 || defined(PIONEER_Q38_READ_SOURCE_BOUNDARY_IMAGE) \
 || defined(PIONEER_Q38_COUNTER_GATHER_IMAGE) || defined(PIONEER_Q38_PACKET_IDENTITY_BOUNDARY_IMAGE) \
 || defined(PIONEER_Q38_TOKEN_FRONTIER_BENCHMARK_CLIENT) || defined(PIONEER_Q38_TOKEN_FRONTEND_PROFILE_CLIENT) \
 || defined(PIONEER_Q38_TOKEN_DECODE_BENCHMARK_CLIENT) || defined(PIONEER_Q38_TOKEN_HIDDEN_VALIDATION_CLIENT) \
 || defined(PIONEER_Q38_CPU_VALIDATION) || defined(PIONEER_Q38_FULL_MODEL_CPU_REFERENCE) \
 || defined(PIONEER_Q38_WORK_INFERENCE) || defined(TT_PIONEER_Q38_WORK_MODE) \
 || defined(PIONEER_Q38_TOKEN_SELECTED_PROGRAMS_H)
#error Empty-receipt-poll DATA alone has NO native,physical,USER,ROOT BUILD,CPU or Work owner
#endif
static_assert(PioneerQ38TokenEmptyReceiptPollSelection==1,"late empty-receipt-poll DATA selection");
#endif
#include "PioneerQ38TokenCounterGatherProgramsRole.h"
#else
#if defined(PIONEER_Q38_TOKEN_EMPTY_RECEIPT_POLL_NATIVE_CONTEXT) || defined(PIONEER_Q38_TOKEN_EMPTY_RECEIPT_POLL_BENCHMARK_NATIVE)
#error Empty-receipt-poll context requires its distinct immutable code package
#endif
static_assert(PioneerQ38TokenEmptyReceiptPollSelection==0,"late empty-receipt-poll DATA removal");
#endif
