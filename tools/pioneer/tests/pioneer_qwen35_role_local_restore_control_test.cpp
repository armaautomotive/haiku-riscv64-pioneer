/* SPDX-License-Identifier: MIT */
// Actual restricted control and PCI adapter over simulated per-core MMIO.
// The separate RV32 test runs the exact machine code; this fake models its
// receipt and mutation effects only. No accelerator is accessed.
#define PIONEER_QWEN35_ROLE_PRESTACK_QUALIFY 1
#define PIONEER_QWEN35_ROLE_LOCAL_RESTORE_QUALIFY 1
#define TEST_ROLE_PRESTACK_CONTROL 1
#include "PioneerQwen35LocalRestoreProbeImages.h"
#include "pioneer_qwen35_role_stage_control_test.cpp"

int main()
{
    (void)&EndGuard;
    (void)&Qwen35DataflowGenerate;
    for (unsigned version=1;version<=2;version++) {
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
        f.context.io.simulateLocalRestore=true;
        gLocalRestoreFailReader=failure!=0;
        auto r=Request();r.version=3;
        assert(Qwen35RoleStageControl(&f.cookie,&r,sizeof(r))==B_OK);
        assert(f.context.io.sharedResetWrites==1 && f.context.io.localRestoreMutations==1);
        if (!failure) {
            assert(r.operation_status==B_OK && r.stage==TT_Q35_ROLE_LOCAL_RESTORE_COMPLETE
                && r.captured && r.staged && r.restored && r.mappings_closed);
            assert(!f.context.qwen35RoleStageQualification && !sColdRecoveryRequired && !gRetains);
            assert(f.context.io.holdWrites==1 && f.context.io.reset==0x47800
                && gLocalOriginalReader.size()==DataflowRoleLocalRamWords
                && gMemory[DataflowRolePhysicalBriscRam]==gLocalOriginalReader
                && gMemory[DataflowRolePhysicalNcriscRam]==gLocalOriginalWriter);
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
            assert(owner && owner->prestack.copiesCaptured && owner->prestack.cancelAttempted
                && !owner->prestack.holdAttempted && f.context.dispatchOwnerRetained
                && sColdRecoveryRequired && gRetains==1);
            assert(gMemory[DataflowRolePhysicalBriscRam][0]!=gLocalOriginalReader[0]);
            // Test-only cleanup after a simulated external reset.
            assert(acquire_sem(sLock)==B_OK);
            assert(f.context.qwen35RoleStageQualification->views.Close());
            assert(release_sem(sLock)==B_OK);
            delete f.context.qwen35RoleStageQualification;f.context.qwen35RoleStageQualification=nullptr;
        }
        gLocalRestoreFailReader=false;gPerCoreRoleWindows=false;gAllowWrite=false;sColdRecoveryRequired=false;
        for (auto& windows:gRoleTileWindows) windows.clear();
    }
    std::puts("PASS local restore control, saved-image cleanup and retained restore failure; SIMULATED ONLY");
}
