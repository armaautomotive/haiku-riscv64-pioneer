/* SPDX-License-Identifier: MIT */
#if defined(_KERNEL_MODE) || defined(PIONEER_Q38_TOKEN_NATIVE_CONTEXT) || defined(PIONEER_Q38_WORK_INFERENCE) \
 || defined(TT_PIONEER_Q38_WORK_MODE) || defined(PIONEER_Q38_CPU_VALIDATION) || defined(PIONEER_Q38_FULL_MODEL_CPU_REFERENCE) \
 || defined(PIONEER_Q38_TOKEN_PHASE_PROFILE_NATIVE_CONTEXT) || defined(PIONEER_Q38_TOKEN_PHASE_PROFILE_BENCHMARK_NATIVE)
#error Timing expected ROM bridge is immutable HOST DATA only
#endif
#define PIONEER_Q38_TOKEN_PHASE_PROFILE_PROGRAMS 1
#include "../PioneerQ38TokenPhaseProfilePrograms.h"
#include "pioneer_q38_precise_beta_program_data_bridge.h"
bool Q38PreciseBetaTestProgramWord(unsigned worker,uint32_t address,uint32_t& output){
 if(!worker || worker>=120)return false;
 return PioneerQ38TokenPhaseProfilePrograms::ProgramWord(worker,address,output);
} // Immutable installed peer code ONLY,not ROOT/PC/product/receipt/state injection.
