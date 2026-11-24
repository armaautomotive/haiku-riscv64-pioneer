/* SPDX-License-Identifier: MIT */
// Non-loadable witness. The caller must hold sLock for this object's entire
// lifetime. This checks the retained host session only; it cannot establish
// exclusion of firmware or other device-side writers and grants no MMIO.
#if !defined(PIONEER_QWEN35_RECOVERY_COMPILE_ONLY) \
    || !defined(PIONEER_QWEN35_SNAPSHOT_TAKEOVER_WITNESS) \
    || !defined(PIONEER_QWEN35_RECOVERY_RUN_ENABLE)
#error Q35 fabric host scope is restricted to the non-loadable snapshot witness
#endif
class PioneerQ35FabricHostScope : public PioneerQ35FabricArcCompileScope {
    PioneerProbeCookie* const fInspector;
    PioneerProbeCookie* const fOwner;
    PioneerStoreContext* const fContext;
    PioneerPostResetMapping* const fMapping;
    pioneer_driver_recovery_guard_v2_info* const fGuard;
    const uint64 fTicket, fGeneration;
    const thread_id fThread;
    const team_id fTeam;
public:
    PioneerQ35FabricHostScope(PioneerProbeCookie* inspector,PioneerProbeCookie* owner,
        PioneerStoreContext* context,pioneer_driver_recovery_guard_v2_info* guard,
        uint64 ticket,thread_id thread,team_id team)
        : fInspector(inspector),fOwner(owner),fContext(context),
          fMapping(context?context->recoveryMapping:NULL),fGuard(guard),
          fTicket(ticket),fGeneration(sReloadGeneration),fThread(thread),fTeam(team) {}
    PioneerQ35FabricHostScope(const PioneerQ35FabricHostScope&)=delete;
    PioneerQ35FabricHostScope& operator=(const PioneerQ35FabricHostScope&)=delete;

    // Metadata and ticket checks only. They must not touch the shared TLB.
    bool HostOwnershipValid() const {
        if (!fInspector || !fOwner || !fContext || !fMapping || !fGuard
            || fGuard->version!=2 || !fGuard->validate || !fTicket || !fGeneration
            || fThread<0 || fTeam<0 || find_thread(NULL)!=fThread
            || fInspector->magic!=0x50313030 || fInspector->team!=fTeam
            || fOwner->magic!=0x50313030 || fOwner->context!=fContext
            || sRuntimeOwner!=fOwner || sQwen35ExclusiveCookie!=fInspector
            || sOpenCookies!=1 || sReloadGeneration!=fGeneration || sReloadQuiesced
            || !sColdRecoveryRequired || !sBankStorageOnly || !fContext->bankStorageOnly
            || fContext->bytes!=PioneerResidentBanks::Total || !fContext->dispatchOwnerRetained
            || !fContext->q35PciReservationHeld || fContext->q35PciReleaseAttempted
            || fContext->recoveryMapping!=fMapping) return false;
        if (fInspector==fOwner) {
            if (fOwner->finalFreeRetained) return false;
        } else if (fInspector->context || !fOwner->finalFreeRetained) return false;
        thread_info info={};
        return get_thread_info(fThread,&info)==B_OK && info.team==fTeam
            && fGuard->validate(fTicket)==B_OK;
    }

    // Host ownership is necessary but cannot establish that firmware has
    // stopped issuing writes. Do not admit the fabric transaction yet.
    bool FabricRecoveryAdmission(PioneerStoreContext& context,
        pioneer_driver_recovery_guard_v2_info* guard,uint64 ticket) const {
        if (&context!=fContext || guard!=fGuard || ticket!=fTicket || !HostOwnershipValid()) return false;
        return false;
    }
};
