/* SPDX-License-Identifier: MIT */
// Actual fleet controller and PCI adapter against simulated per-tile state.
#define PIONEER_QWEN35_ROLE_PRESTACK_QUALIFY 1
#define PIONEER_QWEN35_ROLE_LOCAL_RESTORE_QUALIFY 1
#define PIONEER_QWEN35_ROLE_SERVICE_QUALIFY 1
#define PIONEER_QWEN35_ROLE_FLEET_SERVICE_QUALIFY 1
#define TEST_ROLE_PRESTACK_CONTROL 1
#include "PioneerQwen35LocalRestoreProbeImages.h"
#include "PioneerQwen35RoleServiceProbeImages.h"
#include "pioneer_qwen35_role_stage_control_test.cpp"

int main()
{
    (void)&EndGuard;(void)&Qwen35DataflowGenerate;
    {
    Fixture f;auto r=Request();r.version=4;
    assert(Qwen35RoleStageControl(&f.cookie,&r,sizeof(r))==B_BAD_VALUE
        && !f.context.qwen35RoleStageAttempted);
    }
    {
    Fixture f;assert(f.Run()==B_OK && f.request.operation_status==B_OK);
    ResetMaps();gAllowWrite=true;gPerCoreRoleWindows=true;
    gRoleWindowCore=gRoleSelectedCore=f.context.io.runtimeCore;
    f.context.io.perCoreReset=true;
    for (unsigned core=0;core<DataflowBarrierWorkers;core++) {
        f.context.io.tileReset[core]=0x47800;
        f.context.io.roleHandoffs[core]={0x1000+core,0x2000+core,0x3000+core};
        auto& box=f.context.io.roleBoxes[core];
        auto* words=reinterpret_cast<uint32_t*>(&box);
        for (unsigned i=0;i<sizeof(box)/sizeof(uint32_t);i++) words[i]=0x48000000U+core*32+i;
    }
    for (unsigned noc=0;noc<2;noc++) f.context.io.nocAvailable[noc]=0x10101010U;
    f.context.io.simulateService=true;f.context.io.requireAllServicesBeforeStop=true;
    // After all starts, sample both service loops in the actual shim. The
    // reader sample matches the 0x700cc observed on Pioneer hardware.
    f.context.io.pauseHook=[](PioneerSessionIO* io) {
        if (io->serviceStarts==DataflowBarrierWorkers) {
            io->fleetRolePc[0][DataflowRoleReader]=0x700cc;
            io->fleetRolePc[0][DataflowRoleWriter]=DataflowServiceProbeWriterLoopAddress;
            io->pauseHook=nullptr;
        }
    };
    auto r=Request();r.version=6;
    assert(Qwen35RoleStageControl(&f.cookie,&r,sizeof(r))==B_OK);
    if (r.operation_status!=B_OK) std::fprintf(stderr,"fleet failed status=%d stage=%u capture=%u stage_image=%u restore=%u close=%u starts=%u stops=%u reset=%u hold=%u core=%u\n",
        r.operation_status,r.stage,r.captured,r.staged,r.restored,r.mappings_closed,
        f.context.io.serviceStarts,f.context.io.serviceStops,f.context.io.sharedResetWrites,
        f.context.io.holdWrites,f.context.io.runtimeCore);
    if (r.operation_status!=B_OK && f.context.qwen35RoleStageQualification) {
        const auto& state=f.context.qwen35RoleStageQualification->fleetPrestack[f.context.io.runtimeCore];
        std::fprintf(stderr,"state copies=%d stop=%d verified=%d reset_restore=%d complete=%d reset=0x%x\n",
            state.copiesCaptured,state.serviceStopped,state.copiesVerified,state.resetRestoreAttempted,
            state.complete,f.context.io.reset);
    }
    assert(r.operation_status==B_OK && r.stage==TT_Q35_ROLE_FLEET_SERVICE_COMPLETE
        && r.captured && r.staged && r.restored && r.mappings_closed);
    assert(f.context.io.sharedResetWrites==DataflowBarrierWorkers
        && f.context.io.serviceStarts==DataflowBarrierWorkers
        && f.context.io.serviceStops==DataflowBarrierWorkers
        && f.context.io.holdWrites==DataflowBarrierWorkers);
    assert(!f.context.qwen35RoleStageQualification && !sColdRecoveryRequired && !gRetains);
    for (unsigned core=0;core<DataflowBarrierWorkers;core++) {
        const auto handoff=f.context.io.roleHandoffs[core];
        assert(handoff.readerReady==0x1000+core && handoff.writerReady==0x2000+core
            && handoff.release==0x3000+core);
        const auto* words=reinterpret_cast<const uint32_t*>(&f.context.io.roleBoxes[core]);
        for (unsigned i=0;i<sizeof(DataflowRoleMailbox)/sizeof(uint32_t);i++)
            assert(words[i]==0x48000000U+core*32+i);
        assert(f.context.io.SelectRuntimeCore(core) && f.context.io.reset==0x47800);
    }
    assert(f.context.io.SelectRuntimeCore(0));
    for (unsigned core=0;core<DataflowBarrierWorkers;core++)
        for (unsigned which=0;which<=DataflowRolePhysicalControl;which++)
            for (uint32_t word:gRoleTileWindows[core][which])
                assert(word==RoleWindowInitial(core,which));
    gPerCoreRoleWindows=false;gAllowWrite=false;
    for (auto& windows:gRoleTileWindows) windows.clear();
    }
    {
    Fixture f;assert(f.Run()==B_OK && f.request.operation_status==B_OK);
    ResetMaps();gAllowWrite=true;gPerCoreRoleWindows=true;
    gRoleWindowCore=gRoleSelectedCore=f.context.io.runtimeCore;
    for (unsigned noc=0;noc<2;noc++) f.context.io.nocAvailable[noc]=0x10101010U;
    gFailAfterStageWrite=true;gFailStageWriteOnce=true;gStageWriteFailureInjected=false;
    auto r=Request();r.version=6;
    assert(Qwen35RoleStageControl(&f.cookie,&r,sizeof(r))==B_OK);
    assert(r.operation_status==B_DEV_NOT_READY && r.stage==5 && r.captured && !r.staged
        && r.restored && r.mappings_closed && gStageWriteFailureInjected
        && !f.context.io.sharedResetWrites && !f.context.qwen35RoleStageQualification
        && !f.context.dispatchOwnerRetained && !sColdRecoveryRequired && !gRetains);
    assert(f.context.io.SelectRuntimeCore(0));
    for (unsigned core=0;core<DataflowBarrierWorkers;core++)
        for (unsigned which=0;which<=DataflowRolePhysicalControl;which++)
            for (uint32_t word:gRoleTileWindows[core][which])
                assert(word==RoleWindowInitial(core,which));
    gFailStageWriteOnce=false;gPerCoreRoleWindows=false;gAllowWrite=false;
    for (auto& windows:gRoleTileWindows) windows.clear();
    }
    for (unsigned stopFailure=0;stopFailure<2;stopFailure++) {
    Fixture f;assert(f.Run()==B_OK && f.request.operation_status==B_OK);
    ResetMaps();gAllowWrite=true;gPerCoreRoleWindows=true;
    gRoleWindowCore=gRoleSelectedCore=f.context.io.runtimeCore;
    f.context.io.perCoreReset=true;
    for (unsigned core=0;core<DataflowBarrierWorkers;core++) {
        f.context.io.tileReset[core]=0x47800;
        f.context.io.roleHandoffs[core]={0x1000+core,0x2000+core,0x3000+core};
    }
    for (unsigned noc=0;noc<2;noc++) f.context.io.nocAvailable[noc]=0x10101010U;
    f.context.io.simulateService=true;f.context.io.failPrestackHandoffReadCore=7;
    f.context.io.dropServiceStop=stopFailure;
    auto r=Request();r.version=6;
    assert(Qwen35RoleStageControl(&f.cookie,&r,sizeof(r))==B_OK);
    assert(r.operation_status==B_DEV_NOT_READY && r.stage==9 && r.captured && r.staged
        && f.context.io.sharedResetWrites==7 && f.context.io.serviceStarts==7);
    if (!stopFailure) {
        assert(r.restored && r.mappings_closed && f.context.io.serviceStops==7
            && f.context.io.holdWrites==7 && !f.context.qwen35RoleStageQualification
            && !f.context.dispatchOwnerRetained && !sColdRecoveryRequired && !gRetains);
        for (unsigned core=0;core<DataflowBarrierWorkers;core++) {
            const auto handoff=f.context.io.roleHandoffs[core];
            assert(handoff.readerReady==0x1000+core && handoff.writerReady==0x2000+core
                && handoff.release==0x3000+core);
            assert(f.context.io.SelectRuntimeCore(core) && f.context.io.reset==0x47800);
        }
        assert(f.context.io.SelectRuntimeCore(0));
        for (unsigned core=0;core<DataflowBarrierWorkers;core++)
            for (unsigned which=0;which<=DataflowRolePhysicalControl;which++)
                for (uint32_t word:gRoleTileWindows[core][which])
                    assert(word==RoleWindowInitial(core,which));
    } else {
        assert(!r.restored && !r.mappings_closed && f.context.io.serviceStops==1
            && !f.context.io.holdWrites && f.context.qwen35RoleStageQualification
            && f.context.dispatchOwnerRetained && sColdRecoveryRequired && gRetains==1);
        auto* owner=f.context.qwen35RoleStageQualification;
        assert(owner->fleetPrestack[6].serviceStopAttempted && !owner->fleetPrestack[6].complete
            && owner->fleetPrestack[7].attempted && !owner->fleetPrestack[7].releaseAttempted);
        assert(acquire_sem(sLock)==B_OK && owner->views.Close() && release_sem(sLock)==B_OK);
        delete owner;f.context.qwen35RoleStageQualification=nullptr;
    }
    gPerCoreRoleWindows=false;gAllowWrite=false;sColdRecoveryRequired=false;
    for (auto& windows:gRoleTileWindows) windows.clear();
    }
    for (unsigned restoreFailure=0;restoreFailure<3;restoreFailure++) {
    Fixture f;assert(f.Run()==B_OK && f.request.operation_status==B_OK);
    ResetMaps();gAllowWrite=true;gPerCoreRoleWindows=true;
    gRoleWindowCore=gRoleSelectedCore=f.context.io.runtimeCore;
    f.context.io.perCoreReset=true;
    for (unsigned core=0;core<DataflowBarrierWorkers;core++) {
        f.context.io.tileReset[core]=0x47800;
        f.context.io.roleHandoffs[core]={0x1000+core,0x2000+core,0x3000+core};
        auto* words=reinterpret_cast<uint32_t*>(&f.context.io.roleBoxes[core]);
        for (unsigned i=0;i<sizeof(DataflowRoleMailbox)/sizeof(uint32_t);i++) words[i]=0x48000000U+core*32+i;
    }
    for (unsigned noc=0;noc<2;noc++) f.context.io.nocAvailable[noc]=0x10101010U;
    f.context.io.simulateService=true;f.context.io.failMailboxReadCore=7;
    f.context.io.dropMailboxRestore=restoreFailure==1;
    f.context.io.dropMailboxInit=restoreFailure==2;
    auto r=Request();r.version=6;
    assert(Qwen35RoleStageControl(&f.cookie,&r,sizeof(r))==B_OK);
    assert(r.operation_status==B_DEV_NOT_READY && r.stage==9 && r.captured && r.staged
        && f.context.io.failMailboxReadInjected==(restoreFailure!=2) && f.context.io.sharedResetWrites==7
        && f.context.io.serviceStarts==7);
    if (!restoreFailure) {
        assert(r.restored && r.mappings_closed && f.context.io.mailboxRestoreWrites==16
            && f.context.io.serviceStops==7 && f.context.io.holdWrites==7
            && !f.context.qwen35RoleStageQualification && !f.context.dispatchOwnerRetained
            && !sColdRecoveryRequired && !gRetains);
        for (unsigned core=0;core<DataflowBarrierWorkers;core++) {
            const auto handoff=f.context.io.roleHandoffs[core];
            assert(handoff.readerReady==0x1000+core && handoff.writerReady==0x2000+core
                && handoff.release==0x3000+core);
            const auto* words=reinterpret_cast<const uint32_t*>(&f.context.io.roleBoxes[core]);
            for (unsigned i=0;i<sizeof(DataflowRoleMailbox)/sizeof(uint32_t);i++)
                assert(words[i]==0x48000000U+core*32+i);
            assert(f.context.io.SelectRuntimeCore(core) && f.context.io.reset==0x47800);
        }
        assert(f.context.io.SelectRuntimeCore(0));
        for (unsigned core=0;core<DataflowBarrierWorkers;core++)
            for (unsigned which=0;which<=DataflowRolePhysicalControl;which++)
                for (uint32_t word:gRoleTileWindows[core][which])
                    assert(word==RoleWindowInitial(core,which));
    } else if (restoreFailure==1) {
        auto* owner=f.context.qwen35RoleStageQualification;
        assert(!r.restored && !r.mappings_closed && f.context.io.mailboxRestoreWrites==1
            && !f.context.io.serviceStops && owner && owner->fleetPrestack[7].preReleaseRestoreAttempted
            && !owner->fleetPrestack[7].preReleaseRestored && f.context.dispatchOwnerRetained
            && sColdRecoveryRequired && gRetains==1);
        assert(acquire_sem(sLock)==B_OK && owner->views.Close() && release_sem(sLock)==B_OK);
        delete owner;f.context.qwen35RoleStageQualification=nullptr;
    } else {
        auto* owner=f.context.qwen35RoleStageQualification;
        assert(!r.restored && !r.mappings_closed && f.context.io.mailboxInitWrites==5
            && !f.context.io.mailboxRestoreWrites && !f.context.io.serviceStops && owner
            && owner->fleetPrestack[7].mailboxInitAttempts!=owner->fleetPrestack[7].mailboxInitConfirmed
            && !owner->fleetPrestack[7].preReleaseRestoreAttempted && f.context.dispatchOwnerRetained
            && sColdRecoveryRequired && gRetains==1);
        assert(acquire_sem(sLock)==B_OK && owner->views.Close() && release_sem(sLock)==B_OK);
        delete owner;f.context.qwen35RoleStageQualification=nullptr;
    }
    gPerCoreRoleWindows=false;gAllowWrite=false;sColdRecoveryRequired=false;
    for (auto& windows:gRoleTileWindows) windows.clear();
    }
    {
    Fixture f;assert(f.Run()==B_OK && f.request.operation_status==B_OK);
    ResetMaps();gAllowWrite=true;gPerCoreRoleWindows=true;
    gRoleWindowCore=gRoleSelectedCore=f.context.io.runtimeCore;
    f.context.io.perCoreReset=true;
    for (unsigned core=0;core<DataflowBarrierWorkers;core++) f.context.io.tileReset[core]=0x47800;
    for (unsigned noc=0;noc<2;noc++) f.context.io.nocAvailable[noc]=0x10101010U;
    f.context.io.simulateService=true;f.context.io.dropPrestackReleaseCore=7;
    auto r=Request();r.version=6;
    assert(Qwen35RoleStageControl(&f.cookie,&r,sizeof(r))==B_OK);
    assert(r.operation_status==B_DEV_NOT_READY && r.stage==9 && r.captured && r.staged
        && !r.restored && !r.mappings_closed && f.context.io.sharedResetWrites==8
        && f.context.io.serviceStarts==7 && !f.context.io.serviceStops);
    auto* owner=f.context.qwen35RoleStageQualification;
    assert(owner && owner->fleetPrestack[7].releaseAttempted
        && !owner->fleetPrestack[7].serviceStarted && !owner->fleetPrestack[8].attempted
        && f.context.dispatchOwnerRetained && sColdRecoveryRequired && gRetains==1);
    // Free only the fake test state. A real uncertain release stays retained.
    assert(acquire_sem(sLock)==B_OK && owner->views.Close() && release_sem(sLock)==B_OK);
    delete owner;f.context.qwen35RoleStageQualification=nullptr;
    gPerCoreRoleWindows=false;gAllowWrite=false;sColdRecoveryRequired=false;
    for (auto& windows:gRoleTileWindows) windows.clear();
    }
    for (unsigned failureKind=0;failureKind<3;failureKind++) {
    Fixture f;assert(f.Run()==B_OK && f.request.operation_status==B_OK);
    ResetMaps();gAllowWrite=true;gPerCoreRoleWindows=true;
    gRoleWindowCore=gRoleSelectedCore=f.context.io.runtimeCore;
    f.context.io.perCoreReset=true;
    for (unsigned core=0;core<DataflowBarrierWorkers;core++) f.context.io.tileReset[core]=0x47800;
    for (unsigned noc=0;noc<2;noc++) f.context.io.nocAvailable[noc]=0x10101010U;
    f.context.io.simulateService=true;
    if (!failureKind) f.context.io.pauseHook=[](PioneerSessionIO* io) {
            if (io->serviceStarts==DataflowBarrierWorkers) {
                io->roleBoxes[7].started[DataflowRoleWriter]=0;
                io->pauseHook=nullptr;
            }
        };
    else if (failureKind==1) f.context.io.pauseHook=[](PioneerSessionIO* io) {
            if (io->serviceStarts==DataflowBarrierWorkers) {
                io->fleetRolePc[0][DataflowRoleReader]=0;
                io->pauseHook=nullptr;
            }
        };
    else f.context.io.pauseHook=[](PioneerSessionIO* io) {
            if (io->serviceStarts==DataflowBarrierWorkers) {
                io->fleetRolePc[0][DataflowRoleReader]=DataflowServiceProbeReaderShimParkAddress;
                io->pauseHook=nullptr;
            }
        };
    auto r=Request();r.version=6;
    assert(Qwen35RoleStageControl(&f.cookie,&r,sizeof(r))==B_OK);
    assert(r.operation_status==B_DEV_NOT_READY && r.stage==9 && r.captured && r.staged
        && !r.restored && !r.mappings_closed && f.context.io.serviceStarts==DataflowBarrierWorkers
        && !f.context.io.serviceStops && f.context.dispatchOwnerRetained && sColdRecoveryRequired);
    auto* owner=f.context.qwen35RoleStageQualification;
    const unsigned failedCore=failureKind?0:7;
    assert(owner && owner->fleetPrestack[failedCore].serviceStarted && !owner->fleetPrestack[failedCore].serviceStopped
        && owner->fleetPrestack[failedCore].fleetAuditFailure==(failureKind?4U:3U)
        && owner->fleetPrestack[failedCore].fleetAuditDetail==(failureKind?0U:1U<<2));
    assert(acquire_sem(sLock)==B_OK && owner->views.Close() && release_sem(sLock)==B_OK);
    delete owner;f.context.qwen35RoleStageQualification=nullptr;
    gPerCoreRoleWindows=false;gAllowWrite=false;sColdRecoveryRequired=false;
    for (auto& windows:gRoleTileWindows) windows.clear();
    }
    std::puts("PASS 120 resident reader/writer pairs started concurrently and restored; SIMULATED ONLY");
}
