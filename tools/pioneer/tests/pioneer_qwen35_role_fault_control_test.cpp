/* SPDX-License-Identifier: MIT */
// Actual restricted controller against simulated role receipts and MMIO.
// No live kernel build or hardware is admitted by this qualification.
#define PIONEER_QWEN35_ROLE_PRESTACK_QUALIFY 1
#define PIONEER_QWEN35_ROLE_LOCAL_RESTORE_QUALIFY 1
#define PIONEER_QWEN35_ROLE_SERVICE_QUALIFY 1
#define PIONEER_QWEN35_ROLE_FAULT_QUALIFY 1
#define TEST_ROLE_PRESTACK_CONTROL 1
#include "PioneerQwen35LocalRestoreProbeImages.h"
#include "PioneerQwen35RoleServiceProbeImages.h"
#include "pioneer_qwen35_role_stage_control_test.cpp"

int main()
{
    (void)&EndGuard;(void)&Qwen35DataflowGenerate;
    for (unsigned failure=0;failure<5;failure++) {
        Fixture f;assert(f.Run()==B_OK && f.request.operation_status==B_OK);
        ResetMaps();gAllowWrite=true;gPerCoreRoleWindows=true;
        gRoleWindowCore=gRoleSelectedCore=f.context.io.runtimeCore;
        for (unsigned noc=0;noc<2;noc++) f.context.io.nocAvailable[noc]=0x10101010U;
        f.context.io.roleHandoffs[0]={0x1234,0x5678,0x9abc};
        uint32_t originalMailbox[16]{};
        for (unsigned i=0;i<16;i++) originalMailbox[i]=0x91a20000U+i;
        std::memcpy(&f.context.io.roleBoxes[0],originalMailbox,sizeof(originalMailbox));
        uint32_t originalQueues[DataflowQueueStateBytes/4]{};
        for (unsigned i=0;i<DataflowQueueStateBytes/4;i++) originalQueues[i]=0x57110000U+i;
        std::memcpy(f.context.io.queueBoxes[0],originalQueues,sizeof(originalQueues));
        const uint32_t invocation=f.context.io.faultInvocationWord;
        f.context.io.simulateService=true;
        f.context.io.dropFaultReceipt=failure==1;
        f.context.io.injectFaultNoc=failure==2;
        f.context.io.dropFaultRestore=failure==3;
        f.context.io.dropFaultInit=failure==4;
        auto r=Request();r.version=4;
        assert(Qwen35RoleStageControl(&f.cookie,&r,sizeof(r))==B_OK);
        if (failure==4) assert(!f.context.io.sharedResetWrites && !f.context.io.serviceStarts
            && !f.context.io.faultCommands);
        else assert(f.context.io.sharedResetWrites==1 && f.context.io.serviceStarts==1
            && f.context.io.faultCommands==1);
        if (!failure) {
            assert(r.operation_status==B_OK && r.stage==TT_Q35_ROLE_SERVICE_COMPLETE
                && r.captured && r.staged && r.restored && r.mappings_closed);
            assert(!f.context.qwen35RoleStageQualification && !sColdRecoveryRequired && !gRetains);
            assert(f.context.io.holdWrites==1 && f.context.io.serviceStops==1
                && f.context.io.reset==0x47800 && f.context.io.faultInvocationWord==invocation);
            assert(!std::memcmp(f.context.io.queueBoxes[0],originalQueues,sizeof(originalQueues))
                && !std::memcmp(&f.context.io.roleBoxes[0],originalMailbox,sizeof(originalMailbox)));
            const auto handoff=f.context.io.roleHandoffs[0];
            assert(handoff.readerReady==0x1234 && handoff.writerReady==0x5678 && handoff.release==0x9abc);
            assert(gMemory[DataflowRolePhysicalBriscRam]==gLocalOriginalReader
                && gMemory[DataflowRolePhysicalNcriscRam]==gLocalOriginalWriter);
        } else {
            assert(r.operation_status==B_DEV_NOT_READY && r.stage==9 && r.captured && r.staged
                && !r.restored && !r.mappings_closed);
            const auto* owner=f.context.qwen35RoleStageQualification;
            assert(owner && owner->prestack.faultCaptured && !owner->prestack.faultRestored
                && f.context.dispatchOwnerRetained && sColdRecoveryRequired && gRetains==1);
            if (failure==4) assert(!owner->prestack.faultPrepared
                && !owner->prestack.faultCommandAttempted && owner->prestack.faultWriteAttempts==1);
            else assert(owner->prestack.faultPrepared && owner->prestack.faultCommandAttempted);
            if (failure==1) assert(!owner->prestack.faultSettled && !f.context.io.serviceStops);
            if (failure==2) assert(owner->prestack.faultSettled && !f.context.io.serviceStops);
            if (failure==3) assert(owner->prestack.faultSettled && owner->prestack.serviceStopped
                && owner->prestack.copiesVerified && f.context.io.holdWrites==1);
            assert(acquire_sem(sLock)==B_OK);
            assert(f.context.qwen35RoleStageQualification->views.Close());
            assert(release_sem(sLock)==B_OK);
            delete f.context.qwen35RoleStageQualification;f.context.qwen35RoleStageQualification=nullptr;
        }
        gLocalRestoreFailReader=false;gPerCoreRoleWindows=false;gAllowWrite=false;sColdRecoveryRequired=false;
        for (auto& windows:gRoleTileWindows) windows.clear();
    }
    std::puts("PASS resident role command fault, settlement, stop and exact saved-state restore; SIMULATED ONLY");
}
