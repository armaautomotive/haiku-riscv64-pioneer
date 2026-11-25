/* SPDX-License-Identifier: MIT */
// Non-loadable witness. The caller must hold sLock for this object's entire
// lifetime. This scope admits the fixed one-shot recovery transaction for
// the retained host session. It does not establish exclusion of firmware or
// other device-side writers, or prove that recovery succeeded: the owner
// checks the physical routes and reset state and retains every uncertain step.
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
    bool fAdmitted=false;
    unsigned fStage=0;
    using Owner=PioneerQ35FabricArcCompileOwner;

    bool OwnerValid(const Owner& owner,const PioneerPostResetMapping& mapping,
        uint64 ticket,Owner::Phase phase) const {
        return fAdmitted && fStage>=1 && fStage<=17 && ticket==fTicket
            && &mapping==fMapping && fContext && fContext->recoveryFabricArcOwner==&owner
            && owner.Ticket()==ticket && owner.CurrentPhase()==phase
            && phase!=Owner::Failed && HostOwnershipValid();
    }
    static bool StagePhase(unsigned stage,Owner::Phase phase) {
        switch (stage) {
        case 1: case 2: return phase==Owner::Fresh;
        case 3: return phase==Owner::Fresh || phase==Owner::ClockForced;
        case 4: return phase==Owner::ClockForced;
        case 5: return phase==Owner::ClockForced || phase==Owner::RiscsHeld;
        case 6: return phase==Owner::RiscsHeld || phase==Owner::TensixToggled;
        case 7: return phase==Owner::TensixToggled || phase==Owner::NocReinitialized;
        case 8: return phase==Owner::NocReinitialized || phase==Owner::TilesHeld;
        case 9: return phase==Owner::TilesHeld || phase==Owner::RiscsReleased;
        case 10: return phase==Owner::RiscsReleased || phase==Owner::ClockReleased;
        case 11: case 12: case 13: case 14: return phase==Owner::ClockReleased;
        case 15: return phase==Owner::ClockReleased || phase==Owner::Retired;
        case 16: case 17: return phase==Owner::Retired;
        default: return false;
        }
    }
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
    bool HostOwnershipValidForMapping(bool retired) const {
        if (!fInspector || !fOwner || !fContext || !fGuard
            || fGuard->version!=2 || !fGuard->validate || !fTicket || !fGeneration
            || fThread<0 || fTeam<0 || find_thread(NULL)!=fThread
            || fInspector->magic!=0x50313030 || fInspector->team!=fTeam
            || fOwner->magic!=0x50313030 || fOwner->context!=fContext
            || sRuntimeOwner!=fOwner || sQwen35ExclusiveCookie!=fInspector
            || sOpenCookies!=1 || sReloadGeneration!=fGeneration || sReloadQuiesced
            || !sColdRecoveryRequired || !sBankStorageOnly || !fContext->bankStorageOnly
            || fContext->bytes!=PioneerResidentBanks::Total || !fContext->dispatchOwnerRetained
            || !fContext->q35PciReservationHeld || fContext->q35PciReleaseAttempted
            || (retired?(fContext->recoveryMapping || fContext->recoveryFabricArcOwner)
                :(!fMapping || fContext->recoveryMapping!=fMapping))) return false;
        if (fInspector==fOwner) {
            if (fOwner->finalFreeRetained) return false;
        } else if (fInspector->context || !fOwner->finalFreeRetained) return false;
        thread_info info={};
        return get_thread_info(fThread,&info)==B_OK && info.team==fTeam
            && fGuard->validate(fTicket)==B_OK;
    }
    bool HostOwnershipValid() const { return HostOwnershipValidForMapping(false); }
    bool HostOwnershipValidAfterAuditRelease() const { return HostOwnershipValidForMapping(true); }

    // This is an operational admission for the fixed reset sequence, not a
    // lifetime guarantee about firmware. The caller must have already opened
    // and probed this mapping under the same retained PCI reservation.
    bool FabricRecoveryAdmission(PioneerStoreContext& context,
        pioneer_driver_recovery_guard_v2_info* guard,uint64 ticket) {
        if (fAdmitted || fStage || &context!=fContext || guard!=fGuard || ticket!=fTicket
            || !HostOwnershipValid() || context.recoveryFabricArcOwner || !fMapping->ready
            || !fMapping->firmwareReady || !fMapping->topologyReady || !fMapping->savedValid
            || fMapping->queueAttempted || fMapping->dmaModified || !HostOwnershipValid()) return false;
        fAdmitted=true;return true;
    }
    bool FabricRecoveryStageAllowed(PioneerStoreContext& context,const Owner& owner,
        pioneer_driver_recovery_guard_v2_info* guard,uint64 ticket,unsigned stage) {
        if (!fAdmitted || &context!=fContext || guard!=fGuard || ticket!=fTicket
            || stage<1 || stage>17 || (stage!=fStage && stage!=fStage+1)
            || !StagePhase(stage,owner.CurrentPhase())) return false;
        // Check against the proposed stage without making a rejected stage
        // current. OwnerValid requires an attached, exact retained owner.
        if (context.recoveryFabricArcOwner!=&owner || owner.Ticket()!=ticket
            || !HostOwnershipValid()) return false;
        fStage=stage;return true;
    }
    bool FabricArcAllowed(const Owner& owner,const PioneerPostResetMapping& mapping,
        uint64 ticket,Owner::Phase phase) const {
        return OwnerValid(owner,mapping,ticket,phase) && StagePhase(fStage,phase);
    }
    bool FabricArcPhaseReady(const Owner& owner,const PioneerPostResetMapping& mapping,
        uint64 ticket,Owner::Phase phase) const {
        return FabricArcAllowed(owner,mapping,ticket,phase)
            && ((fStage==3 && phase==Owner::Fresh) || (fStage==6 && phase==Owner::RiscsHeld)
                || (fStage==7 && phase==Owner::TensixToggled) || (fStage==10 && phase==Owner::RiscsReleased));
    }
    bool FabricClockQueryAllowed(const Owner& owner,const PioneerPostResetMapping& mapping,
        uint64 ticket,Owner::Phase phase) const {
        return FabricArcAllowed(owner,mapping,ticket,phase)
            && ((fStage==2 && phase==Owner::Fresh) || (fStage==4 && phase==Owner::ClockForced)
                || (fStage==11 && phase==Owner::ClockReleased));
    }
    bool FabricResetUnitAllowed(const Owner& owner,const PioneerPostResetMapping& mapping,
        uint64 ticket,Owner::Phase phase,bool hold) const {
        return FabricArcAllowed(owner,mapping,ticket,phase)
            && (hold?(fStage==5 && phase==Owner::ClockForced):(fStage==9 && phase==Owner::TilesHeld));
    }
    bool FabricTileCoverage(const Owner& owner,const PioneerPostResetMapping& mapping,uint64 ticket) const {
        return fStage==8 && FabricArcAllowed(owner,mapping,ticket,Owner::NocReinitialized);
    }
    bool FabricTileResetAllowed(const Owner& owner,const PioneerPostResetMapping& mapping,
        uint64 ticket,unsigned core) const {
        return core<120 && FabricTileCoverage(owner,mapping,ticket);
    }
    bool FabricRetirementAllowed(const Owner& owner,const PioneerPostResetMapping& mapping,uint64 ticket) const {
        return fStage==15 && FabricArcAllowed(owner,mapping,ticket,Owner::ClockReleased)
            && mapping.fleetAudited && mapping.dmaAudited && owner.ClockRestored();
    }
};
