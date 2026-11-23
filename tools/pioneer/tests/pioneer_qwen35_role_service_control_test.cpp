/* SPDX-License-Identifier: MIT */
// The actual restricted service control and PCI adapter over simulated MMIO.
// Its fake models role receipts; linked machine code is tested separately.
#define PIONEER_QWEN35_ROLE_PRESTACK_QUALIFY 1
#define PIONEER_QWEN35_ROLE_LOCAL_RESTORE_QUALIFY 1
#define PIONEER_QWEN35_ROLE_SERVICE_QUALIFY 1
#define TEST_ROLE_PRESTACK_CONTROL 1
#include "PioneerQwen35LocalRestoreProbeImages.h"
#include "PioneerQwen35RoleServiceProbeImages.h"
#include "pioneer_qwen35_role_stage_control_test.cpp"

int main()
{
    (void)&EndGuard;(void)&Qwen35DataflowGenerate;
    for (unsigned version=1;version<=3;version++) {
        Fixture f;auto r=Request();r.version=version;
        assert(Qwen35RoleStageControl(&f.cookie,&r,sizeof(r))==B_BAD_VALUE
            && !f.context.qwen35RoleStageAttempted);
    }
    for (unsigned failure=0;failure<2;failure++) {
        Fixture f;assert(f.Run()==B_OK && f.request.operation_status==B_OK);
        ResetMaps();gAllowWrite=true;gPerCoreRoleWindows=true;
        gRoleWindowCore=gRoleSelectedCore=f.context.io.runtimeCore;
        for (unsigned noc=0;noc<2;noc++) f.context.io.nocAvailable[noc]=0x10101010U;
        f.context.io.roleHandoffs[0]={0x1234,0x5678,0x9abc};
        uint32_t original[16]{};
        for (unsigned i=0;i<16;i++) original[i]=0x91a20000U+i;
        std::memcpy(&f.context.io.roleBoxes[0],original,sizeof(original));
        f.context.io.simulateService=true;f.context.io.dropServiceStop=failure!=0;
        auto r=Request();r.version=4;
        assert(Qwen35RoleStageControl(&f.cookie,&r,sizeof(r))==B_OK);
        assert(f.context.io.sharedResetWrites==1 && f.context.io.serviceStarts==1
            && f.context.io.serviceStops==1);
        if (!failure) {
            assert(r.operation_status==B_OK && r.stage==TT_Q35_ROLE_SERVICE_COMPLETE
                && r.captured && r.staged && r.restored && r.mappings_closed);
            assert(!f.context.qwen35RoleStageQualification && !sColdRecoveryRequired && !gRetains);
            assert(f.context.io.holdWrites==1 && f.context.io.reset==0x47800
                && f.context.io.localRestoreMutations==1
                && gMemory[DataflowRolePhysicalBriscRam]==gLocalOriginalReader
                && gMemory[DataflowRolePhysicalNcriscRam]==gLocalOriginalWriter);
            assert(!std::memcmp(&f.context.io.roleBoxes[0],original,sizeof(original)));
            const auto handoff=f.context.io.roleHandoffs[0];
            assert(handoff.readerReady==0x1234 && handoff.writerReady==0x5678 && handoff.release==0x9abc);
            assert(gMemory[DataflowRolePhysicalControl][0x224/4]==0x76540006U);
            assert(f.context.io.SelectRuntimeCore((f.context.io.runtimeCore+1)%DataflowBarrierWorkers));
            for (unsigned core=0;core<DataflowBarrierWorkers;core++)
                for (unsigned which=0;which<=DataflowRolePhysicalExtraCode;which++)
                    for (uint32_t word:gRoleTileWindows[core][which]) assert(word==RoleWindowInitial(core,which));
        } else {
            assert(r.operation_status==B_DEV_NOT_READY && r.stage==9 && r.captured && r.staged
                && !r.restored && !r.mappings_closed);
            const auto* owner=f.context.qwen35RoleStageQualification;
            assert(owner && owner->prestack.copiesCaptured && owner->prestack.serviceStarted
                && owner->prestack.serviceStopAttempted && !owner->prestack.serviceStopped
                && !owner->prestack.holdAttempted && f.context.dispatchOwnerRetained
                && sColdRecoveryRequired && gRetains==1);
            assert(acquire_sem(sLock)==B_OK);
            assert(f.context.qwen35RoleStageQualification->views.Close());
            assert(release_sem(sLock)==B_OK);
            delete f.context.qwen35RoleStageQualification;f.context.qwen35RoleStageQualification=nullptr;
        }
        gLocalRestoreFailReader=false;gPerCoreRoleWindows=false;gAllowWrite=false;sColdRecoveryRequired=false;
        for (auto& windows:gRoleTileWindows) windows.clear();
    }
    std::puts("PASS resident role startup, stop, local restore and failure retention; SIMULATED ONLY");
}
