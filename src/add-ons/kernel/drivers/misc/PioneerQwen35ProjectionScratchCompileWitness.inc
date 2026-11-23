/* SPDX-License-Identifier: MIT */
// Type-check the private scratch transaction with the real Haiku session IO.
// This witness grants no card authority and is present only in a driver whose
// initialization deliberately rejects loading.
#if !defined(PIONEER_QWEN35_RECOVERY_COMPILE_ONLY) || !defined(PIONEER_QWEN35_DEVICE_DISPATCH)
#error Projection scratch compile witness requires the non-loadable recovery driver
#endif
#include "PioneerQwen35ProjectionScratch.h"

class PioneerQ35ProjectionScratchCompileScope {
public:
    bool permitted=false;
    bool ScratchReserveAllowed(const void*,unsigned,unsigned,unsigned) const { return permitted; }
    bool ScratchTransferAllowed(const void*,unsigned,unsigned,bool) const { return permitted; }
    bool ScratchStageAllowed(const void*) const { return permitted; }
    bool ScratchRunAllowed(const void*) const { return permitted; }
    bool ScratchStopped(const void*) const { return permitted; }
    bool ScratchOutputAllowed(const void*) const { return permitted; }
    bool ScratchRestoreAllowed(const void*) const { return permitted; }
    void RetainScratch(const void*) { permitted=false; }
};

static __attribute__((used,noinline)) bool
PioneerQ35ProjectionScratchCompileWitness(PioneerStoreContext& context,
    PioneerQ35ProjectionScratch& scratch,PioneerQ35ProjectionScratchCompileScope& scope,
    uint32_t* saved,unsigned words,unsigned action)
{
    auto& dram=context.baseline.baseline.baseline.baseline.dram;
    auto& noc=context.baseline.baseline.baseline.noc;
    if (action==0) return scratch.Capture(context.io,scope,dram,noc,context.host,context.lease,
        context.banks,0,0,context.initialized,PioneerResidentBanks::Pages,saved,words);
    if (action==1) return scratch.Stage(context.io,scope,dram,noc,context.host,context.lease,context.banks);
    if (action==2) return scratch.MarkRunning(scope);
    if (action==3) return scratch.MarkStopped(scope);
    if (action==4) return scratch.VerifyOutput(context.io,scope,dram,noc,context.host,context.lease,context.banks);
    if (action==5) return scratch.Restore(context.io,scope,dram,noc,context.host,context.lease,context.banks);
    return false;
}
