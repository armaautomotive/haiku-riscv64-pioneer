/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_QWEN35_DATAFLOW_FLEET_INSPECTION_H
#define PIONEER_QWEN35_DATAFLOW_FLEET_INSPECTION_H

#include "PioneerQwen35DataflowL2CpuResetProbe.h"
#include "PioneerQwen35DataflowHostDmaSample.h"

// Fixed, bounded observations used before staging the resident role images.
// This is not a firmware-exclusion or reset-continuity proof: later firmware
// commands, NoC traffic and resets remain possible. A caller must retain the
// device if restoration of a temporary route cannot be verified.
struct PioneerQwen35FleetInspectionResult {
    enum Stage { Admission, ArcReset, HeldFleet, HostDma, Complete };
    Stage stage=Admission;
    unsigned failedCore=PioneerQwen35::DataflowBarrierWorkers;
    uint32_t l2cpuReset=UINT32_MAX;
    bool restoreFailed=false;
};

template<class IO,class Scope,class Guard>
bool PioneerQwen35InspectHeldFleet(IO& io,Scope& scope,Guard& route,
    PioneerQwen35FleetInspectionResult& result)
{
    result=PioneerQwen35FleetInspectionResult{};
    if (!scope.ProveRoleStatusInspectionAllowed(io)) return false;
    result.stage=PioneerQwen35FleetInspectionResult::ArcReset;
    if (!PioneerQwen35SampleL2CpuReset(io,scope,route,result.l2cpuReset,result.restoreFailed)
        || !PioneerQwen35L2CpuRiscsHeld(result.l2cpuReset)) return false;
    result.stage=PioneerQwen35FleetInspectionResult::HeldFleet;
    if (!PioneerQwen35SampleHeldFleet(io,scope,route,result.failedCore,result.restoreFailed)) return false;
    result.stage=PioneerQwen35FleetInspectionResult::HostDma;
    if (!PioneerQwen35SampleHostDma(io,scope,route,result.restoreFailed)) return false;
    result.stage=PioneerQwen35FleetInspectionResult::Complete;
    return true;
}
#endif
