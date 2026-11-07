/* SPDX-License-Identifier: MIT */
// Preprocessor/compile guard fixture; never instantiate a driver.
#define PIONEER_Q38_GDN_MUTABLE_STATE 1
#define PIONEER_BANKS_MOCK_ONLY 1
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38Qgw8KernelLayout.h"
static_assert(sizeof(PioneerQ38Qgw8KernelLayout)>131072,"kernel geometry is heap-only");
