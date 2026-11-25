/* SPDX-License-Identifier: MIT */
// Real owner bindings for the non-loadable recovery coordinator. No physical
// scope or ioctl currently grants this path; callers hold the global lock and
// both guard references for its entire duration.
#if !defined(PIONEER_QWEN35_FABRIC_GUARD_WITNESS) \
    || !defined(PIONEER_QWEN35_RECOVERY_COMPILE_ONLY) || defined(PIONEER_QWEN35_RECOVERY_QUALIFY) \
    || !defined(PIONEER_QWEN35_RESIDENT_SERVICE_WITNESS) || !defined(PIONEER_QWEN35_SNAPSHOT_TAKEOVER_WITNESS)
#error Fabric recovery coordinator is restricted to the non-loadable resident witness
#endif
#include "PioneerQwen35FabricRecoveryCoordinator.h"

template<class SessionScope,class RoleScope,class ParentScope,class BarGuard,class FinishScope>
struct PioneerQ35FabricCoordinatorOperations {
    PioneerProbeCookie* inspector;PioneerProbeCookie* owner;PioneerStoreContext* context;
    PioneerQ35FabricHostScope& host;pioneer_driver_recovery_guard_v2_info* validation;
    pioneer_driver_fabric_recovery_guard_info* fabric;uint64 ticket;unsigned polls;
    SessionScope& scope;RoleScope& roleScope;ParentScope& parentScope;BarGuard& bar;
    FinishScope& finishScope;tt_pioneer_recovery_run& result;
    // Coordinator callbacks must be metadata-only; the scope independently
    // proves physical writer/DMA exclusion and all mapping identities.
    bool Access() {
        return context && inspector && owner && validation && fabric && ticket && host.MatchesParties(inspector,owner)
            && host.OwnsAfterAuditRelease(*context,validation,ticket)
            && scope.FabricCoordinatorAccessAllowed(*inspector,*owner,*context,ticket)
            && host.MatchesParties(inspector,owner) && host.OwnsAfterAuditRelease(*context,validation,ticket);
    }
    bool Admission() {
        return polls && context && owner && inspector && validation && fabric && ticket
            && validation->version==2 && fabric->version==1 && validation->begin && validation->abort
            && validation->validate && fabric->begin && fabric->abort && fabric->validate && fabric->complete
            && validation->begin==fabric->begin && validation->abort==fabric->abort
            && validation->validate==fabric->validate && !context->recoveryFreshSession
            && context->qwen35SharedRoleBinding && context->qwen35GraphOwner
            && Access() && scope.FabricCoordinatorAdmission(*inspector,*owner,*context,ticket) && Access();
    }
    PioneerQ35FreshRecoverySession* AllocateSession() { return PioneerQ35FreshRecoverySession::Allocate(); }
    bool PrepareSession(PioneerQ35FreshRecoverySession& session) {
        return Access() && session.PrepareFabricRole(owner,*context,validation,scope);
    }
    void RetainOrDiscard(PioneerQ35FreshRecoverySession& session) {
        if (context->recoveryFreshSession!=&session) {
            if (!session.DiscardUnstarted()) session.RetainUncertain(scope);
        } else session.RetainUncertain(scope);
    }
    void RetainSession(PioneerQ35FreshRecoverySession& session) { session.RetainUncertain(scope); }
    bool SessionAttached(PioneerQ35FreshRecoverySession& session) {
        return Access() && context->recoveryFreshSession==&session && session.Ticket()==ticket && session.RoleReady();
    }
    bool RestoreSession(PioneerQ35FreshRecoverySession& session) {
        return PioneerQ35RestoreFabricRoleSequence(session,*context,validation,scope,roleScope,parentScope,bar,polls);
    }
    bool DetachSession(PioneerQ35FreshRecoverySession& session) {
        return Access() && session.RetireRoleSession(validation,scope);
    }
    bool SessionDetached() { return Access() && !context->recoveryFreshSession; }
    bool ReleaseRoleOwner() {
        auto check=[&]() { return Access(); };
        return PioneerQ35ReleaseFabricRoleOwner(*context,ticket,check);
    }
    bool RecordGraphRetirement() {
        auto check=[&]() { return Access(); };
        return PioneerQ35RecordFabricGraphRetirement(*context,ticket,check);
    }
    void RetainContext() { sColdRecoveryRequired=true; }
    status_t Finish() {
        return PioneerQ35FabricFinishLocked(inspector,owner,context,host,validation,fabric,ticket,finishScope,result);
    }
};

template<class SessionScope,class RoleScope,class ParentScope,class BarGuard,class FinishScope>
static status_t
PioneerQ35ResumeFabricRoleRecoveryLocked(PioneerProbeCookie* inspector,PioneerProbeCookie* owner,
    PioneerStoreContext* context,PioneerQ35FabricHostScope& host,pioneer_driver_recovery_guard_v2_info* validation,
    pioneer_driver_fabric_recovery_guard_info* fabric,uint64 ticket,unsigned polls,SessionScope& scope,
    RoleScope& roleScope,ParentScope& parentScope,BarGuard& bar,FinishScope& finishScope,tt_pioneer_recovery_run& result)
{
    PioneerQ35FabricCoordinatorOperations<SessionScope,RoleScope,ParentScope,BarGuard,FinishScope> ops{
        inspector,owner,context,host,validation,fabric,ticket,polls,scope,roleScope,parentScope,bar,finishScope,result};
    return PioneerQ35CoordinateFabricRecovery(ops);
}
