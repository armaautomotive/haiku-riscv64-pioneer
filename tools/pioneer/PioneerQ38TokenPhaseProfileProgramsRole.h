/* SPDX-License-Identifier: MIT */
#include "PioneerQ38TokenPhaseProfileDataBoundary.h"
#ifndef PIONEER_Q38_TOKEN_PHASE_PROFILE_PARENT_BINDING_H
#define PIONEER_Q38_TOKEN_PHASE_PROFILE_PARENT_BINDING_H
#ifdef PIONEER_Q38_TOKEN_PHASE_PROFILE_PROGRAMS
enum {PioneerQ38TokenPhaseProfileParentBinding=1};
#else
enum {PioneerQ38TokenPhaseProfileParentBinding=0};
#endif
#endif
#ifdef PIONEER_Q38_TOKEN_PHASE_PROFILE_PROGRAMS
static_assert(PioneerQ38TokenPhaseProfileParentBinding==1,"late phase timing parent binding");
#if defined(PIONEER_Q38_TOKEN_PHASE_PROFILE_NATIVE_CONTEXT) || defined(PIONEER_Q38_TOKEN_PHASE_PROFILE_BENCHMARK_NATIVE)
#include "PioneerQ38TokenWideGdnProgramsRole.h"
#include "../../src/add-ons/kernel/drivers/misc/PioneerQ38TokenPhaseProfileRole.h"
#endif
#else
static_assert(PioneerQ38TokenPhaseProfileParentBinding==0,"late phase timing parent removal");
#endif
