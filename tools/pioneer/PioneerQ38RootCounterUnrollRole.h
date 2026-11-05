/* SPDX-License-Identifier: MIT */
// Inspection-only prepared ROOT revision. Same ordered volatile checks;
// no skipped counter/source/quiet check or alternate completion policy.
#include "PioneerQ38PreparedQ4RootRole.h"
#if !defined(PIONEER_Q38_ROOT_COUNTER_UNROLL_IMAGE) || PIONEER_Q38_ROOT_COUNTER_UNROLL_IMAGE != 1
#error Root counter unroll requires its exact distinct prepared ROOT image
#endif
