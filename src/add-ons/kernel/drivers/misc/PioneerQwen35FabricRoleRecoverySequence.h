/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_QWEN35_FABRIC_ROLE_RECOVERY_SEQUENCE_H
#define PIONEER_QWEN35_FABRIC_ROLE_RECOVERY_SEQUENCE_H

// Ordered intermediate recovery of the already-attached fabric-role session.
// Caller holds the retained owner's lock and guard ticket. The supplied scope
// must independently prove lifetime physical writer/DMA exclusion and protect
// every fresh/audit mapping. Successful return does not release the owners,
// reservation or cold latch, and is not a complete recovery or launch permit.
// The sequence callbacks are metadata-only; BarGuard, RoleScope and
// ParentScope must guard any device access they perform independently.
template<class Session,class Context,class Guard,class SessionScope,class RoleScope,class ParentScope,class BarGuard>
bool PioneerQ35RestoreFabricRoleSequence(Session& session,Context& old,Guard* guard,
    SessionScope& scope,RoleScope& roleScope,ParentScope& parentScope,BarGuard& bar,unsigned polls)
{
    auto* fresh=session.Fresh();
    auto* role=old.qwen35SharedRoleBinding;
    auto* graph=old.qwen35GraphOwner;
    const auto ticket=session.Ticket();
    if (!guard || !guard->validate || !ticket || !polls || !fresh || !role || !graph
        || old.recoveryFreshSession!=&session || !session.RoleReady()) return false;
    if (!scope.FabricRoleSequenceAdmission(session,old,*fresh,ticket)) {
        session.RetainUncertain(scope);return false;
    }
    bool lost=false;
    auto allowed=[&]() {
        if (lost || old.recoveryFreshSession!=&session || session.Fresh()!=fresh || session.Ticket()!=ticket
            || old.qwen35SharedRoleBinding!=role || old.qwen35GraphOwner!=graph
            || guard->validate(ticket)!=B_OK
            || !scope.FabricRoleSequenceAccessAllowed(session,old,*fresh,ticket)
            || old.recoveryFreshSession!=&session || session.Fresh()!=fresh || session.Ticket()!=ticket
            || old.qwen35SharedRoleBinding!=role || old.qwen35GraphOwner!=graph
            || guard->validate(ticket)!=B_OK) { lost=true;return false; }
        return true;
    };
    auto protectedArea=[&](int32_t area) {
        return fresh->io.ReferencesRecoveryHostArea(area)
            || scope.FabricRoleSequenceAreaProtected(session,old,*fresh,ticket,area);
    };
    auto fail=[&]() { session.RetainUncertain(scope);return false; };
    if (!allowed() || !role->OpenFabricRecoveryViews(bar,allowed,protectedArea)
        || !allowed() || !role->BeginFabricCoreRecovery(fresh->io,roleScope,allowed)
        || !allowed() || !role->RestoreFabricCoreFleet(fresh->io,roleScope,allowed,polls)
        || !allowed() || !graph->RestoreFabricParent(*fresh,parentScope,allowed)
        || !allowed() || !role->VerifyFabricRolesAfterParent(fresh->io,roleScope,allowed)
        || !allowed() || !role->CloseFabricRecoveryViews(allowed,protectedArea)
        || !allowed() || !session.VerifyRoleRestoration(guard,scope)
        || !allowed() || !session.CloseRoleTransport(guard,scope)
        || !allowed() || !session.DisposeRoleOldHostBuffers(guard,scope)
        || !allowed() || !session.PublishRoleMappingsDisposed(guard,scope)
        || !allowed() || !session.RetireRoleGraph(guard,scope)
        || !allowed() || !session.ReleaseRoleFreshHostPage(guard,scope)
        || !allowed() || !session.RecordRoleOwnerRetirement(guard,scope)
        || !allowed()) return fail();
    return true;
}
#endif
