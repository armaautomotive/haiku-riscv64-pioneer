/* SPDX-License-Identifier: MIT */
// Host-only CLI syscall tests. No driver/device implementation; the sole ABI
// constant is from headers/os/drivers/Drivers.h. NEVER use for native builds.
#ifndef PIONEER_CLI_TEST_DRIVERS_H
#define PIONEER_CLI_TEST_DRIVERS_H
enum { B_DEVICE_OP_CODES_END=9999 };
#endif
