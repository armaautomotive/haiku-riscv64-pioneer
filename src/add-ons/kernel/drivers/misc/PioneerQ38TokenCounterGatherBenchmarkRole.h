/* SPDX-License-Identifier: MIT */
// Bounded6607+6608 diagnostic;complete module SHA,not tag75,is identity.
#if !defined(PIONEER_Q38_TOKEN_COUNTER_GATHER_BENCHMARK_NATIVE) || PIONEER_Q38_TOKEN_COUNTER_GATHER_BENCHMARK_NATIVE != 1 \
 || !defined(PIONEER_Q38_TOKEN_COUNTER_GATHER_PROGRAMS) || PIONEER_Q38_TOKEN_COUNTER_GATHER_PROGRAMS != 1 \
 || !defined(PIONEER_Q38_TOKEN_TID_BOUNDARY_BENCHMARK_NATIVE) || PIONEER_Q38_TOKEN_TID_BOUNDARY_BENCHMARK_NATIVE != 1 \
 || !defined(PIONEER_Q38_TOKEN_TID_BOUNDARY_PROGRAMS) || PIONEER_Q38_TOKEN_TID_BOUNDARY_PROGRAMS != 1 \
 || defined(PIONEER_Q38_TOKEN_COUNTER_GATHER_NATIVE_CONTEXT) || defined(PIONEER_Q38_COUNTER_GATHER_IMAGE)
#error Exact counter-gather diagnostic required,not DATA,private NODRV or ROOT BUILD
#endif
#include "PioneerQ38TokenTidBoundaryBenchmarkRole.h"
#ifndef PIONEER_Q38_TOKEN_COUNTER_GATHER_PHYSICAL_BINDING_H
#define PIONEER_Q38_TOKEN_COUNTER_GATHER_PHYSICAL_BINDING_H
enum {PioneerQ38TokenCounterGatherPhysicalSelection=1};
#endif
static_assert(PioneerQ38TokenCounterGatherPhysicalSelection==1,"late counter-gather diagnostic binding");
