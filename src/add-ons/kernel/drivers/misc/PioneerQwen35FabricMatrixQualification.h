/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_QWEN35_FABRIC_MATRIX_QUALIFICATION_H
#define PIONEER_QWEN35_FABRIC_MATRIX_QUALIFICATION_H

// Compose the real fixed-projection transaction with its recovery receipt.
// Both scopes must independently guard every device access for the lifetime
// of the retained ticket. These outer checks are metadata-only and do not
// exclude firmware or DMA, nor do they authorize any hardware operation.
template<class Session,class Context,class Guard,class RecoveryScope,class QualificationScope>
bool PioneerQ35RunFabricMatrixQualification(Session& session,Context& old,Context& fresh,
    Guard* guard,RecoveryScope& recovery,QualificationScope& qualification,
    uint32_t& stage,uint32_t& routeStage,uint32_t& routeCore,bool& outputVerified,bool& cleaned)
{
    stage=routeStage=0;routeCore=0xff;outputVerified=cleaned=false;
    const auto ticket=session.Ticket();
    if (!guard || !guard->validate || !ticket || session.Fresh()!=&fresh
        || old.recoveryFreshSession!=&session || session.CurrentStage()!=Session::RoleVerified) return false;
    bool lost=false;
    auto allowed=[&]() {
        if (lost || session.Fresh()!=&fresh || old.recoveryFreshSession!=&session
            || session.Ticket()!=ticket || session.CurrentStage()!=Session::RoleVerified
            || guard->validate(ticket)!=0
            || !recovery.FabricMatrixRunAccessAllowed(session,old,fresh,ticket)
            || session.Fresh()!=&fresh || old.recoveryFreshSession!=&session
            || session.Ticket()!=ticket || session.CurrentStage()!=Session::RoleVerified
            || guard->validate(ticket)!=0) { lost=true;return false; }
        return true;
    };
    auto fail=[&]() { session.RetainUncertain(recovery);return false; };
    if (!allowed() || !recovery.FabricMatrixRunAdmission(session,old,fresh,ticket) || !allowed()) return fail();
    auto receipt=[&]() {
        return allowed() && session.RecordFabricMatrixQualification(guard,recovery) && allowed();
    };
    const bool okay=fresh.RunDataflowQualificationTransactionWithReceipt(qualification,receipt,
        stage,routeStage,routeCore,outputVerified,cleaned);
    if (!okay || !outputVerified || !cleaned || !allowed()
        || !old.recoveryFabricMatrixReceipt.Matches(old,ticket,guard,recovery)
        || !recovery.FabricMatrixRunComplete(session,old,fresh,ticket)
        || !allowed() || !old.recoveryFabricMatrixReceipt.Matches(old,ticket,guard,recovery)) return fail();
    return true;
}
#endif
