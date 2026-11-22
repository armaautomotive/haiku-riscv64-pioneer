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
    DataflowRoleCodeOwner fCore[DataflowBarrierWorkers];

    template<class IO> static bool AllHeld(IO& io) {
        for (unsigned core=0;core<DataflowBarrierWorkers;core++)
            if (!io.AllRolesHeld(core)) return false;
        return true;
    }
public:
    DataflowRoleFleetCodeOwner()=default;
    DataflowRoleFleetCodeOwner(const DataflowRoleFleetCodeOwner&)=delete;
    DataflowRoleFleetCodeOwner& operator=(const DataflowRoleFleetCodeOwner&)=delete;
    static constexpr unsigned RequiredWords() {
        return DataflowBarrierWorkers*DataflowRoleCodeOwner::RequiredWords();
    }
    bool StagedAndVerified() const { return fPhase==Staged; }
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
        if (fPhase!=Staged) return false;
        if (!AllHeld(io)) { fPhase=Faulted; return false; }
        for (unsigned core=0;core<DataflowBarrierWorkers;core++)
            if (!fCore[core].VerifyStaged(io)) { fPhase=Faulted; return false; }
        if (!AllHeld(io)) { fPhase=Faulted; return false; }
        return true;
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
        if (fPhase!=Faulted && fPhase!=Staged) return false;
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
};
}
#endif
