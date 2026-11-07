/* SPDX-License-Identifier: MIT */
// Check the canonical header alone: QGW8 inspection must never leak into a
// native/Work build even when the QGW8 receipt header is not included.
#define PIONEER_Q38_GDN_MUTABLE_STATE 1
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38CanonicalModelPlan.h"
