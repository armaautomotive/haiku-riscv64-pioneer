/* SPDX-License-Identifier: MIT */
// This translation unit MUST fail: multi-token simulation cannot be selected
// after the protocol was already bound to the original one-token role.
#define PIONEER_Q38_QGW8_FULL_SESSION_INSPECTION 1
#define PIONEER_Q38_QGW8_USER_SESSION_INSPECTION 1
#define PIONEER_Q38_QGWD_UPLOAD_SESSION_INSPECTION 1
#define PIONEER_Q38_QGWD_FULL_SESSION_INSPECTION 1
#define B_DEVICE_OP_CODES_END 1000
#ifdef PIONEER_Q38_TEST_MULTI_REMOVAL
#define PIONEER_Q38_QGWD_MULTI_TOKEN_SIMULATION 1
#endif
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38HybridFullProtocol.h"
#ifdef PIONEER_Q38_TEST_MULTI_REMOVAL
#undef PIONEER_Q38_QGWD_MULTI_TOKEN_SIMULATION
#else
#define PIONEER_Q38_QGWD_MULTI_TOKEN_SIMULATION 1
#endif
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38HybridFullProtocol.h"
