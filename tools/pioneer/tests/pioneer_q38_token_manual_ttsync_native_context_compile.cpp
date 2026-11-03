/* SPDX-License-Identifier: MIT */
// PRIVATE subject only: exact new ROM + inherited original-model owner.
#define PIONEER_Q38_TOKEN_MANUAL_TTSYNC_PROGRAMS 1
#define PIONEER_Q38_TOKEN_MANUAL_TTSYNC_NATIVE_CONTEXT 1
#include "pioneer_q38_token_constant_q4_native_context_compile.cpp"
static_assert(PioneerQ38TokenManualTTSyncDataSelection==2
 && PioneerQ38TokenPrograms::ManualTTSync==1 && PioneerQ38TokenPrograms::ManualTTSyncRevision==1
 && PioneerQ38TokenPrograms::ManualTTSyncOverrideClearRequired==1
 && PioneerQ38TokenPrograms::ManualTTSyncLocalTimeout==0,
 "new complete returning worker requires a real native override guard and external watchdog");
