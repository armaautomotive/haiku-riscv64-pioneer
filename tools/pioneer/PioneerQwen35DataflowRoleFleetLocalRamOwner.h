/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_QWEN35_DATAFLOW_ROLE_FLEET_LOCAL_RAM_OWNER_H
#define PIONEER_QWEN35_DATAFLOW_ROLE_FLEET_LOCAL_RAM_OWNER_H
#include "PioneerQwen35DataflowRoleLocalRamOwner.h"
namespace PioneerQwen35 {
// Offline fleet ledger for B/NC private RAM only. Capture and mark all 120
// snapshots dirty BEFORE the caller attempts ANY role release. This class
// does not release roles or prove reset, NoC, boot, code or cache safety.
class DataflowRoleFleetLocalRamOwner {
    enum Phase { Empty, Captured, Dirty, Restored, Unavailable };
    Phase fPhase=Empty;
    bool fReleaseLedgerReady=false;
    DataflowRoleLocalRamOwner fCore[DataflowBarrierWorkers];
    template<class IO> static bool AllStable(IO& io) {
        for (unsigned core=0;core<DataflowBarrierWorkers;core++)
            if (!io.AllRolesHeld(core) || !io.LocalRamStable(core)) return false;
        return true;
    }
    bool RetainAllPossiblyDirty() {
        for (unsigned core=0;core<DataflowBarrierWorkers;core++)
            if (!fCore[core].MarkPossiblyDirty()) return false;
        fPhase=Dirty; fReleaseLedgerReady=false; return true;
    }
public:
    DataflowRoleFleetLocalRamOwner()=default;
    DataflowRoleFleetLocalRamOwner(const DataflowRoleFleetLocalRamOwner&)=delete;
    DataflowRoleFleetLocalRamOwner& operator=(const DataflowRoleFleetLocalRamOwner&)=delete;
    static constexpr unsigned RequiredWords() {
        return DataflowBarrierWorkers*DataflowRoleLocalRamOwner::RequiredWords();
    }
    bool MarkedForPossibleRelease() const { return fPhase==Dirty && fReleaseLedgerReady; }
    bool RestorationRequired() const { return fPhase==Dirty; }
    bool RestoredOriginal() const { return fPhase==Restored; }
    template<class IO> bool CaptureAll(uint32_t* storage,unsigned words,IO& io) {
        if (fPhase!=Empty || !storage || words!=RequiredWords()) return false;
        if (!AllStable(io)) { fPhase=Unavailable; return false; }
        const unsigned perCore=DataflowRoleLocalRamOwner::RequiredWords();
        for (unsigned core=0;core<DataflowBarrierWorkers;core++)
            if (!fCore[core].Capture(core,storage+core*perCore,perCore,io)) {
                fPhase=Unavailable; return false;
            }
        if (!AllStable(io)) { if (!RetainAllPossiblyDirty()) fPhase=Unavailable; return false; }
        fPhase=Captured; return true;
    }
    template<class IO> bool MarkMayRunAll(IO& io) {
        if (fPhase!=Captured) return false;
        if (!AllStable(io)) { if (!RetainAllPossiblyDirty()) fPhase=Unavailable; return false; }
        for (unsigned core=0;core<DataflowBarrierWorkers;core++)
            if (!fCore[core].MarkMayRun(io)) {
                if (!RetainAllPossiblyDirty()) fPhase=Unavailable;
                return false;
            }
        fPhase=Dirty;
        if (!AllStable(io)) return false;
        fReleaseLedgerReady=true;
        return true;
    }
    template<class IO> bool VerifyOriginalAll(IO& io) {
        if (fPhase!=Captured && fPhase!=Restored) return false;
        if (!AllStable(io)) { fPhase=Dirty; return false; }
        for (unsigned core=0;core<DataflowBarrierWorkers;core++)
            if (!fCore[core].VerifyOriginal(io)) { fPhase=Dirty; return false; }
        if (!AllStable(io)) { fPhase=Dirty; return false; }
        return true;
    }
    template<class IO> bool RestoreAll(IO& io) {
        if (fPhase!=Dirty) return false;
        fReleaseLedgerReady=false;
        if (!AllStable(io)) return false;
        for (unsigned core=0;core<DataflowBarrierWorkers;core++)
            if (fCore[core].RestorationRequired() && !fCore[core].Restore(io)) return false;
        // Verify every snapshot after all writes to catch cross-core misroutes.
        for (unsigned core=0;core<DataflowBarrierWorkers;core++)
            if (!fCore[core].VerifyOriginal(io)) return false;
        if (!AllStable(io)) return false;
        fPhase=Restored; return true;
    }
};
}
#endif
