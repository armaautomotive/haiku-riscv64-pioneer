/* SPDX-License-Identifier: MIT */
// This translation unit MUST fail: a full-session identity cannot be
// silently changed after the protocol was first included.
#define PIONEER_Q38_QGW8_FULL_SESSION_INSPECTION 1
#define PIONEER_Q38_QGW8_USER_SESSION_INSPECTION 1
#define PIONEER_Q38_QGWD_UPLOAD_SESSION_INSPECTION 1
#define PIONEER_Q38_QGWD_FULL_SESSION_INSPECTION 1
#define B_DEVICE_OP_CODES_END 1000
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38HybridFullProtocol.h"
#undef PIONEER_Q38_QGWD_FULL_SESSION_INSPECTION
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38HybridFullProtocol.h"
