/* SPDX-License-Identifier: MIT */
// Type-check reset scratch restoration against the real Haiku session IO.
// This synthetic scope grants no physical authority; the driver rejects load.
#include "PioneerQwen35ProjectionResetRecovery.h"
// Type-check the fresh session against the actual Haiku IO and guard. The
// synthetic scope is never registered as an ioctl or a hardware authority.
struct PioneerQ35FreshRecoveryCompileScope {
    bool allowed;
    bool FreshRecoveryAdmission(PioneerQ35FreshRecoverySession&,PioneerProbeCookie*,
        PioneerStoreContext&,uint64) const { return allowed; }
    bool FreshRecoveryBoundary(PioneerQ35FreshRecoverySession&,PioneerStoreContext&,
        PioneerStoreContext&,unsigned) const { return allowed; }
    void RetainFreshRecovery(PioneerQ35FreshRecoverySession&) { allowed=false; }
};
static __attribute__((used,noinline)) bool
PioneerQ35FreshRecoveryCompileWitness(PioneerQ35FreshRecoverySession* session,
    PioneerProbeCookie* owner,PioneerStoreContext& old,pioneer_driver_recovery_guard_v2_info* guard,bool allowed)
{
    if (!session) return false;
    PioneerQ35FreshRecoveryCompileScope scope{allowed};
    return session->Prepare(owner,old,guard,scope);
}
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
