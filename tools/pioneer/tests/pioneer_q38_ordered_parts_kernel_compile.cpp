/* SPDX-License-Identifier: MIT */
// Compile/stack witness only;no IO implementation or linked driver.
#include <stddef.h>
#define B_DEVICE_OP_CODES_END 1000
#define PIONEER_Q38_ORDERED_EXTENT_FAST
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38DispatchProtocol.h"
extern "C" bool pioneer_q38_ordered_dispatch_shape(const tt_q38_dispatch_probe& request) {
	return PioneerQ38DispatchValid(request);
}
