/* SPDX-License-Identifier: MIT */
// Held-core, fixed-image snapshot/stage/restore qualification. It never
// releases a core, issues a reset, uploads model data or performs inference.
// Any uncertain operation retains the heap owner and its snapshots/mappings.
#if !defined(PIONEER_QWEN35_ROLE_STAGE_QUALIFY) || !defined(PIONEER_QWEN35_FLEET_INSPECTION_QUALIFY) || !defined(PIONEER_QWEN35_EXCLUSIVE_TAKEOVER_WITNESS)
#error Role stage qualification requires its restricted held-fleet build
#endif
#include "PioneerQwen35RoleStageProtocol.h"
#include "../../../../../tools/pioneer/PioneerQwen35DataflowRoleFleetImagesOwner.h"
#ifdef PIONEER_QWEN35_ROLE_PRESTACK_QUALIFY
#include "PioneerQwen35RolePrestackProbe.h"
#endif

class PioneerQ35RoleStageQualification {
public:
    PioneerQwen35RoleHaikuMap platform;
    PioneerQwen35::DataflowRoleWritableViews<PioneerQwen35RoleHaikuMap> views;
    PioneerQwen35::DataflowRoleFleetImagesOwner images;
#ifdef PIONEER_QWEN35_ROLE_PRESTACK_QUALIFY
    PioneerQwen35RolePrestackState prestack;
#endif
    uint32_t* snapshot;
    PioneerQ35RoleStageQualification():views(platform),snapshot(
        static_cast<uint32_t*>(malloc(sizeof(uint32_t)*PioneerQwen35::DataflowRoleFleetImagesOwner::RequiredWords()))) {}
    ~PioneerQ35RoleStageQualification() { free(snapshot); }
    PioneerQ35RoleStageQualification(const PioneerQ35RoleStageQualification&)=delete;
    PioneerQ35RoleStageQualification& operator=(const PioneerQ35RoleStageQualification&)=delete;
};

static void
PioneerQ35RoleStageProgress(unsigned phase)
{
#ifdef _KERNEL_MODE
#ifdef PIONEER_QWEN35_ROLE_PRESTACK_QUALIFY
    if (phase==9) { dprintf("P100A_Q35_ROLE_STAGE_PROGRESS phase=9;PRESTACK_QUALIFICATION\n"); return; }
#endif
    dprintf("P100A_Q35_ROLE_STAGE_PROGRESS phase=%u;HELD_CORES_NO_RELEASE\n",phase);
#else
    (void)phase;
#endif
}

static status_t
Qwen35RoleStageControl(PioneerProbeCookie* cookie,void* data,size_t length)
{
    using namespace PioneerQwen35;
    if (!cookie || !data || length!=sizeof(tt_q35_role_stage_qualification)) return B_BAD_VALUE;
    thread_info info={};status_t status=get_thread_info(find_thread(NULL),&info);
    if (status!=B_OK) return status;
    if (info.team!=cookie->team) return B_NOT_ALLOWED;
    tt_q35_role_stage_qualification r={};status=user_memcpy(&r,data,sizeof(r));
    if (status!=B_OK || !PioneerQ35RoleStageQualificationRequest(r)) return status!=B_OK?status:B_BAD_VALUE;
    status=acquire_sem(sLock);if (status!=B_OK) return status;
    PioneerStoreContext* c=cookie->context;
    if (r.expected_tag!=PIONEER_DRIVER_BUILD_TAG || r.expected_generation!=sReloadGeneration)
        r.operation_status=B_BAD_VALUE;
    else if (!c || c->qwen35RoleStageAttempted || c->qwen35RoleStageQualification
        || !c->qwen35FleetInspectionPassed || c->uploadedPages || c->Unsafe())
        r.operation_status=B_NOT_ALLOWED;
    else {
        PioneerQ35FleetInspectionScope scope(cookie,c);
        if (!scope.ProveRoleStatusInspectionAllowed(c->io)) r.operation_status=B_NOT_ALLOWED;
        else {
            c->qwen35RoleStageAttempted=true;
            PioneerQwen35RoleRouteGuard<PioneerSessionIO,PioneerQ35FleetInspectionScope> route(c->io,scope);
            PioneerQwen35FleetInspectionResult observation;
            r.stage=1;PioneerQ35RoleStageProgress(r.stage);
            if (!PioneerQwen35InspectHeldFleet(c->io,scope,route,observation)) {
                r.operation_status=B_DEV_NOT_READY;
                if (observation.restoreFailed) {
                    tt_resident_session_result retained={};RetainQ38DispatchOwner(*c,retained);
                }
            } else {
                r.stage=2;PioneerQ35RoleStageProgress(r.stage);
                auto* owner=new(std::nothrow) PioneerQ35RoleStageQualification;
                if (!owner || !owner->snapshot) { delete owner;r.operation_status=B_NO_MEMORY; }
                else {
                    // Publish retained ownership before the first mapping or
                    // device image access; BANK_END refuses this owner.
                    c->qwen35RoleStageQualification=owner;
                    r.stage=3;PioneerQ35RoleStageProgress(r.stage);
                    bool okay=scope.BeginStage(c->io)
                        && owner->views.Open(c->io.profile.bar0_host_start,c->io.profile.bar0_host_end,route);
                    PioneerQwen35RoleFleetPciIO<PioneerSessionIO,PioneerQ35FleetInspectionScope,
                        DataflowRoleWritableViews<PioneerQwen35RoleHaikuMap>> adapter(c->io,scope,owner->views);
#ifdef PIONEER_QWEN35_ROLE_PRESTACK_QUALIFY
                    PioneerQwen35RolePrestackProbe<PioneerSessionIO,PioneerQ35FleetInspectionScope,
                        DataflowRoleWritableViews<PioneerQwen35RoleHaikuMap>,decltype(adapter)>
                        prestack(c->io,scope,owner->views,adapter,owner->prestack);
#endif
                    if (okay) {
                        r.stage=4;PioneerQ35RoleStageProgress(r.stage);
                        okay=owner->images.CaptureAll(owner->snapshot,
                            DataflowRoleFleetImagesOwner::RequiredWords(),adapter);
                        r.captured=okay;
                    }
                    if (okay) {
                        r.stage=5;PioneerQ35RoleStageProgress(r.stage);
                        okay=scope.EnableStageWrites(c->io)
                            && owner->images.StageAll(DataflowSharedReaderCodeWords,
                                sizeof(DataflowSharedReaderCodeWords)/sizeof(uint32_t),
                                DataflowSharedWriterCodeWords,sizeof(DataflowSharedWriterCodeWords)/sizeof(uint32_t),
                                DataflowSharedReaderShimWords,sizeof(DataflowSharedReaderShimWords)/sizeof(uint32_t),
                                DataflowSharedWriterShimWords,sizeof(DataflowSharedWriterShimWords)/sizeof(uint32_t),adapter);
                        r.staged=okay;
                    }
#ifdef PIONEER_QWEN35_ROLE_PRESTACK_QUALIFY
                    if (okay && r.version==2) {
                        r.stage=9;PioneerQ35RoleStageProgress(r.stage);
                        okay=prestack.Run(DataflowSharedReaderParkAddress,DataflowSharedWriterParkAddress,65536);
                    }
#endif
                    if (okay) {
                        r.stage=6;PioneerQ35RoleStageProgress(r.stage);
                        okay=owner->images.RestoreAll(adapter);r.restored=okay;
                    }
#ifdef PIONEER_QWEN35_ROLE_PRESTACK_QUALIFY
                    if (okay && r.version==2) okay=prestack.VerifyAfterImages();
#endif
                    if (okay) {
                        r.stage=7;PioneerQ35RoleStageProgress(r.stage);
                        okay=owner->views.Close() && !owner->views.HasMappings();r.mappings_closed=okay;
                    }
                    // A transient image-stage or image-restore error can be
                    // recovered while this same locked owner still holds all
                    // snapshots and routes. Do not clear quarantine or retry
                    // an uncertain mapping operation. Restoration performs
                    // the complete fleet-wide original-image audit; a second
                    // failure retains the owner exactly as before.
                    if (!okay && r.captured && !r.mappings_closed
#ifdef PIONEER_QWEN35_ROLE_PRESTACK_QUALIFY
                        && r.version==1
#endif
                        && owner->views.Opened() && owner->views.ImageWritesConfirmed() && !c->Unsafe()
                        && scope.ProveRoleDedicatedObservedStage(c->io)) {
                        const bool restored=owner->images.RestorationRequired()
                            ?owner->images.RestoreAll(adapter)
                            :owner->images.ImagesRestored()
                                && owner->images.VerifyOriginalAll(adapter);
                        if (restored) {
                            r.restored=1;
                            r.mappings_closed=owner->views.Close() && !owner->views.HasMappings();
                            if (r.mappings_closed) {
                                delete owner;c->qwen35RoleStageQualification=NULL;
                                // The original operation still failed: the
                                // client must report failure and end the bank.
                                r.operation_status=B_DEV_NOT_READY;
                                owner=NULL;
                            }
                        }
                    }
                    if (okay) {
                        delete owner;c->qwen35RoleStageQualification=NULL;
                        r.stage=
#ifdef PIONEER_QWEN35_ROLE_PRESTACK_QUALIFY
                            r.version==2?TT_Q35_ROLE_PRESTACK_COMPLETE:
#endif
                            TT_Q35_ROLE_STAGE_COMPLETE;
                        PioneerQ35RoleStageProgress(r.stage);r.operation_status=B_OK;
                    } else if (owner) {
                        tt_resident_session_result retained={};RetainQ38DispatchOwner(*c,retained);
                        r.operation_status=B_DEV_NOT_READY;
                    }
                }
            }
        }
    }
    r.build_tag=PIONEER_DRIVER_BUILD_TAG;r.generation=sReloadGeneration;
    status=user_memcpy(data,&r,sizeof(r));release_sem(sLock);return status;
}
