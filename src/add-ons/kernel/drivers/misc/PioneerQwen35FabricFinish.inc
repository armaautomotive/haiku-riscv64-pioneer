/* SPDX-License-Identifier: MIT */
// Final context retirement for the non-loadable fabric witness. The caller
// holds sLock and owns both guard module references. No live scope currently
// proves the required writer/DMA exclusion or calls this operation.
#if !defined(PIONEER_QWEN35_FABRIC_GUARD_WITNESS) \
    || !defined(PIONEER_QWEN35_RECOVERY_COMPILE_ONLY) || defined(PIONEER_QWEN35_RECOVERY_QUALIFY) \
    || !defined(PIONEER_QWEN35_RESIDENT_SERVICE_WITNESS) || !defined(PIONEER_QWEN35_SNAPSHOT_TAKEOVER_WITNESS)
#error Fabric finish is restricted to the non-loadable resident witness
#endif

// Admission callbacks are metadata-only and must independently prove that
// the graph, bank, firmware and DMA cannot have remaining external users.
// Graph/sink storage stays alive until the old bank context is destroyed.
// A failed or uncertain unreserve/guard completion is never replayed.
// On success the caller must discard context, owner (if distinct from the
// inspector), and host-scope references; guard module references stay owned
// by the caller. OWNERS_RELEASED records logical retirement, while the inert
// graph and bank storage remain pinned until the immediately following delete.
template<class Scope>
static status_t
PioneerQ35FabricFinishLocked(PioneerProbeCookie* inspector,PioneerProbeCookie* owner,
    PioneerStoreContext* context,PioneerQ35FabricHostScope& host,
    pioneer_driver_recovery_guard_v2_info* validation,
    pioneer_driver_fabric_recovery_guard_info* fabric,uint64 ticket,
    Scope& scope,tt_pioneer_recovery_run& result)
{
    if (!inspector || !owner || !context || !validation || !fabric || !ticket
        || validation->version!=2 || fabric->version!=1 || !validation->begin || !validation->abort
        || !validation->validate || !fabric->begin || !fabric->abort || !fabric->validate || !fabric->complete
        || validation->begin!=fabric->begin || validation->abort!=fabric->abort
        || validation->validate!=fabric->validate || !sReloadGuard || !sReloadGeneration
        || owner!=sRuntimeOwner || owner->context!=context || !host.MatchesParties(inspector,owner)
        || context->recoveryFabricFinishAttempted)
        return B_NOT_ALLOWED;
    auto* graph=context->qwen35GraphOwner;
    const PioneerDriverFabricRecoveryProof prior=context->recoveryFabricEvidence;
    auto ready=[&]() {
        return graph && context->qwen35GraphOwner==graph && !context->qwen35SharedRoleBinding
            && !context->recoveryFreshSession && !context->recoveryMapping && !context->recoveryFabricArcOwner
            && context->recoveryFabricRoleOwnerReleaseAttempted && context->recoveryFabricRoleOwnerReleased
            && context->recoveryFabricGraphRetirementAttempted && context->recoveryFabricGraphRetirementRecorded
            && context->recoveryFabricEvidenceRecorded && prior.ReadyToRetireOneOwner()
            && prior.fabric_generation==ticket && prior.firmware_generation==ticket
            && !memcmp(&prior,&context->recoveryFabricEvidence,sizeof(prior))
            && !context->qwen35QualificationOwner.Active() && graph->FabricContextRetirementReady(ticket)
            && host.OwnsAfterAuditRelease(*context,validation,ticket)
            && fabric->validate(ticket)==B_OK;
    };
    result.stage=PIONEER_RECOVERY_STAGE_OWNER_RELEASE;
    // Metadata-only scope check for a separately completed fixed projection:
    // it must bind this ticket and card, audit all 120 workers, verify their
    // numerical outputs, and verify scratch and service restoration. A reset
    // response, a held tile, or an idle NoC does not establish this proof.
    // The physical qualification and its scope do not yet exist.
    if (!ready() || !scope.FabricMatrixQualificationMatches(*inspector,*owner,*context,ticket)
        || !ready() || !scope.FabricFinishAdmission(*inspector,*owner,*context,ticket)
        || !ready() || !scope.FabricMatrixQualificationMatches(*inspector,*owner,*context,ticket)
        || !ready()) return B_DEV_NOT_READY;
    context->recoveryFabricFinishAttempted=true;
    result.stage=PIONEER_RECOVERY_STAGE_UNRESERVE;
    const status_t unreserve=Qwen35ReleasePciReservation(*context);
    if (unreserve!=B_OK) return unreserve;
    // The host scope uses a distinct post-unreserve identity check. The
    // graph stays allocated and the old bank claim remains intact here.
    auto afterUnreserve=[&]() {
        return context->qwen35GraphOwner==graph && !context->qwen35SharedRoleBinding
            && !context->recoveryFreshSession && !context->recoveryMapping && !context->recoveryFabricArcOwner
            && context->recoveryFabricRoleOwnerReleased && context->recoveryFabricGraphRetirementRecorded
            && !memcmp(&prior,&context->recoveryFabricEvidence,sizeof(prior))
            && graph->FabricContextRetirementReady(ticket,true)
            && host.OwnsAfterReservationRelease(*context,validation,ticket)
            && fabric->validate(ticket)==B_OK;
    };
    if (!afterUnreserve() || !scope.FabricMatrixQualificationMatches(*inspector,*owner,*context,ticket)
        || !afterUnreserve()
        || !scope.FabricFinishAfterUnreserve(*inspector,*owner,*context,ticket)
        || !afterUnreserve() || !scope.FabricMatrixQualificationMatches(*inspector,*owner,*context,ticket)
        || !afterUnreserve()) return B_DEV_NOT_READY;
    PioneerDriverFabricRecoveryProof complete=prior;
    complete.flags|=PIONEER_FABRIC_PROOF_OWNERS_RELEASED | PIONEER_FABRIC_PROOF_PCI_RESERVATION_RELEASED
        | PIONEER_FABRIC_PROOF_MATRIX_VERIFIED;
    complete.retained_owners=0;
    if (!complete.Complete()) return B_DEV_NOT_READY;
    result.stage=PIONEER_RECOVERY_STAGE_GUARD_COMPLETE;
    uint64 generation=0;
    const status_t completed=fabric->complete(ticket,&complete,&generation);
    if (completed!=B_OK || !generation || generation<=sReloadGeneration) {
        sColdRecoveryRequired=true;return completed!=B_OK?completed:B_DEV_NOT_READY;
    }
    sReloadGeneration=generation;
    owner->context=NULL;sRuntimeOwner=NULL;sBankStorageOnly=false;
    delete context; // graph/sink remains alive until the bank's containing context dies
    if (inspector!=owner) { owner->magic=0;delete owner; }
    if (sReloadGuard->cold()) { sColdRecoveryRequired=true;return B_DEV_NOT_READY; }
    sColdRecoveryRequired.ClearAfterVerifiedRecovery();
    result.new_generation=generation;result.stage=PIONEER_RECOVERY_STAGE_COMPLETE;
    return B_OK;
}
