/* SPDX-License-Identifier: MIT */
#if defined(_KERNEL_MODE) || defined(PIONEER_Q38_TOKEN_NATIVE_CONTEXT) || defined(PIONEER_Q38_WORK_INFERENCE) \
 || defined(TT_PIONEER_Q38_WORK_MODE) || defined(PIONEER_Q38_CPU_VALIDATION) || defined(PIONEER_Q38_FULL_MODEL_CPU_REFERENCE) \
 || defined(PIONEER_Q38_TOKEN_MANUAL_TTSYNC_NATIVE_CONTEXT) || defined(PIONEER_Q38_TOKEN_MANUAL_TTSYNC_BENCHMARK_NATIVE)
#error Manual completion expected ROM is pure HOST DATA,not native or an oracle
#endif
#define PIONEER_Q38_TOKEN_MANUAL_TTSYNC_PROGRAMS 1
#include "../PioneerQ38TokenManualTTSyncPrograms.h"
#include "pioneer_q38_precise_beta_program_data_bridge.h"
bool Q38PreciseBetaTestProgramWord(unsigned worker,uint32_t address,uint32_t& output){
 if(!worker || worker>=120)return false;
 return PioneerQ38TokenManualTTSyncPrograms::ProgramWord(worker,address,output);
} // Expected immutable code ONLY;installed actual L1 is independently fetched.
