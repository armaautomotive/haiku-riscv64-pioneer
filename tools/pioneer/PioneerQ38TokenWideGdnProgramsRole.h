/* SPDX-License-Identifier: MIT */
#include "PioneerQ38TokenWideGdnDataBoundary.h"
#ifndef PIONEER_Q38_TOKEN_WIDE_GDN_PARENT_BINDING_H
#define PIONEER_Q38_TOKEN_WIDE_GDN_PARENT_BINDING_H
#ifdef PIONEER_Q38_TOKEN_WIDE_GDN_PROGRAMS
enum {PioneerQ38TokenWideGdnParentBinding=1};
#else
enum {PioneerQ38TokenWideGdnParentBinding=0};
#endif
#endif
#ifdef PIONEER_Q38_TOKEN_WIDE_GDN_PROGRAMS
static_assert(PioneerQ38TokenWideGdnParentBinding==1,"late wide GDN outer binding");
#if defined(PIONEER_Q38_TOKEN_WIDE_GDN_NATIVE_CONTEXT) || defined(PIONEER_Q38_TOKEN_WIDE_GDN_BENCHMARK_NATIVE)
#include "PioneerQ38TokenBatchedOutputClosureProgramsRole.h"
#include "../../src/add-ons/kernel/drivers/misc/PioneerQ38TokenWideGdnRole.h"
#endif
#else
static_assert(PioneerQ38TokenWideGdnParentBinding==0,"late wide GDN outer removal");
#endif
