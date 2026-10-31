/* SPDX-License-Identifier: MIT */
// Explicit fault-ROM REVISION of tag75,not an old-tag/default/NODRV promotion.
// Numeric tag75 alone cannot identify this image;deployment must pin its SHA.
#if !defined(PIONEER_Q38_TOKEN_FAULT_VALIDATION_NATIVE) || PIONEER_Q38_TOKEN_FAULT_VALIDATION_NATIVE != 1 \
 || !defined(PIONEER_Q38_TOKEN_FAULT_PROGRAMS) || PIONEER_Q38_TOKEN_FAULT_PROGRAMS != 1 \
 || !defined(PIONEER_Q38_TOKEN_HIDDEN_VALIDATION_NATIVE) || PIONEER_Q38_TOKEN_HIDDEN_VALIDATION_NATIVE != 1 \
 || !defined(PIONEER_Q38_TOKEN_NATIVE_CONTEXT) || PIONEER_Q38_TOKEN_NATIVE_CONTEXT != 1
#error Exact separate physical fault-ROM revision and native parents required
#endif
#include "PioneerQ38TokenHiddenValidationRole.h"
// The inherited role requires ALL23 physical parents,conventional ABI and
// exact tag75;it rejects compile-only,private hidden,CPU,Work and TEST aliases.
#ifndef PIONEER_Q38_TOKEN_FAULT_NATIVE_ROLE_KIND_H
#define PIONEER_Q38_TOKEN_FAULT_NATIVE_ROLE_KIND_H
enum {PioneerQ38TokenFaultNativePhysicalSelection=1};
#endif
static_assert(PioneerQ38TokenFaultNativePhysicalSelection==1,"late physical fault-ROM role change");
