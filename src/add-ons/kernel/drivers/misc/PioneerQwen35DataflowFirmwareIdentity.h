/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_QWEN35_DATAFLOW_FIRMWARE_IDENTITY_H
#define PIONEER_QWEN35_DATAFLOW_FIRMWARE_IDENTITY_H
#include "BlackholeArcTelemetryProbe.h"

// The existing session baseline is a whitelisted health telemetry snapshot.
// Limit the experimental role-status path to the family observed on this
// card. These reported version numbers are not a firmware hash, proof of
// source equivalence, or proof that firmware cannot issue later traffic.
inline bool PioneerQwen35ReportedFirmwareMatches(const tt_arc_telemetry_result& t)
{
    return t.version==1 && t.state==TT_ARC_TELEMETRY_OK && t.tlb_restored==1
        && (t.present_mask&3U)==3U && t.values[0]==0x12050001U
        && t.values[1]==0x000e0000U;
}
#endif
