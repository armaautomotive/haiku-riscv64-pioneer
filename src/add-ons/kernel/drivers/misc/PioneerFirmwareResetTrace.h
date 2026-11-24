/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_FIRMWARE_RESET_TRACE_H
#define PIONEER_FIRMWARE_RESET_TRACE_H

// Host-side observations only: the callback must not access a pre-reset
// BAR, issue a reset, acknowledge an error, or alter the polling sequence.
enum PioneerFirmwareResetPollTrace {
    PioneerFirmwareResetBeforeFirstPoll=1,
    PioneerFirmwareResetAfterFirstVendor=2,
    PioneerFirmwareResetAfterFirstMarker=3
};
using PioneerFirmwareResetTrace = void (*)(void*,PioneerFirmwareResetPollTrace);
#endif
