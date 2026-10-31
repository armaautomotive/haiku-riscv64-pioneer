/* SPDX-License-Identifier: MIT */
#if defined(_KERNEL_MODE) || defined(PIONEER_Q38_TOKEN_NATIVE_CONTEXT) || defined(PIONEER_Q38_WORK_INFERENCE) \
 || defined(TT_PIONEER_Q38_WORK_MODE) || defined(PIONEER_Q38_CPU_VALIDATION) || defined(PIONEER_Q38_FULL_MODEL_CPU_REFERENCE)
#error Expected ROM DATA bridge is HOST-only, never tensor Work, CPU oracle or native code
#endif
#define PIONEER_Q38_TOKEN_PRECISE_BETA_PROGRAMS 1
#define PIONEER_Q38_TOKEN_FAULT_PROGRAMS 1
#define PIONEER_Q38_TOKEN_PUSHED_PROGRAMS 1
#include "../PioneerQ38TokenSelectedPrograms.h"
#include "pioneer_q38_precise_beta_program_data_bridge.h"
bool Q38PreciseBetaTestProgramWord(unsigned worker,uint32_t address,uint32_t& output){
 return PioneerQ38TokenPrograms::ProgramWord(worker,address,output);
}
