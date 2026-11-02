/* SPDX-License-Identifier: MIT */
#include "PioneerQ38TokenConstantQ4DataBoundary.h"
#ifndef PIONEER_Q38_TOKEN_CONSTANT_Q4_PARENT_BINDING_H
#define PIONEER_Q38_TOKEN_CONSTANT_Q4_PARENT_BINDING_H
#ifdef PIONEER_Q38_TOKEN_CONSTANT_Q4_PROGRAMS
enum {PioneerQ38TokenConstantQ4ParentBinding=1};
#else
enum {PioneerQ38TokenConstantQ4ParentBinding=0};
#endif
#endif
#ifdef PIONEER_Q38_TOKEN_CONSTANT_Q4_PROGRAMS
static_assert(PioneerQ38TokenConstantQ4ParentBinding==1,"late constant Q4 parent binding");
#if defined(PIONEER_Q38_TOKEN_CONSTANT_Q4_NATIVE_CONTEXT) || defined(PIONEER_Q38_TOKEN_CONSTANT_Q4_BENCHMARK_NATIVE)
#include "PioneerQ38TokenPhaseProfileProgramsRole.h"
#include "../../src/add-ons/kernel/drivers/misc/PioneerQ38TokenConstantQ4Role.h"
#endif
#else
static_assert(PioneerQ38TokenConstantQ4ParentBinding==0,"late constant Q4 parent removal");
#endif
