/* SPDX-License-Identifier: MIT */
// Isolated returning BUILD. Read-only metadata may precede a source fault
// at the next write/full boundary. No DATA, native, hardware or Work grant.
#include "PioneerQ38CounterGatherRole.h"
#ifdef PIONEER_Q38_READ_SOURCE_BOUNDARY_IMAGE
#if PIONEER_Q38_READ_SOURCE_BOUNDARY_IMAGE != 1 \
 || !defined(PIONEER_Q38_COUNTER_GATHER_IMAGE) || PIONEER_Q38_COUNTER_GATHER_IMAGE != 1 \
 || !defined(PIONEER_Q38_GLOBAL_CHAIN_COMPONENT_IMAGE) || PIONEER_Q38_GLOBAL_CHAIN_COMPONENT_IMAGE != 1 \
 || !defined(PIONEER_Q38_LAYER_COMPONENT_RECURRENT) || PIONEER_Q38_LAYER_COMPONENT_RECURRENT != 1 \
 || !defined(PIONEER_Q38_GLOBAL_CHAIN_BOUNDARY_SEALS) || PIONEER_Q38_GLOBAL_CHAIN_BOUNDARY_SEALS != 1
#error Read-source boundaries require the exact returning counter-gather BUILD
#endif
#if defined(PIONEER_Q38_TOKEN_READ_SOURCE_BOUNDARY_PROGRAMS) \
 || defined(PIONEER_Q38_TOKEN_READ_SOURCE_BOUNDARY_NATIVE_CONTEXT) \
 || defined(PIONEER_Q38_TOKEN_READ_SOURCE_BOUNDARY_BENCHMARK_NATIVE)
#error Read-source BUILD cannot alias DATA or native ownership
#endif
#endif
#ifndef PIONEER_Q38_READ_SOURCE_BOUNDARY_BINDING_H
#define PIONEER_Q38_READ_SOURCE_BOUNDARY_BINDING_H
#ifdef PIONEER_Q38_READ_SOURCE_BOUNDARY_IMAGE
enum {PioneerQ38ReadSourceBoundaryProfile=1};
#else
enum {PioneerQ38ReadSourceBoundaryProfile=0};
#endif
#endif
#ifdef PIONEER_Q38_READ_SOURCE_BOUNDARY_IMAGE
static_assert(PioneerQ38ReadSourceBoundaryProfile==1,"late read-source boundary selection");
#else
static_assert(PioneerQ38ReadSourceBoundaryProfile==0,"late read-source boundary removal");
#endif
