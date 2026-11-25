/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_QWEN35_DATAFLOW_ROLE_FLEET_RECOVERY_OWNER_H
#define PIONEER_QWEN35_DATAFLOW_ROLE_FLEET_RECOVERY_OWNER_H
#include "PioneerQwen35DataflowRoleFleetImagesOwner.h"
#include "PioneerQwen35DataflowRoleFleetLocalRamOwner.h"
#include "PioneerQwen35DataflowRoleRestoreHandoffOwner.h"
namespace PioneerQwen35 {
// Blackhole RISCV_DEBUG_REG_DISABLE_RESET bits 6..9 preserve BRISC/NC local
// RAM and debug registers on soft-reset exit. Shared compute can additionally
// preserve T0 local RAM (bit 0); all other bits remain exactly as found. This
// does not capture T0 registers, CSR state or prove that its program leaves
// private RAM untouched. This owner still needs exclusive
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
    uint32_t fPreserveResetMask=DataflowRolePreserveResetMask;
    bool fStackless=false;
    bool fCoreLocalRestoreMode=false;
    bool fAutoRestoreMode=false;
    bool fCoreLocalReleaseLedger=false;
    bool fHeldImageOnlyStage=false,fHeldImageOnlyRestored=false;
    bool fCoreResetRestoreAttempted=false;
#ifdef PIONEER_QWEN35_FABRIC_GUARD_WITNESS
    bool fFabricCoreRecoveryAttempted=false,fFabricCoreRecoveryActive=false;
    bool fFabricAutoRetired[DataflowBarrierWorkers]{};
#endif
    bool fCoreRestoreStaged[DataflowBarrierWorkers]{};
    bool fCoreRestoreReleaseAttempted[DataflowBarrierWorkers]{};
    bool fAutoRestoreMayRun[DataflowBarrierWorkers]{};
    bool fAutoRestoreArmed[DataflowBarrierWorkers]{};
    bool fAutoRestoreCompleted[DataflowBarrierWorkers]{};
    bool fAutoRestoreCancelled[DataflowBarrierWorkers]{};
    DataflowRoleRestoreHandoffOwner fCoreRestoreHandoff[DataflowBarrierWorkers];
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
    // The physical copy scope must cover exactly the fleet ledger, including
    // cores other than the immediate target. Otherwise a misrouted shim write
    // could destroy an unreported captured copy.
    template<class IO> bool VerifyHeldCopyLedger(IO& io) {
        for (unsigned core=0;core<DataflowBarrierWorkers;core++) {
            const uint32_t* reader=nullptr;const uint32_t* writer=nullptr;bool captured=false;
            if (!io.StacklessHeldCopySources(core,reader,writer,captured)
                || captured!=fRam.StacklessCopiesCaptured(core)) return false;
            if (captured) {
                if (!reader || !writer || reader!=fRam.OriginalWords(core,DataflowRoleReader)
                    || writer!=fRam.OriginalWords(core,DataflowRoleWriter)
                    || !io.StacklessHeldCopySourceSafe(core,reader,writer)) return false;
            } else if (reader || writer) return false;
        }
        return true;
    }
    // Retained host RAM snapshots must match this exact owner even after L1
    // copy windows have been overwritten during image retirement.
    template<class IO> bool VerifyRetirementLedger(IO& io) {
        for (unsigned core=0;core<DataflowBarrierWorkers;core++) {
            const uint32_t* reader=nullptr;const uint32_t* writer=nullptr;bool captured=false;
            if (!io.RetirementRamSnapshotSources(core,reader,writer,captured)
                || captured!=fRam.StacklessCopiesCaptured(core)) return false;
            if (captured) {
                if (reader!=fRam.OriginalWords(core,DataflowRoleReader)
                    || writer!=fRam.OriginalWords(core,DataflowRoleWriter) || !reader || !writer) return false;
#ifdef PIONEER_QWEN35_FABRIC_GUARD_WITNESS
                if (fFabricCoreRecoveryActive && fFabricAutoRetired[core]) {
                    if (!fCoreRestoreHandoff[core].Untouched() || !fRam.CoreRestoredByCore(core)
                        || !io.AutoRestoreRetirementSafe(core,reader,writer,fAutoRestoreCancelled[core])) return false;
                } else
#endif
                if (!fCoreRestoreHandoff[core].HandoffRestored()) return false;
            } else if (reader || writer || !fCoreRestoreHandoff[core].Untouched()) return false;
        }
        return true;
    }
    // In the resident stop-to-restore flow the parent resident owner owns
    // the original handoff. It restores that handoff later, so the IO proof
    // must remain valid both before and after that parent's restoration.
    template<class IO> bool VerifyAutoRetirementLedger(IO& io) {
        for (unsigned core=0;core<DataflowBarrierWorkers;core++) {
            const uint32_t* reader=nullptr;const uint32_t* writer=nullptr;bool captured=false;
            if (!io.RetirementRamSnapshotSources(core,reader,writer,captured)
                || captured!=fRam.StacklessCopiesCaptured(core)) return false;
            if (captured) {
                if (!reader || !writer || reader!=fRam.OriginalWords(core,DataflowRoleReader)
                    || writer!=fRam.OriginalWords(core,DataflowRoleWriter)
                    || (!fAutoRestoreArmed[core] && !fAutoRestoreCancelled[core])
                    || !fAutoRestoreCompleted[core]
                    || !fCoreRestoreHandoff[core].Untouched()
                    || !io.AutoRestoreRetirementSafe(core,reader,writer,fAutoRestoreCancelled[core])) return false;
            } else if (reader || writer || fAutoRestoreArmed[core] || fAutoRestoreCompleted[core]
                || fAutoRestoreCancelled[core] || !fCoreRestoreHandoff[core].Untouched()) return false;
        }
        return true;
    }
    template<class IO> bool VerifyResetDisable(IO& io,bool preserved) {
        if (!fResetDisableCaptured) return false;
        for (unsigned core=0;core<DataflowBarrierWorkers;core++) {
            uint32_t first=0,second=0;
            const uint32_t expected=fResetDisable[core]
                |(preserved?fPreserveResetMask:0U);
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
            const uint32_t desired=fResetDisable[core]|fPreserveResetMask;
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
    // Select before capture, for the shared service that releases T0 as well
    // as B/NC. This preserves its RAM from reset; it is not a RAM snapshot or
    // authority to run a T0 image which writes that RAM.
    bool SelectComputeLocalRamPreservation() {
        if (fPhase!=Empty || fPreserveResetMask!=DataflowRolePreserveResetMask) return false;
        fPreserveResetMask=DataflowSharedPreserveResetMask;return true;
    }
    bool ComputeLocalRamPreservationSelected() const {
        return fPreserveResetMask==DataflowSharedPreserveResetMask;
    }
    // Metadata only: the physical scope must still prove the selected tile
    // and fleet are held and exclusively owned. Permit only transitions
    // between this tile's saved control word and its preservation variant,
    // while the owner has marked a control write as potentially attempted.
    bool ResetDisableTransitionMatches(unsigned core,uint32_t expected,uint32_t desired) const {
        if (core>=DataflowBarrierWorkers || !fResetDisableCaptured || !fResetDisableTouched
            || fPhase!=Faulted) return false;
        const uint32_t original=fResetDisable[core],preserved=original|fPreserveResetMask;
        return (expected==original || expected==preserved)
            && (desired==original || desired==preserved);
    }
    bool ResetDisableRollbackTransitionMatches(unsigned core,uint32_t expected,uint32_t desired) const {
        return core<DataflowBarrierWorkers && desired==fResetDisable[core]
            && ResetDisableTransitionMatches(core,expected,desired);
    }
    bool ResetDisableStageTransitionMatches(unsigned core,uint32_t expected,uint32_t desired) const {
        return core<DataflowBarrierWorkers && expected==fResetDisable[core]
            && desired==(fResetDisable[core]|fPreserveResetMask)
            && ResetDisableTransitionMatches(core,expected,desired);
    }
    bool ReleaseLedgerPrepared() const { return fPhase==Armed
        && (fStackless?fRam.StacklessCapturePrepared():fRam.MarkedForPossibleRelease()); }
    bool RestorationRequired() const {
        return fPhase==Faulted || fPhase==Staged || fPhase==Armed || fPhase==ReleaseAttempted;
    }
    bool CoveredStateRestored() const { return fPhase==Restored; }
    // Metadata for the fixed qualification's pre-release image transaction.
    // It is not authority for MMIO or proof that a processor is held.
    bool HeldImageRollbackReady() const {
        return fHeldImageOnlyStage && !fHeldImageOnlyRestored && fStackless
            && !fCoreLocalReleaseLedger && !fResetDisableTouched
            && (fPhase==Faulted || fPhase==Staged) && fRam.StacklessCapturePrepared();
    }
    bool HeldImageRollbackComplete() const { return fHeldImageOnlyRestored && fPhase==Restored; }
    // This ledger only describes pre-release reset-preservation writes. It
    // does not authorize MMIO or prove that a failed write has drained.
    bool HeldPreservationRollbackReady() const {
        return fHeldImageOnlyStage && !fHeldImageOnlyRestored && fStackless
            && !fCoreLocalReleaseLedger && fResetDisableCaptured && fResetDisableTouched
            && (fPhase==Faulted || fPhase==Armed) && fRam.StacklessCapturePrepared();
    }
    bool HeldPreservationStageActive() const {
        return fHeldImageOnlyStage && !fHeldImageOnlyRestored && fStackless
            && !fCoreLocalReleaseLedger && fResetDisableCaptured
            && (fPhase==Staged || fPhase==Faulted) && fRam.StacklessCapturePrepared();
    }
    bool HeldImageRollbackProofActive() const {
        return fHeldImageOnlyRestored && (fPhase==Restored || fPhase==Faulted)
            && !fCoreLocalReleaseLedger && !fResetDisableTouched && fRam.StacklessCapturePrepared();
    }
    // Historical ledger only: a full successful projection must have captured,
    // run and locally restored every role. This does not prove that reset,
    // firmware or a NoC writer stayed excluded after the observations.
    bool AutoRestoredFleetLedgerComplete() const {
        if (fPhase!=Restored || !fAutoRestoreMode || !fCoreLocalReleaseLedger || !fStackless
            || !fResetDisableCaptured || fResetDisableTouched || !fImages.ImagesRestored()
            || !fRam.StacklessRestoredByCore()) return false;
        for (unsigned core=0;core<DataflowBarrierWorkers;core++)
            if (!fRam.StacklessCopiesCaptured(core) || !fRam.CoreRestoredByCore(core)
                || !fAutoRestoreMayRun[core] || !fAutoRestoreArmed[core]
                || !fAutoRestoreCompleted[core] || fAutoRestoreCancelled[core]) return false;
        return true;
    }
    // Opt-in for the future core-local restore lifecycle. This changes only
    // the ledger; it does not provide physical scope or release authority.
    bool SelectCoreLocalRestoreMode() {
        if (fPhase!=Empty || fCoreLocalRestoreMode) return false;
        fCoreLocalRestoreMode=true;return true;
    }
    bool SelectAutoRestoreMode() {
        if (!SelectCoreLocalRestoreMode()) return false;
        fAutoRestoreMode=true;return true;
    }
    bool AutoRestoreMode() const { return fAutoRestoreMode; }
    bool CoreLocalRestoreMode() const { return fCoreLocalRestoreMode; }
#ifdef PIONEER_QWEN35_FABRIC_GUARD_WITNESS
    bool FabricCoreRecoveryAttempted() const { return fFabricCoreRecoveryAttempted; }
    bool FabricCoreRecoveryActive() const { return fFabricCoreRecoveryActive; }
    bool FabricCoreAlreadyRestored(unsigned core) const {
        return fFabricCoreRecoveryActive && core<DataflowBarrierWorkers && fFabricAutoRetired[core];
    }
    // After a verified fabric hold, a resident auto-restore run interrupted
    // including already restored/cancelled cores can enter the separate
    // core-local recovery sequence. IO must bind the exact retained ticket,
    // parent handoff, all snapshots, routes, and lifetime writer exclusion;
    // these metadata and sampled checks do not establish those facts.
    // A failed audit is one-shot and retains the original auto-mode owner.
    template<class IO> bool BeginFabricCoreRecovery(IO& io) {
        if (fFabricCoreRecoveryAttempted || !fAutoRestoreMode || !fCoreLocalRestoreMode
            || !fCoreLocalReleaseLedger || !fStackless || fCoreResetRestoreAttempted
            || (fPhase!=ReleaseAttempted && fPhase!=Faulted)
            || !io.FabricRecoveryConversionAllowed()) return false;
        for (unsigned core=0;core<DataflowBarrierWorkers;core++) {
            if ((fAutoRestoreCancelled[core] && !fAutoRestoreCompleted[core])
                || fCoreRestoreStaged[core] || fCoreRestoreReleaseAttempted[core]
                || !fCoreRestoreHandoff[core].Untouched()) return false;
        }
        fFabricCoreRecoveryAttempted=true;fPhase=Faulted;
        if (!io.FabricRecoveryConversionAllowed() || !AllHeldAndRouted(io)
            || !VerifyHeldCopyLedger(io) || !VerifyResetDisable(io,true)
            || !fImages.VerifyStagedAllWithCopies(io)) return false;
        for (unsigned core=0;core<DataflowBarrierWorkers;core++) {
            if (!fRam.StacklessCopiesCaptured(core)) {
                if (fAutoRestoreCompleted[core] || !io.StacklessUncapturedRamProtected(core)) return false;
            } else if (fAutoRestoreCompleted[core]) {
                const uint32_t* reader=fRam.OriginalWords(core,DataflowRoleReader);
                const uint32_t* writer=fRam.OriginalWords(core,DataflowRoleWriter);
                if (!reader || !writer || !fRam.VerifyCoreRestoredByCore(core,io)
                    || !io.AutoRestoreRetirementSafe(core,reader,writer,fAutoRestoreCancelled[core])) return false;
            }
        }
        if (!io.FabricRecoveryConversionAllowed() || !VerifyHeldCopyLedger(io)
            || !VerifyResetDisable(io,true) || !AllHeldAndRouted(io)
            || !io.FabricRecoveryConversionAllowed()) return false;
        for (unsigned core=0;core<DataflowBarrierWorkers;core++) fFabricAutoRetired[core]=fAutoRestoreCompleted[core];
        fAutoRestoreMode=false;fFabricCoreRecoveryActive=true;return true;
    }
#endif
    bool CopyAwareReleaseStarted() const { return fCoreLocalReleaseLedger; }
    const uint32_t* OriginalRamWords(unsigned core,unsigned role) const {
        return fRam.OriginalWords(core,role);
    }
    // Valid only as historical evidence under CaptureStacklessCore's IO
    // contract; this alone does not exclude a later reset or authorize NoC IO.
    bool StacklessCopiesCaptured(unsigned core) const {
        return fStackless && fRam.StacklessCopiesCaptured(core);
    }
    // Bind a per-core source to this owner's exact retained allocation.
    // This is host ledger identity only; callers separately establish held
    // routes, reset continuity and protection of uncaptured tiles.
    bool StacklessSources(unsigned core,const uint32_t* storage,unsigned words,
        const uint32_t*& reader,const uint32_t*& writer,bool& captured) const {
        reader=writer=nullptr;captured=false;
        if (core>=DataflowBarrierWorkers || !fStackless || !fCoreLocalReleaseLedger
            || (fPhase!=ReleaseAttempted && fPhase!=Faulted)
            || !fRam.StacklessStorageMatches(storage,words)) return false;
        const bool has=fRam.StacklessCopiesCaptured(core);
        const uint32_t* r=fRam.OriginalWords(core,DataflowRoleReader);
        const uint32_t* w=fRam.OriginalWords(core,DataflowRoleWriter);
        if (has) {
            const uint32_t* expected=storage+core*DataflowRoleLocalRamOwner::RequiredWords();
            if (r!=expected || w!=expected+DataflowRoleLocalRamWords) return false;
        } else if (r || w) return false;
        reader=r;writer=w;captured=has;return true;
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
    // Capture only held images and reset-control words. This does not read
    // private RAM, write images, or authorize any release; StageImages still
    // requires the stronger fleet-preparation proof independently.
    template<class IO> bool CaptureAllStacklessHeldImages(uint32_t* code,unsigned codeWords,uint32_t* ram,
        unsigned ramWords,IO& io) {
        if (fPhase!=Empty || !AllHeldAndRouted(io)) return false;
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
        if (!AllHeldAndRouted(io)) { fPhase=Unavailable; return false; }
        fStackless=true;
        fPhase=Captured; return true;
    }
    bool StacklessHeldImagesCaptured() const {
        return fPhase==Captured && fStackless && fRam.StacklessCapturePrepared();
    }
    // The legacy combined admission keeps the stronger pre/post checks.
    // They cover prior RAM preservation and recovery of partial releases.
    template<class IO> bool CaptureAllStackless(uint32_t* code,unsigned codeWords,uint32_t* ram,
        unsigned ramWords,IO& io) {
        if (fPhase!=Empty || !AllStacklessPreparationSafe(io)
            || !CaptureAllStacklessHeldImages(code,codeWords,ram,ramWords,io)) return false;
        if (!AllStacklessPreparationSafe(io)) { fPhase=Faulted; return false; }
        return true;
    }
    template<class IO> bool CaptureStacklessCore(unsigned core,IO& io) {
        if (!fStackless || fPhase!=ReleaseAttempted) return false;
        if (!fRam.CaptureStacklessCore(core,io)) { fPhase=Faulted; return false; }
        return true;
    }
    template<class IO> bool MarkStacklessMayRun(unsigned core,IO& io) {
        if (!fStackless || fPhase!=ReleaseAttempted) return false;
        if (!fRam.MarkStacklessMayRun(core,io)) { fPhase=Faulted; return false; }
        if (fAutoRestoreMode) fAutoRestoreMayRun[core]=true;
        return true;
    }
    // Arm immediately before the first stack release attempt, including an
    // uncertain transport write. This is only a ledger: IO must bind the
    // exact retained sources, fresh handoff, images and exclusive writers.
    template<class IO> bool ArmAutoRestoreCore(unsigned core,IO& io) {
        if (!fAutoRestoreMode || !fCoreLocalReleaseLedger || !fStackless
            || fPhase!=ReleaseAttempted || core>=DataflowBarrierWorkers
            || !fRam.StacklessCopiesCaptured(core) || !fAutoRestoreMayRun[core]
            || fAutoRestoreArmed[core]) return false;
        const uint32_t* reader=fRam.OriginalWords(core,DataflowRoleReader);
        const uint32_t* writer=fRam.OriginalWords(core,DataflowRoleWriter);
        if (!reader || !writer || !io.AutoRestoreArmSafe(core,reader,writer)
            || !io.AutoRestoreArmSafe(core,reader,writer)) return false;
        fAutoRestoreArmed[core]=true;return true;
    }
    // Called only after observed stop, restore park and held reset. The
    // observations do not replace IO's independent source and writer proof.
    template<class IO> bool MarkAutoRestoredCore(unsigned core,IO& io) {
        if (!fAutoRestoreMode || !fCoreLocalReleaseLedger || !fStackless
            || (fPhase!=ReleaseAttempted && fPhase!=Faulted) || core>=DataflowBarrierWorkers
            || !fAutoRestoreArmed[core] || fAutoRestoreCompleted[core]) return false;
        fPhase=Faulted;
        const uint32_t* reader=fRam.OriginalWords(core,DataflowRoleReader);
        const uint32_t* writer=fRam.OriginalWords(core,DataflowRoleWriter);
        if (!reader || !writer || !io.AutoRestoreHeldSafe(core,reader,writer)
            || !fRam.MarkCoreRestoredByCore(core,io)
            || !io.AutoRestoreHeldSafe(core,reader,writer)) return false;
        fAutoRestoreCompleted[core]=true;return true;
    }
    // A pre-stack shim that cancelled and parked has not used private RAM.
    // A posted stack-release write must first be proved drained and absent.
    // IO also proves that cancellation, saved sources and writer exclusion
    // remain valid while the fleet is held and through later retirement.
    template<class IO> bool MarkAutoCancelledCore(unsigned core,bool releaseAttempted,
        bool releaseDrainedAbsent,IO& io) {
        if (!fAutoRestoreMode || !fCoreLocalReleaseLedger || !fStackless
            || (fPhase!=ReleaseAttempted && fPhase!=Faulted) || core>=DataflowBarrierWorkers
            || !fRam.StacklessCopiesCaptured(core) || fAutoRestoreCompleted[core]
            || releaseAttempted!=releaseDrainedAbsent
            || (releaseAttempted && !fAutoRestoreArmed[core])) return false;
        fPhase=Faulted;
        const uint32_t* reader=fRam.OriginalWords(core,DataflowRoleReader);
        const uint32_t* writer=fRam.OriginalWords(core,DataflowRoleWriter);
        if (!reader || !writer || !io.AutoRestoreCancelHeldSafe(core,reader,writer,releaseAttempted)
            || !fRam.MarkCoreRestoredByCore(core,io)
            || !io.AutoRestoreCancelHeldSafe(core,reader,writer,releaseAttempted)) return false;
        fAutoRestoreCancelled[core]=true;fAutoRestoreCompleted[core]=true;return true;
    }
    // Replace a held core's startup shims with its local restore shims. The
    // fleet's own RAM ledger and the IO scope must identify the same retained
    // copies; the image owner then audits all 120 tiles around the writes.
    // This only stages code. It does not grant a physical release or prove
    // that the restore program ran. A failed audit retains the fleet.
    template<class IO> bool StageCoreLocalRestore(unsigned core,const uint32_t* reader,
        unsigned readerWords,const uint32_t* writer,unsigned writerWords,IO& io) {
        if (!fCoreLocalRestoreMode || fAutoRestoreMode || !fCoreLocalReleaseLedger || !fStackless
            || (fPhase!=ReleaseAttempted && fPhase!=Faulted)
            || !fRam.StacklessCopiesCaptured(core) || fCoreRestoreStaged[core]
            || fCoreRestoreReleaseAttempted[core]) return false;
#ifdef PIONEER_QWEN35_FABRIC_GUARD_WITNESS
        if (fFabricAutoRetired[core]) return false;
#endif
        fPhase=Faulted;
        const uint32_t* expectedReader=fRam.OriginalWords(core,DataflowRoleReader);
        const uint32_t* expectedWriter=fRam.OriginalWords(core,DataflowRoleWriter);
        const uint32_t* observedReader=nullptr;const uint32_t* observedWriter=nullptr;
        bool captured=false;
        if (!expectedReader || !expectedWriter || !AllHeldAndRouted(io) || !VerifyHeldCopyLedger(io)
            || !VerifyResetDisable(io,true)
            || !io.StacklessHeldCopySources(core,observedReader,observedWriter,captured)
            || !captured || observedReader!=expectedReader || observedWriter!=expectedWriter
            || !io.StacklessHeldCopySourceSafe(core,expectedReader,expectedWriter)
            || !fImages.RestageOneWithCopies(core,reader,readerWords,writer,writerWords,io))
            return false;
        observedReader=observedWriter=nullptr;captured=false;
        if (!(io.StacklessHeldCopySources(core,observedReader,observedWriter,captured)
            && captured && observedReader==expectedReader && observedWriter==expectedWriter
            && io.StacklessHeldCopySourceSafe(core,expectedReader,expectedWriter)
            && VerifyHeldCopyLedger(io) && VerifyResetDisable(io,true) && AllHeldAndRouted(io))) return false;
        fCoreRestoreStaged[core]=true;return true;
    }
    // Retain and clear the actual handoff only after the fixed restore shim
    // has been staged. IO must own the words and drain competing writers.
    template<class IO> bool PrepareCoreLocalRestoreHandoff(unsigned core,IO& io) {
        if (!fCoreLocalRestoreMode || fPhase!=Faulted || core>=DataflowBarrierWorkers
            || !fCoreRestoreStaged[core] || fCoreRestoreReleaseAttempted[core]
            || !AllHeldAndRouted(io) || !VerifyHeldCopyLedger(io) || !VerifyResetDisable(io,true))
            return false;
        return fCoreRestoreHandoff[core].Capture(core,io)
            && fCoreRestoreHandoff[core].ClearForRelease(io);
    }
    // Consume a distinct one-shot ledger immediately before a restore-shim
    // release attempt, including one that may be uncertain. IO must also
    // establish fresh cleared receipts, verified shim/reset state and lifetime
    // exclusion of conflicting releases/writers. This does not release a core.
    template<class IO> bool BeginCoreLocalRestoreRelease(unsigned core,IO& io) {
        if (!fCoreLocalRestoreMode || !fCoreLocalReleaseLedger || !fStackless
            || fPhase!=Faulted || core>=DataflowBarrierWorkers || !fCoreRestoreStaged[core]
            || fCoreRestoreReleaseAttempted[core] || !fRam.StacklessCopiesCaptured(core)
            || !fCoreRestoreHandoff[core].ArmedForRelease()) return false;
        const uint32_t* reader=fRam.OriginalWords(core,DataflowRoleReader);
        const uint32_t* writer=fRam.OriginalWords(core,DataflowRoleWriter);
        if (!reader || !writer || !AllHeldAndRouted(io) || !VerifyHeldCopyLedger(io)
            || !VerifyResetDisable(io,true) || !fImages.VerifyStagedAllWithCopies(io)
            || !io.StacklessCoreRestoreReleaseReady(core,reader,writer)
            || !VerifyHeldCopyLedger(io) || !VerifyResetDisable(io,true)
            || !AllHeldAndRouted(io)
            || !io.StacklessCoreRestoreReleaseReady(core,reader,writer)) return false;
        if (!fCoreRestoreHandoff[core].BeginReleaseAttempt(io)) return false;
        fCoreRestoreReleaseAttempted[core]=true;return true;
    }
    // Read completion while the released shims are parked; the IO scope must
    // prove their identity, fresh receipts and exclusion of competing writers.
    template<class IO> bool ConfirmCoreLocalRestoreHandoff(unsigned core,IO& io) {
        return fCoreLocalRestoreMode && fPhase==Faulted && core<DataflowBarrierWorkers
            && fCoreRestoreReleaseAttempted[core] && fCoreRestoreHandoff[core].ConfirmCompletion(io);
    }
    bool CoreLocalRestoreHandoffCompleted(unsigned core) const {
        return core<DataflowBarrierWorkers && fCoreRestoreHandoff[core].CompletionConfirmed();
    }
    // The real scope must prove the exact retained source, fresh restore
    // receipt, held routes and writer exclusion. This records RAM proof only;
    // image retirement, reset restoration and safe resume remain separate.
    template<class IO> bool MarkCoreRestoredByCore(unsigned core,IO& io) {
        if (!fCoreLocalRestoreMode || fAutoRestoreMode || !fStackless
            || (fPhase!=ReleaseAttempted && fPhase!=Faulted)
            || core>=DataflowBarrierWorkers || !fCoreRestoreReleaseAttempted[core]
            || !fCoreRestoreHandoff[core].CompletionConfirmed()) return false;
        fPhase=Faulted;
        return fRam.MarkCoreRestoredByCore(core,io);
    }
    template<class IO> bool RestoreCoreLocalHandoff(unsigned core,IO& io) {
        if (!fCoreLocalRestoreMode || fPhase!=Faulted || core>=DataflowBarrierWorkers
            || (fCoreRestoreHandoff[core].ReleaseWasAttempted() && !fRam.CoreRestoredByCore(core)))
            return false;
        return fCoreRestoreHandoff[core].RestoreOriginal(io);
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
    // Fixed qualification only: stage images while all workers remain held.
    // IO independently admits the exact retained owner and each write. This
    // does not prepare a release or establish private-RAM continuity.
    template<class IO> bool StageHeldImages(const uint32_t* reader,unsigned readerWords,
        const uint32_t* writer,unsigned writerWords,const uint32_t* readerShim,
        unsigned readerShimWords,const uint32_t* writerShim,unsigned writerShimWords,IO& io) {
        if (fPhase!=Captured || !fStackless || !fRam.StacklessCapturePrepared()
            || fHeldImageOnlyStage || fCoreLocalReleaseLedger || fResetDisableTouched
            || !io.HeldImageStageAllowed() || !AllHeldAndRouted(io)
            || !io.HeldImageStageAllowed()) return false;
        fHeldImageOnlyStage=true;fPhase=Faulted;
        if (!fImages.StageAll(reader,readerWords,writer,writerWords,
                readerShim,readerShimWords,writerShim,writerShimWords,io)
            || !AllHeldAndRouted(io) || !io.HeldImageStageAllowed()) return false;
        fPhase=Staged;return true;
    }
    // No release/reset-preservation attempt may have begun. Restoring these
    // images never reads or writes role-private RAM. An uncertain write or
    // failed proof retains the snapshots; callers must not retry blindly.
    template<class IO> bool RestoreHeldImagesBeforeRelease(IO& io) {
        if (!HeldImageRollbackReady() || !io.HeldImageWritesConfirmed()
            || !io.HeldImageRollbackAllowed() || !AllHeldAndRouted(io)) return false;
        fPhase=Faulted;
        if (fImages.RestorationRequired()) {
            if (!fImages.RestoreAll(io)) return false;
        } else if (!fImages.VerifyOriginalAll(io)) return false;
        if (!io.HeldImageWritesConfirmed() || !io.HeldImageRollbackAllowed()
            || !fImages.VerifyOriginalAll(io) || !VerifyResetDisable(io,false)
            || !AllHeldAndRouted(io)) return false;
        fHeldImageOnlyRestored=true;fPhase=Restored;return true;
    }
    // Restore the saved preservation controls while all processors remain
    // held and before the copy-aware release ledger begins. A control write
    // whose completion is uncertain must never be followed by this replay.
    template<class IO> bool RestoreHeldPreservationBeforeRelease(IO& io) {
        if (!HeldPreservationRollbackReady() || !io.HeldPreservationWritesConfirmed()
            || !io.HeldPreservationRollbackAllowed() || !AllHeldAndRouted(io)) return false;
        fPhase=Faulted;
        if (!RestoreResetDisable(io) || !io.HeldPreservationWritesConfirmed()
            || !io.HeldPreservationRollbackAllowed() || !VerifyResetDisable(io,false)
            || !AllHeldAndRouted(io)) return false;
        return HeldImageRollbackReady();
    }
    // Prepare the preservation bits while every processor is held. The
    // separately gated BeginReleaseAttempt still checks the stronger fleet
    // and retained-copy conditions before a processor can be released.
    template<class IO> bool PrepareHeldPreservation(IO& io) {
        if (fPhase!=Staged || !HeldPreservationStageActive() || fResetDisableTouched
            || !io.HeldPreservationWritesConfirmed() || !io.HeldPreservationStageAllowed()) return false;
        fPhase=Faulted;
        if (!AllHeldAndRouted(io) || !fImages.VerifyStagedAll(io)
            || !io.HeldPreservationStageAllowed() || !ArmResetPreservation(io)
            || !io.HeldPreservationWritesConfirmed() || !fImages.VerifyStagedAll(io)
            || !AllHeldAndRouted(io) || !io.HeldPreservationStageAllowed()) return false;
        fPhase=Armed;return true;
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
    // Separately gated core-local cleanup. IO must prove restored RAM and
    // unmodified uncaptured cores across all image writes, including after
    // the copy windows are overwritten. Incomplete cleanup retains this
    // owner. No host private-RAM access or physical proof is supplied here.
    template<class IO> bool RestoreAfterCoreLocalFleetProof(IO& io) {
        if (!fCoreLocalRestoreMode || fAutoRestoreMode || !fCoreLocalReleaseLedger || !fStackless
            || (fPhase!=ReleaseAttempted && fPhase!=Faulted)) return false;
        fPhase=Faulted;
        if (!io.FleetRamImageRetirementSafe() || !AllHeldAndRouted(io)
            || (!fCoreResetRestoreAttempted && !VerifyResetDisable(io,true))
            || (fCoreResetRestoreAttempted && !io.FleetRamImageRetirementRetrySafe())
            || !VerifyRetirementLedger(io) || !fRam.VerifyStacklessRestoredByCore(io)) return false;
        if (fImages.RestorationRequired()) {
            if (!fImages.RestoreAfterFleetRamProof(io)) return false;
        } else if (!fImages.VerifyOriginalAll(io)) return false;
        if (!io.FleetRamImageRetirementSafe() || !VerifyRetirementLedger(io)
            || !fRam.VerifyStacklessRestoredByCore(io) || !fImages.VerifyOriginalAll(io)) return false;
        fCoreResetRestoreAttempted=true;
        if (!RestoreResetDisable(io) || !io.FleetRamImageRetirementSafe()
            || !VerifyRetirementLedger(io) || !fRam.VerifyStacklessRestoredByCore(io)
            || !fImages.VerifyOriginalAll(io) || !AllHeldAndRouted(io)) return false;
        fPhase=Restored;return true;
    }
    // Auto-restored handoffs are owned by the resident parent. They are not
    // rewritten here; IO must prove that retained state through retirement.
    template<class IO> bool RestoreAfterAutoRestoreFleetProof(IO& io) {
        if (!fAutoRestoreMode || !fCoreLocalReleaseLedger || !fStackless
            || (fPhase!=ReleaseAttempted && fPhase!=Faulted)) return false;
        fPhase=Faulted;
        if (!io.FleetRamImageRetirementSafe() || !AllHeldAndRouted(io)
            || (!fCoreResetRestoreAttempted && !VerifyResetDisable(io,true))
            || (fCoreResetRestoreAttempted && !io.FleetRamImageRetirementRetrySafe())
            || !VerifyAutoRetirementLedger(io) || !fRam.VerifyStacklessRestoredByCore(io)) return false;
        if (fImages.RestorationRequired()) {
            if (!fImages.RestoreAfterFleetRamProof(io)) return false;
        } else if (!fImages.VerifyOriginalAll(io)) return false;
        if (!io.FleetRamImageRetirementSafe() || !VerifyAutoRetirementLedger(io)
            || !fRam.VerifyStacklessRestoredByCore(io) || !fImages.VerifyOriginalAll(io)) return false;
        fCoreResetRestoreAttempted=true;
        if (!RestoreResetDisable(io) || !io.FleetRamImageRetirementSafe()
            || !VerifyAutoRetirementLedger(io) || !fRam.VerifyStacklessRestoredByCore(io)
            || !fImages.VerifyOriginalAll(io) || !AllHeldAndRouted(io)) return false;
        fPhase=Restored;return true;
    }
    template<class IO> bool VerifyAfterAutoResidentRestore(IO& io) {
        if (!fAutoRestoreMode || !fCoreLocalReleaseLedger || !fStackless || fPhase!=Restored) return false;
        fPhase=Faulted;
        if (!io.FleetRamImageRetirementSafe() || !AllHeldAndRouted(io)
            || !VerifyAutoRetirementLedger(io) || !fRam.VerifyStacklessRestoredByCore(io)
            || !fImages.VerifyOriginalAll(io) || !VerifyResetDisable(io,false)
            || !AllHeldAndRouted(io)) return false;
        fPhase=Restored;return true;
    }
    // The resident owner may touch overlapping L1 after the role cleanup.
    // Reaudit without falling back to a host private-RAM read.
    template<class IO> bool VerifyAfterCoreLocalResidentRestore(IO& io) {
        if (!fCoreLocalRestoreMode || fAutoRestoreMode || !fCoreLocalReleaseLedger || !fStackless || fPhase!=Restored) return false;
        fPhase=Faulted;
        if (!io.FleetRamImageRetirementSafe() || !AllHeldAndRouted(io)
            || !VerifyRetirementLedger(io) || !fRam.VerifyStacklessRestoredByCore(io)
            || !fImages.VerifyOriginalAll(io) || !VerifyResetDisable(io,false)
            || !AllHeldAndRouted(io)) return false;
        fPhase=Restored;return true;
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
        if (fHeldImageOnlyRestored) {
            if (fCoreLocalReleaseLedger || fResetDisableTouched || !fRam.StacklessCapturePrepared()
                || !io.HeldImageRollbackAllowed() || !AllHeldAndRouted(io)
                || !fImages.VerifyOriginalAll(io) || !VerifyResetDisable(io,false)
                || !AllHeldAndRouted(io)) return false;
            fPhase=Restored;return true;
        }
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
