/* SPDX-License-Identifier: MIT */
// Type/stack witness for the root-only audit; the scope is declaration-only.
#define PIONEER_QWEN35_TREE_SETTLEMENT 1
#include "pioneer_qwen35_resident_fleet_audit_target_probe.cpp"
#include "../PioneerQwen35DataflowResidentTreeAudit.h"
struct ResidentTreeAuditProbeIO : ResidentFleetAuditProbeIO {
    bool ResidentTreeAuditAllowed(const DataflowResidentCommand&);
};
DataflowResidentFleetAuditResult PioneerQ35ResidentTreeAuditProbe(DataflowResidentTreeAudit& audit,
    const DataflowResidentCommand& command,ResidentTreeAuditProbeIO& io) {
    return audit.Audit(command,io);
}
