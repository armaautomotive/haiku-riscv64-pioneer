/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_QWEN35_DATAFLOW_ROLE_FLEET_IMAGES_OWNER_H
#define PIONEER_QWEN35_DATAFLOW_ROLE_FLEET_IMAGES_OWNER_H
#include "PioneerQwen35DataflowRoleFleetCodeOwner.h"
#include "PioneerQwen35DataflowRoleFleetBootOwner.h"
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
    DataflowRoleFleetCodeOwner fCode;
    DataflowRoleFleetBootOwner fBoot;
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
    bool StagedAndVerified() const { return fPhase==Staged && !fCopyAwareMode; }
    bool CopyAwareStagedAndVerified() const { return fPhase==Staged && fCopyAwareMode && fCopyAwareVerified; }
    // Call before the first release that can populate private-RAM copies.
    // This is a software retention ledger, not a physical ownership proof.
    bool BeginCopyAwareOwnership() {
        if (fPhase!=Staged || fCopyAwareMode || !fCode.BeginCopyAwareOwnership()) return false;
        fCopyAwareMode=true;fCopyAwareVerified=false;return true;
    }
    bool RestorationRequired() const { return fPhase==Faulted || fPhase==Staged; }
    bool ImagesRestored() const { return fPhase==Restored; }
    // Keep a distinct frame for the Haiku qualification's stack-budget audit.
    template<class IO> __attribute__((noinline)) bool CaptureAll(uint32_t* storage,unsigned words,IO& io) {
        if (fPhase!=Empty || !AllHeld(io)) return false;
        if (!fCode.CaptureAll(storage,words,io) || !fBoot.CaptureAll(io) || !AllHeld(io)) {
            fPhase=Unavailable; return false;
        }
        fPhase=Captured;
        return true;
    }
    template<class IO> bool StageAll(const uint32_t* reader,unsigned readerWords,
        const uint32_t* writer,unsigned writerWords,const uint32_t* readerShim,
        unsigned readerShimWords,const uint32_t* writerShim,unsigned writerShimWords,IO& io) {
        if (fPhase!=Captured || !reader || !readerWords || !writer || !writerWords
            || !readerShim || !readerShimWords || !writerShim || !writerShimWords) return false;
        if (!AllHeld(io)) { fPhase=Faulted; return false; }
        fPhase=Faulted;
        // High code first, shims and reset entry words second. No processor
        // may start here: complete fleet readback is only one launch proof.
        if (!fCode.StageAll(reader,readerWords,writer,writerWords,io)
            || !fBoot.StageAll(readerShim,readerShimWords,writerShim,writerShimWords,io)
            || !fCode.VerifyStagedAll(io) || !fBoot.VerifyStagedAll(io) || !AllHeld(io))
            return false;
        fPhase=Staged;
        return true;
    }
    template<class IO> bool VerifyStagedAll(IO& io) {
        if (fPhase!=Staged || fCopyAwareMode) return false;
        if (!AllHeld(io) || !fCode.VerifyStagedAll(io) || !fBoot.VerifyStagedAll(io)
            || !AllHeld(io)) { fPhase=Faulted; return false; }
        return true;
    }
    // An independent retained-copy proof is required for every captured
    // core; ordinary VerifyStagedAll deliberately still rejects those changes.
    template<class IO> bool VerifyStagedAllWithCopies(IO& io) {
        if (fPhase!=Staged || !fCopyAwareMode) return false;
        fCopyAwareVerified=false;
        if (!AllHeld(io) || !fCode.VerifyStagedAllWithCopies(io) || !fBoot.VerifyStagedAll(io)
            || !AllHeld(io)) { fPhase=Faulted; return false; }
        fCopyAwareVerified=true;
        return true;
    }
    // The startup qualification executes one tile, but snapshots and checks
    // all 120 so writes routed to another tile remain detectable.
    template<class IO> bool StageOne(unsigned core,const uint32_t* reader,unsigned readerWords,
        const uint32_t* writer,unsigned writerWords,const uint32_t* readerShim,
        unsigned readerShimWords,const uint32_t* writerShim,unsigned writerShimWords,IO& io) {
        if (fPhase!=Captured || core>=DataflowBarrierWorkers || !reader || !readerWords
            || !writer || !writerWords || !readerShim || !readerShimWords
            || !writerShim || !writerShimWords) return false;
        fPhase=Faulted;
        if (!AllHeld(io) || !fCode.StageOne(core,reader,readerWords,writer,writerWords,io)
            || !fBoot.StageOne(core,readerShim,readerShimWords,writerShim,writerShimWords,io)
            || !fCode.VerifyStagedAll(io) || !fBoot.VerifyStagedAll(io) || !AllHeld(io)) return false;
        fPhase=Staged;return true;
    }
    // Offline-only building block for replacing held-core shims. Code, other
    // cores and original snapshots remain owned and audited throughout. This
    // still requires the original staged extra-L1 contents: role-produced
    // RAM copies change that region and need the separate copy-aware path
    // and its currently unavailable physical authority. Never clear a
    // copy merely to make this audit pass.
    template<class IO> bool RestageOne(unsigned core,const uint32_t* reader,unsigned readerWords,
        const uint32_t* writer,unsigned writerWords,IO& io) {
        if (fPhase!=Staged || fCopyAwareMode || !fBoot.CanRestageCore(core)
            || !reader || !readerWords || !writer || !writerWords
            || readerWords>(DataflowRoleReaderShimLimit-DataflowRoleReaderShimAddress)/4
            || writerWords>(DataflowRoleWriterShimLimit-DataflowRoleWriterShimAddress)/4) return false;
        if (!VerifyStagedAll(io)) return false;
        fPhase=Faulted;
        if (!fBoot.RestageOne(core,reader,readerWords,writer,writerWords,io)
            || !fCode.VerifyStagedAll(io) || !fBoot.VerifyStagedAll(io) || !AllHeld(io)) return false;
        fPhase=Staged;return true;
    }
    // Copy-aware counterpart for a future stackless restore transaction. IO
    // must bind all captured L1 copy windows to protected retained snapshots,
    // and separately prove held fleet, reset, route and writer exclusion.
    // This method itself does not release roles or establish those proofs.
    template<class IO> bool RestageOneWithCopies(unsigned core,const uint32_t* reader,unsigned readerWords,
        const uint32_t* writer,unsigned writerWords,IO& io) {
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
    }
    template<class IO> bool VerifyOriginalAll(IO& io) {
        if (fPhase!=Captured && fPhase!=Restored) return false;
        if (!AllHeld(io) || !fBoot.VerifyOriginalAll(io) || !fCode.VerifyOriginalAll(io)
            || !AllHeld(io)) { fPhase=Faulted; return false; }
        return true;
    }
    template<class IO> bool RestoreAll(IO& io) {
        // Do not erase role-produced L1 copies until a separately proved
        // private-RAM restore has retired this copy-aware lifecycle.
        if (fCopyAwareMode || (fPhase!=Faulted && fPhase!=Staged)) return false;
        fPhase=Faulted;
        if (!AllHeld(io)) return false;
        // Restore boot entry points before the backing code. Unchanged owners
        // are still checked because a misrouted write may have touched them.
        if (fBoot.RestorationRequired()) {
            if (!fBoot.RestoreAll(io)) return false;
        } else if (!fBoot.VerifyOriginalAll(io)) return false;
        if (fCode.RestorationRequired()) {
            if (!fCode.RestoreAll(io)) return false;
        } else if (!fCode.VerifyOriginalAll(io)) return false;
        // A later restoration can misroute into an earlier region.
        if (!fBoot.VerifyOriginalAll(io) || !fCode.VerifyOriginalAll(io) || !AllHeld(io))
            return false;
        fPhase=Restored;
        return true;
    }
};
}
#endif
