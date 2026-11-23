/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_QWEN35_DATAFLOW_ROLE_RESTORE_HANDOFF_OWNER_H
#define PIONEER_QWEN35_DATAFLOW_ROLE_RESTORE_HANDOFF_OWNER_H
#include "PioneerQwen35DataflowRoleMailbox.h"
namespace PioneerQwen35 {
// One held core's restore receipt transaction. IO must independently prove
// exclusive ownership of the handoff and all competing writers throughout
// each operation. A receipt or a sampled PC alone is not such a proof.
// This owner never releases a processor or accesses B/NC private RAM.
class DataflowRoleRestoreHandoffOwner {
    enum Phase { Empty, Captured, Clearing, Armed, ReleaseAttempted, Completed, Restoring, Restored };
    Phase fPhase=Empty;
    unsigned fCore=DataflowBarrierWorkers;
    DataflowRoleSnapshotHandoff fOriginal{};
    unsigned fClearAttempts=0,fRestoreAttempts=0;
    bool fHasOriginal=false,fReleaseAttempted=false;
    static bool Same(const DataflowRoleSnapshotHandoff& a,const DataflowRoleSnapshotHandoff& b) {
        return a.readerReady==b.readerReady && a.writerReady==b.writerReady && a.release==b.release;
    }
    template<class IO> bool ReadHeld(IO& io,DataflowRoleSnapshotHandoff& result) {
        DataflowRoleSnapshotHandoff first{},second{};
        if (!io.RestoreHandoffHeldSafe(fCore) || !io.ReadRestoreHandoff(fCore,first)
            || !io.ReadRestoreHandoff(fCore,second) || !Same(first,second)
            || !io.RestoreHandoffHeldSafe(fCore)) return false;
        result=second;return true;
    }
public:
    DataflowRoleRestoreHandoffOwner()=default;
    DataflowRoleRestoreHandoffOwner(const DataflowRoleRestoreHandoffOwner&)=delete;
    DataflowRoleRestoreHandoffOwner& operator=(const DataflowRoleRestoreHandoffOwner&)=delete;
    bool Untouched() const { return fPhase==Empty || fPhase==Captured; }
    bool ArmedForRelease() const { return fPhase==Armed; }
    bool CompletionConfirmed() const { return fPhase==Completed; }
    bool ReleaseWasAttempted() const { return fReleaseAttempted; }
    unsigned ClearAttempts() const { return fClearAttempts; }
    unsigned RestoreAttempts() const { return fRestoreAttempts; }
    bool RestorationRequired() const { return fPhase!=Empty && fPhase!=Captured && fPhase!=Restored; }
    bool HandoffRestored() const { return fPhase==Restored; }

    template<class IO> bool Capture(unsigned core,IO& io) {
        if (fPhase!=Empty || core>=DataflowBarrierWorkers) return false;
        fCore=core;
        if (!ReadHeld(io,fOriginal)) { fCore=DataflowBarrierWorkers;return false; }
        fHasOriginal=true;fPhase=Captured;return true;
    }
    template<class IO> bool ClearForRelease(IO& io) {
        if (fPhase!=Captured || !fHasOriginal) return false;
        DataflowRoleSnapshotHandoff observed{};
        if (!ReadHeld(io,observed) || !Same(observed,fOriginal)) return false;
        fPhase=Clearing;
        for (unsigned word=0;word<3;word++) {
            if (!io.RestoreHandoffHeldSafe(fCore)) return false;
            fClearAttempts|=1U<<word; // set before the possibly uncertain write
            if (!io.WriteRestoreHandoffWord(fCore,word,0)) return false;
        }
        if (!ReadHeld(io,observed) || observed.readerReady || observed.writerReady || observed.release)
            return false;
        fPhase=Armed;return true;
    }
    // Caller records this and its separate fleet-release ledger before a
    // physical release attempt. A failed or uncertain release is not retried.
    template<class IO> bool BeginReleaseAttempt(IO& io) {
        if (fPhase!=Armed) return false;
        DataflowRoleSnapshotHandoff observed{};
        if (!ReadHeld(io,observed) || observed.readerReady || observed.writerReady || observed.release)
            return false;
        fReleaseAttempted=true;fPhase=ReleaseAttempted;return true;
    }
    // CompletionSafe must establish the exact released shims, their parked
    // state, receipt freshness and exclusion of all other handoff writers.
    template<class IO> bool ConfirmCompletion(IO& io) {
        if (fPhase!=ReleaseAttempted || !io.RestoreCompletionSafe(fCore)) return false;
        DataflowRoleSnapshotHandoff first{},second{};
        if (!io.ReadRestoreHandoff(fCore,first) || !io.ReadRestoreHandoff(fCore,second)
            || !Same(first,second) || first.readerReady!=PIONEER_Q35_ROLE_RESTORE_READER_DONE
            || first.writerReady!=PIONEER_Q35_ROLE_RESTORE_WRITER_DONE || first.release
            || !io.RestoreCompletionSafe(fCore)) return false;
        fPhase=Completed;return true;
    }
    // Before a release attempt, captured handoff can be restored after an
    // interrupted clear. After one, only proved completion permits cleanup.
    // HeldSafe must also drain every earlier host write. After release, IO
    // must separately preserve the completed-core proof while the receipt
    // is overwritten. A retry after an uncertain write additionally needs
    // independent drainage, route, fleet protection and writer-exclusion
    // proof; its repeated writes are idempotent saved handoff values only.
    template<class IO> bool RestoreOriginal(IO& io) {
        if (!fHasOriginal || (fPhase!=Clearing && fPhase!=Armed && fPhase!=Completed && fPhase!=Restoring)
            || !io.RestoreHandoffHeldSafe(fCore)
            || (fReleaseAttempted && !io.RestoreCompletedAndHeld(fCore))) return false;
        const bool retry=fPhase==Restoring || (fPhase==Clearing && fClearAttempts);
        if (retry && !io.RestoreHandoffRetrySafe(fCore)) return false;
        fPhase=Restoring;
        const uint32_t original[3]={fOriginal.readerReady,fOriginal.writerReady,fOriginal.release};
        for (unsigned word=0;word<3;word++) {
            if (!io.RestoreHandoffHeldSafe(fCore)) return false;
            fRestoreAttempts|=1U<<word;
            if (!io.WriteRestoreHandoffWord(fCore,word,original[word])) return false;
        }
        DataflowRoleSnapshotHandoff observed{};
        if (!ReadHeld(io,observed) || !Same(observed,fOriginal)
            || (fReleaseAttempted && !io.RestoreCompletedAndHeld(fCore))
            || (retry && !io.RestoreHandoffRetrySafe(fCore))) return false;
        fPhase=Restored;return true;
    }
};
}
#endif
