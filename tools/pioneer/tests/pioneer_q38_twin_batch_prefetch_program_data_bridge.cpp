/* SPDX-License-Identifier: MIT */
#if defined(_KERNEL_MODE) || defined(PIONEER_Q38_TOKEN_NATIVE_CONTEXT) || defined(PIONEER_Q38_WORK_INFERENCE) \
 || defined(TT_PIONEER_Q38_WORK_MODE) || defined(PIONEER_Q38_CPU_VALIDATION) || defined(PIONEER_Q38_FULL_MODEL_CPU_REFERENCE) \
 || defined(PIONEER_Q38_TOKEN_TWIN_BATCH_PREFETCH_NATIVE_CONTEXT)
#error Expected twin-batch ROM bridge is immutable HOST DATA only
#endif
#define PIONEER_Q38_TOKEN_TWIN_BATCH_PREFETCH_PROGRAMS 1
#include "../PioneerQ38TokenTwinBatchPrefetchPrograms.h"
#include "pioneer_q38_precise_beta_program_data_bridge.h"
bool Q38PreciseBetaTestProgramWord(unsigned worker,uint32_t address,uint32_t& output){
 if(!worker || worker>=120)return false;
 return PioneerQ38TokenTwinBatchPrefetchPrograms::ProgramWord(worker,address,output);
} // same opaque const-ROM ABI;ROOT independently authenticated by native fixture
