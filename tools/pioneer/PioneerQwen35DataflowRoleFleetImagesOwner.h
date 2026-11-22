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
    bool StagedAndVerified() const { return fPhase==Staged; }
    bool RestorationRequired() const { return fPhase==Faulted || fPhase==Staged; }
    bool ImagesRestored() const { return fPhase==Restored; }
    template<class IO> bool CaptureAll(uint32_t* storage,unsigned words,IO& io) {
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
        if (fPhase!=Staged) return false;
        if (!AllHeld(io) || !fCode.VerifyStagedAll(io) || !fBoot.VerifyStagedAll(io)
            || !AllHeld(io)) { fPhase=Faulted; return false; }
        return true;
    }
    template<class IO> bool VerifyOriginalAll(IO& io) {
        if (fPhase!=Captured && fPhase!=Restored) return false;
        if (!AllHeld(io) || !fBoot.VerifyOriginalAll(io) || !fCode.VerifyOriginalAll(io)
            || !AllHeld(io)) { fPhase=Faulted; return false; }
        return true;
    }
    template<class IO> bool RestoreAll(IO& io) {
        if (fPhase!=Faulted && fPhase!=Staged) return false;
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
