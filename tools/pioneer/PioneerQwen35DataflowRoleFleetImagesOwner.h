/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_QWEN35_DATAFLOW_ROLE_FLEET_IMAGES_OWNER_H
#define PIONEER_QWEN35_DATAFLOW_ROLE_FLEET_IMAGES_OWNER_H
#include "PioneerQwen35DataflowRoleFleetCodeOwner.h"
#include "PioneerQwen35DataflowRoleFleetBootOwner.h"
#ifdef PIONEER_QWEN35_FLASH_CODE_OWNER_WITNESS
#include "PioneerQwen35DataflowFlashCodeOwner.h"
#endif
namespace PioneerQwen35 {
// Offline composition of 120 extra-code, boot-word and low-shim snapshots.
// It never starts a processor or proves that one parked. Before any operation
// IO must serialize the whole fleet, prove all relevant roles held and NoCs
// settled, and establish default boot routing. Local RAM, reset, cache, queues,
// the existing TRISC0 image and physical PCI integration remain separate.
class DataflowRoleFleetImagesOwner {
    enum Phase { Empty, Captured, Faulted, Staged, Restored, Unavailable };
    Phase fPhase=Empty;
    bool fCopyAwareMode=false;
    bool fCopyAwareVerified=false;
    bool fCopyRetirementStarted=false;
    DataflowRoleFleetCodeOwner fCode;
    DataflowRoleFleetBootOwner fBoot;
#ifdef PIONEER_QWEN35_FLASH_CODE_OWNER_WITNESS
    DataflowFlashCodeFleetOwner fFlash;
    uint32_t* fRoleSnapshot=nullptr;uint32_t* fFlashSnapshot=nullptr;
    const uint32_t *fReaderImage=nullptr,*fWriterImage=nullptr,*fReaderShim=nullptr,*fWriterShim=nullptr;
    unsigned fReaderWords=0,fWriterWords=0,fReaderShimWords=0,fWriterShimWords=0;
    bool fFlashCaptured=false,fFlashRestoreAttempted=false;
    static bool HostDisjoint(const uint32_t* a,unsigned aWords,const uint32_t* b,unsigned bWords) {
        const uintptr_t x=reinterpret_cast<uintptr_t>(a),y=reinterpret_cast<uintptr_t>(b);
        if (!a || !b || (x&3) || (y&3) || !aWords || !bWords
            || uintptr_t(aWords)>UINTPTR_MAX/4 || uintptr_t(bWords)>UINTPTR_MAX/4
            || x>UINTPTR_MAX-uintptr_t(aWords)*4 || y>UINTPTR_MAX-uintptr_t(bWords)*4) return false;
        return x+uintptr_t(aWords)*4<=y || y+uintptr_t(bWords)*4<=x;
    }
    template<class IO> bool FlashScope(IO& io,bool frozen) const {
        // During capture the role snapshot stays immutable while only the
        // flash snapshot is writable. Once frozen both remain immutable.
        return fRoleSnapshot && fFlashSnapshot
            && io.FlashRoleCommonAuthority(this,&fFlash)
            && io.FlashRoleSnapshotsSafe(fRoleSnapshot,RequiredWords(),fFlashSnapshot,
                DataflowFlashCodeFleetOwner::RequiredWords(),frozen);
    }
    template<class IO> bool FlashRoleImagesScope(IO& io) const {
        return fReaderImage && fReaderWords && fWriterImage && fWriterWords
            && fReaderShim && fReaderShimWords && fWriterShim && fWriterShimWords
            && io.FlashRoleImagesSafe(fReaderImage,fReaderWords,fWriterImage,fWriterWords,
                fReaderShim,fReaderShimWords,fWriterShim,fWriterShimWords);
    }
#endif
    template<class IO> static bool AllHeld(IO& io) {
        for (unsigned core=0;core<DataflowBarrierWorkers;core++)
            if (!io.AllRolesHeld(core) || !io.DefaultBootRouting(core)) return false;
        return true;
    }
public:
    DataflowRoleFleetImagesOwner()=default;
    DataflowRoleFleetImagesOwner(const DataflowRoleFleetImagesOwner&)=delete;
    DataflowRoleFleetImagesOwner& operator=(const DataflowRoleFleetImagesOwner&)=delete;
    static constexpr unsigned RequiredWords() { return DataflowRoleFleetCodeOwner::RequiredWords(); }
#ifdef PIONEER_QWEN35_FLASH_CODE_OWNER_WITNESS
    static constexpr unsigned FlashRequiredWords() { return DataflowFlashCodeFleetOwner::RequiredWords(); }
    bool FlashCaptured() const {return fFlashCaptured;}
    bool FlashStagedAndVerified() const {return fFlash.StagedAndVerified();}
    bool FlashRegionRestored() const {return fFlash.RegionRestored();}
#endif
    bool StagedAndVerified() const { return fPhase==Staged && !fCopyAwareMode; }
    bool CopyAwareStagedAndVerified() const { return fPhase==Staged && fCopyAwareMode && fCopyAwareVerified; }
    // Call before the first release that can populate private-RAM copies.
    // This is a software retention ledger, not a physical ownership proof.
    bool BeginCopyAwareOwnership() {
        if (fPhase!=Staged || fCopyAwareMode
#ifdef PIONEER_QWEN35_FLASH_CODE_OWNER_WITNESS
            || !fFlash.StagedAndVerified()
#endif
            || !fCode.BeginCopyAwareOwnership()) return false;
        fCopyAwareMode=true;fCopyAwareVerified=false;return true;
    }
    bool RestorationRequired() const { return fPhase==Faulted || fPhase==Staged
#ifdef PIONEER_QWEN35_FLASH_CODE_OWNER_WITNESS
        || (fPhase==Captured && fFlash.RestorationRequired())
#endif
        ; }
    bool ImagesRestored() const { return fPhase==Restored; }
    // Keep a distinct frame for the Haiku qualification's stack-budget audit.
    template<class IO> __attribute__((noinline)) bool CaptureAll(uint32_t* storage,unsigned words,IO& io) {
        if (fPhase!=Empty || !AllHeld(io)) return false;
        if (!fCode.CaptureAll(storage,words,io) || !fBoot.CaptureAll(io) || !AllHeld(io)) {
            fPhase=Unavailable; return false;
        }
#ifdef PIONEER_QWEN35_FLASH_CODE_OWNER_WITNESS
        fRoleSnapshot=storage;
#endif
        fPhase=Captured;
        return true;
    }
#ifdef PIONEER_QWEN35_FLASH_CODE_OWNER_WITNESS
    // Held-only preparation: capture both disjoint windows before any write.
    // The IO must bind both scopes to the same retained physical session.
    template<class IO> bool CaptureFlashAll(uint32_t* storage,unsigned words,IO& io) {
        if (fPhase!=Captured || fFlashCaptured || words!=FlashRequiredWords()
            || !HostDisjoint(fRoleSnapshot,RequiredWords(),storage,words)) return false;
        fFlashSnapshot=storage;
        if (!FlashScope(io,false)) {fFlashSnapshot=nullptr;return false;}
        if (!fFlash.CaptureAll(storage,words,io)) {fPhase=Faulted;return false;}
        fFlashCaptured=true;
        if (!FlashScope(io,true) || !fCode.VerifyOriginalAll(io) || !fBoot.VerifyOriginalAll(io)
            || !fFlash.VerifyOriginalAll(io) || !AllHeld(io)) {fPhase=Faulted;return false;}
        return true;
    }
    // Stage the extra window first, then verify every still-original role
    // image before allowing role writes. Any uncertain operation retains both
    // snapshots for a separately proven rollback.
    template<class IO> bool StageFlashAll(const uint32_t* image,unsigned words,IO& io) {
        if (fPhase!=Captured || !fFlashCaptured || !HostDisjoint(image,words,fRoleSnapshot,RequiredWords())
            || !FlashScope(io,true)) return false;
        fPhase=Faulted;
        if (!fCode.VerifyOriginalAll(io) || !fBoot.VerifyOriginalAll(io)
            || !fFlash.StageAll(image,words,io) || !FlashScope(io,true)
            || !fCode.VerifyOriginalAll(io) || !fBoot.VerifyOriginalAll(io)
            || !fFlash.VerifyStagedAll(io) || !AllHeld(io)) return false;
        fPhase=Captured;return true;
    }
#endif
    template<class IO> bool StageAll(const uint32_t* reader,unsigned readerWords,
        const uint32_t* writer,unsigned writerWords,const uint32_t* readerShim,
        unsigned readerShimWords,const uint32_t* writerShim,unsigned writerShimWords,IO& io) {
        if (fPhase!=Captured || !reader || !readerWords || !writer || !writerWords
            || !readerShim || !readerShimWords || !writerShim || !writerShimWords) return false;
#ifdef PIONEER_QWEN35_FLASH_CODE_OWNER_WITNESS
        if (!fFlashCaptured || !fFlash.StagedAndVerified() || !FlashScope(io,true)
            || !HostDisjoint(reader,readerWords,fFlashSnapshot,FlashRequiredWords())
            || !HostDisjoint(writer,writerWords,fFlashSnapshot,FlashRequiredWords())
            || !HostDisjoint(readerShim,readerShimWords,fFlashSnapshot,FlashRequiredWords())
            || !HostDisjoint(writerShim,writerShimWords,fFlashSnapshot,FlashRequiredWords())
            || !HostDisjoint(reader,readerWords,fRoleSnapshot,RequiredWords())
            || !HostDisjoint(writer,writerWords,fRoleSnapshot,RequiredWords())
            || !HostDisjoint(readerShim,readerShimWords,fRoleSnapshot,RequiredWords())
            || !HostDisjoint(writerShim,writerShimWords,fRoleSnapshot,RequiredWords())) return false;
        fReaderImage=reader;fReaderWords=readerWords;fWriterImage=writer;fWriterWords=writerWords;
        fReaderShim=readerShim;fReaderShimWords=readerShimWords;fWriterShim=writerShim;fWriterShimWords=writerShimWords;
        if (!FlashRoleImagesScope(io)) return false;
#endif
        if (!AllHeld(io)) { fPhase=Faulted; return false; }
        fPhase=Faulted;
#ifdef PIONEER_QWEN35_FLASH_CODE_OWNER_WITNESS
        if (!fFlash.VerifyStagedAll(io) || !FlashScope(io,true) || !FlashRoleImagesScope(io)) return false;
#endif
        // High code first, shims and reset entry words second. No processor
        // may start here: complete fleet readback is only one launch proof.
        if (!fCode.StageAll(reader,readerWords,writer,writerWords,io)
            || !fBoot.StageAll(readerShim,readerShimWords,writerShim,writerShimWords,io)
            || !fCode.VerifyStagedAll(io) || !fBoot.VerifyStagedAll(io)
#ifdef PIONEER_QWEN35_FLASH_CODE_OWNER_WITNESS
            || !FlashScope(io,true) || !FlashRoleImagesScope(io) || !fFlash.VerifyStagedAll(io)
#endif
            || !AllHeld(io))
            return false;
        fPhase=Staged;
        return true;
    }
    template<class IO> bool VerifyStagedAll(IO& io) {
        if (fPhase!=Staged || fCopyAwareMode) return false;
#ifdef PIONEER_QWEN35_FLASH_CODE_OWNER_WITNESS
        if (!FlashScope(io,true) || !FlashRoleImagesScope(io)) {fPhase=Faulted;return false;}
#endif
        if (!AllHeld(io) || !fCode.VerifyStagedAll(io) || !fBoot.VerifyStagedAll(io)
#ifdef PIONEER_QWEN35_FLASH_CODE_OWNER_WITNESS
            || !FlashScope(io,true) || !fFlash.VerifyStagedAll(io) || !FlashRoleImagesScope(io)
#endif
            || !AllHeld(io)) { fPhase=Faulted; return false; }
        return true;
    }
    // An independent retained-copy proof is required for every captured
    // core; ordinary VerifyStagedAll deliberately still rejects those changes.
    template<class IO> bool VerifyStagedAllWithCopies(IO& io) {
        if (fPhase!=Staged || !fCopyAwareMode) return false;
        fCopyAwareVerified=false;
#ifdef PIONEER_QWEN35_FLASH_CODE_OWNER_WITNESS
        if (!FlashScope(io,true) || !FlashRoleImagesScope(io)) {fPhase=Faulted;return false;}
#endif
        if (!AllHeld(io) || !fCode.VerifyStagedAllWithCopies(io) || !fBoot.VerifyStagedAll(io)
#ifdef PIONEER_QWEN35_FLASH_CODE_OWNER_WITNESS
            || !FlashScope(io,true) || !fFlash.VerifyStagedAll(io) || !FlashRoleImagesScope(io)
#endif
            || !AllHeld(io)) { fPhase=Faulted; return false; }
        fCopyAwareVerified=true;
        return true;
    }
    // The startup qualification executes one tile, but snapshots and checks
    // all 120 so writes routed to another tile remain detectable.
    template<class IO> bool StageOne(unsigned core,const uint32_t* reader,unsigned readerWords,
        const uint32_t* writer,unsigned writerWords,const uint32_t* readerShim,
        unsigned readerShimWords,const uint32_t* writerShim,unsigned writerShimWords,IO& io) {
#ifdef PIONEER_QWEN35_FLASH_CODE_OWNER_WITNESS
        // A single-core flash stage and its fleet-wide audit are not defined.
        (void)core;(void)reader;(void)readerWords;(void)writer;(void)writerWords;
        (void)readerShim;(void)readerShimWords;(void)writerShim;(void)writerShimWords;(void)io;
        return false;
#else
        if (fPhase!=Captured || core>=DataflowBarrierWorkers || !reader || !readerWords
            || !writer || !writerWords || !readerShim || !readerShimWords
            || !writerShim || !writerShimWords) return false;
        fPhase=Faulted;
        if (!AllHeld(io) || !fCode.StageOne(core,reader,readerWords,writer,writerWords,io)
            || !fBoot.StageOne(core,readerShim,readerShimWords,writerShim,writerShimWords,io)
            || !fCode.VerifyStagedAll(io) || !fBoot.VerifyStagedAll(io) || !AllHeld(io)) return false;
        fPhase=Staged;return true;
#endif
    }
    // Offline-only building block for replacing held-core shims. Code, other
    // cores and original snapshots remain owned and audited throughout. This
    // still requires the original staged extra-L1 contents: role-produced
    // RAM copies change that region and need the separate copy-aware path
    // and its currently unavailable physical authority. Never clear a
    // copy merely to make this audit pass.
    template<class IO> bool RestageOne(unsigned core,const uint32_t* reader,unsigned readerWords,
        const uint32_t* writer,unsigned writerWords,IO& io) {
#ifdef PIONEER_QWEN35_FLASH_CODE_OWNER_WITNESS
        (void)core;(void)reader;(void)readerWords;(void)writer;(void)writerWords;(void)io;
        return false;
#else
        if (fPhase!=Staged || fCopyAwareMode || !fBoot.CanRestageCore(core)
            || !reader || !readerWords || !writer || !writerWords
            || readerWords>(DataflowRoleReaderShimLimit-DataflowRoleReaderShimAddress)/4
            || writerWords>(DataflowRoleWriterShimLimit-DataflowRoleWriterShimAddress)/4) return false;
        if (!VerifyStagedAll(io)) return false;
        fPhase=Faulted;
        if (!fBoot.RestageOne(core,reader,readerWords,writer,writerWords,io)
            || !fCode.VerifyStagedAll(io) || !fBoot.VerifyStagedAll(io) || !AllHeld(io)) return false;
        fPhase=Staged;return true;
#endif
    }
    // Copy-aware counterpart for a future stackless restore transaction. IO
    // must bind all captured L1 copy windows to protected retained snapshots,
    // and separately prove held fleet, reset, route and writer exclusion.
    // This method itself does not release roles or establish those proofs.
    template<class IO> bool RestageOneWithCopies(unsigned core,const uint32_t* reader,unsigned readerWords,
        const uint32_t* writer,unsigned writerWords,IO& io) {
#ifdef PIONEER_QWEN35_FLASH_CODE_OWNER_WITNESS
        (void)core;(void)reader;(void)readerWords;(void)writer;(void)writerWords;(void)io;
        return false;
#else
        if (fPhase!=Staged || !fCopyAwareMode || !fBoot.CanRestageCore(core)
            || !reader || !readerWords || !writer || !writerWords
            || readerWords>(DataflowRoleReaderShimLimit-DataflowRoleReaderShimAddress)/4
            || writerWords>(DataflowRoleWriterShimLimit-DataflowRoleWriterShimAddress)/4) return false;
        if (!VerifyStagedAllWithCopies(io)) return false;
        const uint32_t* savedReader=nullptr;const uint32_t* savedWriter=nullptr;bool captured=false;
        if (!io.StacklessHeldCopySources(core,savedReader,savedWriter,captured) || !captured
            || !savedReader || !savedWriter || !io.StacklessHeldCopySourceSafe(core,savedReader,savedWriter)) {
            fPhase=Faulted;fCopyAwareVerified=false;return false;
        }
        fPhase=Faulted;fCopyAwareVerified=false;
        if (!fBoot.RestageOne(core,reader,readerWords,writer,writerWords,io)
            || !fCode.VerifyStagedAllWithCopies(io) || !fBoot.VerifyStagedAll(io) || !AllHeld(io)
            || !io.StacklessHeldCopySourceSafe(core,savedReader,savedWriter)) return false;
        fPhase=Staged;fCopyAwareVerified=true;return true;
#endif
    }
    template<class IO> bool VerifyOriginalAll(IO& io) {
        if (fPhase!=Captured && fPhase!=Restored) return false;
#ifdef PIONEER_QWEN35_FLASH_CODE_OWNER_WITNESS
        if (!fFlashCaptured || fFlash.RestorationRequired() || !FlashScope(io,true)) return false;
#endif
        if (!AllHeld(io) || !fBoot.VerifyOriginalAll(io) || !fCode.VerifyOriginalAll(io)
#ifdef PIONEER_QWEN35_FLASH_CODE_OWNER_WITNESS
            || !fFlash.VerifyOriginalAll(io) || !FlashScope(io,true)
#endif
            || !AllHeld(io)) { fPhase=Faulted; return false; }
        return true;
    }
    template<class IO> bool RestoreAll(IO& io) {
        // Do not erase role-produced L1 copies until a separately proved
        // private-RAM restore has retired this copy-aware lifecycle.
        if (fCopyAwareMode || (fPhase!=Faulted && fPhase!=Staged
#ifdef PIONEER_QWEN35_FLASH_CODE_OWNER_WITNESS
            && !(fPhase==Captured && fFlash.RestorationRequired())
#endif
            )) return false;
        fPhase=Faulted;
#ifdef PIONEER_QWEN35_FLASH_CODE_OWNER_WITNESS
        if (!fFlashCaptured || !FlashScope(io,true) || !io.FlashRoleRestoreSafe(fFlashRestoreAttempted)) return false;
        fFlashRestoreAttempted=true;
#endif
        if (!AllHeld(io)) return false;
        // Restore boot entry points before the backing code. Unchanged owners
        // are still checked because a misrouted write may have touched them.
        if (fBoot.RestorationRequired()) {
            if (!fBoot.RestoreAll(io)) return false;
        } else if (!fBoot.VerifyOriginalAll(io)) return false;
        if (fCode.RestorationRequired()) {
            if (!fCode.RestoreAll(io)) return false;
        } else if (!fCode.VerifyOriginalAll(io)) return false;
#ifdef PIONEER_QWEN35_FLASH_CODE_OWNER_WITNESS
        if (fFlash.RestorationRequired()) {
            if (!fFlash.RestoreAll(io)) return false;
        } else if (!fFlash.VerifyOriginalAll(io)) return false;
#endif
        // A later restoration can misroute into an earlier region.
        if (!fBoot.VerifyOriginalAll(io) || !fCode.VerifyOriginalAll(io)
#ifdef PIONEER_QWEN35_FLASH_CODE_OWNER_WITNESS
            || !fFlash.VerifyOriginalAll(io) || !FlashScope(io,true) || !io.FlashRoleRestoreSafe(false)
#endif
            || !AllHeld(io))
            return false;
        fPhase=Restored;
        return true;
    }
    // The scope's fleet proof must remain true while boot and extra-L1
    // images are restored, even after private-RAM copies are overwritten.
    // A failure retains all host snapshots and requires a separate drain and
    // route proof before retry. No physical scope currently grants this.
    template<class IO> bool RestoreAfterFleetRamProof(IO& io) {
        if (!fCopyAwareMode || (fPhase!=Staged && fPhase!=Faulted)
            || !io.FleetRamImageRetirementSafe() || !AllHeld(io)) return false;
#ifdef PIONEER_QWEN35_FLASH_CODE_OWNER_WITNESS
        if (!fFlashCaptured || !FlashScope(io,true) || !io.FlashRoleRestoreSafe(fFlashRestoreAttempted)) return false;
        fFlashRestoreAttempted=true;
#endif
        if (!fCopyRetirementStarted) {
            if (fPhase!=Staged || !VerifyStagedAllWithCopies(io)) return false;
        } else if (!io.FleetRamImageRetirementRetrySafe()) return false;
        fCopyRetirementStarted=true;fCopyAwareVerified=false;fPhase=Faulted;
        if (fBoot.RestorationRequired()) {
            if (!fBoot.RestoreAll(io)) return false;
        } else if (!fBoot.VerifyOriginalAll(io)) return false;
        if (!io.FleetRamImageRetirementSafe() || !AllHeld(io)) return false;
        if (fCode.RestorationRequired()) {
            if (!fCode.RestoreAfterFleetRamProof(io)) return false;
        } else if (!fCode.VerifyOriginalAll(io)) return false;
#ifdef PIONEER_QWEN35_FLASH_CODE_OWNER_WITNESS
        if (fFlash.RestorationRequired()) {
            if (!fFlash.RestoreAll(io)) return false;
        } else if (!fFlash.VerifyOriginalAll(io)) return false;
#endif
        if (!fBoot.VerifyOriginalAll(io) || !fCode.VerifyOriginalAll(io)
#ifdef PIONEER_QWEN35_FLASH_CODE_OWNER_WITNESS
            || !fFlash.VerifyOriginalAll(io) || !FlashScope(io,true) || !io.FlashRoleRestoreSafe(false)
#endif
            || !io.FleetRamImageRetirementSafe() || !AllHeld(io)) return false;
        fPhase=Restored;return true;
    }
};
}
#endif
