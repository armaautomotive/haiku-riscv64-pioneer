/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_QWEN35_DATAFLOW_ROLE_FLEET_CODE_OWNER_H
#define PIONEER_QWEN35_DATAFLOW_ROLE_FLEET_CODE_OWNER_H
#include "PioneerQwen35DataflowRoleCodeOwner.h"
namespace PioneerQwen35 {
// Offline fleet transaction for the additional L1 code region only. IO must
// serialize exclusive reset/mapping control and prove all roles held and the
// relevant NoCs settled. Boot/shims, local RAM, reset, caches, queues and
// original TRISC0 state are separately owned and not covered by this class.
class DataflowRoleFleetCodeOwner {
    enum Phase { Empty, Captured, Faulted, Staged, Restored, Unavailable };
    Phase fPhase=Empty;
    // Sentinel means the ordinary all-core staging path.
    unsigned fStagedCore=DataflowBarrierWorkers;
    bool fCopyAwareMode=false;
    bool fCopyAwareVerified=false;
    bool fCopyRetirementStarted=false;
    const uint32_t* fRetirementReader[DataflowBarrierWorkers]{};
    const uint32_t* fRetirementWriter[DataflowBarrierWorkers]{};
    bool fRetirementCaptured[DataflowBarrierWorkers]{};
    DataflowRoleCodeOwner fCore[DataflowBarrierWorkers];

    template<class IO> static bool AllHeld(IO& io) {
        for (unsigned core=0;core<DataflowBarrierWorkers;core++)
            if (!io.AllRolesHeld(core)) return false;
        return true;
    }
    template<class IO> bool CheckRetirementSources(IO& io,bool capture) {
        for (unsigned core=0;core<DataflowBarrierWorkers;core++) {
            const uint32_t* reader=nullptr;const uint32_t* writer=nullptr;bool captured=false;
            if (!io.RetirementRamSnapshotSources(core,reader,writer,captured)
                || (captured?(!reader || !writer):(reader || writer))
                || captured!=fCore[core].CopyAwareOwnership()) return false;
            if (capture) {
                fRetirementReader[core]=reader;fRetirementWriter[core]=writer;fRetirementCaptured[core]=captured;
            } else if (reader!=fRetirementReader[core] || writer!=fRetirementWriter[core]
                || captured!=fRetirementCaptured[core]) return false;
        }
        return true;
    }
public:
    DataflowRoleFleetCodeOwner()=default;
    DataflowRoleFleetCodeOwner(const DataflowRoleFleetCodeOwner&)=delete;
    DataflowRoleFleetCodeOwner& operator=(const DataflowRoleFleetCodeOwner&)=delete;
    static constexpr unsigned RequiredWords() {
        return DataflowBarrierWorkers*DataflowRoleCodeOwner::RequiredWords();
    }
    bool StagedAndVerified() const { return fPhase==Staged && !fCopyAwareMode; }
    bool CopyAwareStagedAndVerified() const { return fPhase==Staged && fCopyAwareMode && fCopyAwareVerified; }
    // Retain all possible L1 copies before releasing even the first role.
    bool BeginCopyAwareOwnership() {
        if (fPhase!=Staged || fCopyAwareMode) return false;
        fCopyAwareMode=true;fCopyAwareVerified=false;return true;
    }
    bool RestorationRequired() const { return fPhase==Faulted || fPhase==Staged; }
    bool RegionRestored() const { return fPhase==Restored; }

    template<class IO> bool CaptureAll(uint32_t* storage,unsigned words,IO& io) {
        if (fPhase!=Empty || !storage || words!=RequiredWords()) return false;
        if (!AllHeld(io)) { fPhase=Unavailable; return false; }
        const unsigned perCore=DataflowRoleCodeOwner::RequiredWords();
        for (unsigned core=0;core<DataflowBarrierWorkers;core++) {
            if (!fCore[core].Capture(core,storage+core*perCore,perCore,io)) {
                fPhase=Unavailable; return false;
            }
        }
        if (!AllHeld(io)) { fPhase=Unavailable; return false; }
        fPhase=Captured;
        return true;
    }
    template<class IO> bool StageAll(const uint32_t* reader,unsigned readerWords,
        const uint32_t* writer,unsigned writerWords,IO& io) {
        if (fPhase!=Captured || !reader || !readerWords || !writer || !writerWords) return false;
        if (!AllHeld(io)) { fPhase=Faulted; return false; }
        // From the first attempted stage, retain every snapshot until verified
        // restoration; a failed transfer may have partially written a core.
        fPhase=Faulted;
        for (unsigned core=0;core<DataflowBarrierWorkers;core++) {
            if (!fCore[core].Stage(true,reader,readerWords,io)
                || !fCore[core].Stage(false,writer,writerWords,io)) return false;
        }
        // Recheck the entire fleet after all writes so a later misrouted write
        // cannot silently corrupt an earlier core's verified image.
        for (unsigned core=0;core<DataflowBarrierWorkers;core++)
            if (!fCore[core].VerifyStaged(io)) return false;
        if (!AllHeld(io)) return false;
        fPhase=Staged;
        return true;
    }
    template<class IO> bool VerifyStagedAll(IO& io) {
        if (fPhase!=Staged || fCopyAwareMode) return false;
        if (!AllHeld(io)) { fPhase=Faulted; return false; }
        for (unsigned core=0;core<DataflowBarrierWorkers;core++)
            if (!(fStagedCore==DataflowBarrierWorkers || core==fStagedCore
                    ?fCore[core].VerifyStaged(io):fCore[core].VerifyOriginal(io)))
                { fPhase=Faulted; return false; }
        if (!AllHeld(io)) { fPhase=Faulted; return false; }
        return true;
    }
    // For post-start restoration, IO must provide an independently owned,
    // immutable pair of snapshots for each captured core, and must prove that
    // its capture ledger and all source/destination writers stay excluded for
    // the whole fleet audit. Uncaptured cores still match the original L1.
    // This does not authorize an image write or a processor release.
    template<class IO> bool VerifyStagedAllWithCopies(IO& io) {
        if (fPhase!=Staged || !fCopyAwareMode) return false;
        fCopyAwareVerified=false;
        if (!AllHeld(io)) { fPhase=Faulted; return false; }
        for (unsigned core=0;core<DataflowBarrierWorkers;core++) {
            const uint32_t* reader=nullptr; const uint32_t* writer=nullptr; bool captured=false;
            if (!io.StacklessHeldCopySources(core,reader,writer,captured)) { fPhase=Faulted; return false; }
            const bool staged=fStagedCore==DataflowBarrierWorkers || core==fStagedCore;
            if (captured) {
                if (!staged || !reader || !writer
                    || (!fCore[core].CopyAwareOwnership() && !fCore[core].BeginCopyAwareOwnership())
                    || !fCore[core].VerifyStagedWithCopies(reader,writer,io)) {
                    fPhase=Faulted; return false;
                }
            } else if (reader || writer || !(staged?fCore[core].VerifyStaged(io):fCore[core].VerifyOriginal(io))) {
                fPhase=Faulted; return false;
            }
        }
        if (!AllHeld(io)) { fPhase=Faulted; return false; }
        fCopyAwareVerified=true;
        return true;
    }
    // Restricted single-core qualification still captures and audits every
    // tile, so a misrouted write cannot silently damage an untouched worker.
    template<class IO> bool StageOne(unsigned core,const uint32_t* reader,unsigned readerWords,
        const uint32_t* writer,unsigned writerWords,IO& io) {
        if (fPhase!=Captured || core>=DataflowBarrierWorkers || !reader || !readerWords
            || !writer || !writerWords) return false;
        fPhase=Faulted;fStagedCore=core;
        if (!AllHeld(io) || !fCore[core].Stage(true,reader,readerWords,io)
            || !fCore[core].Stage(false,writer,writerWords,io)) return false;
        for (unsigned i=0;i<DataflowBarrierWorkers;i++)
            if (!(i==core?fCore[i].VerifyStaged(io):fCore[i].VerifyOriginal(io))) return false;
        if (!AllHeld(io)) return false;
        fPhase=Staged;return true;
    }
    template<class IO> bool VerifyOriginalAll(IO& io) {
        if (fPhase!=Captured && fPhase!=Restored) return false;
        if (!AllHeld(io)) { fPhase=Faulted; return false; }
        for (unsigned core=0;core<DataflowBarrierWorkers;core++)
            if (!fCore[core].VerifyOriginal(io)) { fPhase=Faulted; return false; }
        if (!AllHeld(io)) { fPhase=Faulted; return false; }
        return true;
    }
    template<class IO> bool RestoreAll(IO& io) {
        // The ordinary L1 restore would erase retained private-RAM copies.
        // A future, separately proved local restore must retire them first.
        if (fCopyAwareMode || (fPhase!=Faulted && fPhase!=Staged)) return false;
        fPhase=Faulted;
        if (!AllHeld(io)) return false;
        for (unsigned core=0;core<DataflowBarrierWorkers;core++)
            if (fCore[core].NeedsRestore() && !fCore[core].Restore(io)) return false;
        // Verify even cores we did not intentionally write: a misrouted
        // transfer may have damaged one of their captured original regions.
        for (unsigned core=0;core<DataflowBarrierWorkers;core++)
            if (!fCore[core].VerifyOriginal(io)) return false;
        if (!AllHeld(io)) return false;
        fPhase=Restored;
        return true;
    }
    // IO must independently prove that all captured private RAM has already
    // been restored, every uncaptured core remains protected, retained host
    // snapshots stay immutable, and these L1 writes cannot target private RAM
    // or another tile. The proof spans the entire operation, including the
    // destruction of the L1 copies. A retry additionally requires drainage of
    // all earlier uncertain writes. No live scope currently grants this.
    template<class IO> bool RestoreAfterFleetRamProof(IO& io) {
        if (!fCopyAwareMode || (fPhase!=Staged && fPhase!=Faulted)
            || !io.FleetRamImageRetirementSafe() || !AllHeld(io)) return false;
        if (!fCopyRetirementStarted) {
            if (fPhase!=Staged || !VerifyStagedAllWithCopies(io) || !CheckRetirementSources(io,true)) return false;
        } else if (!io.FleetRamImageRetirementRetrySafe() || !CheckRetirementSources(io,false)) return false;
        fCopyRetirementStarted=true;fCopyAwareVerified=false;fPhase=Faulted;
        for (unsigned core=0;core<DataflowBarrierWorkers;core++) {
            if (!io.FleetRamImageRetirementSafe() || !AllHeld(io)) return false;
            if (fCore[core].NeedsRestore()) {
                if (fRetirementCaptured[core]) {
                    if (!fCore[core].RestoreAfterCoreRamProof(fRetirementReader[core],fRetirementWriter[core],io))
                        return false;
                } else if (!fCore[core].Restore(io)) return false;
            } else if (!fCore[core].VerifyOriginal(io)) return false;
            if (!io.FleetRamImageRetirementSafe()) return false;
        }
        for (unsigned core=0;core<DataflowBarrierWorkers;core++)
            if (!fCore[core].VerifyOriginal(io)) return false;
        if (!io.FleetRamImageRetirementSafe() || !CheckRetirementSources(io,false) || !AllHeld(io)) return false;
        fPhase=Restored;return true;
    }
};
}
#endif
