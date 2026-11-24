/* SPDX-License-Identifier: MIT */
// One locked modified-scratch recovery transaction. The caller owns the
// original reservation and guard module, has disposed old session mappings,
// and must supply a real physical-identity and writer-exclusion scope. A
// failed attached session is retained; an unstarted one may be discarded.
#if !defined(PIONEER_QWEN35_RECOVERY_COMPILE_ONLY) || defined(PIONEER_QWEN35_RECOVERY_QUALIFY) \
    || !defined(PIONEER_QWEN35_DEVICE_DISPATCH) || !defined(PIONEER_QWEN35_RECOVERY_RUN_ENABLE) \
    || !defined(PIONEER_QWEN35_SNAPSHOT_TAKEOVER_WITNESS) || !defined(PIONEER_QWEN35_SHARED_ROLE_OWNER_WITNESS)
#error Modified Q35 recovery transaction requires the non-loadable snapshot witness
#endif

template<class Session,class Scope>
static status_t
PioneerQ35RunModifiedRecoveryTransaction(PioneerProbeCookie* owner,PioneerStoreContext* context,
    pioneer_driver_recovery_guard_v2_info* guard,Scope& scope)
{
    if (!owner || !context || !guard || owner->context!=context || context->recoveryFreshSession
        || !context->qwen35QualificationOwner.ModifiedResetEligible(context->banks)
        || context->qwen35SharedRoleBinding || context->qwen35GraphOwner
        || !scope.RecoveryTransactionAdmission(owner,*context,guard)) return B_NOT_ALLOWED;
    auto* session=Session::Allocate();
    if (!session) return B_NO_MEMORY;
    if (!session->Prepare(owner,*context,guard,scope)) {
        // A setup rejection before attachment has not touched PCI. Once
        // attached, even a failed operation may have left an unknown write.
        if (context->recoveryFreshSession!=session) {
            if (!session->DiscardUnstarted()) session->RetainUncertain(scope);
        } else session->RetainUncertain(scope);
        return B_DEV_NOT_READY;
    }
    if (context->recoveryFreshSession!=session) {
        session->RetainUncertain(scope);return B_DEV_NOT_READY;
    }
    if (!session->RestoreScratch(guard,scope) || !session->CloseTransport(guard,scope)) {
        session->RetainUncertain(scope);return B_DEV_NOT_READY;
    }
    // Retire destroys the session only on success; do not touch that pointer
    // afterward. The old owner/snapshot survives until its bank is destroyed.
    if (!session->Retire(guard,scope)) { session->RetainUncertain(scope);return B_DEV_NOT_READY; }
    if (context->recoveryFreshSession || context->qwen35QualificationOwner.Active()
        || !context->qwen35QualificationOwner.Retired()) return B_DEV_NOT_READY;
    return B_OK;
}
