/* SPDX-License-Identifier: MIT */
#if defined(_KERNEL_MODE) || defined(PIONEER_Q38_TOKEN_NATIVE_CONTEXT) || defined(PIONEER_Q38_WORK_INFERENCE) \
 || defined(TT_PIONEER_Q38_WORK_MODE) || defined(PIONEER_Q38_CPU_VALIDATION) || defined(PIONEER_Q38_FULL_MODEL_CPU_REFERENCE) \
 || defined(PIONEER_Q38_TOKEN_MATRIX_PHASE_NATIVE_CONTEXT) || defined(PIONEER_Q38_TOKEN_MATRIX_PHASE_BENCHMARK_NATIVE)
#error Sampler expected ROM bridge is immutable HOST DATA only
#endif
#define PIONEER_Q38_TOKEN_MATRIX_PHASE_PROGRAMS 1
#include "../PioneerQ38TokenMatrixPhasePrograms.h"
#include "pioneer_q38_precise_beta_program_data_bridge.h"
bool Q38PreciseBetaTestProgramWord(unsigned worker,uint32_t address,uint32_t& output){
 if(!worker || worker>=120)return false;
 return PioneerQ38TokenMatrixPhasePrograms::ProgramWord(worker,address,output);
} // Complete installed PEER code DATA,not a PC/product/receipt/owner injection.
