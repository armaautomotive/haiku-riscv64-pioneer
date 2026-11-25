/* SPDX-License-Identifier: MIT */
// One-shot retained-session controller for the non-loadable snapshot witness.
// The caller owns sLock and an active v2 guard ticket. Success of the fabric
// sequence is not complete recovery: all old owners and the cold latch remain.
#if !defined(PIONEER_QWEN35_RECOVERY_COMPILE_ONLY) || !defined(PIONEER_QWEN35_DEVICE_DISPATCH) \
    || !defined(PIONEER_QWEN35_SNAPSHOT_TAKEOVER_WITNESS) || !defined(PIONEER_QWEN35_RECOVERY_RUN_ENABLE) \
    || defined(PIONEER_QWEN35_RECOVERY_QUALIFY)
#error Fabric recovery start is restricted to the non-loadable snapshot witness
#endif
#include "PioneerRecoveryProtocol.h"
#ifdef PIONEER_QWEN35_FABRIC_GUARD_WITNESS
// Implemented by the real role binding after its class definition. These
// inspect metadata or dispose only CPU mappings; snapshots stay retained.
static bool PioneerQ35FabricRoleViewsLayoutValid(PioneerStoreContext&);
static bool PioneerQ35FabricRoleViewsReferenceArea(PioneerStoreContext&,int32_t);
static bool PioneerQ35FabricDisposeRoleViews(PioneerStoreContext&,PioneerQ35FabricHostScope&,
    pioneer_driver_recovery_guard_v2_info*,uint64);
#endif
static status_t
Qwen35FabricRecoveryStartLocked(PioneerProbeCookie* inspector,PioneerProbeCookie* owner,
    PioneerStoreContext* context,pioneer_driver_recovery_guard_v2_info* guard,uint64 ticket,
    thread_id thread,team_id team,tt_pioneer_recovery_run& result)
{
    if (!inspector || !owner || !context || !guard || guard->version!=2 || !guard->validate
        || !ticket || thread<0 || team<0 || find_thread(NULL)!=thread
        || inspector->magic!=0x50313030 || inspector->team!=team || owner->magic!=0x50313030
        || sQwen35ExclusiveCookie!=inspector || sOpenCookies!=1 || sReloadQuiesced
        || !sBankStorageOnly || !context->bankStorageOnly || context->bytes!=PioneerResidentBanks::Total
        || (inspector==owner?owner->finalFreeRetained:(inspector->context || !owner->finalFreeRetained))
        || context->io.qwen35BorrowedRoleScope || context->recoveryFreshSession
        || context->recoveryFabricStartAttempted || context->recoveryFabricStartTicket
        || context->recoveryFabricPciIdleAttempted || context->recoveryFabricPciIdle
        || context->recoveryFabricAuditReleaseAttempted || context->recoveryFabricAuditReleased
#ifdef PIONEER_QWEN35_FABRIC_GUARD_WITNESS
        || context->recoveryFabricPcieAuditAttempted || context->recoveryFabricPcieAudited
        || context->recoveryFabricRoleViewsDisposed
        || context->recoveryFabricRoleOwnerReleaseAttempted || context->recoveryFabricRoleOwnerReleased
        || context->recoveryFabricGraphRetirementAttempted || context->recoveryFabricGraphRetirementRecorded
        || context->recoveryFabricFinishAttempted
#endif
        || context->recoveryAttempted || context->recoveryMapping || context->recoveryFabricArcOwner
        || !sColdRecoveryRequired || !context->dispatchOwnerRetained || !context->q35PciReservationHeld
        || context->q35PciReleaseAttempted || sRuntimeOwner!=owner || owner->context!=context
        || guard->validate(ticket)!=B_OK)
        return B_NOT_ALLOWED;
    // Allocate before consuming the one-shot; allocation alone has not
    // touched the device and can be safely refused without retaining a ticket.
    context->recoveryMapping=new(std::nothrow) PioneerPostResetMapping(context->profile);
    if (!context->recoveryMapping) return B_NO_MEMORY;
    // Keep the ticket, mapping and owner reachable after any observation or
    // uncertain operation. This attempt cannot be replayed automatically.
    context->recoveryFabricStartAttempted=true;context->recoveryFabricStartTicket=ticket;
    context->recoveryAttempted=true;
    PioneerQ35FabricHostScope scope(inspector,owner,context,guard,ticket,thread,team);
    auto permit=[&]() { return scope.HostOwnershipValid() && guard->validate(ticket)==B_OK; };
    result.stage=PIONEER_RECOVERY_STAGE_DISABLE_MASTER;
    if (!permit() || !PioneerPreResetDisableMasterGuarded(context->io,permit)) return B_DEV_NOT_READY;
    result.stage=PIONEER_RECOVERY_STAGE_FABRIC_PREPARE;
    if (!permit() || !context->recoveryMapping->OpenQ35Guarded(permit)) return B_DEV_NOT_READY;
    tt_arc_scratch_result firmware{};
    if (!permit() || !context->recoveryMapping->ProbeQ35FabricFirmwareGuarded(100,firmware,permit))
        return B_DEV_NOT_READY;
    tt_arc_queue_result queue{};PioneerPostResetAuditResult fleet{};PioneerQ35PostResetL2Result l2{};
    unsigned stage=0;result.stage=PIONEER_RECOVERY_STAGE_FABRIC_SEQUENCE;
    if (!permit() || !context->RunQ35FabricRecovery(guard,ticket,firmware,queue,fleet,l2,scope,stage)) {
        result.detail=stage;return B_DEV_NOT_READY;
    }
    result.stage=PIONEER_RECOVERY_STAGE_FABRIC_PCI_IDLE;
    if (!permit()) return B_DEV_NOT_READY;
    context->recoveryFabricPciIdleAttempted=true;
    unsigned pciStage=0;
    if (!PioneerPostResetPciFinishGuarded(context->io,context->Pci(),permit,pciStage) || !permit()) {
        result.detail=pciStage;return B_DEV_NOT_READY;
    }
    context->recoveryFabricPciIdle=true;
    result.stage=PIONEER_RECOVERY_STAGE_FABRIC_OLD_MMIO;
    auto cleanupPermit=[&]() {
        return context->recoveryFabricPciIdleAttempted && context->recoveryFabricPciIdle
            && context->recoveryFabricArcOwner && context->recoveryMapping
            && permit();
    };
#ifdef PIONEER_QWEN35_FABRIC_GUARD_WITNESS
    if (!cleanupPermit() || !PioneerQ35FabricRoleViewsLayoutValid(*context) || !cleanupPermit())
        return B_DEV_NOT_READY;
    auto roleArea=[&](int32_t area) { return PioneerQ35FabricRoleViewsReferenceArea(*context,area); };
    const status_t disposed=context->io.DisposeFabricOldMmio(cleanupPermit,roleArea);
#else
    const status_t disposed=context->io.DisposeFabricOldMmio(cleanupPermit);
#endif
    if (disposed!=B_OK || !cleanupPermit()) { result.detail=uint32_t(disposed);return B_DEV_NOT_READY; }
    result.stage=PIONEER_RECOVERY_STAGE_FABRIC_AUDIT_RELEASE;
    if (!cleanupPermit() || !context->io.fabricOldMmioDisposeComplete
        || !context->recoveryMapping->q35CloseComplete
        || context->recoveryFabricArcOwner->CurrentPhase()!=PioneerQ35FabricArcCompileOwner::Retired)
        return B_DEV_NOT_READY;
    context->recoveryFabricAuditReleaseAttempted=true;
    delete context->recoveryFabricArcOwner;context->recoveryFabricArcOwner=NULL;
    if (!permit()) return B_DEV_NOT_READY;
    delete context->recoveryMapping;context->recoveryMapping=NULL;
    if (!scope.HostOwnershipValidAfterAuditRelease() || guard->validate(ticket)!=B_OK) return B_DEV_NOT_READY;
    context->recoveryFabricAuditReleased=true;
    result.detail=PIONEER_RECOVERY_DETAIL_FABRIC_AUDIT_RELEASED;
#ifdef PIONEER_QWEN35_FABRIC_GUARD_WITNESS
    // These are bounded PCIe samples only. A later fresh-session scope must
    // still prove exclusion and revalidate access for the entire transfer.
    auto afterReleasePermit=[&]() {
        return context->recoveryFabricPciIdle && context->recoveryFabricAuditReleased
            && context->io.fabricOldMmioDisposeComplete && scope.HostOwnershipValidAfterAuditRelease()
            && guard->validate(ticket)==B_OK;
    };
    const uint32_t priorFlags=PIONEER_FABRIC_PROOF_RESET_SEQUENCE | PIONEER_FABRIC_PROOF_CLOCK_RESTORED
        | PIONEER_FABRIC_PROOF_FIRMWARE_READY | PIONEER_FABRIC_PROOF_ALL_CORES_HELD
        | PIONEER_FABRIC_PROOF_NOC_IDLE | PIONEER_FABRIC_PROOF_HOST_DMA_IDLE;
    auto& evidence=context->recoveryFabricEvidence;
    if (!afterReleasePermit() || !context->recoveryFabricEvidenceRecorded || evidence.flags!=priorFlags
        || evidence.fabric_generation!=ticket || evidence.firmware_generation!=ticket) return B_DEV_NOT_READY;
    result.stage=PIONEER_RECOVERY_STAGE_FABRIC_PCIE_AUDIT;
    context->recoveryFabricPcieAuditAttempted=true;
    PioneerQ35PcieIdleAuditResult pcie{};
    if (!PioneerQ35PcieIdleAudit(context->io,context->Pci(),afterReleasePermit,pcie)
        || pcie.samples!=3 || !afterReleasePermit()) return B_DEV_NOT_READY;
    evidence.flags=priorFlags | PIONEER_FABRIC_PROOF_PCIE_HEALTHY;
    if (!evidence.ReadyForScratchRestore() || !afterReleasePermit()) {
        evidence.flags=priorFlags;return B_DEV_NOT_READY;
    }
    context->recoveryFabricPcieAudited=true;
    result.detail=PIONEER_RECOVERY_DETAIL_FABRIC_PCIE_AUDITED;
    result.stage=PIONEER_RECOVERY_STAGE_FABRIC_ROLE_VIEWS;
    if (!PioneerQ35FabricDisposeRoleViews(*context,scope,guard,ticket) || !afterReleasePermit())
        return B_DEV_NOT_READY;
    context->recoveryFabricRoleViewsDisposed=true;
    if (!afterReleasePermit()) { context->recoveryFabricRoleViewsDisposed=false;return B_DEV_NOT_READY; }
    result.detail=PIONEER_RECOVERY_DETAIL_FABRIC_ROLE_VIEWS_DISPOSED;
#endif
    return B_DEV_NOT_READY; // retained scratch/owners and cold state are not yet restored
}
