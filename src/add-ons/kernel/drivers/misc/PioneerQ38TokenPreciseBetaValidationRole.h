/* SPDX-License-Identifier: MIT */
// Separate diagnostic code revision,not an old physical tag75 or Work promotion.
// A numeric tag is not byte identity: deployment must bind the new module SHA.
#if !defined(PIONEER_Q38_TOKEN_PRECISE_BETA_VALIDATION_NATIVE) || PIONEER_Q38_TOKEN_PRECISE_BETA_VALIDATION_NATIVE != 1 \
 || !defined(PIONEER_Q38_TOKEN_PRECISE_BETA_PROGRAMS) || PIONEER_Q38_TOKEN_PRECISE_BETA_PROGRAMS != 1 \
 || !defined(PIONEER_Q38_TOKEN_POSTED_CANCEL) || PIONEER_Q38_TOKEN_POSTED_CANCEL != 1
#error Exact separate precise-beta diagnostic and posted-cancel selection required
#endif
#include "PioneerQ38TokenFaultValidationRole.h"
// Inherit ALL25 exact fault-ROM/physical parents,tag75,conventional ABI,
// and every NODRV,CPU-reference,Work,TEST and fast-math refusal unchanged.
#ifndef PIONEER_Q38_TOKEN_PRECISE_BETA_NATIVE_ROLE_KIND_H
#define PIONEER_Q38_TOKEN_PRECISE_BETA_NATIVE_ROLE_KIND_H
enum {PioneerQ38TokenPreciseBetaNativePhysicalSelection=1};
#endif
static_assert(PioneerQ38TokenPreciseBetaNativePhysicalSelection==1,"late precise-beta physical role change");
