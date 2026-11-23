/* SPDX-License-Identifier: MIT */
// Exercise the actual restricted controller and PCI adapter with fake Haiku
// MMIO. The fake release models the two pre-stack shims; this is not silicon.
#define PIONEER_QWEN35_ROLE_PRESTACK_QUALIFY 1
#define TEST_ROLE_PRESTACK_CONTROL 1
#include "pioneer_qwen35_role_stage_control_test.cpp"

int main()
{
    (void)&EndGuard;
    (void)&Qwen35DataflowGenerate;
    {
        Fixture f;auto r=Request();r.version=3;
        assert(Qwen35RoleStageControl(&f.cookie,&r,sizeof(r))==B_BAD_VALUE
            && !f.context.qwen35RoleStageAttempted);
    }
    {
        Fixture f;assert(f.Run()==B_OK && f.request.operation_status==B_OK);
        ResetMaps();gAllowWrite=true;gPerCoreRoleWindows=true;
        gRoleWindowCore=gRoleSelectedCore=f.context.io.runtimeCore;
        for (unsigned noc=0;noc<2;noc++) f.context.io.nocAvailable[noc]=0x10101010U;
        f.context.io.roleHandoffs[0]={0x1234,0x5678,0x9abc};
        auto r=Request();r.version=2;
        assert(Qwen35RoleStageControl(&f.cookie,&r,sizeof(r))==B_OK
            && r.operation_status==B_OK && r.stage==TT_Q35_ROLE_PRESTACK_COMPLETE
            && r.captured && r.staged && r.restored && r.mappings_closed);
        assert(!f.context.qwen35RoleStageQualification && !sColdRecoveryRequired && !gRetains);
        assert(f.context.io.sharedResetWrites==1 && f.context.io.holdWrites==1
            && f.context.io.reset==0x47800);
        const auto handoff=f.context.io.roleHandoffs[0];
        assert(handoff.readerReady==0x1234 && handoff.writerReady==0x5678 && handoff.release==0x9abc);
        assert(gMemory[DataflowRolePhysicalControl][0x224/4]==0x76540006U);
        assert(f.context.io.SelectRuntimeCore((f.context.io.runtimeCore+1)%DataflowBarrierWorkers));
        for (unsigned core=0;core<DataflowBarrierWorkers;core++)
            for (unsigned which=0;which<=DataflowRolePhysicalExtraCode;which++)
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
        gPrestackCorruptCopy=true;
        auto r=Request();r.version=2;
        assert(Qwen35RoleStageControl(&f.cookie,&r,sizeof(r))==B_OK
            && r.operation_status==B_DEV_NOT_READY && r.stage==9 && r.captured && r.staged
            && !r.restored && !r.mappings_closed);
        const auto* owner=f.context.qwen35RoleStageQualification;
        assert(owner && owner->prestack.releaseAttempted && owner->prestack.cancelAttempted
            && owner->prestack.copiesCaptured && owner->prestack.holdAttempted && !owner->prestack.copiesVerified
            && !owner->prestack.handoffRestoreAttempts && !owner->prestack.resetRestoreAttempted
            && f.context.dispatchOwnerRetained && sColdRecoveryRequired && gRetains==1);
        const unsigned reader=(DataflowRoleReaderRamCopy-DataflowRoleExtraL1Address)/4;
        assert(owner->prestack.copies[DataflowRoleReader][0]==gMemory[DataflowRolePhysicalBriscRam][0]
            && owner->prestack.copies[DataflowRoleReader][0]!=gMemory[DataflowRolePhysicalExtraCode][reader]);
        assert(f.context.io.sharedResetWrites==1 && f.context.io.holdWrites==1);
        assert(acquire_sem(sLock)==B_OK);
        assert(f.context.qwen35RoleStageQualification->views.Close());
        assert(release_sem(sLock)==B_OK);
        delete f.context.qwen35RoleStageQualification;f.context.qwen35RoleStageQualification=nullptr;
        gPrestackCorruptCopy=false;gPerCoreRoleWindows=false;gAllowWrite=false;sColdRecoveryRequired=false;
        for (auto& windows:gRoleTileWindows) windows.clear();
    }
    {
        Fixture f;assert(f.Run()==B_OK && f.request.operation_status==B_OK);
        ResetMaps();gAllowWrite=true;gPerCoreRoleWindows=true;
        gRoleWindowCore=gRoleSelectedCore=f.context.io.runtimeCore;
        for (unsigned noc=0;noc<2;noc++) f.context.io.nocAvailable[noc]=0x10101010U;
        f.context.io.dropPrestackRelease=true;
        auto r=Request();r.version=2;
        assert(Qwen35RoleStageControl(&f.cookie,&r,sizeof(r))==B_OK
            && r.operation_status==B_DEV_NOT_READY && r.stage==9 && r.captured && r.staged
            && !r.restored && !r.mappings_closed);
        const auto* owner=f.context.qwen35RoleStageQualification;
        assert(owner && owner->prestack.attempted && owner->prestack.captured
            && owner->prestack.preserveAttempted && owner->prestack.releaseAttempted
            && !owner->prestack.cancelAttempted && !owner->prestack.holdAttempted
            && !owner->prestack.complete && f.context.io.sharedResetWrites==1
            && f.context.dispatchOwnerRetained && sColdRecoveryRequired && gRetains==1);
        // Fake shutdown only; a real unconfirmed release remains quarantined.
        assert(acquire_sem(sLock)==B_OK);
        assert(f.context.qwen35RoleStageQualification->views.Close());
        assert(release_sem(sLock)==B_OK);
        delete f.context.qwen35RoleStageQualification;f.context.qwen35RoleStageQualification=nullptr;
        gPerCoreRoleWindows=false;gAllowWrite=false;sColdRecoveryRequired=false;
        for (auto& windows:gRoleTileWindows) windows.clear();
    }
    std::puts("PASS actual pre-stack control: B/NC-only release, L1 copy audit, cancel, restoration and retained faults; SIMULATED ONLY");
}
