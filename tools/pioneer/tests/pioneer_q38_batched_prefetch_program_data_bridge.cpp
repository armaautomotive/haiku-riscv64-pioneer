/* SPDX-License-Identifier: MIT */
#if defined(_KERNEL_MODE) || defined(PIONEER_Q38_TOKEN_NATIVE_CONTEXT) || defined(PIONEER_Q38_WORK_INFERENCE) \
 || defined(TT_PIONEER_Q38_WORK_MODE) || defined(PIONEER_Q38_CPU_VALIDATION) || defined(PIONEER_Q38_FULL_MODEL_CPU_REFERENCE)
#error Expected batched ROM bridge is immutable HOST DATA only
#endif
#define PIONEER_Q38_TOKEN_BATCHED_PREFETCH_PROGRAMS 1
#define PIONEER_Q38_TOKEN_PREFETCH_PROGRAMS 1
#define PIONEER_Q38_TOKEN_FLAT_MATRIX_PROGRAMS 1
#define PIONEER_Q38_TOKEN_EMPTY_RECEIPT_POLL_PROGRAMS 1
#define PIONEER_Q38_TOKEN_COUNTER_GATHER_PROGRAMS 1
#define PIONEER_Q38_TOKEN_TID_BOUNDARY_PROGRAMS 1
#define PIONEER_Q38_TOKEN_PACKET_IDENTITY_BOUNDARY_PROGRAMS 1
#define PIONEER_Q38_TOKEN_COHORT_LOCAL_SOURCES_PROGRAMS 1
#define PIONEER_Q38_TOKEN_COMMIT_BOUNDARY_PROGRAMS 1
#define PIONEER_Q38_TOKEN_RETAINED_COHORT_PROGRAMS 1
#define PIONEER_Q38_TOKEN_FRONTIER_PROGRAMS 1
#define PIONEER_Q38_TOKEN_JOB_BOOT_FOLD_PROGRAMS 1
#define PIONEER_Q38_TOKEN_PRECISE_BETA_PROGRAMS 1
#define PIONEER_Q38_TOKEN_FAULT_PROGRAMS 1
#define PIONEER_Q38_TOKEN_PUSHED_PROGRAMS 1
#include "../PioneerQ38TokenBatchedPrefetchPrograms.h"
#include "pioneer_q38_precise_beta_program_data_bridge.h"
bool Q38PreciseBetaTestProgramWord(unsigned worker,uint32_t address,uint32_t& output){
 if(!worker || worker>=120)return false;
 return PioneerQ38TokenBatchedPrefetchPrograms::ProgramWord(worker,address,output);
} // SAME opaque ABI;distinct ten-section expected peer ROM,no native/CPU roles.
