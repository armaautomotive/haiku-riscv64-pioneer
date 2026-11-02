/* SPDX-License-Identifier: MIT */
#include "PioneerQ38TokenConstantQ4ClientRole.h"
#include "PioneerQ38FrontierFailureReportRole.h"
#ifndef PIONEER_Q38_TOKEN_PHASE_PROFILE_CLIENT_SELECTION_H
#define PIONEER_Q38_TOKEN_PHASE_PROFILE_CLIENT_SELECTION_H
#ifdef PIONEER_Q38_TOKEN_PHASE_PROFILE_CLIENT
enum {PioneerQ38TokenPhaseProfileClientSelection=1};
#else
enum {PioneerQ38TokenPhaseProfileClientSelection=0};
#endif
#endif
#ifdef PIONEER_Q38_TOKEN_PHASE_PROFILE_CLIENT
#if PIONEER_Q38_TOKEN_PHASE_PROFILE_CLIENT != 1 \
 || !defined(PIONEER_Q38_TOKEN_FRONTIER_FAILURE_REPORT_CLIENT) || PIONEER_Q38_TOKEN_FRONTIER_FAILURE_REPORT_CLIENT != 1
#error Exact phase timing USER client and complete fixed34 failed-reply parents required
#endif
#if defined(PIONEER_Q38_TOKEN_PHASE_PROFILE_PROGRAMS) || defined(PIONEER_Q38_TOKEN_PHASE_PROFILE_NATIVE_CONTEXT) \
 || defined(PIONEER_Q38_TOKEN_PHASE_PROFILE_BENCHMARK_NATIVE) || defined(PIONEER_Q38_TOKEN_PHASE_PROFILE_IMAGE) \
 || defined(PIONEER_Q38_TOKEN_WIDE_GDN_PROGRAMS) || defined(PIONEER_Q38_TOKEN_WIDE_GDN_NATIVE_CONTEXT) \
 || defined(PIONEER_Q38_TOKEN_WIDE_GDN_BENCHMARK_NATIVE)
#error Phase timing USER client is not program DATA,shader or native owner
#endif
static_assert(PioneerQ38TokenPhaseProfileClientSelection==1,"late phase timing USER selection");
#else
static_assert(PioneerQ38TokenPhaseProfileClientSelection==0,"late phase timing USER removal");
#endif
