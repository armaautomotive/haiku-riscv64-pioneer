/* SPDX-License-Identifier: MIT */
// Type-check the heap-owned fleet audit against the Haiku SDK; no live IO.
#include "../PioneerQwen35DataflowResidentFleetAudit.h"
using namespace PioneerQwen35;
struct ResidentFleetAuditProbeIO {
    bool ResidentAuditAllowed(const DataflowResidentCommand&);
    bool BeginResidentAudit(const DataflowResidentCommand&);
    bool FenceResidentAudit(const DataflowResidentCommand&);
    bool AuditResidentCoreRuntime(unsigned,const DataflowResidentCommand&);
    bool ReadStableResidentCore(unsigned,DataflowResidentFleetSample&);
    bool EndResidentAudit(const DataflowResidentCommand&);
    void RetainResidentAudit();
};
DataflowResidentFleetAuditResult PioneerQ35ResidentFleetAuditProbe(DataflowResidentFleetAudit& audit,
    const DataflowResidentCommand& command,ResidentFleetAuditProbeIO& io) {
    return audit.Audit(command,io);
}
