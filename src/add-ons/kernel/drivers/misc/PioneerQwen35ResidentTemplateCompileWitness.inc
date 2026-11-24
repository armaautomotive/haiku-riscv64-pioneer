/* SPDX-License-Identifier: MIT */
// Type-check resident template staging against the actual Haiku session IO.
// The scope is a non-loadable witness: fleet admission always denies, and no
// driver operation invokes this setup. It is not physical ownership proof.
#if !defined(PIONEER_QWEN35_RECOVERY_COMPILE_ONLY) || !defined(PIONEER_QWEN35_DEVICE_DISPATCH) \
    || !defined(PIONEER_QWEN35_SHARED_ROLE_OWNER_WITNESS) || defined(PIONEER_QWEN35_RECOVERY_QUALIFY)
#error Resident template witness requires the non-loadable shared-role driver
#endif
#include "../../../../../tools/pioneer/PioneerQwen35DataflowResidentTemplateSetup.h"
struct PioneerQ35ResidentTemplateCompileScope {
    PioneerStoreContext& context;
    const PioneerQwen35::DataflowStreamPlan* plan=nullptr;
    bool permitted=false,retained=false;
    bool ResidentTemplateOwnerAllowed(PioneerSessionIO& io,const PioneerQwen35::DataflowStreamPlan& requested,
        const PioneerQwen35::DataflowDynamicTokenContext&,unsigned slot) const {
        return permitted && &io==&context.io && plan==&requested && slot<PioneerQwen35::ActivationBuffers
            && io.qwen35DataflowContext==&context && sRuntimeOwner && sRuntimeOwner->context==&context;
    }
    bool ResidentTemplateFleetHeld(PioneerSessionIO&) const { return false; }
    bool ResidentTemplateCoreHeld(PioneerSessionIO& io,unsigned core) const {
        return permitted && &io==&context.io && io.CoreHeld(core);
    }
    bool ResidentTemplateAccessAllowed(PioneerSessionIO& io,unsigned core,bool) const {
        return permitted && &io==&context.io && core<PioneerQwen35::DataflowCoreCount;
    }
    void RetainResidentTemplates() { retained=true; }
};
template bool PioneerQwen35::DataflowResidentTemplateSetup<PioneerQ35ResidentTemplateCompileScope>::Stage<PioneerSessionIO>(PioneerSessionIO&);
template bool PioneerQwen35::DataflowResidentTemplateSetup<PioneerQ35ResidentTemplateCompileScope>::Verify<PioneerSessionIO>(PioneerSessionIO&);
