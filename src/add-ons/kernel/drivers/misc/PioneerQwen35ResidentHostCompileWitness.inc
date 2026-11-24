/* SPDX-License-Identifier: MIT */
// Compile the root-only host transaction against the actual Haiku session IO.
// This scope is a non-loadable type witness, not a firmware/fleet proof; its
// audit always denies. No driver operation calls this transaction yet.
#if !defined(PIONEER_QWEN35_RECOVERY_COMPILE_ONLY) || !defined(PIONEER_QWEN35_DEVICE_DISPATCH) \
    || !defined(PIONEER_QWEN35_SHARED_ROLE_OWNER_WITNESS) || defined(PIONEER_QWEN35_RECOVERY_QUALIFY)
#error Resident host compile witness requires the non-loadable shared-role driver
#endif
#include "PioneerQwen35ResidentHostPci.h"
struct PioneerQ35ResidentHostCompileScope {
    PioneerStoreContext& context;
    bool permitted=false,retained=false;
    bool ResidentSessionAllowed(PioneerSessionIO& io,const PioneerQwen35::DataflowResidentCommand&,unsigned) const {
        return permitted && &io==&context.io && io.qwen35DataflowContext==&context
            && sRuntimeOwner && sRuntimeOwner->context==&context && !sReloadQuiesced;
    }
    bool ResidentRootAllowed(PioneerSessionIO& io,const PioneerQwen35::DataflowResidentCommand&,unsigned) const {
        return permitted && &io==&context.io && io.runtimeCore==0;
    }
    bool AuditResidentFleet(PioneerSessionIO&,const PioneerQwen35::DataflowResidentCommand&) const { return false; }
    void RetainResident() { retained=true; }
};
template class PioneerQwen35ResidentHostPci<PioneerSessionIO,PioneerQ35ResidentHostCompileScope>;
