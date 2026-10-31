/* SPDX-License-Identifier: MIT */
// Separate diagnostic revision. Tag75 is ABI,NOT firmware/module identity.
#if !defined(PIONEER_Q38_TOKEN_JOB_BOOT_FOLD_VALIDATION_NATIVE) || PIONEER_Q38_TOKEN_JOB_BOOT_FOLD_VALIDATION_NATIVE != 1 \
 || !defined(PIONEER_Q38_TOKEN_JOB_BOOT_FOLD_PROGRAMS) || PIONEER_Q38_TOKEN_JOB_BOOT_FOLD_PROGRAMS != 1 \
 || !defined(PIONEER_Q38_TOKEN_PRECISE_BETA_VALIDATION_NATIVE) || PIONEER_Q38_TOKEN_PRECISE_BETA_VALIDATION_NATIVE != 1 \
 || defined(PIONEER_Q38_TOKEN_JOB_BOOT_FOLD_NATIVE_CONTEXT)
#error Exact separate physical job boot fold code revision required,not private NODRV
#endif
#include "PioneerQ38TokenPreciseBetaValidationRole.h"
// Inherit every old physical parent,conventional ABI,tag75,posted cancel,
// and NODRV/CPU/Work/TEST refusal. Exact SHA deployment remains separate.
#ifndef PIONEER_Q38_TOKEN_JOB_BOOT_FOLD_PHYSICAL_KIND_H
#define PIONEER_Q38_TOKEN_JOB_BOOT_FOLD_PHYSICAL_KIND_H
enum {PioneerQ38TokenJobBootFoldNativePhysicalSelection=1};
#endif
static_assert(PioneerQ38TokenJobBootFoldNativePhysicalSelection==1,"late physical job boot fold role change");
