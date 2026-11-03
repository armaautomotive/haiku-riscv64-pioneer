/* SPDX-License-Identifier: MIT */
#if !defined(PIONEER_Q38_TOKEN_NATIVE_MANUAL_TTSYNC_FIXTURE) || PIONEER_Q38_TOKEN_NATIVE_MANUAL_TTSYNC_FIXTURE != 1 \
 || !defined(PIONEER_Q38_TOKEN_MANUAL_TTSYNC_PROGRAMS) || PIONEER_Q38_TOKEN_MANUAL_TTSYNC_PROGRAMS != 1 \
 || !defined(PIONEER_Q38_TOKEN_MANUAL_TTSYNC_NATIVE_CONTEXT) || PIONEER_Q38_TOKEN_MANUAL_TTSYNC_NATIVE_CONTEXT != 1 \
 || !defined(PIONEER_Q38_TOKEN_NATIVE_CONSTANT_Q4_FIXTURE) || PIONEER_Q38_TOKEN_NATIVE_CONSTANT_Q4_FIXTURE != 1 \
 || defined(PIONEER_Q38_TOKEN_MANUAL_TTSYNC_BENCHMARK_NATIVE) || defined(PIONEER_Q38_MANUAL_TTSYNC_IMAGE)
#error Distinct manual-wait TEST requires whole original first-layer/cancel/STOP/HOLD/drain/full-END history
#endif
#include "../PioneerQ38TokenManualTTSyncProgramsRole.h"
#include "pioneer_q38_token_native_constant_q4_role.h"
static_assert(PioneerQ38TokenManualTTSyncDataSelection==2 && PioneerQ38TokenPrograms::ManualTTSync==1
 && PioneerQ38TokenPrograms::ManualTTSyncOverrideClearRequired==1 && PioneerQ38TokenPrograms::ManualTTSyncLocalTimeout==0
 && sizeof(PioneerQ38TokenPrograms::PeerText)==6260,"actual complete new ROM,not a HOST endpoint-only alias");
