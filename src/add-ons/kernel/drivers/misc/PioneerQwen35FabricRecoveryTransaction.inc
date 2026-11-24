/* SPDX-License-Identifier: MIT */
// Included inside PioneerStoreContext in the non-loadable snapshot rewrite.
// The caller holds sLock and supplies an already-open, firmware-probed mapping,
// an active recovery ticket and the independent physical-ownership scope.
// Success means only that the Tensix sequence, audits and mapping close
// completed: the context, owner, mapping object, PCI reservation and cold
// latch remain retained for scratch restoration and final recovery.
// A live scope must independently maintain physical ownership and fault
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
        || !dispatchOwnerRetained || !sColdRecoveryRequired || !sRuntimeOwner
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
            && dispatchOwnerRetained && sColdRecoveryRequired && sRuntimeOwner
            && sRuntimeOwner->context==this && guard->validate(ticket)==B_OK
            && scope.FabricRecoveryStageAllowed(*this,*owner,guard,ticket,stage)
            && guard->validate(ticket)==B_OK && recoveryFabricArcOwner==owner
            && recoveryMapping==mapping && dispatchOwnerRetained && sColdRecoveryRequired
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
        || !step(14,[&]() { return mapping->AuditQ35HostDmaGuarded(valid); })
        || !step(15,[&]() { return owner->Retire(guard,scope); })
        || !step(16,[&]() { return mapping->CloseQ35Guarded(valid); })) return false;
    stage=17;
    return valid() && owner->CurrentPhase()==FabricOwner::Retired && mapping->q35CloseComplete;
}
