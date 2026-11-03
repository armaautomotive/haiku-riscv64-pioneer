/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_MANUAL_WATCHDOG_BRIDGE_H
#define PIONEER_Q38_MANUAL_WATCHDOG_BRIDGE_H
#include <cstdint>
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38TokenFrontierPacket.h"
struct Q38ManualWatchdogCallbacks {
 void* context;
 bool (*frontier)(void*,PioneerQ38TokenFrontierControl::Packet&);
 bool (*poll)(void*);
 bool (*terminal)(void*);
 void (*pause)(void*);
 void (*close)(void*);
}; // HOST TEST callbacks into actual native controls;no guest/owner/product inputs
struct Q38ManualWatchdogResult {unsigned reason,observations,finalCount,finishes,closes;};
Q38ManualWatchdogResult Q38RunManualWatchdog(const Q38ManualWatchdogCallbacks&,unsigned);
#endif
