/* SPDX-License-Identifier: MIT */
// Type-check reset scratch restoration against the real Haiku session IO.
// This synthetic scope grants no physical authority; the driver rejects load.
#include "PioneerQwen35ProjectionResetRecovery.h"
struct PioneerQ35ResetScratchCompileScope {
    bool permitted;
    bool RecoveryAdmissionAllowed(PioneerQ35ProjectionScratch&,PioneerSessionIO&,
        const PioneerResidentBanks&,const tt_dram_samples_result&,PioneerResidentBanks&,const tt_dram_samples_result&,
        const tt_compute_noc_result&,const tt_host_page_result&,PioneerResidentLease&,
        uint64_t,const uint32_t*,unsigned,const uint8_t*,unsigned) const {
        return permitted;
    }
    bool RecoveryTransferAllowed(PioneerQ35ProjectionScratch&,PioneerSessionIO&,
        const PioneerResidentBanks&,PioneerResidentBanks&,const tt_compute_noc_result&,const tt_host_page_result&,
        PioneerResidentLease&,uint64_t,const uint32_t*,unsigned,unsigned,bool) const {
        return permitted;
    }
    bool RecoveryCompleteAllowed(PioneerQ35ProjectionScratch&,PioneerSessionIO&,
        const PioneerResidentBanks&,PioneerResidentBanks&,const tt_compute_noc_result&,const tt_host_page_result&,
        PioneerResidentLease&,uint64_t,const uint32_t*) const { return permitted; }
    void RetainRecovery(PioneerQ35ProjectionScratch&) { permitted=false; }
};
static __attribute__((used,noinline)) bool
PioneerQ35ResetScratchCompileWitness(PioneerQ35ProjectionResetRecovery* recovery,
    PioneerQ35ProjectionScratch* scratch,uint64_t ticket,const PioneerDriverRecoveryProof& proof,
    PioneerSessionIO& io,PioneerResidentBanks& oldBanks,const tt_dram_samples_result& oldDram,
    PioneerResidentBanks& newBanks,const tt_dram_samples_result& newDram,const tt_compute_noc_result& noc,
    const tt_host_page_result& host,PioneerResidentLease& lease,const uint8_t* initialized,unsigned pages,bool allowed)
{
    if (!recovery || !scratch) return false;
    PioneerQ35ResetScratchCompileScope scope{allowed};
    return recovery->Restore(*scratch,ticket,proof,io,scope,oldBanks,oldDram,newBanks,newDram,
        noc,host,lease,initialized,pages);
}
