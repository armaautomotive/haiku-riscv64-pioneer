/* SPDX-License-Identifier: MIT */
// Restricted empty-session qualification: prepare the same dedicated routes
// used by the resident rewrite and inspect their held state. The caller must
// issue BANK_END explicitly; uncertain preparation or temporary-route restore
// retains the owner for recovery. No worker image is written or launched.
#if !defined(PIONEER_QWEN35_FLEET_INSPECTION_QUALIFY) || !defined(PIONEER_QWEN35_RECOVERY_QUALIFY) || !defined(PIONEER_QWEN35_DEDICATED_FLEET_WITNESS)
#error Fleet inspection control is restricted to its dedicated qualification build
#endif
#include "PioneerQwen35FleetInspectionProtocol.h"
#include "PioneerQwen35DataflowFleetInspection.h"

// Constructed only inside the locked control call. This permits sampling,
// not role-memory writes, reset release, firmware exclusion, or inference.
class PioneerQ35FleetInspectionScope {
    PioneerProbeCookie* fCookie;
    PioneerStoreContext* fContext;
    thread_id fThread;
    uint64 fGeneration;
public:
    PioneerQ35FleetInspectionScope(PioneerProbeCookie* cookie,PioneerStoreContext* context)
        :fCookie(cookie),fContext(context),fThread(find_thread(NULL)),fGeneration(sReloadGeneration) {}
    bool ProveExclusiveRewrite(PioneerSessionIO& io) const {
        thread_info info={};
        return fCookie && fContext && &io==&fContext->io && fThread>=0
            && find_thread(NULL)==fThread && get_thread_info(fThread,&info)==B_OK
            && info.team==fCookie->team && fCookie->magic==0x50313030
            && fCookie->context==fContext && sRuntimeOwner==fCookie
            && sQwen35ExclusiveCookie==fCookie && sOpenCookies==1
            && !sColdRecoveryRequired && !sReloadQuiesced && fGeneration
            && sReloadGeneration==fGeneration && sBankStorageOnly && fContext->bankStorageOnly
            && fContext->bytes==PioneerResidentBanks::Total && io.qwen35DataflowContext==fContext
            && !io.qwen35BorrowedRoleScope && !fContext->dispatchOwnerRetained
            && fContext->qwen35DedicatedFleet && fContext->q38DispatchRoutes
            && fContext->q38Dispatch && fContext->q38Dispatch->prepared==120
            && !fContext->q38Dispatch->bootAttempted && !fContext->q38Dispatch->booted
            && !fContext->q38Dispatch->closed && !PioneerQ38DispatchUnsafe(*fContext->q38Dispatch)
            && !fContext->qwen35DataflowService && !fContext->qwen35DataflowMemory
            && !fContext->qwen35SharedRoleBinding && !fContext->qwen35GraphOwner;
    }
    bool ProveRoleStatusInspectionAllowed(PioneerSessionIO& io) const {
        return ProveExclusiveRewrite(io) && io.SharedRoleFirmwareMatches();
    }
};

static status_t
Qwen35FleetInspectionControl(PioneerProbeCookie* cookie,void* data,size_t length)
{
    if (!cookie || !data || length!=sizeof(tt_q35_fleet_inspection)) return B_BAD_VALUE;
    thread_info info={};status_t status=get_thread_info(find_thread(NULL),&info);
    if (status!=B_OK) return status;
    if (info.team!=cookie->team) return B_NOT_ALLOWED;
    tt_q35_fleet_inspection r={};status=user_memcpy(&r,data,sizeof(r));
    if (status!=B_OK || !PioneerQ35FleetInspectionRequest(r)) return status!=B_OK?status:B_BAD_VALUE;
    status=acquire_sem(sLock);if (status!=B_OK) return status;
    PioneerStoreContext* c=cookie->context;
    if (r.expected_tag!=PIONEER_DRIVER_BUILD_TAG || r.expected_generation!=sReloadGeneration)
        r.operation_status=B_BAD_VALUE;
    else if (!c || cookie->magic!=0x50313030 || sRuntimeOwner!=cookie
        || sQwen35ExclusiveCookie!=cookie || sOpenCookies!=1 || sColdRecoveryRequired
        || sReloadQuiesced || !sReloadGeneration || !sBankStorageOnly || !c->bankStorageOnly
        || c->bytes!=PioneerResidentBanks::Total || !c->initialized || c->uploadedPages
        || c->Unsafe() || c->dispatchOwnerRetained || c->q38Dispatch || c->qwen35DedicatedFleet
        || c->qwen35FleetInspectionAttempted || c->io.qwen35BorrowedRoleScope
        || c->qwen35DataflowService || c->qwen35DataflowMemory || c->qwen35SharedRoleBinding
        || c->qwen35GraphOwner || !c->io.SharedRoleFirmwareMatches())
        r.operation_status=B_NOT_ALLOWED;
    else {
        c->qwen35FleetInspectionAttempted=true;
        if (!PrepareQ38Dispatch(*c)) {
            tt_resident_session_result retained={};RetainQ38DispatchOwner(*c,retained);
            r.operation_status=B_DEV_NOT_READY;
        } else {
            r.prepared=c->q38Dispatch->prepared;
            PioneerQ35FleetInspectionScope scope(cookie,c);
            PioneerQwen35RoleRouteGuard<PioneerSessionIO,PioneerQ35FleetInspectionScope> route(c->io,scope);
            PioneerQwen35FleetInspectionResult result;
            const bool inspected=PioneerQwen35InspectHeldFleet(c->io,scope,route,result);
            r.stage=result.stage;r.failed_core=result.failedCore;r.l2cpu_reset=result.l2cpuReset;
            r.restore_failed=result.restoreFailed;
            r.operation_status=inspected?B_OK:B_DEV_NOT_READY;
            if (result.restoreFailed) {
                tt_resident_session_result retained={};RetainQ38DispatchOwner(*c,retained);
            }
        }
    }
    r.build_tag=PIONEER_DRIVER_BUILD_TAG;r.generation=sReloadGeneration;
    status=user_memcpy(data,&r,sizeof(r));
    release_sem(sLock);return status;
}
