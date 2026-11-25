/* SPDX-License-Identifier: MIT */
// Type-check reset scratch restoration against the real Haiku session IO.
// This synthetic scope grants no physical authority; the driver rejects load.
#include "PioneerQwen35ProjectionResetRecovery.h"
// Type-check the fresh session against the actual Haiku IO and guard. The
// synthetic scope is never registered as an ioctl or a hardware authority.
struct PioneerQ35FreshRecoveryCompileScope {
    bool allowed;
    bool RecoveryTransactionAdmission(PioneerProbeCookie*,PioneerStoreContext&,
        pioneer_driver_recovery_guard_v2_info*) const { return allowed; }
#ifdef PIONEER_QWEN35_FABRIC_GUARD_WITNESS
    bool FabricRoleSequenceAdmission(PioneerQ35FreshRecoverySession&,PioneerStoreContext&,
        PioneerStoreContext&,uint64) const { return allowed; }
    bool FabricRoleSequenceAccessAllowed(PioneerQ35FreshRecoverySession&,PioneerStoreContext&,
        PioneerStoreContext&,uint64) const { return allowed; }
    bool FabricRoleSequenceAreaProtected(PioneerQ35FreshRecoverySession&,PioneerStoreContext&,
        PioneerStoreContext&,uint64,int32_t) const { return false; }
    bool FabricRecoveryTransactionAdmission(PioneerProbeCookie*,PioneerStoreContext&,
        pioneer_driver_recovery_guard_v2_info*) const { return allowed; }
    bool FreshRoleRecoveryAdmission(PioneerQ35FreshRecoverySession&,PioneerProbeCookie*,
        PioneerStoreContext&,uint64) const { return allowed; }
    bool FreshRoleRecoveryRestored(PioneerQ35FreshRecoverySession&,PioneerStoreContext&,
        PioneerStoreContext&) const { return allowed; }
    template<class Receipt> bool FreshRoleRecoveryCloseAdmission(PioneerQ35FreshRecoverySession&,
        PioneerStoreContext&,PioneerStoreContext&,const Receipt&) const { return allowed; }
    template<class Receipt> bool FreshRoleRecoveryPostLeaseAudit(PioneerQ35FreshRecoverySession&,
        PioneerStoreContext&,PioneerStoreContext&,const Receipt&) const { return allowed; }
    template<class Receipt> bool FreshRoleOldHostDisposeAdmission(PioneerQ35FreshRecoverySession&,
        PioneerStoreContext&,PioneerStoreContext&,const Receipt&) const { return allowed; }
    template<class Receipt> bool FreshRoleOldHostAccessAllowed(PioneerQ35FreshRecoverySession&,
        PioneerStoreContext&,PioneerStoreContext&,const Receipt&) const { return allowed; }
    template<class Receipt> bool FreshRoleOldHostAreaProtected(PioneerQ35FreshRecoverySession&,
        PioneerStoreContext&,PioneerStoreContext&,const Receipt&,int32_t) const { return false; }
    template<class Receipt> bool FreshRoleOldHostDisposeComplete(PioneerQ35FreshRecoverySession&,
        PioneerStoreContext&,PioneerStoreContext&,const Receipt&) const { return allowed; }
    template<class Receipt> bool FreshRoleMappingsDisposedAdmission(PioneerQ35FreshRecoverySession&,
        PioneerStoreContext&,PioneerStoreContext&,const Receipt&) const { return allowed; }
    template<class Receipt> bool FreshRoleMappingsDisposedComplete(PioneerQ35FreshRecoverySession&,
        PioneerStoreContext&,PioneerStoreContext&,const Receipt&) const { return allowed; }
    template<class Receipt> bool FreshRoleGraphRetirementAdmission(PioneerQ35FreshRecoverySession&,
        PioneerStoreContext&,PioneerStoreContext&,const Receipt&) const { return allowed; }
    template<class Receipt> bool FreshRoleGraphRetirementComplete(PioneerQ35FreshRecoverySession&,
        PioneerStoreContext&,PioneerStoreContext&,const Receipt&) const { return allowed; }
    template<class Receipt> bool FreshRoleHostReleaseAdmission(PioneerQ35FreshRecoverySession&,
        PioneerStoreContext&,PioneerStoreContext&,const Receipt&) const { return allowed; }
    template<class Receipt> bool FreshRoleHostReleaseComplete(PioneerQ35FreshRecoverySession&,
        PioneerStoreContext&,PioneerStoreContext&,const Receipt&) const { return allowed; }
    template<class Receipt> bool FreshRoleOwnerRetirementAllowed(PioneerQ35FreshRecoverySession&,
        PioneerStoreContext&,PioneerStoreContext&,const Receipt&) const { return allowed; }
    bool FreshRecoverySetupAccessAllowed(PioneerQ35FreshRecoverySession&,PioneerStoreContext&,
        PioneerStoreContext&,unsigned) const { return allowed; }
#endif
    bool FreshRecoveryAdmission(PioneerQ35FreshRecoverySession&,PioneerProbeCookie*,
        PioneerStoreContext&,uint64) const { return allowed; }
    bool FreshRecoveryBoundary(PioneerQ35FreshRecoverySession&,PioneerStoreContext&,
        PioneerStoreContext&,unsigned) const { return allowed; }
    template<class Receipt> bool FreshRecoveryRestoreAdmission(PioneerQ35FreshRecoverySession&,
        PioneerStoreContext&,PioneerStoreContext&,const Receipt&) const { return allowed; }
    template<class Receipt> bool FreshRecoveryRestoreComplete(PioneerQ35FreshRecoverySession&,
        PioneerStoreContext&,PioneerStoreContext&,const Receipt&) const { return allowed; }
    template<class Receipt> bool FreshRecoveryCloseAdmission(PioneerQ35FreshRecoverySession&,
        PioneerStoreContext&,PioneerStoreContext&,const Receipt&) const { return allowed; }
    template<class Receipt> bool FreshRecoveryPostTransferAudit(PioneerQ35FreshRecoverySession&,
        PioneerStoreContext&,PioneerStoreContext&,const Receipt&) const { return allowed; }
#ifdef PIONEER_QWEN35_FABRIC_GUARD_WITNESS
    template<class Receipt> bool FreshRecoveryLeaseAccessAllowed(PioneerQ35FreshRecoverySession&,
        PioneerStoreContext&,PioneerStoreContext&,const Receipt&) const { return allowed; }
    template<class Receipt> bool FreshRecoveryAuditAccessAllowed(PioneerQ35FreshRecoverySession&,
        PioneerStoreContext&,PioneerStoreContext&,const Receipt&) const { return allowed; }
    template<class Receipt> bool FreshRecoveryPciAccessAllowed(PioneerQ35FreshRecoverySession&,
        PioneerStoreContext&,PioneerStoreContext&,const Receipt&) const { return allowed; }
    template<class Receipt> bool FreshRecoveryAreaProtected(PioneerQ35FreshRecoverySession&,
        PioneerStoreContext&,PioneerStoreContext&,const Receipt&,int32_t) const { return false; }
#endif
    template<class Receipt> bool FreshRecoveryPciIdle(PioneerQ35FreshRecoverySession&,
        PioneerStoreContext&,PioneerStoreContext&,const Receipt&) const { return allowed; }
    template<class Receipt> bool FreshRecoveryRetirementAdmission(PioneerQ35FreshRecoverySession&,
        PioneerStoreContext&,PioneerStoreContext&,const Receipt&) const { return allowed; }
    template<class Receipt> bool FreshRecoveryAfterOwnerRetired(PioneerQ35FreshRecoverySession&,
        PioneerStoreContext&,PioneerStoreContext&,const Receipt&) const { return allowed; }
    template<class Receipt,class Session> bool QualificationModifiedResetRetirementAllowed(
        const PioneerQ35ProjectionQualificationOwner&,const PioneerQ35ProjectionScratch&,
        const Receipt&,const Session&) const { return allowed; }
    bool RecoveryAdmissionAllowed(PioneerQ35ProjectionScratch&,PioneerSessionIO&,
        const PioneerResidentBanks&,const tt_dram_samples_result&,PioneerResidentBanks&,const tt_dram_samples_result&,
        const tt_compute_noc_result&,const tt_host_page_result&,PioneerResidentLease&,
        uint64_t,const uint32_t*,unsigned,const uint8_t*,unsigned) const { return allowed; }
    bool RecoveryTransferAllowed(PioneerQ35ProjectionScratch&,PioneerSessionIO&,
        const PioneerResidentBanks&,PioneerResidentBanks&,const tt_compute_noc_result&,const tt_host_page_result&,
        PioneerResidentLease&,uint64_t,const uint32_t*,unsigned,unsigned,bool) const { return allowed; }
#ifdef PIONEER_QWEN35_FABRIC_GUARD_WITNESS
    bool RecoveryAccessAllowed(PioneerQ35ProjectionScratch&,PioneerSessionIO&,
        const PioneerResidentBanks&,PioneerResidentBanks&,const tt_compute_noc_result&,const tt_host_page_result&,
        PioneerResidentLease&,uint64_t,const uint32_t*,unsigned,unsigned,bool) const { return allowed; }
#endif
    bool RecoveryCompleteAllowed(PioneerQ35ProjectionScratch&,PioneerSessionIO&,
        const PioneerResidentBanks&,PioneerResidentBanks&,const tt_compute_noc_result&,const tt_host_page_result&,
        PioneerResidentLease&,uint64_t,const uint32_t*) const { return allowed; }
    void RetainRecovery(PioneerQ35ProjectionScratch&) { allowed=false; }
    void RetainFreshRecovery(PioneerQ35FreshRecoverySession&) { allowed=false; }
};
static __attribute__((used,noinline)) bool
PioneerQ35FreshRecoveryCompileWitness(PioneerQ35FreshRecoverySession* session,
    PioneerProbeCookie* owner,PioneerStoreContext& old,pioneer_driver_recovery_guard_v2_info* guard,
    bool allowed,unsigned action)
{
    if (!session) return false;
    PioneerQ35FreshRecoveryCompileScope scope{allowed};
    if (action==0) return session->Prepare(owner,old,guard,scope);
    if (action==1) return session->RestoreScratch(guard,scope);
    if (action==2) return session->CloseTransport(guard,scope);
    if (action==3) return session->Retire(guard,scope);
#ifdef PIONEER_QWEN35_FABRIC_GUARD_WITNESS
    if (action==5) return session->PrepareFabric(owner,old,guard,scope);
    if (action==6) return PioneerQ35RunModifiedFabricRecoveryTransaction<PioneerQ35FreshRecoverySession>(owner,&old,guard,scope)==B_OK;
    if (action==7) return session->PrepareFabricRole(owner,old,guard,scope);
    if (action==8) return session->VerifyRoleRestoration(guard,scope);
    if (action==9) return session->CloseRoleTransport(guard,scope);
    if (action==10) return session->DisposeRoleOldHostBuffers(guard,scope);
    if (action==11) return session->PublishRoleMappingsDisposed(guard,scope);
    if (action==12) return session->RetireRoleGraph(guard,scope);
    if (action==13) return session->ReleaseRoleFreshHostPage(guard,scope);
    if (action==14) return session->RecordRoleOwnerRetirement(guard,scope);
#endif
    return action==4 && PioneerQ35RunModifiedRecoveryTransaction<PioneerQ35FreshRecoverySession>(owner,&old,guard,scope)==B_OK;
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
