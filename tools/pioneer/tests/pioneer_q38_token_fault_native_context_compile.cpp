/* SPDX-License-Identifier: MIT */
// Actual native owner selecting the separate marker ROM. Never a driver ABI.
#define PIONEER_Q38_TOKEN_FAULT_PROGRAMS 1
#include "pioneer_q38_token_hidden_read_native_context_compile.cpp"
static_assert(PioneerQ38TokenFaultProgramSelection==1,"private fault code selection");
static_assert(PioneerQ38TokenPrograms::FaultMarker==0x51465431,"qualified assembly error marker");
static_assert(sizeof(PioneerQ38TokenTerminalResult)==64,"unchanged private result extent");
static_assert(__builtin_offsetof(PioneerQ38TokenTerminalResult,faultError)==56
 && __builtin_offsetof(PioneerQ38TokenTerminalResult,faultSavedLink)==60,"last two private DATA words");
