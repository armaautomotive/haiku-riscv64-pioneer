/* SPDX-License-Identifier: MIT */
#include "PioneerQ38HybridKernelCompileOnly.h"
#if defined(PIONEER_Q38_HYBRID_PROGRAMS_DIAGNOSTIC)
#if PIONEER_Q38_HYBRID_PROGRAMS_DIAGNOSTIC != 1 || !defined(PIONEER_Q38_HYBRID_PROGRAMS_HEADER) \
 || !defined(PIONEER_Q38_HYBRID_JOURNAL_DIAGNOSTIC) || PIONEER_Q38_HYBRID_JOURNAL_DIAGNOSTIC != 1 \
 || !defined(PIONEER_Q38_TOKEN_BF16_PROGRAMS) || PIONEER_Q38_TOKEN_BF16_PROGRAMS != 1 \
 || defined(PIONEER_Q38_TOKEN_BF16_PROGRAMS_H) || defined(PIONEER_Q38_TOKEN_BF16_PROFILE_PROGRAMS) \
 || defined(PIONEER_Q38_TOKEN_BF16_PAIR_ISSUE_PROGRAMS) || defined(PIONEER_Q38_TOKEN_BF16_DEFERRED_PROGRAMS) \
 || defined(PIONEER_Q38_TOKEN_PUSHED_PROGRAMS) || defined(PIONEER_Q38_TOKEN_PRECISE_PROGRAMS) \
 || (defined(_KERNEL_MODE) && !defined(PIONEER_Q38_HYBRID_KERNEL_COMPILE_ONLY) && !defined(PIONEER_Q38_HYBRID_NATIVE_COMPILE_ONLY) && !defined(PIONEER_Q38_HYBRID_FULL_NATIVE)) \
 || ((defined(PIONEER_Q38_TOKEN_NATIVE_CONTEXT) || defined(PIONEER_DRIVER_BUILD_TAG)) && !defined(PIONEER_Q38_HYBRID_NATIVE_COMPILE_ONLY) && !defined(PIONEER_Q38_HYBRID_FULL_NATIVE)) \
 || defined(PIONEER_Q38_WORK_INFERENCE) || defined(TT_PIONEER_Q38_WORK_MODE) \
 || (defined(PIONEER_Q38_FULL_OFFLOAD_NATIVE) && !defined(PIONEER_Q38_HYBRID_FULL_NATIVE)) || defined(PIONEER_Q38_FULL_OFFLOAD_CLIENT) \
 || defined(PIONEER_Q38_BF16_FULL_NATIVE) || defined(PIONEER_Q38_BF16_FULL_CLIENT) \
 || defined(PIONEER_Q38_CPU_VALIDATION) || defined(PIONEER_Q38_FULL_MODEL_CPU_REFERENCE)
#error Hybrid QGW5 program selection is an offline journal fixture only
#endif
#include PIONEER_Q38_HYBRID_PROGRAMS_HEADER
#if defined(PIONEER_Q38_QGWD_TWO_NATIVE_142) && (!defined(PIONEER_Q38_QGWD_TWO_NATIVE_DATA_142) || PIONEER_Q38_QGWD_TWO_NATIVE_DATA_142 != 1)
#error QGWD tag142 requires its distinct two-token candidate DATA
#endif
#if !defined(PIONEER_Q38_QGWD_TWO_NATIVE_142) && defined(PIONEER_Q38_QGWD_TWO_NATIVE_DATA_142)
#error QGWD tag142 DATA cannot be used by another role
#endif
#include "PioneerQ38TokenFootprint.h"
#ifndef PIONEER_Q38_TOKEN_SELECTED_PROGRAMS_H
#define PIONEER_Q38_TOKEN_SELECTED_PROGRAMS_H
namespace PioneerQ38TokenPrograms=PioneerQ38HybridPrograms;
#endif
#if defined(PIONEER_Q38_QGWF_UNROLL_NATIVE_145)
static_assert(PioneerQ38TokenPrograms::Family==0x4b505451 && PioneerQ38TokenL1::Family==0x42505451,
 "Unrolled QGWF programs require their distinct QTPK identity and expanded dense-Q8 journal");
#elif defined(PIONEER_Q38_QGWF_FRONTIER_PROGRAMS_INSPECTION)
static_assert(PioneerQ38TokenPrograms::Family==0x4a505451 && PioneerQ38TokenL1::Family==0x42505451,
 "QGWF frontier programs require their distinct QTPJ identity and expanded dense-Q8 journal");
#elif defined(PIONEER_Q38_QGWF_PROGRAMS_INSPECTION)
static_assert(PioneerQ38TokenPrograms::Family==0x49505451 && PioneerQ38TokenL1::Family==0x42505451,
 "QGWF programs require their distinct QTPI identity and expanded dense-Q8 journal");
#elif defined(PIONEER_Q38_QGWE_PROGRAMS_INSPECTION)
static_assert(PioneerQ38TokenPrograms::Family==0x48505451 && PioneerQ38TokenL1::Family==0x42505451,
 "QGWE programs require their distinct QTPH identity and expanded dense-Q8 journal");
#elif defined(PIONEER_Q38_QGWD_PROGRAMS_INSPECTION)
static_assert(PioneerQ38TokenPrograms::Family==0x47505451 && PioneerQ38TokenL1::Family==0x42505451,
 "QGWD programs require their distinct QTPG identity and expanded dense-Q8 journal");
#elif defined(PIONEER_Q38_QGWC_PROGRAMS_INSPECTION)
static_assert(PioneerQ38TokenPrograms::Family==0x46505451 && PioneerQ38TokenL1::Family==0x35505451,
 "QGWC programs require their distinct QTPF identity and hybrid journal geometry");
#elif defined(PIONEER_Q38_QGW8_DENSE_VALUE_PROGRAMS_INSPECTION) || defined(PIONEER_Q38_QGW8_DENSE_VALUE_NATIVE_134)
static_assert(PioneerQ38TokenPrograms::Family==0x45505451 && PioneerQ38TokenL1::Family==0x35505451,
 "dense-value offline programs require their distinct QTPE identity and expanded journal geometry");
#elif defined(PIONEER_Q38_QGWA_BATCH_PROGRAMS_INSPECTION)
static_assert(PioneerQ38TokenPrograms::Family==0x44505451 && PioneerQ38TokenL1::Family==0x35505451,
 "QGWA offline programs require their distinct QTPD identity and expanded journal geometry");
#elif defined(PIONEER_Q38_QGW8_DENSE_INPUT_NATIVE_133)
static_assert(PioneerQ38TokenPrograms::Family==0x43505451 && PioneerQ38TokenL1::Family==0x35505451,
 "tag133 requires its distinct QTPC dense-input program and expanded hybrid journal geometry");
#elif defined(PIONEER_Q38_QGW8_DENSE_STAGE_NATIVE_132)
static_assert(PioneerQ38TokenPrograms::Family==0x42505451 && PioneerQ38TokenL1::Family==0x35505451,
 "tag132 requires its distinct QTPB dense-stage program and expanded hybrid journal geometry");
#elif defined(PIONEER_Q38_QGW8_TRACE_NATIVE_131)
static_assert(PioneerQ38TokenPrograms::Family==0x41505451 && PioneerQ38TokenL1::Family==0x35505451,
 "tag131 requires its distinct QTPA trace program and expanded hybrid journal geometry");
#elif defined(PIONEER_Q38_QGW8_PROGRAMS_INSPECTION)
#ifdef PIONEER_Q38_DENSE_Q8_MATRIX_FOOTPRINT_INSPECTION
static_assert(PioneerQ38TokenPrograms::Family==0x38505451 && PioneerQ38TokenL1::Family==0x42505451,
 "QGW8 DATA may pair with dense Q8 footprint only in the isolated journal fixture");
#else
static_assert(PioneerQ38TokenPrograms::Family==0x38505451 && PioneerQ38TokenL1::Family==0x35505451,
 "QGW8 inspection programs require their distinct identity and expanded hybrid journal geometry");
#endif
#elif defined(PIONEER_Q38_HYBRID_MATRIX_ATTENTION_INSPECTION)
static_assert(PioneerQ38TokenPrograms::Family==0x37505451 && PioneerQ38TokenL1::Family==0x35505451,
 "QGW7 inspection programs require the expanded hybrid journal geometry");
#else
static_assert(PioneerQ38TokenPrograms::Family==0x35505451 && PioneerQ38TokenL1::Family==0x35505451,
 "QGW5 program and expanded journal must remain paired");
#endif
#else
#if defined(PIONEER_Q38_TOKEN_BF16_DEFERRED_PROGRAMS) && (!defined(PIONEER_Q38_TOKEN_BF16_PROGRAMS) || PIONEER_Q38_TOKEN_BF16_PROGRAMS != 1 \
 || !defined(PIONEER_Q38_TOKEN_BF16_PROFILE_PROGRAMS) || PIONEER_Q38_TOKEN_BF16_PROFILE_PROGRAMS != 1)
#error Deferred BF16 DATA requires its complete distinct program selection
#endif
#if defined(PIONEER_Q38_TOKEN_BF16_PAIR_ISSUE_PROGRAMS) && (!defined(PIONEER_Q38_TOKEN_BF16_PROGRAMS) || PIONEER_Q38_TOKEN_BF16_PROGRAMS != 1 \
 || !defined(PIONEER_Q38_TOKEN_BF16_PROFILE_PROGRAMS) || PIONEER_Q38_TOKEN_BF16_PROFILE_PROGRAMS != 1)
#error Paired BF16 DATA requires its complete distinct program selection
#endif
#if defined(PIONEER_Q38_TOKEN_BF16_PROFILE_PROGRAMS) && (!defined(PIONEER_Q38_TOKEN_BF16_PROGRAMS) || PIONEER_Q38_TOKEN_BF16_PROGRAMS != 1)
#error BF16 timing DATA requires its actual QTP4 parent,not a silent old-package fallback
#endif
#ifdef PIONEER_Q38_TOKEN_BF16_PROGRAMS
#ifdef PIONEER_Q38_TOKEN_BF16_DEFERRED_PROGRAMS
#include "PioneerQ38TokenBf16DeferredProgramsRole.h"
#include "PioneerQ38TokenBf16DeferredPrograms.h"
#elif defined(PIONEER_Q38_TOKEN_BF16_PAIR_ISSUE_PROGRAMS)
#include "PioneerQ38TokenBf16PairIssueProgramsRole.h"
#include "PioneerQ38TokenBf16PairIssuePrograms.h"
#elif defined(PIONEER_Q38_TOKEN_BF16_PROFILE_PROGRAMS)
#include "PioneerQ38TokenBf16ProfileProgramsRole.h"
#include "PioneerQ38TokenBf16ProfilePrograms.h"
#else
#include "PioneerQ38TokenBf16ProgramsRole.h"
#include "PioneerQ38TokenBf16Programs.h"
#endif
#include "PioneerQ38TokenFootprint.h"
#ifndef PIONEER_Q38_TOKEN_SELECTED_PROGRAMS_H
#define PIONEER_Q38_TOKEN_SELECTED_PROGRAMS_H
#ifdef PIONEER_Q38_TOKEN_BF16_DEFERRED_PROGRAMS
namespace PioneerQ38TokenPrograms=PioneerQ38TokenBf16DeferredPrograms;
#elif defined(PIONEER_Q38_TOKEN_BF16_PAIR_ISSUE_PROGRAMS)
namespace PioneerQ38TokenPrograms=PioneerQ38TokenBf16PairIssuePrograms;
#elif defined(PIONEER_Q38_TOKEN_BF16_PROFILE_PROGRAMS)
namespace PioneerQ38TokenPrograms=PioneerQ38TokenBf16ProfilePrograms;
#else
namespace PioneerQ38TokenPrograms=PioneerQ38TokenBf16Programs;
#endif
#endif
static_assert(PioneerQ38TokenPrograms::Family==0x34505451,"late BF16 immutable package selection");
#ifdef PIONEER_Q38_TOKEN_BF16_DEFERRED_PROGRAMS
static_assert(PioneerQ38TokenPrograms::Version==4 && PioneerQ38TokenPrograms::DeferredAudit==1 && PioneerQ38TokenPrograms::PairedIssue==1,"late deferred BF16 program selection");
#elif defined(PIONEER_Q38_TOKEN_BF16_PAIR_ISSUE_PROGRAMS)
static_assert(PioneerQ38TokenPrograms::Version==3 && PioneerQ38TokenPrograms::PairedIssue==1,"late paired BF16 program selection");
#elif defined(PIONEER_Q38_TOKEN_BF16_PROFILE_PROGRAMS)
static_assert(PioneerQ38TokenPrograms::Version==2,"late BF16 timing program selection");
#endif
#else
// DATA selection only; no native source/lifetime, journal or launch grant.
// Rechecked outside the guard so late/defined-zero changes cannot alias data.
#include "PioneerQ38TokenManualTTSyncProgramsRole.h"
#if defined(PIONEER_Q38_TOKEN_MANUAL_TTSYNC_PROGRAMS) && !defined(PIONEER_Q38_TOKEN_MANUAL_TTSYNC_NATIVE_CONTEXT) \
 && !defined(PIONEER_Q38_TOKEN_MANUAL_TTSYNC_BENCHMARK_NATIVE)
#error Manual completion DATA requires its separate native subject;no old fallback
#endif
#include "PioneerQ38TokenMatrixPhaseProgramsRole.h"
#if defined(PIONEER_Q38_TOKEN_MATRIX_PHASE_PROGRAMS) && !defined(PIONEER_Q38_TOKEN_MATRIX_PHASE_NATIVE_CONTEXT) \
 && !defined(PIONEER_Q38_TOKEN_MATRIX_PHASE_BENCHMARK_NATIVE)
#error Matrix-phase DATA requires its distinct private native subject;no old fallback
#endif
#include "PioneerQ38TokenConstantQ4ProgramsRole.h"
#if defined(PIONEER_Q38_TOKEN_CONSTANT_Q4_PROGRAMS) \
 && !defined(PIONEER_Q38_TOKEN_CONSTANT_Q4_NATIVE_CONTEXT) && !defined(PIONEER_Q38_TOKEN_CONSTANT_Q4_BENCHMARK_NATIVE)
#error Constant Q4 DATA requires its distinct native subject;no silent old-package fallback
#endif
#include "PioneerQ38TokenPhaseProfileProgramsRole.h"
#if defined(PIONEER_Q38_TOKEN_PHASE_PROFILE_PROGRAMS) \
 && !defined(PIONEER_Q38_TOKEN_PHASE_PROFILE_NATIVE_CONTEXT) && !defined(PIONEER_Q38_TOKEN_PHASE_PROFILE_BENCHMARK_NATIVE)
#error Phase timing DATA requires its distinct native subject;never silently select old programs
#endif
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
#ifdef PIONEER_Q38_FULL_OFFLOAD_NATIVE
#ifdef PIONEER_Q38_FULL_OFFLOAD_FAST_DECODE_PROFILE
#include "PioneerQ38FullOffloadFastDecodePrograms.h"
#elif defined(PIONEER_Q38_FULL_OFFLOAD_FIXED_CONFIG_Q4)
#include "PioneerQ38FullOffloadFixedConfigQ4Programs.h"
#elif defined(PIONEER_Q38_FULL_OFFLOAD_QUEUED_Q4)
#include "PioneerQ38FullOffloadQueuedQ4Programs.h"
#elif defined(PIONEER_Q38_FULL_OFFLOAD_INSTRUCTION_PREFETCH)
#include "PioneerQ38FullOffloadInstructionPrefetchQ4Programs.h"
#elif defined(PIONEER_Q38_FULL_OFFLOAD_TWO_PRODUCT_REPLAY)
#include "PioneerQ38FullOffloadTwoProductReplayQ4Programs.h"
#elif defined(PIONEER_Q38_FULL_OFFLOAD_PHASE_PROFILE)
#include "PioneerQ38FullOffloadPreparedProfileQ4Programs.h"
#elif defined(PIONEER_Q38_FULL_OFFLOAD_COUNTER_UNROLL)
#include "PioneerQ38FullOffloadPreparedCounterQ4Programs.h"
#elif defined(PIONEER_Q38_FULL_OFFLOAD_PREPARED_Q4)
#include "PioneerQ38FullOffloadPreparedQ4Programs.h"
#elif defined(PIONEER_Q38_FULL_OFFLOAD_MACRO_Q4)
#include "PioneerQ38FullOffloadMacroQ4Programs.h"
#elif defined(PIONEER_Q38_FULL_OFFLOAD_VECTOR_Q4)
#include "PioneerQ38FullOffloadVectorQ4Programs.h"
#else
#ifdef PIONEER_Q38_FULL_OFFLOAD_BATCHED
#include "PioneerQ38FullOffloadBatchedPrograms.h"
#else
#include "PioneerQ38FullOffloadPrograms.h"
#endif
#endif
#elif defined(PIONEER_Q38_TOKEN_MANUAL_TTSYNC_PROGRAMS)
#include "PioneerQ38TokenManualTTSyncPrograms.h"
#elif defined(PIONEER_Q38_TOKEN_MATRIX_PHASE_PROGRAMS)
#include "PioneerQ38TokenMatrixPhasePrograms.h"
#elif defined(PIONEER_Q38_TOKEN_CONSTANT_Q4_PROGRAMS)
#include "PioneerQ38TokenConstantQ4Programs.h"
#elif defined(PIONEER_Q38_TOKEN_PHASE_PROFILE_PROGRAMS)
#include "PioneerQ38TokenPhaseProfilePrograms.h"
#elif defined(PIONEER_Q38_TOKEN_WIDE_GDN_PROGRAMS)
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
#ifdef PIONEER_Q38_FULL_OFFLOAD_NATIVE
#ifdef PIONEER_Q38_FULL_OFFLOAD_FAST_DECODE_PROFILE
namespace PioneerQ38TokenPrograms=PioneerQ38FullOffloadFastDecodePrograms;
static_assert(PioneerQ38TokenPrograms::FastDecodeProfile==1 && PioneerQ38TokenPrograms::TokenPhaseProfile==1
 && PioneerQ38TokenPrograms::PhaseProfileVersion==3 && PioneerQ38TokenPrograms::RootSections==5
 && PioneerQ38TokenPrograms::VectorQ4Metadata==1 && PioneerQ38TokenPrograms::WideGdn==1
 && PioneerQ38TokenPrograms::EogCount==5,"exact fast decode ROOT and unchanged vector92 workers required");
#elif defined(PIONEER_Q38_FULL_OFFLOAD_FIXED_CONFIG_Q4)
namespace PioneerQ38TokenPrograms=PioneerQ38FullOffloadFixedConfigQ4Programs;
static_assert(PioneerQ38TokenPrograms::FixedConfigQ4==1 && PioneerQ38TokenPrograms::FixedConfigFields==6
 && PioneerQ38TokenPrograms::FixedConfigSeedWords==24 && PioneerQ38TokenPrograms::FixedConfigLowBytes==2
 && PioneerQ38TokenPrograms::FixedConfigFinalReads==6 && PioneerQ38TokenPrograms::QueuedQ4Block==1
 && PioneerQ38TokenPrograms::QueuedQ4Wait==0xa2ff9816U && PioneerQ38TokenPrograms::QueuedQ4EscapeBytes==1280
 && PioneerQ38TokenPrograms::InstructionPrefetch==1 && PioneerQ38TokenPrograms::TwoProductQ4==1
 && PioneerQ38TokenPrograms::AffineReplayWords==13 && PioneerQ38TokenPrograms::TokenPhaseProfile==1
 && PioneerQ38TokenPrograms::CounterUnroll==1 && PioneerQ38TokenPrograms::PreparedQ4==1
 && PioneerQ38TokenPrograms::RootSections==5 && PioneerQ38TokenPrograms::EogCount==5,
 "exact fixed-config ROOT+worker package required");
#elif defined(PIONEER_Q38_FULL_OFFLOAD_QUEUED_Q4)
namespace PioneerQ38TokenPrograms=PioneerQ38FullOffloadQueuedQ4Programs;
static_assert(PioneerQ38TokenPrograms::QueuedQ4Block==1 && PioneerQ38TokenPrograms::QueuedQ4Wait==0xa2ff9816U
 && PioneerQ38TokenPrograms::QueuedQ4EscapeBase==0x9c610 && PioneerQ38TokenPrograms::QueuedQ4EscapeStride==80
 && PioneerQ38TokenPrograms::QueuedQ4EscapeBytes==1280 && PioneerQ38TokenPrograms::InstructionPrefetch==1
 && PioneerQ38TokenPrograms::TwoProductQ4==1 && PioneerQ38TokenPrograms::AffineReplayWords==13
 && PioneerQ38TokenPrograms::TokenPhaseProfile==1 && PioneerQ38TokenPrograms::CounterUnroll==1
 && PioneerQ38TokenPrograms::PreparedQ4==1 && PioneerQ38TokenPrograms::RootSections==5
 && PioneerQ38TokenPrograms::EogCount==5,"exact queued-block ROOT+worker package required");
#elif defined(PIONEER_Q38_FULL_OFFLOAD_INSTRUCTION_PREFETCH)
namespace PioneerQ38TokenPrograms=PioneerQ38FullOffloadInstructionPrefetchQ4Programs;
static_assert(PioneerQ38TokenPrograms::InstructionPrefetch==1 && PioneerQ38TokenPrograms::PrefetchLimitRegister==203
 && PioneerQ38TokenPrograms::PrefetchControlRegister==208 && PioneerQ38TokenPrograms::PrefetchRequests==4
 && PioneerQ38TokenPrograms::PrefetchLastByte==0xc0c67 && PioneerQ38TokenPrograms::EogCount==5
 && PioneerQ38TokenPrograms::TwoProductQ4==1 && PioneerQ38TokenPrograms::AffineReplayWords==13
 && PioneerQ38TokenPrograms::TokenPhaseProfile==1 && PioneerQ38TokenPrograms::CounterUnroll==1
 && PioneerQ38TokenPrograms::PreparedQ4==1 && PioneerQ38TokenPrograms::RootSections==5,
 "exact bounded instruction-prefetch ROOT+worker package required");
#elif defined(PIONEER_Q38_FULL_OFFLOAD_TWO_PRODUCT_REPLAY)
namespace PioneerQ38TokenPrograms=PioneerQ38FullOffloadTwoProductReplayQ4Programs;
static_assert(PioneerQ38TokenPrograms::EogCount==5 && PioneerQ38TokenPrograms::TwoProductQ4==1
 && PioneerQ38TokenPrograms::AffineReplayWords==13 && PioneerQ38TokenPrograms::TokenPhaseProfile==1
 && PioneerQ38TokenPrograms::CounterUnroll==1 && PioneerQ38TokenPrograms::PreparedQ4==1
 && PioneerQ38TokenPrograms::RootSections==5,"exact replay13 two-product ROOT+worker package required");
#elif defined(PIONEER_Q38_FULL_OFFLOAD_PHASE_PROFILE)
namespace PioneerQ38TokenPrograms=PioneerQ38FullOffloadPreparedProfileQ4Programs;
static_assert(PioneerQ38TokenPrograms::EogCount==5 && PioneerQ38TokenPrograms::TokenPhaseProfile==1
 && PioneerQ38TokenPrograms::CounterUnroll==1 && PioneerQ38TokenPrograms::PreparedQ4==1
 && PioneerQ38TokenPrograms::RootSections==5,"exact phase-profile prepared ROOT+worker package required");
#elif defined(PIONEER_Q38_FULL_OFFLOAD_COUNTER_UNROLL)
namespace PioneerQ38TokenPrograms=PioneerQ38FullOffloadPreparedCounterQ4Programs;
static_assert(PioneerQ38TokenPrograms::EogCount==5 && PioneerQ38TokenPrograms::CounterUnroll==1
 && PioneerQ38TokenPrograms::PreparedQ4==1 && PioneerQ38TokenPrograms::RootSections==5,"exact counter-unroll prepared ROOT+worker package required");
#elif defined(PIONEER_Q38_FULL_OFFLOAD_PREPARED_Q4)
namespace PioneerQ38TokenPrograms=PioneerQ38FullOffloadPreparedQ4Programs;
static_assert(PioneerQ38TokenPrograms::EogCount==5 && PioneerQ38TokenPrograms::PreparedQ4==1
 && PioneerQ38TokenPrograms::NativeQ4Exact==1 && PioneerQ38TokenPrograms::WideGdn==1
 && PioneerQ38TokenPrograms::RootSections==5,"exact paired prepared ROOT+worker package required");
#elif defined(PIONEER_Q38_FULL_OFFLOAD_MACRO_Q4)
namespace PioneerQ38TokenPrograms=PioneerQ38FullOffloadMacroQ4Programs;
static_assert(PioneerQ38TokenPrograms::EogCount==5 && PioneerQ38TokenPrograms::WideGdn==1
 && PioneerQ38TokenPrograms::VectorQ4Metadata==1 && PioneerQ38TokenPrograms::BatchedOutputClosure==1
 && PioneerQ38TokenPrograms::TwinBatchPrefetch==1 && PioneerQ38TokenPrograms::MacroQ4Decode==1
 && PioneerQ38TokenPrograms::MacroQ4PrivateJobConfig==1 && PioneerQ38TokenPrograms::MacroQ4InertReturn==1,
 "exact full-card macro Q4 package required");
#elif defined(PIONEER_Q38_FULL_OFFLOAD_VECTOR_Q4)
namespace PioneerQ38TokenPrograms=PioneerQ38FullOffloadVectorQ4Programs;
static_assert(PioneerQ38TokenPrograms::EogCount==5 && PioneerQ38TokenPrograms::WideGdn==1
 && PioneerQ38TokenPrograms::VectorQ4Metadata==1 && PioneerQ38TokenPrograms::BatchedOutputClosure==1
 && PioneerQ38TokenPrograms::TwinBatchPrefetch==1,"exact full-card vector Q4 package required");
#else
#ifdef PIONEER_Q38_FULL_OFFLOAD_BATCHED
namespace PioneerQ38TokenPrograms=PioneerQ38FullOffloadBatchedPrograms;
static_assert(PioneerQ38TokenPrograms::EogCount==5 && PioneerQ38TokenPrograms::WideGdn==1
 && PioneerQ38TokenPrograms::BatchedOutputClosure==1 && PioneerQ38TokenPrograms::TwinBatchPrefetch==1,
 "full-card batched firmware cannot silently select the original G3 image");
#else
namespace PioneerQ38TokenPrograms=PioneerQ38FullOffloadPrograms;
static_assert(PioneerQ38TokenPrograms::EogCount==5,"full offload cannot silently use the old two-EOS firmware");
#endif
#endif
#elif defined(PIONEER_Q38_TOKEN_MANUAL_TTSYNC_PROGRAMS)
namespace PioneerQ38TokenPrograms=PioneerQ38TokenManualTTSyncPrograms;
#elif defined(PIONEER_Q38_TOKEN_MATRIX_PHASE_PROGRAMS)
namespace PioneerQ38TokenPrograms=PioneerQ38TokenMatrixPhasePrograms;
#elif defined(PIONEER_Q38_TOKEN_CONSTANT_Q4_PROGRAMS)
namespace PioneerQ38TokenPrograms=PioneerQ38TokenConstantQ4Programs;
#elif defined(PIONEER_Q38_TOKEN_PHASE_PROFILE_PROGRAMS)
namespace PioneerQ38TokenPrograms=PioneerQ38TokenPhaseProfilePrograms;
#elif defined(PIONEER_Q38_TOKEN_WIDE_GDN_PROGRAMS)
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
#endif // distinct QTP4 selection versus all old packages
#endif // offline QGW5 fixture versus existing native/legacy selection
