/* SPDX-License-Identifier: MIT */
#define PIONEER_QWEN35_FABRIC_GUARD_WITNESS 1
#define main qwen35_existing_fleet_owner_test_main
#include "pioneer_qwen35_dataflow_role_fleet_recovery_owner_test.cpp"
#undef main
#include "../PioneerQwen35FabricCoreRecoverySequence.h"

struct SequenceIO : RecoveryIO {
    enum Mode { Normal,ReleaseUncertain,HoldUncertain,Timeout,RevokedAfterRelease } mode=Normal;
    bool allowed=true;unsigned releases=0,holds=0,pauses=0;
    bool FabricRecoveryConversionAllowed() { return allowed; }
    bool ReleaseCoreLocalRestore(unsigned core,bool& attempted) {
        assert(!attempted && held);attempted=true;++releases;held=false;restoreCompletion=true;
        if (mode!=Timeout) restoreHandoff[core]={PIONEER_Q35_ROLE_RESTORE_READER_DONE,
            PIONEER_Q35_ROLE_RESTORE_WRITER_DONE,0};
        if (mode==RevokedAfterRelease) allowed=false;
        return mode!=ReleaseUncertain;
    }
    bool HoldCoreLocalRestore(unsigned core,bool& attempted) {
        assert(!attempted && !held);attempted=true;++holds;
        if (mode==HoldUncertain) return false;
        held=true;restoreCompletedHeld=true;coreRestoreProof=true;
        for (unsigned i=0;i<RamPerCore;i++) ram[core*RamPerCore+i]=retainedRam[core*RamPerCore+i];
        return true;
    }
    bool RestoreCoreHoldObserved(unsigned core) const { return held && core<DataflowBarrierWorkers; }
    void Pause() { ++pauses; }
};

static int FullFleetTest() {
    const uint32_t reader[]={1,2,3},writer[]={4,5},readerShim[]={6,7},writerShim[]={8,9};
    std::vector<uint32_t> code(DataflowRoleFleetRecoveryOwner::CodeWords());
    std::vector<uint32_t> ram(DataflowRoleFleetRecoveryOwner::RamWords());
    SequenceIO io;const auto originalRam=io.ram,originalCode=io.code,originalBoots=io.boots,
        originalShims=io.shims,originalDisable=io.resetDisable;
    DataflowRoleFleetRecoveryOwner owner;
    assert(owner.SelectAutoRestoreMode() && owner.CaptureAllStackless(code.data(),code.size(),ram.data(),ram.size(),io)
        && owner.StageImages(reader,3,writer,2,readerShim,2,writerShim,2,io)
        && owner.PreparePossibleRelease(io) && owner.BeginReleaseAttempt(io));
    io.retainedRam=ram.data();io.held=false;io.autoArmProof=true;
    for (unsigned core=0;core<DataflowBarrierWorkers;core++)
        assert(owner.CaptureStacklessCore(core,io) && owner.MarkStacklessMayRun(core,io)
            && owner.ArmAutoRestoreCore(core,io));
    io.held=true;
    for (unsigned core=0;core<DataflowBarrierWorkers;core++) io.InstallHeldCopies(core);
    assert(owner.BeginFabricCoreRecovery(io));
    for (unsigned core=0;core<DataflowBarrierWorkers;core++) io.ram[core*SequenceIO::RamPerCore]^=1;
    io.restoreReleaseReady=io.retirementProof=true;
    bool releases[DataflowBarrierWorkers]{},holds[DataflowBarrierWorkers]{};
    DataflowFabricCoreRecoverySequence sequence;const unsigned privateReads=io.privateRamReads;
    assert(sequence.Run(owner,io,writerShim,2,readerShim,2,releases,holds,3)
        && owner.CoveredStateRestored() && io.releases==120 && io.holds==120
        && io.privateRamReads==privateReads && owner.VerifyAfterCoreLocalResidentRestore(io)
        && io.ram==originalRam && io.code==originalCode && io.boots==originalBoots
        && io.shims==originalShims && io.resetDisable==originalDisable);
    for (unsigned core=0;core<DataflowBarrierWorkers;core++) assert(releases[core] && holds[core]);
    std::puts("PASS actual fabric role owner sequence restores all 120 simulated cores; NO HARDWARE");
    return 0;
}
int main(int argc,char**) {
    if (argc>1) return FullFleetTest();
    const uint32_t reader[]={1,2,3},writer[]={4,5},readerShim[]={6,7},writerShim[]={8,9};
    for (unsigned scenario=0;scenario<5;scenario++) {
        std::vector<uint32_t> code(DataflowRoleFleetRecoveryOwner::CodeWords());
        std::vector<uint32_t> ram(DataflowRoleFleetRecoveryOwner::RamWords());
        SequenceIO io;const auto originalRam=io.ram,originalCode=io.code,originalBoots=io.boots,
            originalShims=io.shims,originalDisable=io.resetDisable;
        DataflowRoleFleetRecoveryOwner owner;
        assert(owner.SelectAutoRestoreMode() && owner.CaptureAllStackless(code.data(),code.size(),ram.data(),ram.size(),io)
            && owner.StageImages(reader,3,writer,2,readerShim,2,writerShim,2,io)
            && owner.PreparePossibleRelease(io) && owner.BeginReleaseAttempt(io));
        io.retainedRam=ram.data();io.held=false;io.autoArmProof=true;
        for (unsigned core=0;core<2;core++)
            assert(owner.CaptureStacklessCore(core,io) && owner.MarkStacklessMayRun(core,io)
                && owner.ArmAutoRestoreCore(core,io));
        io.held=true;io.InstallHeldCopies(0);io.InstallHeldCopies(1);
        io.autoHeldProof=io.coreRestoreProof=io.autoRetirementProof=true;
        assert(owner.MarkAutoRestoredCore(0,io) && owner.BeginFabricCoreRecovery(io)
            && owner.FabricCoreAlreadyRestored(0) && !owner.FabricCoreAlreadyRestored(1));
        io.ram[SequenceIO::RamPerCore]^=1;io.restoreReleaseReady=io.retirementProof=true;
        io.mode=static_cast<SequenceIO::Mode>(scenario);
        bool releases[DataflowBarrierWorkers]{},holds[DataflowBarrierWorkers]{};
        DataflowFabricCoreRecoverySequence sequence;
        const unsigned privateReads=io.privateRamReads;
        const bool okay=sequence.Run(owner,io,writerShim,2,readerShim,2,releases,holds,3);
        if (!scenario) {
            assert(okay && sequence.CurrentPhase()==DataflowFabricCoreRecoverySequence::Restored
                && owner.CoveredStateRestored() && io.releases==1 && io.holds==1
                && !releases[0] && releases[1] && holds[1] && io.privateRamReads==privateReads
                && owner.VerifyAfterCoreLocalResidentRestore(io) && io.ram==originalRam
                && io.code==originalCode && io.boots==originalBoots && io.shims==originalShims
                && io.resetDisable==originalDisable);
        } else {
            const auto expected=(scenario==1 || scenario==4)?DataflowFabricCoreRecoverySequence::Release
                :scenario==2?DataflowFabricCoreRecoverySequence::Hold:DataflowFabricCoreRecoverySequence::Complete;
            assert(!okay && sequence.CurrentPhase()==DataflowFabricCoreRecoverySequence::Retained
                && sequence.FailureStep()==expected && sequence.FailureCore()==1
                && releases[1] && io.releases==1 && io.privateRamReads==privateReads);
            const unsigned writes=io.writes,issued=io.releases;
            assert(!sequence.Run(owner,io,writerShim,2,readerShim,2,releases,holds,3)
                && io.writes==writes && io.releases==issued);
        }
    }
    std::puts("PASS actual fabric role owner sequence: mixed fleet, restoration and uncertain releases; SIMULATED ONLY");
}
