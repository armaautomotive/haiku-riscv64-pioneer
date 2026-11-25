/* SPDX-License-Identifier: MIT */
// Included inside PioneerStoreContext in the non-loadable snapshot rewrite.
// The caller holds sLock and supplies an already-open, firmware-probed mapping,
// an active recovery ticket and an operation scope. The Haiku host scope
// checks retained ownership and stage order but does not exclude external
// firmware activity. This sequence remains non-loadable and unqualified.
// Success means only that the Tensix sequence, audits and mapping close
// completed: the context, owner, mapping object, PCI reservation and cold
// latch remain retained for scratch restoration and final recovery.
// A live operation must establish the necessary physical ownership and fault
// fencing throughout each audit and the entire recovery transaction.
#if !defined(PIONEER_QWEN35_RECOVERY_COMPILE_ONLY) || !defined(PIONEER_QWEN35_DEVICE_DISPATCH) \
    || defined(PIONEER_QWEN35_RECOVERY_QUALIFY) \
    || !defined(PIONEER_QWEN35_RECOVERY_RUN_ENABLE) || !defined(PIONEER_QWEN35_SNAPSHOT_TAKEOVER_WITNESS)
#error Q35 fabric transaction requires the non-loadable snapshot recovery witness
#endif
template<class Scope>
bool RunQ35FabricRecovery(pioneer_driver_recovery_guard_v2_info* guard,uint64 ticket,
    const tt_arc_scratch_result& firmware,tt_arc_queue_result& queue,
    PioneerPostResetAuditResult& fleet,PioneerQ35PostResetL2Result& l2,
    Scope& scope,unsigned& stage)
{
    stage=0;
    if (!guard || !guard->validate || !ticket || !recoveryMapping || recoveryFabricArcOwner
#ifdef PIONEER_QWEN35_FABRIC_GUARD_WITNESS
        || recoveryFabricEvidenceRecorded
#endif
        || !dispatchOwnerRetained || !q35PciReservationHeld || q35PciReleaseAttempted
        || !sColdRecoveryRequired || !sRuntimeOwner
        || sRuntimeOwner->context!=this || guard->validate(ticket)!=B_OK
        || !scope.FabricRecoveryAdmission(*this,guard,ticket)
        || guard->validate(ticket)!=B_OK) return false;
    using FabricOwner=PioneerQ35FabricArcOwner<pioneer_driver_recovery_guard_v2_info>;
    auto* owner=new(std::nothrow) FabricOwner(*recoveryMapping,ticket);
    if (!owner) return false;
    // Attach before Open: every partial mapping or uncertain command remains
    // reachable through the retained context, including a lost response.
    recoveryFabricArcOwner=owner;
    PioneerPostResetMapping* const mapping=recoveryMapping;
    auto valid=[&]() {
        return recoveryFabricArcOwner==owner && recoveryMapping==mapping
            && dispatchOwnerRetained && q35PciReservationHeld && !q35PciReleaseAttempted
            && sColdRecoveryRequired && sRuntimeOwner
            && sRuntimeOwner->context==this && guard->validate(ticket)==B_OK
            && scope.FabricRecoveryStageAllowed(*this,*owner,guard,ticket,stage)
            && guard->validate(ticket)==B_OK && recoveryFabricArcOwner==owner
            && recoveryMapping==mapping && dispatchOwnerRetained && q35PciReservationHeld
            && !q35PciReleaseAttempted && sColdRecoveryRequired
            && sRuntimeOwner && sRuntimeOwner->context==this;
    };
    auto step=[&](unsigned next,auto operation) {
        stage=next;return valid() && operation() && valid();
    };
    if (!step(1,[&]() { return owner->Open(guard,scope); })
        || !step(2,[&]() { return owner->CheckClock(guard,scope,firmware,queue); })
        || !step(3,[&]() { return owner->SendNext(guard,scope,firmware,queue); })
        || !step(4,[&]() { return owner->CheckClock(guard,scope,firmware,queue); })
        || !step(5,[&]() { return owner->SetRiscReset(guard,scope,true); })
        || !step(6,[&]() { return owner->SendNext(guard,scope,firmware,queue); })
        || !step(7,[&]() { return owner->SendNext(guard,scope,firmware,queue); })
        || !step(8,[&]() { return owner->HoldAllTiles(guard,scope); })
        || !step(9,[&]() { return owner->SetRiscReset(guard,scope,false); })
        || !step(10,[&]() { return owner->SendNext(guard,scope,firmware,queue); })
        || !step(11,[&]() { return owner->CheckClock(guard,scope,firmware,queue); })
        || !step(12,[&]() { return mapping->AuditQ35Fabric(fleet,valid); })
        || !step(13,[&]() { return mapping->AuditQ35L2CpuHeldGuarded(l2,valid); })
        || !step(14,[&]() { return mapping->AuditQ35HostDmaGuarded(valid)
            && mapping->RevalidateQ35FabricFirmware(firmware,valid)
            && mapping->AuditQ35RootAerGuarded(valid); })
        || !step(15,[&]() { return owner->Retire(guard,scope); })
        || !step(16,[&]() { return mapping->CloseQ35Guarded(valid); })) return false;
    stage=17;
    if (!valid() || owner->CurrentPhase()!=FabricOwner::Retired || !mapping->q35CloseComplete) return false;
#ifdef PIONEER_QWEN35_FABRIC_GUARD_WITNESS
    // Record only facts established by this exact completed transaction.
    // PCIe health, old-mapping disposal, reservation release and owner
    // retirement need separate evidence and remain unset.
    if (!owner->ClockRestored() || owner->ResetWrites()!=0xffU
        || owner->TileWrites(0)!=UINT32_MAX || owner->TileWrites(1)!=UINT32_MAX
        || owner->TileWrites(2)!=UINT32_MAX || owner->TileWrites(3)!=0x00ffffffU
        || !mapping->fleetAudited || !mapping->dmaAudited || !mapping->q35FirmwareRevalidated
        || !mapping->q35RootAerAudited || mapping->queueUnsafe
        || fleet.failure!=PioneerPostResetAuditNone || fleet.pass!=2
        || fleet.audited!=2*PIONEER_POST_RESET_CORES || fleet.core!=PIONEER_POST_RESET_CORES
        || fleet.noc!=PIONEER_POST_RESET_NOCS || fleet.index
        || !l2.sampled || l2.restoreFailed || (l2.reset&0xf0U) || !valid()) return false;
    PioneerDriverFabricRecoveryProof evidence{};
    evidence.flags=PIONEER_FABRIC_PROOF_RESET_SEQUENCE | PIONEER_FABRIC_PROOF_CLOCK_RESTORED | PIONEER_FABRIC_PROOF_FIRMWARE_READY
        | PIONEER_FABRIC_PROOF_ALL_CORES_HELD | PIONEER_FABRIC_PROOF_NOC_IDLE
        | PIONEER_FABRIC_PROOF_HOST_DMA_IDLE;
    evidence.core_count=PIONEER_POST_RESET_CORES;evidence.retained_owners=1;
    evidence.fabric_generation=ticket;evidence.firmware_generation=ticket;
    recoveryFabricEvidence=evidence;
    if (!valid()) { recoveryFabricEvidence={};return false; }
    recoveryFabricEvidenceRecorded=true;
#endif
    return true;
}
