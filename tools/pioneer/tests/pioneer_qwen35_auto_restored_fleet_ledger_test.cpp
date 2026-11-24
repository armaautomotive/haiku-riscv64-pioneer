/* SPDX-License-Identifier: MIT */
// Exercise the real all-core restoration ledger using simulated role IO.
#define main qwen35_existing_role_recovery_test_main
#include "pioneer_qwen35_dataflow_role_fleet_recovery_owner_test.cpp"
#undef main
int main() {
    std::vector<uint32_t> code(DataflowRoleFleetRecoveryOwner::CodeWords());
    std::vector<uint32_t> ram(DataflowRoleFleetRecoveryOwner::RamWords());
    const uint32_t reader[]={1,2,3},writer[]={4,5},readerShim[]={6,7},writerShim[]={8,9};
    RecoveryIO io;DataflowRoleFleetRecoveryOwner owner;
    assert(!owner.AutoRestoredFleetLedgerComplete());
    assert(owner.SelectAutoRestoreMode()
        && owner.CaptureAllStackless(code.data(),code.size(),ram.data(),ram.size(),io)
        && owner.StageImages(reader,3,writer,2,readerShim,2,writerShim,2,io)
        && owner.PreparePossibleRelease(io) && owner.BeginReleaseAttempt(io));
    io.retainedRam=ram.data();io.held=false;io.autoArmProof=true;
    for (unsigned core=0;core<DataflowBarrierWorkers;core++)
        assert(owner.CaptureStacklessCore(core,io) && owner.MarkStacklessMayRun(core,io)
            && owner.ArmAutoRestoreCore(core,io));
    assert(!owner.AutoRestoredFleetLedgerComplete());
    io.held=true;io.autoHeldProof=true;io.coreRestoreProof=true;
    for (unsigned core=0;core<DataflowBarrierWorkers;core++) io.InstallHeldCopies(core);
    for (unsigned core=0;core<DataflowBarrierWorkers;core++) assert(owner.MarkAutoRestoredCore(core,io));
    assert(!owner.AutoRestoredFleetLedgerComplete()); // images still need restoration
    io.retirementProof=true;io.autoRetirementProof=true;
    assert(owner.RestoreAfterAutoRestoreFleetProof(io) && owner.AutoRestoredFleetLedgerComplete());
    assert(owner.VerifyAfterAutoResidentRestore(io) && owner.AutoRestoredFleetLedgerComplete());
    io.autoRetirementProof=false;
    assert(!owner.VerifyAfterAutoResidentRestore(io) && !owner.AutoRestoredFleetLedgerComplete());
    std::puts("PASS all 120 restored-role ledger and failed re-audit; SIMULATED ONLY");
}
