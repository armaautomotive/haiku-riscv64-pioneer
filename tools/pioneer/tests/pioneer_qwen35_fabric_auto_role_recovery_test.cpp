/* SPDX-License-Identifier: MIT */
// Exercise the real resident role owner with simulated fabric/role IO.
#define PIONEER_QWEN35_FABRIC_GUARD_WITNESS 1
#define main qwen35_existing_role_recovery_test_main
#include "pioneer_qwen35_dataflow_role_fleet_recovery_owner_test.cpp"
#undef main

struct FabricRecoveryIO : RecoveryIO {
    bool fabricAllowed=true;unsigned checks=0,failAt=0;
    bool FabricRecoveryConversionAllowed() { return fabricAllowed && ++checks!=failAt; }
};
int main() {
    const uint32_t reader[]={1,2,3},writer[]={4,5},readerShim[]={6,7},writerShim[]={8,9};
    for (unsigned scenario=0;scenario<10;scenario++) {
        std::vector<uint32_t> code(DataflowRoleFleetRecoveryOwner::CodeWords());
        std::vector<uint32_t> ram(DataflowRoleFleetRecoveryOwner::RamWords());
        FabricRecoveryIO io;const auto originalRam=io.ram,originalCode=io.code,originalBoots=io.boots,
            originalShims=io.shims,originalDisable=io.resetDisable;
        DataflowRoleFleetRecoveryOwner owner;
        assert(owner.SelectAutoRestoreMode() && owner.CaptureAllStackless(code.data(),code.size(),ram.data(),ram.size(),io)
            && owner.StageImages(reader,3,writer,2,readerShim,2,writerShim,2,io)
            && owner.PreparePossibleRelease(io) && owner.BeginReleaseAttempt(io));
        io.retainedRam=ram.data();io.held=false;io.autoArmProof=true;
        assert(owner.CaptureStacklessCore(0,io) && owner.MarkStacklessMayRun(0,io) && owner.ArmAutoRestoreCore(0,io));
        io.held=true;io.InstallHeldCopies(0);
        if (scenario==1) io.failAt=2;
        if (scenario==2) io.uncapturedProtected=false;
        if (scenario==3) io.wrongCopyPointer=true;
        if (scenario==4) io.resetDisable[5]^=1;
        if (scenario==5) io.code[(DataflowRoleReaderRamCopy-DataflowRoleExtraL1Address)/4]^=1;
        if (scenario==6) {
            io.autoHeldProof=true;io.coreRestoreProof=true;
            assert(owner.MarkAutoRestoredCore(0,io));
        }
        if (scenario==7) io.failAt=1;
        if (scenario==8) io.failAt=3;
        if (scenario==9) io.failAt=4;
        const unsigned before=io.writes,privateReads=io.privateRamReads;
        if (scenario) {
            assert(!owner.BeginFabricCoreRecovery(io) && owner.AutoRestoreMode()
                && !owner.FabricCoreRecoveryActive() && io.writes==before && io.privateRamReads==privateReads);
            assert(owner.FabricCoreRecoveryAttempted()==(scenario!=6 && scenario!=7));
            if (scenario!=6 && scenario!=7) assert(!owner.BeginFabricCoreRecovery(io) && io.writes==before);
            continue;
        }
        io.fabricAllowed=false;
        assert(!owner.BeginFabricCoreRecovery(io) && !owner.FabricCoreRecoveryAttempted() && io.writes==before);
        io.fabricAllowed=true;
        assert(owner.BeginFabricCoreRecovery(io) && owner.FabricCoreRecoveryActive()
            && !owner.AutoRestoreMode() && !owner.BeginFabricCoreRecovery(io));
        io.ram[0]^=1;
        assert(owner.StageCoreLocalRestore(0,writerShim,2,readerShim,2,io)
            && owner.PrepareCoreLocalRestoreHandoff(0,io));
        io.restoreReleaseReady=true;assert(owner.BeginCoreLocalRestoreRelease(0,io));
        io.held=false;io.restoreCompletion=true;
        io.restoreHandoff[0]={PIONEER_Q35_ROLE_RESTORE_READER_DONE,PIONEER_Q35_ROLE_RESTORE_WRITER_DONE,0};
        assert(owner.ConfirmCoreLocalRestoreHandoff(0,io));
        io.held=true;io.restoreCompletedHeld=true;io.coreRestoreProof=true;io.ram[0]=originalRam[0];
        assert(owner.MarkCoreRestoredByCore(0,io) && owner.VerifyCoreLocalRestores(io)
            && owner.RestoreCoreLocalHandoff(0,io));
        io.retirementProof=true;
        assert(owner.RestoreAfterCoreLocalFleetProof(io) && owner.VerifyAfterCoreLocalResidentRestore(io)
            && io.privateRamReads==privateReads && io.ram==originalRam && io.code==originalCode
            && io.boots==originalBoots && io.shims==originalShims && io.resetDisable==originalDisable);
    }
    {
        std::vector<uint32_t> code(DataflowRoleFleetRecoveryOwner::CodeWords());
        std::vector<uint32_t> ram(DataflowRoleFleetRecoveryOwner::RamWords());
        FabricRecoveryIO io;DataflowRoleFleetRecoveryOwner owner;
        assert(owner.SelectAutoRestoreMode() && owner.CaptureAllStackless(code.data(),code.size(),ram.data(),ram.size(),io)
            && owner.StageImages(reader,3,writer,2,readerShim,2,writerShim,2,io)
            && owner.PreparePossibleRelease(io) && owner.BeginReleaseAttempt(io));
        io.retainedRam=ram.data();io.held=false;io.autoArmProof=true;
        for (unsigned core=0;core<DataflowBarrierWorkers;core++)
            assert(owner.CaptureStacklessCore(core,io) && owner.MarkStacklessMayRun(core,io)
                && owner.ArmAutoRestoreCore(core,io));
        io.held=true;
        for (unsigned core=0;core<DataflowBarrierWorkers;core++) io.InstallHeldCopies(core);
        assert(owner.BeginFabricCoreRecovery(io) && owner.FabricCoreRecoveryActive());
        assert(owner.StageCoreLocalRestore(DataflowBarrierWorkers-1,writerShim,2,readerShim,2,io));
        assert(owner.RestorationRequired() && !owner.CoveredStateRestored());
    }
    std::puts("PASS interrupted auto-restore conversion and core-local recovery; SIMULATED ONLY");
}
