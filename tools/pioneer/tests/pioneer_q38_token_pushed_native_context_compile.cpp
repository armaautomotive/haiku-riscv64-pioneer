/* SPDX-License-Identifier: MIT */
// Actual private G3 body, COMPILE ONLY. No link, installation or invocation.
#define PIONEER_Q38_TOKEN_PUSHED_NATIVE_CONTEXT 1
#define PIONEER_Q38_TOKEN_PUSHED_PROGRAMS 1
#include "pioneer_q38_token_native_context_compile.cpp"
static_assert(PioneerQ38TokenNativeProfile==3,"distinct QT3NODRV profile");
static_assert(PioneerQ38TokenPrograms::Family==0x33505451,"actual G3 code package");
static_assert(PioneerQ38TokenL1::SavedBytes==53913832 && PioneerQ38TokenL1::Precise==1,"actual full G3 journal");
static_assert(PioneerQ38SelectedGlobalWorker::Magic==0x33574751 && PioneerQ38SelectedWorkerBootEnd==55
 && PioneerQ38SelectedWorkerPushed==1,"actual G3 worker boot grammar");
static_assert(sizeof(PioneerQ38TokenMemory)>=PioneerQ38TokenL1::SavedBytes
 && sizeof(PioneerQ38TokenMemory)<PioneerQ38TokenL1::SavedBytes+65536,"full journal is owned by the actual heap object");
