/* SPDX-License-Identifier: MIT */
// Returning BUILD only. No native, DATA, device or Work authorization.
#include "PioneerQ38TidBoundaryRole.h"
#ifdef PIONEER_Q38_COUNTER_GATHER_IMAGE
#if PIONEER_Q38_COUNTER_GATHER_IMAGE != 1 \
 || !defined(PIONEER_Q38_TID_BOUNDARY_IMAGE) || PIONEER_Q38_TID_BOUNDARY_IMAGE != 1
#error Counter gather requires the exact FIFO-enabled returning TID BUILD
#endif
#if defined(PIONEER_Q38_TOKEN_COUNTER_GATHER_PROGRAMS) \
 || defined(PIONEER_Q38_TOKEN_COUNTER_GATHER_NATIVE_CONTEXT) \
 || defined(PIONEER_Q38_TOKEN_COUNTER_GATHER_BENCHMARK_NATIVE)
#error Counter gather BUILD cannot alias DATA or native ownership
#endif
#endif
#ifndef PIONEER_Q38_COUNTER_GATHER_BINDING_H
#define PIONEER_Q38_COUNTER_GATHER_BINDING_H
#ifdef PIONEER_Q38_COUNTER_GATHER_IMAGE
enum {PioneerQ38CounterGatherProfile=1};
#else
enum {PioneerQ38CounterGatherProfile=0};
#endif
#endif
#ifdef PIONEER_Q38_COUNTER_GATHER_IMAGE
static_assert(PioneerQ38CounterGatherProfile==1,"late counter gather selection");
#else
static_assert(PioneerQ38CounterGatherProfile==0,"late counter gather removal");
#endif
