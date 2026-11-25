/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_QWEN35_FABRIC_RECOVERY_COORDINATOR_H
#define PIONEER_QWEN35_FABRIC_RECOVERY_COORDINATOR_H

// Orchestrate the already-reset, retained role owner. All operations are
// one-shot; a detached session or completed context must never be touched
// through its former pointer. The implementation owns all physical proofs.
template<class Operations>
int PioneerQ35CoordinateFabricRecovery(Operations& ops)
{
    if (!ops.Admission()) return B_NOT_ALLOWED;
    auto* session=ops.AllocateSession();
    if (!session) return B_NO_MEMORY;
    if (!ops.PrepareSession(*session)) {
        ops.RetainOrDiscard(*session);return B_DEV_NOT_READY;
    }
    if (!ops.SessionAttached(*session) || !ops.RestoreSession(*session)) {
        ops.RetainSession(*session);return B_DEV_NOT_READY;
    }
    // Success destroys session, so do not refer to it after this call.
    if (!ops.DetachSession(*session)) {
        ops.RetainSession(*session);return B_DEV_NOT_READY;
    }
    if (!ops.SessionDetached() || !ops.ReleaseRoleOwner() || !ops.RecordGraphRetirement()) {
        ops.RetainContext();return B_DEV_NOT_READY;
    }
    // Finish may destroy the old context and owner. Do not use ops afterward.
    return ops.Finish();
}
#endif
