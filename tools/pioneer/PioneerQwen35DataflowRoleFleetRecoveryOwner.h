/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_QWEN35_DATAFLOW_ROLE_FLEET_RECOVERY_OWNER_H
#define PIONEER_QWEN35_DATAFLOW_ROLE_FLEET_RECOVERY_OWNER_H
#include "PioneerQwen35DataflowRoleFleetImagesOwner.h"
#include "PioneerQwen35DataflowRoleFleetLocalRamOwner.h"
namespace PioneerQwen35 {
// Offline composition for 120 tiles' additional code, low shims, boot words
// and B/NC private RAM. It does not release or stop RISCs, reset hardware,
// audit physical PCs/NoCs, own the existing TRISC0 image, or provide PCI IO.
// IO must serialize the entire fleet and prove held/quiet/default routing and
// the private-RAM initialization window independently. Allocate this owner
// outside the kernel stack. A release attempt is allowed only after the
// caller has obtained every OTHER required physical proof as well.
class DataflowRoleFleetRecoveryOwner {
    enum Phase { Empty, Captured, Faulted, Staged, Armed, ReleaseAttempted, Restored, Unavailable };
    Phase fPhase=Empty;
    DataflowRoleFleetImagesOwner fImages;
    DataflowRoleFleetLocalRamOwner fRam;
    template<class IO> static bool AllSafe(IO& io) {
        for (unsigned core=0;core<DataflowBarrierWorkers;core++)
            if (!io.AllRolesHeld(core) || !io.DefaultBootRouting(core)
                || !io.LocalRamStable(core)) return false;
        return true;
    }
public:
    DataflowRoleFleetRecoveryOwner()=default;
    DataflowRoleFleetRecoveryOwner(const DataflowRoleFleetRecoveryOwner&)=delete;
    DataflowRoleFleetRecoveryOwner& operator=(const DataflowRoleFleetRecoveryOwner&)=delete;
    static constexpr unsigned CodeWords() { return DataflowRoleFleetImagesOwner::RequiredWords(); }
    static constexpr unsigned RamWords() { return DataflowRoleFleetLocalRamOwner::RequiredWords(); }
    bool ReleaseLedgerPrepared() const { return fPhase==Armed && fRam.MarkedForPossibleRelease(); }
    bool RestorationRequired() const {
        return fPhase==Faulted || fPhase==Staged || fPhase==Armed || fPhase==ReleaseAttempted;
    }
    bool CoveredStateRestored() const { return fPhase==Restored; }
    template<class IO> bool CaptureAll(uint32_t* code,unsigned codeWords,uint32_t* ram,
        unsigned ramWords,IO& io) {
        if (fPhase!=Empty || !AllSafe(io)) return false;
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
    template<class IO> bool StageImages(const uint32_t* reader,unsigned readerWords,
        const uint32_t* writer,unsigned writerWords,const uint32_t* readerShim,
        unsigned readerShimWords,const uint32_t* writerShim,unsigned writerShimWords,IO& io) {
        if (fPhase!=Captured) return false;
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
    // Ledger only: no processor is released. If any later release attempt is
    // uncertain, all private RAM snapshots remain retained for restoration.
    template<class IO> bool PreparePossibleRelease(IO& io) {
        if (fPhase!=Staged) return false;
        fPhase=Faulted;
        if (!AllSafe(io) || !fImages.VerifyStagedAll(io) || !fRam.VerifyOriginalAll(io)
            || !fRam.MarkMayRunAll(io) || !fImages.VerifyStagedAll(io) || !AllSafe(io))
            return false;
        fPhase=Armed; return true;
    }
    // Consume the one-shot ledger BEFORE the physical release attempt. A
    // partial or uncertain release must never be retried by this owner.
    template<class IO> bool BeginReleaseAttempt(IO& io) {
        if (fPhase!=Armed) return false;
        fPhase=Faulted;
        if (!AllSafe(io) || !fRam.MarkedForPossibleRelease()
            || !fImages.VerifyStagedAll(io) || !AllSafe(io)) return false;
        fPhase=ReleaseAttempted; return true;
    }
    template<class IO> bool RestoreAll(IO& io) {
        if (fPhase!=Faulted && fPhase!=Staged && fPhase!=Armed && fPhase!=ReleaseAttempted) return false;
        fPhase=Faulted;
        if (!AllSafe(io)) return false;
        if (fImages.RestorationRequired()) {
            if (!fImages.RestoreAll(io)) return false;
        } else if (!fImages.VerifyOriginalAll(io)) return false;
        if (fRam.RestorationRequired()) {
            if (!fRam.RestoreAll(io)) return false;
        } else if (!fRam.VerifyOriginalAll(io)) return false;
        // A later restoration may misroute into a previously verified region.
        if (!fImages.VerifyOriginalAll(io) || !fRam.VerifyOriginalAll(io) || !AllSafe(io))
            return false;
        fPhase=Restored; return true;
    }
};
}
#endif
