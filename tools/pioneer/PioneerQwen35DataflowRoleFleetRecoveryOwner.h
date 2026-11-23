/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_QWEN35_DATAFLOW_ROLE_FLEET_RECOVERY_OWNER_H
#define PIONEER_QWEN35_DATAFLOW_ROLE_FLEET_RECOVERY_OWNER_H
#include "PioneerQwen35DataflowRoleFleetImagesOwner.h"
#include "PioneerQwen35DataflowRoleFleetLocalRamOwner.h"
namespace PioneerQwen35 {
// Blackhole RISCV_DEBUG_REG_DISABLE_RESET bits 6..9 preserve BRISC/NC local
// RAM and debug registers on soft-reset exit. Other bits, including TRISC
// settings, must remain exactly as found. This owner still needs exclusive
// hardware ownership and independently proved held/quiet routes.
// Offline composition for 120 tiles' additional code, low shims, boot words
// and B/NC private RAM. It does not release or stop RISCs, reset hardware,
// audit physical PCs/NoCs, own the existing TRISC0 image, or provide PCI IO.
// IO must serialize the fleet and prove held/quiet/default routing. Slow
// private-RAM accesses need an independent initialization-window proof;
// deferred capture instead needs the original-state, preservation and
// unsnapshotted-tile proofs. Allocate outside the kernel stack. A release
// attempt requires every OTHER physical proof as well.
class DataflowRoleFleetRecoveryOwner {
    enum Phase { Empty, Captured, Faulted, Staged, Armed, ReleaseAttempted, Restored, Unavailable };
    Phase fPhase=Empty;
    DataflowRoleFleetImagesOwner fImages;
    DataflowRoleFleetLocalRamOwner fRam;
    uint32_t fResetDisable[DataflowBarrierWorkers]{};
    bool fResetDisableCaptured=false,fResetDisableTouched=false;
    bool fStackless=false;
    bool fCoreLocalRestoreMode=false;
    bool fCoreLocalReleaseLedger=false;
    template<class IO> static bool AllSafe(IO& io) {
        for (unsigned core=0;core<DataflowBarrierWorkers;core++)
            if (!io.AllRolesHeld(core) || !io.DefaultBootRouting(core)
                || !io.LocalRamStable(core)) return false;
        return true;
    }
    template<class IO> static bool AllHeldAndRouted(IO& io) {
        for (unsigned core=0;core<DataflowBarrierWorkers;core++)
            if (!io.AllRolesHeld(core) || !io.DefaultBootRouting(core)) return false;
        return true;
    }
    template<class IO> static bool AllStacklessPreparationSafe(IO& io) {
        for (unsigned core=0;core<DataflowBarrierWorkers;core++)
            if (!io.AllRolesHeld(core) || !io.DefaultBootRouting(core)
                || !io.StacklessFleetPreparationSafe(core)) return false;
        return true;
    }
    template<class IO> bool VerifyResetDisable(IO& io,bool preserved) {
        if (!fResetDisableCaptured) return false;
        for (unsigned core=0;core<DataflowBarrierWorkers;core++) {
            uint32_t first=0,second=0;
            const uint32_t expected=fResetDisable[core]
                |(preserved?DataflowRolePreserveResetMask:0U);
            if (!io.AllRolesHeld(core) || !io.ReadResetDisable(core,first)
                || !io.ReadResetDisable(core,second) || first!=expected || second!=expected)
                return false;
        }
        return true;
    }
    template<class IO> bool ArmResetPreservation(IO& io) {
        if (!VerifyResetDisable(io,false)) return false;
        // Mark all control snapshots touched before the first write: even a
        // failed PCI write can have partially completed or hit another tile.
        fResetDisableTouched=true;
        for (unsigned core=0;core<DataflowBarrierWorkers;core++) {
            const uint32_t desired=fResetDisable[core]|DataflowRolePreserveResetMask;
            uint32_t observed=0;
            if (!io.AllRolesHeld(core) || !io.WriteResetDisable(core,desired)
                || !io.ReadResetDisable(core,observed) || observed!=desired) return false;
        }
        return VerifyResetDisable(io,true);
    }
    template<class IO> bool RestoreResetDisable(IO& io) {
        if (!fResetDisableCaptured) return false;
        if (!fResetDisableTouched && VerifyResetDisable(io,false)) return true;
        fResetDisableTouched=true;
        for (unsigned core=0;core<DataflowBarrierWorkers;core++) {
            uint32_t observed=0;
            if (!io.AllRolesHeld(core) || !io.WriteResetDisable(core,fResetDisable[core])
                || !io.ReadResetDisable(core,observed) || observed!=fResetDisable[core])
                return false;
        }
        if (!VerifyResetDisable(io,false)) return false;
        fResetDisableTouched=false; return true;
    }
public:
    DataflowRoleFleetRecoveryOwner()=default;
    DataflowRoleFleetRecoveryOwner(const DataflowRoleFleetRecoveryOwner&)=delete;
    DataflowRoleFleetRecoveryOwner& operator=(const DataflowRoleFleetRecoveryOwner&)=delete;
    static constexpr unsigned CodeWords() { return DataflowRoleFleetImagesOwner::RequiredWords(); }
    static constexpr unsigned RamWords() { return DataflowRoleFleetLocalRamOwner::RequiredWords(); }
    bool ReleaseLedgerPrepared() const { return fPhase==Armed
        && (fStackless?fRam.StacklessCapturePrepared():fRam.MarkedForPossibleRelease()); }
    bool RestorationRequired() const {
        return fPhase==Faulted || fPhase==Staged || fPhase==Armed || fPhase==ReleaseAttempted;
    }
    bool CoveredStateRestored() const { return fPhase==Restored; }
    // Opt-in for the future core-local restore lifecycle. This changes only
    // the ledger; it does not provide physical scope or release authority.
    bool SelectCoreLocalRestoreMode() {
        if (fPhase!=Empty || fCoreLocalRestoreMode) return false;
        fCoreLocalRestoreMode=true;return true;
    }
    bool CoreLocalRestoreMode() const { return fCoreLocalRestoreMode; }
    const uint32_t* OriginalRamWords(unsigned core,unsigned role) const {
        return fRam.OriginalWords(core,role);
    }
    // Valid only as historical evidence under CaptureStacklessCore's IO
    // contract; this alone does not exclude a later reset or authorize NoC IO.
    bool StacklessCopiesCaptured(unsigned core) const {
        return fStackless && fRam.StacklessCopiesCaptured(core);
    }
    // Capture is read-only; these states have not performed any role write.
    bool Untouched() const { return fPhase==Empty || fPhase==Captured || fPhase==Unavailable; }
    template<class IO> bool CaptureAll(uint32_t* code,unsigned codeWords,uint32_t* ram,
        unsigned ramWords,IO& io) {
        if (fPhase!=Empty || fCoreLocalRestoreMode || !AllSafe(io)) return false;
        for (unsigned core=0;core<DataflowBarrierWorkers;core++) {
            uint32_t first=0,second=0;
            if (!io.AllRolesHeld(core) || !io.ReadResetDisable(core,first)
                || !io.ReadResetDisable(core,second) || first!=second) {
                fPhase=Unavailable; return false;
            }
            fResetDisable[core]=first;
        }
        fResetDisableCaptured=true;
        if (!fImages.CaptureAll(code,codeWords,io)) { fPhase=Unavailable; return false; }
        if (!fRam.CaptureAll(ram,ramWords,io)) {
            fPhase=fRam.RestorationRequired()?Faulted:Unavailable; return false;
        }
        if (!AllSafe(io)) {
            (void)fRam.MarkMayRunAll(io); // conservatively retain all complete RAM snapshots
            fPhase=Faulted; return false;
        }
        fPhase=Captured; return true;
    }
    // Deferred mode snapshots no private RAM before release. IO must prove
    // that the prior RAM state can be preserved and that partial releases
    // and unsnapshotted tiles can be recovered before admitting this mode.
    template<class IO> bool CaptureAllStackless(uint32_t* code,unsigned codeWords,uint32_t* ram,
        unsigned ramWords,IO& io) {
        if (fPhase!=Empty || !AllStacklessPreparationSafe(io)) return false;
        for (unsigned core=0;core<DataflowBarrierWorkers;core++) {
            uint32_t first=0,second=0;
            if (!io.AllRolesHeld(core) || !io.ReadResetDisable(core,first)
                || !io.ReadResetDisable(core,second) || first!=second) {
                fPhase=Unavailable; return false;
            }
            fResetDisable[core]=first;
        }
        fResetDisableCaptured=true;
        if (!fImages.CaptureAll(code,codeWords,io) || !fRam.PrepareStackless(ram,ramWords)) {
            fPhase=Unavailable; return false;
        }
        fStackless=true;
        if (!AllStacklessPreparationSafe(io)) { fPhase=Faulted; return false; }
        fPhase=Captured; return true;
    }
    template<class IO> bool CaptureStacklessCore(unsigned core,IO& io) {
        if (!fStackless || fPhase!=ReleaseAttempted) return false;
        if (!fRam.CaptureStacklessCore(core,io)) { fPhase=Faulted; return false; }
        return true;
    }
    template<class IO> bool MarkStacklessMayRun(unsigned core,IO& io) {
        if (!fStackless || fPhase!=ReleaseAttempted) return false;
        if (!fRam.MarkStacklessMayRun(core,io)) { fPhase=Faulted; return false; }
        return true;
    }
    // The real scope must prove the exact retained source, fresh restore
    // receipt, held routes and writer exclusion. This records RAM proof only;
    // image retirement, reset restoration and safe resume remain separate.
    template<class IO> bool MarkCoreRestoredByCore(unsigned core,IO& io) {
        if (!fCoreLocalRestoreMode || !fStackless
            || (fPhase!=ReleaseAttempted && fPhase!=Faulted)) return false;
        fPhase=Faulted;
        return fRam.MarkCoreRestoredByCore(core,io);
    }
    template<class IO> bool VerifyCoreLocalRestores(IO& io) {
        if (!fCoreLocalRestoreMode || !fStackless
            || (fPhase!=ReleaseAttempted && fPhase!=Faulted)) return false;
        fPhase=Faulted;
        return fRam.VerifyStacklessRestoredByCore(io);
    }
    template<class IO> bool StageImages(const uint32_t* reader,unsigned readerWords,
        const uint32_t* writer,unsigned writerWords,const uint32_t* readerShim,
        unsigned readerShimWords,const uint32_t* writerShim,unsigned writerShimWords,IO& io) {
        if (fPhase!=Captured) return false;
        if (fStackless) {
            fPhase=Faulted;
            if (!AllStacklessPreparationSafe(io)
                || !fImages.StageAll(reader,readerWords,writer,writerWords,
                    readerShim,readerShimWords,writerShim,writerShimWords,io)
                || !AllStacklessPreparationSafe(io)) return false;
            fPhase=Staged; return true;
        }
        if (!AllSafe(io)) {
            (void)fRam.MarkMayRunAll(io); // no release is authorized by a failed audit
            fPhase=Faulted; return false;
        }
        fPhase=Faulted;
        if (!fImages.StageAll(reader,readerWords,writer,writerWords,
                readerShim,readerShimWords,writerShim,writerShimWords,io)
            || !fRam.VerifyOriginalAll(io) || !AllSafe(io)) return false;
        fPhase=Staged; return true;
    }
    // Ledger only: no processor is released. In deferred mode snapshots are
    // captured per tile before stack release, and uncaptured tiles require
    // independent protection throughout recovery.
    template<class IO> bool PreparePossibleRelease(IO& io) {
        if (fPhase!=Staged) return false;
        fPhase=Faulted;
        if (fStackless) {
            if (!AllStacklessPreparationSafe(io) || !fRam.StacklessCapturePrepared()
                || !fImages.VerifyStagedAll(io) || !ArmResetPreservation(io)
                || !fImages.VerifyStagedAll(io) || !AllStacklessPreparationSafe(io)) return false;
            fPhase=Armed; return true;
        }
        if (!AllSafe(io) || !fImages.VerifyStagedAll(io) || !fRam.VerifyOriginalAll(io)
            || !fRam.MarkMayRunAll(io) || !ArmResetPreservation(io)
            || !fImages.VerifyStagedAll(io) || !AllSafe(io))
            return false;
        fPhase=Armed; return true;
    }
    // Consume the one-shot ledger BEFORE the physical release attempt. A
    // partial or uncertain release must never be retried by this owner.
    template<class IO> bool BeginReleaseAttempt(IO& io) {
        if (fPhase!=Armed) return false;
        fPhase=Faulted;
        if (fStackless) {
            if (!AllStacklessPreparationSafe(io) || !fRam.StacklessCapturePrepared()
                || !VerifyResetDisable(io,true) || !fImages.VerifyStagedAll(io)
                || !AllStacklessPreparationSafe(io)) return false;
            if (fCoreLocalRestoreMode) {
                // Sticky before the copy-retention ledger is attempted. A
                // later failure must never expose saved copies to old cleanup.
                fCoreLocalReleaseLedger=true;
                if (!fImages.BeginCopyAwareOwnership()) return false;
            }
            fPhase=ReleaseAttempted; return true;
        }
        if (!AllSafe(io) || !fRam.MarkedForPossibleRelease() || !VerifyResetDisable(io,true)
            || !fImages.VerifyStagedAll(io) || !AllSafe(io)) return false;
        fPhase=ReleaseAttempted; return true;
    }
    template<class IO> bool RestoreAll(IO& io) {
        if (fPhase!=Faulted && fPhase!=Staged && fPhase!=Armed && fPhase!=ReleaseAttempted) return false;
        fPhase=Faulted;
        // Copy-aware image retirement is not implemented. The old cleanup
        // would use host private-RAM accesses and could erase retained copies.
        if (fCoreLocalReleaseLedger) return false;
        if (fStackless) {
            if (!AllHeldAndRouted(io)) return false;
            if (fRam.RestoredOriginal()) {
                if (!fRam.VerifyStacklessRestored(io)) return false;
            } else if (!fRam.RestoreStackless(io)) return false;
            if (fImages.RestorationRequired()) {
                if (!fImages.RestoreAll(io)) return false;
            } else if (!fImages.VerifyOriginalAll(io)) return false;
            if (!fRam.VerifyStacklessRestored(io) || !fImages.VerifyOriginalAll(io)
                || !RestoreResetDisable(io) || !fImages.VerifyOriginalAll(io)
                || !fRam.VerifyStacklessRestored(io) || !AllHeldAndRouted(io)) return false;
            fPhase=Restored; return true;
        }
        if (!AllSafe(io)) return false;
        if (fImages.RestorationRequired()) {
            if (!fImages.RestoreAll(io)) return false;
        } else if (!fImages.VerifyOriginalAll(io)) return false;
        if (fRam.RestorationRequired()) {
            if (!fRam.RestoreAll(io)) return false;
        } else if (!fRam.VerifyOriginalAll(io)) return false;
        // A later restoration may misroute into a previously verified region.
        if (!fImages.VerifyOriginalAll(io) || !fRam.VerifyOriginalAll(io)
            || !RestoreResetDisable(io) || !fImages.VerifyOriginalAll(io)
            || !fRam.VerifyOriginalAll(io) || !AllSafe(io))
            return false;
        fPhase=Restored; return true;
    }
    // The resident TRISC owner restores its saved L1 after this owner has
    // restored the B/NC state. Its low-L1 snapshot overlaps the role shims,
    // and a misrouted later write could touch any role snapshot. Do not
    // release or resume the fleet until this second full audit succeeds.
    // A mismatch returns to Faulted so the affected owners can restore and
    // the caller can retry while all cores remain held.
    template<class IO> bool VerifyAfterResidentRestore(IO& io) {
        if (fPhase!=Restored) return false;
        fPhase=Faulted;
        if (fStackless) {
            if (!AllHeldAndRouted(io) || !fImages.VerifyOriginalAll(io)
                || !fRam.VerifyStacklessRestored(io) || !VerifyResetDisable(io,false)
                || !AllHeldAndRouted(io)) return false;
            fPhase=Restored; return true;
        }
        if (!AllSafe(io) || !fImages.VerifyOriginalAll(io)
            || !fRam.VerifyOriginalAll(io) || !VerifyResetDisable(io,false)
            || !AllSafe(io)) return false;
        fPhase=Restored; return true;
    }
};
}
#endif
