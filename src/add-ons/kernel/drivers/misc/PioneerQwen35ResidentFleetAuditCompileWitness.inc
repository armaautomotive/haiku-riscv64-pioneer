/* SPDX-License-Identifier: MIT */
// Compile the exact PCI fleet-audit adapter against Haiku's session IO.
// Its admission pointer defaults to null and no live operation constructs it;
// the runtime field prevents optimization from removing the real IO calls.
#if !defined(PIONEER_QWEN35_RECOVERY_COMPILE_ONLY) || !defined(PIONEER_QWEN35_DEVICE_DISPATCH) \
    || !defined(PIONEER_QWEN35_SHARED_ROLE_OWNER_WITNESS) || defined(PIONEER_QWEN35_RECOVERY_QUALIFY)
#error Resident fleet audit compile witness requires the non-loadable shared-role driver
#endif
#include "PioneerQwen35ResidentFleetAuditPci.h"
struct PioneerQ35ResidentFleetAuditCompileScope {
    const volatile bool* permitted=nullptr;
    bool retained=false;
    bool Allowed() const { return permitted && *permitted && !retained; }
    bool ResidentAuditAllowed(PioneerSessionIO&,const PioneerQwen35::DataflowResidentCommand&) const { return Allowed(); }
    bool BeginResidentFleetAudit(PioneerSessionIO&,const PioneerQwen35::DataflowResidentCommand&) const { return Allowed(); }
    bool ResidentAuditReadAllowed(PioneerSessionIO&,const PioneerQwen35::DataflowResidentCommand&,unsigned) const { return Allowed(); }
    bool AuditResidentCoreRuntime(PioneerSessionIO&,const PioneerQwen35::DataflowResidentCommand&,unsigned) const { return Allowed(); }
    bool FinishResidentFleetAudit(PioneerSessionIO&,const PioneerQwen35::DataflowResidentCommand&) const { return Allowed(); }
    bool ResidentAuditRootAllowed(PioneerSessionIO&,const PioneerQwen35::DataflowResidentCommand&) const { return Allowed(); }
    void RetainResidentAudit() { retained=true; }
};
using PioneerQ35ResidentFleetAuditCompileIO=PioneerQwen35ResidentFleetAuditPci<
    PioneerSessionIO,PioneerQ35ResidentFleetAuditCompileScope>;
template class PioneerQwen35ResidentFleetAuditPci<PioneerSessionIO,PioneerQ35ResidentFleetAuditCompileScope>;
template PioneerQwen35::DataflowResidentFleetAuditResult PioneerQwen35::DataflowResidentFleetAudit::Audit<
    PioneerQ35ResidentFleetAuditCompileIO>(const PioneerQwen35::DataflowResidentCommand&,PioneerQ35ResidentFleetAuditCompileIO&);
