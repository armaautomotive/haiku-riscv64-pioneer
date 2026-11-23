/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_QWEN35_DATAFLOW_ROLE_FLEET_LOCAL_RAM_OWNER_H
#define PIONEER_QWEN35_DATAFLOW_ROLE_FLEET_LOCAL_RAM_OWNER_H
#include "PioneerQwen35DataflowRoleLocalRamOwner.h"
namespace PioneerQwen35 {
// Offline fleet ledger for B/NC private RAM only. The original mode captures
// and marks all snapshots before role release. The deferred mode requires a
// protected stackless handoff and marks each snapshot before stack release.
// Neither mode proves reset, NoC, boot, code, cache or physical ownership.
class DataflowRoleFleetLocalRamOwner {
    enum Phase { Empty, Captured, Dirty, Restored, Unavailable,
        StacklessPrepared, StacklessFaulted, StacklessRestored };
    Phase fPhase=Empty;
    bool fReleaseLedgerReady=false;
    DataflowRoleLocalRamOwner fCore[DataflowBarrierWorkers];
    uint32_t* fStacklessStorage=nullptr;
    bool fStacklessCaptured[DataflowBarrierWorkers]{};
    template<class IO> static bool AllStable(IO& io) {
        for (unsigned core=0;core<DataflowBarrierWorkers;core++)
            if (!io.AllRolesHeld(core) || !io.LocalRamStable(core)) return false;
        return true;
    }
    template<class IO> bool StacklessHeldAndProtected(IO& io) const {
        for (unsigned core=0;core<DataflowBarrierWorkers;core++) {
            if (!io.AllRolesHeld(core)) return false;
            // This independent proof must cover the entire restoration,
            // including previously issued releases and misrouted writes.
            if (!fStacklessCaptured[core] && !io.StacklessUncapturedRamProtected(core)) return false;
        }
        return true;
    }
    template<class IO> class StacklessProtectedIO {
        DataflowRoleFleetLocalRamOwner& fOwner;
        IO& fIO;
    public:
        StacklessProtectedIO(DataflowRoleFleetLocalRamOwner& owner,IO& io):fOwner(owner),fIO(io) {}
        bool AllRolesHeld(unsigned core) {
            return fOwner.StacklessHeldAndProtected(fIO) && fIO.AllRolesHeld(core);
        }
        bool LocalRamStable(unsigned core) { return fIO.LocalRamStable(core); }
        bool ReadRam(unsigned core,unsigned role,uint32_t address,uint32_t* output,unsigned bytes) {
            return fIO.ReadRam(core,role,address,output,bytes);
        }
        bool WriteRam(unsigned core,unsigned role,uint32_t address,const uint32_t* input,unsigned bytes) {
            return fIO.WriteRam(core,role,address,input,bytes);
        }
    };
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
    bool StacklessCapturePrepared() const { return fPhase==StacklessPrepared && fStacklessStorage; }
    bool RestorationRequired() const {
        return fPhase==Dirty || fPhase==StacklessPrepared || fPhase==StacklessFaulted;
    }
    bool RestoredOriginal() const { return fPhase==Restored || fPhase==StacklessRestored; }
    const uint32_t* OriginalWords(unsigned core,unsigned role) const {
        return core<DataflowBarrierWorkers && fPhase!=Empty && fPhase!=Unavailable
            ?fCore[core].OriginalWords(role):nullptr;
    }
    // Allocate the fleet ledger before any role release. This performs no
    // device access; each tile remains untrusted until its stackless capture
    // succeeds. The caller retains this storage through complete recovery.
    bool PrepareStackless(uint32_t* storage,unsigned words) {
        if (fPhase!=Empty || !storage || words!=RequiredWords()) return false;
        fStacklessStorage=storage; fPhase=StacklessPrepared; return true;
    }
    template<class IO> bool CaptureStacklessCore(unsigned core,IO& io) {
        if (fPhase!=StacklessPrepared || core>=DataflowBarrierWorkers
            || !fStacklessStorage || fStacklessCaptured[core]) return false;
        const unsigned words=DataflowRoleLocalRamOwner::RequiredWords();
        if (!fCore[core].CaptureStackless(core,fStacklessStorage+core*words,words,io)) {
            fPhase=StacklessFaulted; return false;
        }
        fStacklessCaptured[core]=true; return true;
    }
    template<class IO> bool MarkStacklessMayRun(unsigned core,IO& io) {
        if (fPhase!=StacklessPrepared || core>=DataflowBarrierWorkers || !fStacklessCaptured[core])
            return false;
        if (!fCore[core].MarkMayRunStackless(io)) { fPhase=StacklessFaulted; return false; }
        return true;
    }
    // Uncaptured tiles have no bytes to restore. A separate physical proof
    // must establish they remain untouched by roles and all other writers,
    // including misrouted restoration writes, for the whole transaction.
    template<class IO> bool RestoreStackless(IO& io) {
        if (fPhase!=StacklessPrepared && fPhase!=StacklessFaulted) return false;
        fPhase=StacklessFaulted;
        if (!StacklessHeldAndProtected(io)) return false;
        StacklessProtectedIO<IO> protectedIO(*this,io);
        for (unsigned core=0;core<DataflowBarrierWorkers;core++) {
            if (!fStacklessCaptured[core]) continue;
            if (!StacklessHeldAndProtected(io)) return false;
            if (!fCore[core].RestorationRequired() && !fCore[core].RestoredOriginal()
                && !fCore[core].MarkPossiblyDirty()) return false;
            if (fCore[core].RestorationRequired() && !fCore[core].Restore(protectedIO)) return false;
            if (!StacklessHeldAndProtected(io)) return false;
        }
        bool matches=true;
        for (unsigned core=0;core<DataflowBarrierWorkers;core++) {
            if (!fStacklessCaptured[core]) continue;
            const auto audit=fCore[core].AuditOriginal(protectedIO);
            if (audit==DataflowRoleLocalRamOwner::OriginalUnreadable) return false;
            if (audit==DataflowRoleLocalRamOwner::OriginalMismatch) matches=false;
        }
        if (!matches || !StacklessHeldAndProtected(io)) return false;
        fPhase=StacklessRestored; return true;
    }
    template<class IO> bool VerifyStacklessRestored(IO& io) {
        if (fPhase!=StacklessRestored) return false;
        fPhase=StacklessFaulted;
        if (!StacklessHeldAndProtected(io)) return false;
        StacklessProtectedIO<IO> protectedIO(*this,io);
        bool matches=true;
        for (unsigned core=0;core<DataflowBarrierWorkers;core++) {
            if (!fStacklessCaptured[core]) continue;
            const auto audit=fCore[core].AuditOriginal(protectedIO);
            if (audit==DataflowRoleLocalRamOwner::OriginalUnreadable) return false;
            if (audit==DataflowRoleLocalRamOwner::OriginalMismatch) matches=false;
        }
        if (!matches || !StacklessHeldAndProtected(io)) return false;
        fPhase=StacklessRestored; return true;
    }
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
        bool matches=true;
        for (unsigned core=0;core<DataflowBarrierWorkers;core++) {
            const auto audit=fCore[core].AuditOriginal(io);
            if (audit==DataflowRoleLocalRamOwner::OriginalUnreadable) { fPhase=Dirty; return false; }
            if (audit==DataflowRoleLocalRamOwner::OriginalMismatch) matches=false;
        }
        if (!matches || !AllStable(io)) { fPhase=Dirty; return false; }
        return true;
    }
    template<class IO> bool RestoreAll(IO& io) {
        if (fPhase!=Dirty) return false;
        fReleaseLedgerReady=false;
        if (!AllStable(io)) return false;
        for (unsigned core=0;core<DataflowBarrierWorkers;core++)
            if (fCore[core].RestorationRequired() && !fCore[core].Restore(io)) return false;
        // Verify every snapshot after all writes to catch cross-core misroutes.
        bool matches=true;
        for (unsigned core=0;core<DataflowBarrierWorkers;core++) {
            const auto audit=fCore[core].AuditOriginal(io);
            if (audit==DataflowRoleLocalRamOwner::OriginalUnreadable) return false;
            if (audit==DataflowRoleLocalRamOwner::OriginalMismatch) matches=false;
        }
        if (!matches) return false;
        if (!AllStable(io)) return false;
        fPhase=Restored; return true;
    }
};
}
#endif
