/* SPDX-License-Identifier: MIT */
// DATA selection only; no native source/lifetime, journal or launch grant.
// Rechecked outside the guard so late/defined-zero changes cannot alias data.
#include "PioneerQ38TokenRetainedCohortProgramsRole.h"
#ifdef PIONEER_Q38_TOKEN_RETAINED_COHORT_PROGRAMS
#error Retained-cohort DATA has no qualified shared/native selector;include its distinct DATA header only
#endif
#include "PioneerQ38TokenFrontierProgramsRole.h"
#include "PioneerQ38TokenJobBootFoldProgramsRole.h"
#include "PioneerQ38TokenPreciseBetaProgramsRole.h"
#include "PioneerQ38TokenFaultProgramsRole.h"
#ifdef PIONEER_Q38_TOKEN_PUSHED_PROGRAMS
#if PIONEER_Q38_TOKEN_PUSHED_PROGRAMS != 1 || defined(PIONEER_Q38_TOKEN_PRECISE_PROGRAMS) \
 || defined(PIONEER_Q38_TOKEN_PROGRAMS_H)
#error Exact separate QTP3 DATA cannot alias an already selected QTP1/QTP2 package
#endif
#ifdef PIONEER_Q38_TOKEN_NATIVE_CONTEXT
#include "../../src/add-ons/kernel/drivers/misc/PioneerQ38TokenNativeRole.h"
#elif defined(PIONEER_Q38_TOKEN_PUSHED_NATIVE_CONTEXT)
#error QTP3 native selector requires its actual native parent
#endif
#ifdef PIONEER_Q38_TOKEN_FRONTIER_PROGRAMS
#include "PioneerQ38TokenFrontierPrograms.h"
#elif defined(PIONEER_Q38_TOKEN_JOB_BOOT_FOLD_PROGRAMS)
#include "PioneerQ38TokenJobBootFoldPrograms.h"
#elif defined(PIONEER_Q38_TOKEN_PRECISE_BETA_PROGRAMS)
#include "PioneerQ38TokenPreciseBetaPrograms.h"
#elif defined(PIONEER_Q38_TOKEN_FAULT_PROGRAMS)
#include "PioneerQ38TokenFaultPrograms.h"
#else
#include "PioneerQ38TokenPushedPrograms.h"
#endif
#include "PioneerQ38TokenFootprint.h"
#ifndef PIONEER_Q38_TOKEN_SELECTED_PROGRAMS_H
#define PIONEER_Q38_TOKEN_SELECTED_PROGRAMS_H
#ifdef PIONEER_Q38_TOKEN_FRONTIER_PROGRAMS
namespace PioneerQ38TokenPrograms=PioneerQ38TokenFrontierPrograms;
#elif defined(PIONEER_Q38_TOKEN_JOB_BOOT_FOLD_PROGRAMS)
namespace PioneerQ38TokenPrograms=PioneerQ38TokenJobBootFoldPrograms;
#elif defined(PIONEER_Q38_TOKEN_PRECISE_BETA_PROGRAMS)
namespace PioneerQ38TokenPrograms=PioneerQ38TokenPreciseBetaPrograms;
#elif defined(PIONEER_Q38_TOKEN_FAULT_PROGRAMS)
namespace PioneerQ38TokenPrograms=PioneerQ38TokenFaultPrograms;
#else
namespace PioneerQ38TokenPrograms=PioneerQ38TokenPushedPrograms;
#endif
#endif
static_assert(PioneerQ38TokenPrograms::Family==0x33505451,"late QTP3 package selection");
#else
#ifdef PIONEER_Q38_TOKEN_PUSHED_NATIVE_CONTEXT
#error QTP3 native context cannot select old immutable programs
#endif
#include "PioneerQ38TokenPrograms.h"
#endif
