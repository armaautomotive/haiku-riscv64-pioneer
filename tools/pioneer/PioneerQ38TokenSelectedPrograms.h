/* SPDX-License-Identifier: MIT */
// DATA selection only; no native source/lifetime, journal or launch grant.
// Rechecked outside the guard so late/defined-zero changes cannot alias data.
#include "PioneerQ38TokenWideGdnProgramsRole.h"
#if defined(PIONEER_Q38_TOKEN_WIDE_GDN_PROGRAMS) \
 && !defined(PIONEER_Q38_TOKEN_WIDE_GDN_NATIVE_CONTEXT) && !defined(PIONEER_Q38_TOKEN_WIDE_GDN_BENCHMARK_NATIVE)
#error Wide GDN DATA requires its distinct native subject;no silent old-package fallback
#endif
#include "PioneerQ38TokenBatchedOutputClosureProgramsRole.h"
#if defined(PIONEER_Q38_TOKEN_BATCHED_OUTPUT_CLOSURE_PROGRAMS) \
 && !defined(PIONEER_Q38_TOKEN_BATCHED_OUTPUT_CLOSURE_NATIVE_CONTEXT) && !defined(PIONEER_Q38_TOKEN_BATCHED_OUTPUT_CLOSURE_BENCHMARK_NATIVE)
#error Output closure DATA requires its distinct native subject;no silent old-package fallback
#endif
#include "PioneerQ38TokenTwinBatchPrefetchProgramsRole.h"
#if defined(PIONEER_Q38_TOKEN_TWIN_BATCH_PREFETCH_PROGRAMS) \
 && !defined(PIONEER_Q38_TOKEN_TWIN_BATCH_PREFETCH_NATIVE_CONTEXT) && !defined(PIONEER_Q38_TOKEN_TWIN_BATCH_PREFETCH_BENCHMARK_NATIVE)
#error Twin-batch DATA requires its distinct private or diagnostic build;never silently fall back
#endif
#include "PioneerQ38TokenBatchedPrefetchProgramsRole.h"
#if defined(PIONEER_Q38_TOKEN_BATCHED_PREFETCH_PROGRAMS) \
 && !defined(PIONEER_Q38_TOKEN_BATCHED_PREFETCH_NATIVE_CONTEXT) && !defined(PIONEER_Q38_TOKEN_BATCHED_PREFETCH_BENCHMARK_NATIVE)
#error Batched-prefetch DATA requires its distinct private or diagnostic selector;no Work grant
#endif
#include "PioneerQ38TokenPrefetchProgramsRole.h"
#if defined(PIONEER_Q38_TOKEN_PREFETCH_PROGRAMS) \
 && !defined(PIONEER_Q38_TOKEN_PREFETCH_NATIVE_CONTEXT) && !defined(PIONEER_Q38_TOKEN_PREFETCH_BENCHMARK_NATIVE)
#error Prefetch DATA requires its distinct private or diagnostic selector;no Work grant
#endif
#include "PioneerQ38TokenFlatMatrixProgramsRole.h"
#if defined(PIONEER_Q38_TOKEN_FLAT_MATRIX_PROGRAMS) \
 && !defined(PIONEER_Q38_TOKEN_FLAT_MATRIX_NATIVE_CONTEXT) && !defined(PIONEER_Q38_TOKEN_FLAT_MATRIX_BENCHMARK_NATIVE)
#error Flat grammar DATA requires its distinct private or diagnostic selector;no Work grant
#endif
#include "PioneerQ38TokenEmptyReceiptPollProgramsRole.h"
#if defined(PIONEER_Q38_TOKEN_EMPTY_RECEIPT_POLL_PROGRAMS) \
 && !defined(PIONEER_Q38_TOKEN_EMPTY_RECEIPT_POLL_NATIVE_CONTEXT) && !defined(PIONEER_Q38_TOKEN_EMPTY_RECEIPT_POLL_BENCHMARK_NATIVE)
#error Empty-receipt DATA requires its distinct private or diagnostic selector;no Work grant
#endif
#include "PioneerQ38TokenCounterGatherProgramsRole.h"
#if defined(PIONEER_Q38_TOKEN_COUNTER_GATHER_PROGRAMS) \
 && !defined(PIONEER_Q38_TOKEN_COUNTER_GATHER_NATIVE_CONTEXT) && !defined(PIONEER_Q38_TOKEN_COUNTER_GATHER_BENCHMARK_NATIVE)
#error Counter gather DATA requires its distinct private or diagnostic selector;no Work grant
#endif
#include "PioneerQ38TokenTidBoundaryProgramsRole.h"
#if defined(PIONEER_Q38_TOKEN_TID_BOUNDARY_PROGRAMS) \
 && !defined(PIONEER_Q38_TOKEN_TID_BOUNDARY_NATIVE_CONTEXT) && !defined(PIONEER_Q38_TOKEN_TID_BOUNDARY_BENCHMARK_NATIVE)
#error TID boundary DATA requires its distinct private or diagnostic selector;no Work grant
#endif
#include "PioneerQ38TokenPacketIdentityBoundaryProgramsRole.h"
#if defined(PIONEER_Q38_TOKEN_PACKET_IDENTITY_BOUNDARY_PROGRAMS) \
 && !defined(PIONEER_Q38_TOKEN_PACKET_IDENTITY_BOUNDARY_NATIVE_CONTEXT) && !defined(PIONEER_Q38_TOKEN_PACKET_IDENTITY_BOUNDARY_BENCHMARK_NATIVE)
#error Packet-identity DATA requires its distinct private or diagnostic selector;no Work grant
#endif
#include "PioneerQ38TokenCohortLocalSourcesProgramsRole.h"
#if defined(PIONEER_Q38_TOKEN_COHORT_LOCAL_SOURCES_PROGRAMS) \
 && !defined(PIONEER_Q38_TOKEN_COHORT_LOCAL_SOURCES_NATIVE_CONTEXT) && !defined(PIONEER_Q38_TOKEN_COHORT_LOCAL_SOURCES_BENCHMARK_NATIVE)
#error Cohort source DATA requires its distinct private or diagnostic selector;no Work grant
#endif
#include "PioneerQ38TokenCommitBoundaryProgramsRole.h"
#if defined(PIONEER_Q38_TOKEN_COMMIT_BOUNDARY_PROGRAMS) \
 && !defined(PIONEER_Q38_TOKEN_COMMIT_BOUNDARY_NATIVE_CONTEXT) && !defined(PIONEER_Q38_TOKEN_COMMIT_BOUNDARY_BENCHMARK_NATIVE)
#error Commit-boundary DATA requires its distinct private or diagnostic selector;no Work grant
#endif
#include "PioneerQ38TokenRetainedCohortProgramsRole.h"
#if defined(PIONEER_Q38_TOKEN_RETAINED_COHORT_PROGRAMS) \
 && !defined(PIONEER_Q38_TOKEN_RETAINED_COHORT_NATIVE_CONTEXT) && !defined(PIONEER_Q38_TOKEN_RETAINED_COHORT_BENCHMARK_NATIVE)
#error Retained-cohort DATA requires its distinct private or diagnostic selector;no Work grant
#endif
#include "PioneerQ38TokenFrontierProgramsRole.h"
#include "PioneerQ38TokenJobBootFoldProgramsRole.h"
#include "PioneerQ38TokenPreciseBetaProgramsRole.h"
#include "PioneerQ38TokenFaultProgramsRole.h"
#ifdef PIONEER_Q38_TOKEN_PUSHED_PROGRAMS
#if PIONEER_Q38_TOKEN_PUSHED_PROGRAMS != 1 || defined(PIONEER_Q38_TOKEN_PRECISE_PROGRAMS) \
 || defined(PIONEER_Q38_TOKEN_PROGRAMS_H)
#error Exact separate QTP3 DATA cannot alias an already selected QTP1/QTP2 package
#endif
#ifdef PIONEER_Q38_TOKEN_NATIVE_CONTEXT
#include "../../src/add-ons/kernel/drivers/misc/PioneerQ38TokenNativeRole.h"
#elif defined(PIONEER_Q38_TOKEN_PUSHED_NATIVE_CONTEXT)
#error QTP3 native selector requires its actual native parent
#endif
#ifdef PIONEER_Q38_TOKEN_WIDE_GDN_PROGRAMS
#include "PioneerQ38TokenWideGdnPrograms.h"
#elif defined(PIONEER_Q38_TOKEN_BATCHED_OUTPUT_CLOSURE_PROGRAMS)
#include "PioneerQ38TokenBatchedOutputClosurePrograms.h"
#elif defined(PIONEER_Q38_TOKEN_TWIN_BATCH_PREFETCH_PROGRAMS)
#include "PioneerQ38TokenTwinBatchPrefetchPrograms.h"
#elif defined(PIONEER_Q38_TOKEN_BATCHED_PREFETCH_PROGRAMS)
#include "PioneerQ38TokenBatchedPrefetchPrograms.h"
#elif defined(PIONEER_Q38_TOKEN_PREFETCH_PROGRAMS)
#include "PioneerQ38TokenPrefetchPrograms.h"
#elif defined(PIONEER_Q38_TOKEN_FLAT_MATRIX_PROGRAMS)
#include "PioneerQ38TokenFlatMatrixPrograms.h"
#elif defined(PIONEER_Q38_TOKEN_EMPTY_RECEIPT_POLL_PROGRAMS)
#include "PioneerQ38TokenEmptyReceiptPollPrograms.h"
#elif defined(PIONEER_Q38_TOKEN_COUNTER_GATHER_PROGRAMS)
#include "PioneerQ38TokenCounterGatherPrograms.h"
#elif defined(PIONEER_Q38_TOKEN_TID_BOUNDARY_PROGRAMS)
#include "PioneerQ38TokenTidBoundaryPrograms.h"
#elif defined(PIONEER_Q38_TOKEN_PACKET_IDENTITY_BOUNDARY_PROGRAMS)
#include "PioneerQ38TokenPacketIdentityBoundaryPrograms.h"
#elif defined(PIONEER_Q38_TOKEN_COHORT_LOCAL_SOURCES_PROGRAMS)
#include "PioneerQ38TokenCohortLocalSourcesPrograms.h"
#elif defined(PIONEER_Q38_TOKEN_COMMIT_BOUNDARY_PROGRAMS)
#include "PioneerQ38TokenCommitBoundaryPrograms.h"
#elif defined(PIONEER_Q38_TOKEN_RETAINED_COHORT_PROGRAMS)
#include "PioneerQ38TokenRetainedCohortPrograms.h"
#elif defined(PIONEER_Q38_TOKEN_FRONTIER_PROGRAMS)
#include "PioneerQ38TokenFrontierPrograms.h"
#elif defined(PIONEER_Q38_TOKEN_JOB_BOOT_FOLD_PROGRAMS)
#include "PioneerQ38TokenJobBootFoldPrograms.h"
#elif defined(PIONEER_Q38_TOKEN_PRECISE_BETA_PROGRAMS)
#include "PioneerQ38TokenPreciseBetaPrograms.h"
#elif defined(PIONEER_Q38_TOKEN_FAULT_PROGRAMS)
#include "PioneerQ38TokenFaultPrograms.h"
#else
#include "PioneerQ38TokenPushedPrograms.h"
#endif
#include "PioneerQ38TokenFootprint.h"
#ifndef PIONEER_Q38_TOKEN_SELECTED_PROGRAMS_H
#define PIONEER_Q38_TOKEN_SELECTED_PROGRAMS_H
#ifdef PIONEER_Q38_TOKEN_WIDE_GDN_PROGRAMS
namespace PioneerQ38TokenPrograms=PioneerQ38TokenWideGdnPrograms;
#elif defined(PIONEER_Q38_TOKEN_BATCHED_OUTPUT_CLOSURE_PROGRAMS)
namespace PioneerQ38TokenPrograms=PioneerQ38TokenBatchedOutputClosurePrograms;
#elif defined(PIONEER_Q38_TOKEN_TWIN_BATCH_PREFETCH_PROGRAMS)
namespace PioneerQ38TokenPrograms=PioneerQ38TokenTwinBatchPrefetchPrograms;
#elif defined(PIONEER_Q38_TOKEN_BATCHED_PREFETCH_PROGRAMS)
namespace PioneerQ38TokenPrograms=PioneerQ38TokenBatchedPrefetchPrograms;
#elif defined(PIONEER_Q38_TOKEN_PREFETCH_PROGRAMS)
namespace PioneerQ38TokenPrograms=PioneerQ38TokenPrefetchPrograms;
#elif defined(PIONEER_Q38_TOKEN_FLAT_MATRIX_PROGRAMS)
namespace PioneerQ38TokenPrograms=PioneerQ38TokenFlatMatrixPrograms;
#elif defined(PIONEER_Q38_TOKEN_EMPTY_RECEIPT_POLL_PROGRAMS)
namespace PioneerQ38TokenPrograms=PioneerQ38TokenEmptyReceiptPollPrograms;
#elif defined(PIONEER_Q38_TOKEN_COUNTER_GATHER_PROGRAMS)
namespace PioneerQ38TokenPrograms=PioneerQ38TokenCounterGatherPrograms;
#elif defined(PIONEER_Q38_TOKEN_TID_BOUNDARY_PROGRAMS)
namespace PioneerQ38TokenPrograms=PioneerQ38TokenTidBoundaryPrograms;
#elif defined(PIONEER_Q38_TOKEN_PACKET_IDENTITY_BOUNDARY_PROGRAMS)
namespace PioneerQ38TokenPrograms=PioneerQ38TokenPacketIdentityBoundaryPrograms;
#elif defined(PIONEER_Q38_TOKEN_COHORT_LOCAL_SOURCES_PROGRAMS)
namespace PioneerQ38TokenPrograms=PioneerQ38TokenCohortLocalSourcesPrograms;
#elif defined(PIONEER_Q38_TOKEN_COMMIT_BOUNDARY_PROGRAMS)
namespace PioneerQ38TokenPrograms=PioneerQ38TokenCommitBoundaryPrograms;
#elif defined(PIONEER_Q38_TOKEN_RETAINED_COHORT_PROGRAMS)
namespace PioneerQ38TokenPrograms=PioneerQ38TokenRetainedCohortPrograms;
#elif defined(PIONEER_Q38_TOKEN_FRONTIER_PROGRAMS)
namespace PioneerQ38TokenPrograms=PioneerQ38TokenFrontierPrograms;
#elif defined(PIONEER_Q38_TOKEN_JOB_BOOT_FOLD_PROGRAMS)
namespace PioneerQ38TokenPrograms=PioneerQ38TokenJobBootFoldPrograms;
#elif defined(PIONEER_Q38_TOKEN_PRECISE_BETA_PROGRAMS)
namespace PioneerQ38TokenPrograms=PioneerQ38TokenPreciseBetaPrograms;
#elif defined(PIONEER_Q38_TOKEN_FAULT_PROGRAMS)
namespace PioneerQ38TokenPrograms=PioneerQ38TokenFaultPrograms;
#else
namespace PioneerQ38TokenPrograms=PioneerQ38TokenPushedPrograms;
#endif
#endif
static_assert(PioneerQ38TokenPrograms::Family==0x33505451,"late QTP3 package selection");
#else
#ifdef PIONEER_Q38_TOKEN_PUSHED_NATIVE_CONTEXT
#error QTP3 native context cannot select old immutable programs
#endif
#include "PioneerQ38TokenPrograms.h"
#endif
