/* SPDX-License-Identifier: MIT */
// Rechecked even on a repeated/late include. Private QT3NODRV and the
// DISTINCT tag75 diagnostic are separate roles;neither promotes tag74/Work.
#ifndef PIONEER_Q38_TOKEN_HIDDEN_READ_SELECTION_H
#define PIONEER_Q38_TOKEN_HIDDEN_READ_SELECTION_H
#ifdef PIONEER_Q38_TOKEN_HIDDEN_VALIDATION_NATIVE
enum {PioneerQ38TokenHiddenReadSelection=2};
#elif defined(PIONEER_Q38_TOKEN_HELD_HIDDEN_DIAGNOSTIC)
enum {PioneerQ38TokenHiddenReadSelection=1};
#else
enum {PioneerQ38TokenHiddenReadSelection=0};
#endif
#endif
#ifdef PIONEER_Q38_TOKEN_HIDDEN_VALIDATION_NATIVE
static_assert(PioneerQ38TokenHiddenReadSelection==2,"late physical hidden diagnostic selection");
#include "PioneerQ38TokenHiddenValidationRole.h"
#elif defined(PIONEER_Q38_TOKEN_HELD_HIDDEN_DIAGNOSTIC)
static_assert(PioneerQ38TokenHiddenReadSelection==1,"late hidden diagnostic selection");
#if PIONEER_Q38_TOKEN_HELD_HIDDEN_DIAGNOSTIC != 1 \
 || !defined(PIONEER_Q38_TOKEN_CONTEXT_COMPILE_ONLY) || PIONEER_Q38_TOKEN_CONTEXT_COMPILE_ONLY != 1 \
 || !defined(PIONEER_Q38_TOKEN_PUSHED_NATIVE_CONTEXT) || PIONEER_Q38_TOKEN_PUSHED_NATIVE_CONTEXT != 1 \
 || !defined(PIONEER_Q38_TOKEN_PUSHED_PROGRAMS) || PIONEER_Q38_TOKEN_PUSHED_PROGRAMS != 1 \
 || !defined(PIONEER_DRIVER_BUILD_TAG) || PIONEER_DRIVER_BUILD_TAG != 0x5154334e4f445256ULL \
 || defined(PIONEER_Q38_TOKEN_VALIDATION_NATIVE) || defined(PIONEER_Q38_TOKEN_VALIDATION_CLIENT) \
 || defined(PIONEER_Q38_TOKEN_HIDDEN_VALIDATION_CLIENT) || defined(PIONEER_Q38_TOKEN_BOUNDARY_VALIDATION_CLIENT)
#error Held hidden readout requires distinct private QT3NODRV diagnostic selection;NO physical/Work grant
#endif
#include "PioneerQ38TokenNativeRole.h"
#else
static_assert(PioneerQ38TokenHiddenReadSelection==0,"late hidden diagnostic removal");
#endif
