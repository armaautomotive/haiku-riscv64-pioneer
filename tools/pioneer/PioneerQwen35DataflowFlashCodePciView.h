/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_QWEN35_DATAFLOW_FLASH_CODE_PCI_VIEW_H
#define PIONEER_QWEN35_DATAFLOW_FLASH_CODE_PCI_VIEW_H
#include "PioneerQwen35DataflowFlashCodeOwner.h"
namespace PioneerQwen35 {
// Fixed-window mapping witness. Guard must hold a pinned BAR, exclusive TLB
// route and held/quiet fleet, exclude all other writers, and retain snapshots
// and host buffers for each complete operation, not just at the check points.
// This class provides none of those proofs and is not bound to the driver.
// Failed mappings and writes retain their ledgers; no uncertain write is
// retried or treated as drained without an independent guard proof.
template<class Platform> class DataflowFlashCodePciView {
    Platform& fPlatform;int32_t fArea=-1;volatile uint32_t* fView=nullptr;
    uint64_t fBarStart=0,fBarEnd=0;bool fAttempted=false,fOpen=false;
    bool fWriteUnconfirmed=false,fCloseAttempted=false,fWasOpenAtClose=false;
    template<class Guard> bool Route(unsigned core,Guard& guard) const {
        return guard.ProvePinnedBar(fBarStart,fBarEnd) && guard.ProveFlashMappingArea(fArea)
            && guard.ProveSelectedRoute(core);
    }
public:
    explicit DataflowFlashCodePciView(Platform& p):fPlatform(p) {}
    DataflowFlashCodePciView(const DataflowFlashCodePciView&)=delete;
    DataflowFlashCodePciView& operator=(const DataflowFlashCodePciView&)=delete;
    bool Opened() const {return fOpen;}
    bool HasMapping() const {return fArea>=0;}
    bool WritesConfirmed() const {return !fWriteUnconfirmed;}
    template<class Guard> bool Open(uint64_t start,uint64_t end,Guard& guard) {
        if (fAttempted || fOpen || fArea>=0 || end<start || end-start==UINT64_MAX
            || !guard.ProvePinnedBar(start,end)) return false;
        uint64_t physical=0;
        if (!DataflowFlashCodePhysicalAddress(start,end-start+1,physical)
            || !guard.ProveFlashMappingSafe(physical,DataflowFlashCodeBytes)) return false;
        fAttempted=true;fBarStart=start;fBarEnd=end;
        int32_t area=-1;volatile uint32_t* view=nullptr;
        const bool okay=fPlatform.MapReadWrite(physical,DataflowFlashCodeBytes,area,view);
        if (area>=0) {fArea=area;fView=okay?view:nullptr;}
        if (!okay || area<0 || !view || !guard.ProveFlashMappingArea(area)
            || !guard.ProveFlashMappingSafe(physical,DataflowFlashCodeBytes)
            || !guard.ProvePinnedBar(start,end)) return false;
        fOpen=true;return true;
    }
    template<class Guard> bool Read(unsigned core,unsigned address,uint32_t* output,unsigned bytes,Guard& guard) {
        if (!fOpen || !fView || fWriteUnconfirmed || core>=DataflowBarrierWorkers || !output
            || !DataflowFlashCodeRange(address,bytes) || !guard.ProveFlashHostBuffer(output,bytes,true)
            || !Route(core,guard)
            || !guard.ProveFlashReadAllowed(core,address,bytes) || !Route(core,guard)) return false;
        const unsigned offset=(address-DataflowFlashCodeAddress)/4;
        for (unsigned i=0;i<bytes/4;i++) {
            if (!guard.ProveFlashReadAllowed(core,address+i*4,4) || !Route(core,guard)) return false;
            const uint32_t word=fPlatform.ReadWord(fView+offset+i);
            if (!guard.ProveFlashReadAllowed(core,address+i*4,4) || !Route(core,guard)) return false;
            output[i]=word;
        }
        return guard.ProveFlashHostBuffer(output,bytes,true)
            && guard.ProveFlashReadAllowed(core,address,bytes) && Route(core,guard);
    }
    template<class Guard> bool Write(unsigned core,unsigned address,const uint32_t* input,unsigned bytes,Guard& guard) {
        if (!fOpen || !fView || fWriteUnconfirmed || core>=DataflowBarrierWorkers || !input
            || !DataflowFlashCodeRange(address,bytes) || !guard.ProveFlashHostBuffer(input,bytes,false)
            || !Route(core,guard)
            || !guard.ProveFlashWriteAllowed(core,address,bytes) || !Route(core,guard)) return false;
        const unsigned offset=(address-DataflowFlashCodeAddress)/4;
        for (unsigned i=0;i<bytes/4;i++) {
            const unsigned at=address+i*4;
            if (!guard.ProveFlashWriteAllowed(core,at,4) || !Route(core,guard)) return false;
            fWriteUnconfirmed=true;
            fPlatform.WriteWord(fView+offset+i,input[i]);
            if (!Route(core,guard) || !guard.ProveFlashWriteAllowed(core,at,4)
                || !Route(core,guard) || fPlatform.ReadWord(fView+offset+i)!=input[i]
                || !guard.ProveFlashWriteAllowed(core,at,4) || !Route(core,guard)) return false;
            fWriteUnconfirmed=false;
        }
        if (!guard.ProveFlashHostBuffer(input,bytes,false)
            || !guard.ProveFlashWriteAllowed(core,address,bytes) || !Route(core,guard)) {
            fWriteUnconfirmed=true;return false;
        }
        return true;
    }
    // A later successful write is not a drainage proof. Only a separately
    // established retained-session proof may retire the sticky uncertainty.
    template<class Guard> bool ConfirmWriteDrain(Guard& guard) {
        if (!fOpen || !fWriteUnconfirmed || !guard.ProvePinnedBar(fBarStart,fBarEnd)
            || !guard.ProveFlashMappingArea(fArea) || !guard.ProveFlashWriteDrained()
            || !guard.ProvePinnedBar(fBarStart,fBarEnd) || !guard.ProveFlashMappingArea(fArea)
            || !guard.ProveFlashWriteDrained()) return false;
        fWriteUnconfirmed=false;return true;
    }
    template<class Guard> bool Close(Guard& guard) {
        if (fArea<0 || fCloseAttempted || fWriteUnconfirmed
            || !guard.ProvePinnedBar(fBarStart,fBarEnd) || !guard.ProveFlashMappingRetirementSafe(fArea)) return false;
        // An error can leave map state uncertain; this instance cannot retry.
        fCloseAttempted=true;fWasOpenAtClose=fOpen;fOpen=false;
        if (!fPlatform.Unmap(fArea)) return false;
        fArea=-1;fView=nullptr;return true;
    }
    // A failed unmap may have taken effect. Only an independent retained-area
    // identity proof can distinguish an absent original mapping from the same
    // still-live mapping; an area id alone may have been reused. The guard
    // must exclude concurrent unmap/remap and preserve the proof throughout.
    // A partial Open with no trusted view cannot use the mapped branch.
    template<class Guard> bool ReconcileFailedClose(Guard& guard) {
        if (fArea<0 || !fCloseAttempted || fOpen || fWriteUnconfirmed
            || !guard.ProvePinnedBar(fBarStart,fBarEnd)
            || !guard.ProveFlashMappingRetirementSafe(fArea)) return false;
        if (guard.ProveFlashOriginalMappingAbsent(fArea)) {
            if (!guard.ProvePinnedBar(fBarStart,fBarEnd)
                || !guard.ProveFlashMappingRetirementSafe(fArea)
                || !guard.ProveFlashOriginalMappingAbsent(fArea)) return false;
            fArea=-1;fView=nullptr;return true;
        }
        if (!fWasOpenAtClose || !fView || !guard.ProveFlashOriginalMappingExact(fArea,fView)
            || !guard.ProvePinnedBar(fBarStart,fBarEnd)
            || !guard.ProveFlashMappingRetirementSafe(fArea)
            || !guard.ProveFlashOriginalMappingExact(fArea,fView)) return false;
        fCloseAttempted=false;fOpen=true;return true;
    }
};

// Composition adapter for the fleet owner. Guard must keep its route lease
// and the same pinned BAR throughout all pre/post checks; selecting a route
// does not itself prove that any other actor is excluded. No production guard
// implements this contract yet.
template<class View,class Guard> class DataflowFlashCodePciOwnerScope {
    View& fView;Guard& fGuard;
public:
    DataflowFlashCodePciOwnerScope(View& view,Guard& guard):fView(view),fGuard(guard) {}
    bool FlashFleetHeldAndQuiet() {
        return fView.Opened() && fView.WritesConfirmed() && fGuard.ProveFlashFleetHeldAndQuiet();
    }
    bool FlashSnapshotSafe(const uint32_t* storage,unsigned words,bool frozen) {
        return fGuard.ProveFlashSnapshotSafe(storage,words,frozen);
    }
    bool FlashImageSafe(const uint32_t* image,unsigned words,const uint32_t* snapshot,unsigned savedWords) {
        return fGuard.ProveFlashImageSafe(image,words,snapshot,savedWords);
    }
    bool FlashRestoreSafe(bool retry) {
        return fView.WritesConfirmed() && fGuard.ProveFlashRestoreSafe(retry);
    }
    bool FlashReadAllowed(unsigned core,unsigned address,unsigned bytes) {
        return fGuard.SelectFlashRoute(core) && fGuard.ProveFlashReadAllowed(core,address,bytes);
    }
    bool FlashWriteAllowed(unsigned core,unsigned address,unsigned bytes) {
        return fGuard.SelectFlashRoute(core) && fGuard.ProveFlashWriteAllowed(core,address,bytes);
    }
    bool Read(unsigned core,unsigned address,uint32_t* output,unsigned bytes) {
        return fView.Read(core,address,output,bytes,fGuard);
    }
    bool Write(unsigned core,unsigned address,const uint32_t* input,unsigned bytes) {
        return fView.Write(core,address,input,bytes,fGuard);
    }
};
}
#endif
