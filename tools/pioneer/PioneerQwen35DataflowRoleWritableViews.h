/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_QWEN35_DATAFLOW_ROLE_WRITABLE_VIEWS_H
#define PIONEER_QWEN35_DATAFLOW_ROLE_WRITABLE_VIEWS_H
#include "PioneerQwen35DataflowRolePhysicalPlan.h"
namespace PioneerQwen35 {
// Fixed writable mappings for a future role owner. Guard must prove the
// exclusive session, selected TLB route, all relevant cores held and quiet,
// RAM stability, and a retained snapshot before every memory/control write.
// This class neither obtains those proofs nor performs reset/launch. A failed
// write can have taken effect; the caller must retain its recovery ledger.
template<class Platform> class DataflowRoleWritableViews {
    enum : unsigned { Count=DataflowRolePhysicalControl+1 };
    Platform& fPlatform;
    int32_t fArea[Count];
    volatile uint32_t* fView[Count]{};
    unsigned fBytes[Count]{};
    uint64_t fBarStart=0,fBarEnd=0;
    bool fAttempted=false,fOpen=false;
    template<class Guard> bool Route(unsigned core,Guard& guard) {
        return guard.ProvePinnedBar(fBarStart,fBarEnd) && guard.ProveSelectedRoute(core);
    }
public:
    explicit DataflowRoleWritableViews(Platform& platform):fPlatform(platform) {
        for (auto& area:fArea) area=-1;
    }
    DataflowRoleWritableViews(const DataflowRoleWritableViews&)=delete;
    DataflowRoleWritableViews& operator=(const DataflowRoleWritableViews&)=delete;
    bool Opened() const { return fOpen; }
    bool HasMappings() const { for (auto area:fArea) if (area>=0) return true; return false; }
    template<class Guard> bool Open(uint64_t start,uint64_t end,Guard& guard) {
        if (fAttempted || fOpen || HasMappings() || end<start || end-start==UINT64_MAX
            || !guard.ProvePinnedBar(start,end)) return false;
        uint64_t addresses[Count]{}; unsigned lengths[Count]{};
        for (unsigned i=0;i<Count;i++)
            if (!DataflowRolePhysicalAddress(start,end-start+1,i,addresses[i],lengths[i])) return false;
        fAttempted=true; fBarStart=start; fBarEnd=end;
        for (unsigned i=0;i<Count;i++) {
            int32_t area=-1; volatile uint32_t* view=nullptr;
            const bool okay=fPlatform.MapReadWrite(addresses[i],lengths[i],area,view);
            if (area>=0) { fArea[i]=area; fView[i]=okay?view:nullptr; fBytes[i]=lengths[i]; }
            if (!okay || area<0 || !view) return false;
        }
        if (!guard.ProvePinnedBar(start,end)) return false;
        fOpen=true; return true;
    }
    // A partially failed Close can leave some areas mapped and others gone.
    // Re-map only the missing fixed windows so a later close transaction can
    // re-audit snapshots before trying cleanup again. The caller still owns
    // the session and must keep every core held throughout this operation.
    template<class Guard> bool ReopenForRecovery(Guard& guard) {
        if (!fAttempted || fOpen || !HasMappings() || fBarEnd<fBarStart
            || fBarEnd-fBarStart==UINT64_MAX || !guard.ProvePinnedBar(fBarStart,fBarEnd)) return false;
        const uint64_t bytes=fBarEnd-fBarStart+1;
        for (unsigned i=0;i<Count;i++) {
            if (fArea[i]>=0 && !fView[i]) {
                if (!fPlatform.Unmap(fArea[i])) return false;
                fArea[i]=-1; fBytes[i]=0;
            }
            if (fArea[i]>=0) continue;
            uint64_t physical=0; unsigned length=0;
            if (!DataflowRolePhysicalAddress(fBarStart,bytes,i,physical,length)) return false;
            int32_t area=-1; volatile uint32_t* view=nullptr;
            const bool okay=fPlatform.MapReadWrite(physical,length,area,view);
            if (area>=0) { fArea[i]=area; fView[i]=okay?view:nullptr; fBytes[i]=length; }
            if (!okay || area<0 || !view) return false;
        }
        if (!guard.ProvePinnedBar(fBarStart,fBarEnd)) return false;
        fOpen=true; return true;
    }
    template<class Guard> bool Read(unsigned core,unsigned which,unsigned offset,uint32_t& output,Guard& guard) {
        if (!fOpen || core>=DataflowBarrierWorkers || which>=Count || fArea[which]<0
            || !fView[which] || (offset&3) || offset>fBytes[which] || 4>fBytes[which]-offset
            || !DataflowRolePhysicalReadAllowed(which,offset) || !Route(core,guard)
            || !guard.ProveReadAllowed(core,which,offset) || !Route(core,guard)) return false;
        const uint32_t value=fPlatform.ReadWord(fView[which]+offset/4);
        if (!guard.ProveReadAllowed(core,which,offset) || !Route(core,guard)) return false;
        output=value; return true;
    }
    // Read-only comparison of fixed, contiguous image windows. Route and
    // access authority are checked around each bounded chunk; the caller
    // must serialize TLB selection and exclude concurrent image writers.
    template<class Guard> bool Matches(unsigned core,unsigned which,unsigned offset,
        const uint32_t* expected,unsigned words,Guard& guard) {
        if (!fOpen || !expected || !words || words>UINT32_MAX/4 || core>=DataflowBarrierWorkers
            || which>DataflowRolePhysicalExtraCode || fArea[which]<0 || !fView[which]
            || (offset&3) || offset>fBytes[which] || words*4>fBytes[which]-offset
            || !DataflowRolePhysicalReadAllowed(which,offset)
            || !DataflowRolePhysicalReadAllowed(which,offset+(words-1)*4)) return false;
        for (unsigned first=0;first<words;) {
            const unsigned count=words-first<64?words-first:64;
            const unsigned start=offset+first*4,end=start+(count-1)*4;
            if (!Route(core,guard) || !guard.ProveReadAllowed(core,which,start)
                || !guard.ProveReadAllowed(core,which,end) || !Route(core,guard)) return false;
            for (unsigned i=0;i<count;i++)
                if (fPlatform.ReadWord(fView[which]+start/4+i)!=expected[first+i]) return false;
            if (!guard.ProveReadAllowed(core,which,start) || !guard.ProveReadAllowed(core,which,end)
                || !Route(core,guard)) return false;
            first+=count;
        }
        return true;
    }
    // Copy fixed L1 image data in bounded chunks. A failed call can partially
    // fill output; the owner must not publish it as a valid snapshot.
    template<class Guard> bool CopyWords(unsigned core,unsigned which,unsigned offset,
        uint32_t* output,unsigned words,Guard& guard) {
        if (!fOpen || !output || !words || words>UINT32_MAX/4 || core>=DataflowBarrierWorkers
            || which>DataflowRolePhysicalExtraCode || fArea[which]<0 || !fView[which]
            || (offset&3) || offset>fBytes[which] || words*4>fBytes[which]-offset
            || !DataflowRolePhysicalReadAllowed(which,offset)
            || !DataflowRolePhysicalReadAllowed(which,offset+(words-1)*4)) return false;
        for (unsigned first=0;first<words;) {
            const unsigned count=words-first<64?words-first:64;
            const unsigned start=offset+first*4,end=start+(count-1)*4;
            if (!Route(core,guard) || !guard.ProveReadAllowed(core,which,start)
                || !guard.ProveReadAllowed(core,which,end) || !Route(core,guard)) return false;
            for (unsigned i=0;i<count;i++)
                output[first+i]=fPlatform.ReadWord(fView[which]+start/4+i);
            if (!guard.ProveReadAllowed(core,which,start) || !guard.ProveReadAllowed(core,which,end)
                || !Route(core,guard)) return false;
            first+=count;
        }
        return true;
    }
    template<class Guard> bool WriteWords(unsigned core,unsigned which,unsigned offset,
        const uint32_t* input,unsigned bytes,Guard& guard) {
        if (!fOpen || !input || core>=DataflowBarrierWorkers || which>=DataflowRolePhysicalControl
            || fArea[which]<0 || !fView[which] || !DataflowRolePhysicalWriteRange(which,offset,bytes)
            || offset>fBytes[which] || bytes>fBytes[which]-offset) return false;
        for (unsigned n=0;n<bytes/4;n++) {
            const unsigned at=offset+n*4;
            if (!Route(core,guard) || !guard.ProveWriteAllowed(core,which,at,4)
                || !Route(core,guard)) return false;
            fPlatform.WriteWord(fView[which]+at/4,input[n]);
            if (!Route(core,guard) || !guard.ProveWriteAllowed(core,which,at,4)
                || !Route(core,guard) || fPlatform.ReadWord(fView[which]+at/4)!=input[n]
                || !guard.ProveWriteAllowed(core,which,at,4) || !Route(core,guard)) return false;
        }
        return true;
    }
    // Only reset-preservation bits 6..9 may change, and only from the exact
    // expected value under the caller's retained-snapshot proof. Soft reset
    // itself is deliberately not writable through this interface.
    template<class Guard> bool WriteResetDisable(unsigned core,uint32_t expected,uint32_t desired,Guard& guard) {
        const unsigned which=DataflowRolePhysicalControl,offset=0x224;
        if (!fOpen || core>=DataflowBarrierWorkers || fArea[which]<0 || !fView[which]
            || ((expected^desired)&~uint32_t(DataflowRolePreserveResetMask))
            || !DataflowRolePhysicalWriteRange(which,offset,4) || !Route(core,guard)
            || !guard.ProveControlWriteAllowed(core,expected,desired)
            || !Route(core,guard) || fPlatform.ReadWord(fView[which]+offset/4)!=expected
            || !guard.ProveControlWriteAllowed(core,expected,desired) || !Route(core,guard)) return false;
        fPlatform.WriteWord(fView[which]+offset/4,desired);
        return Route(core,guard) && guard.ProveControlWriteAllowed(core,expected,desired)
            && Route(core,guard) && fPlatform.ReadWord(fView[which]+offset/4)==desired
            && guard.ProveControlWriteAllowed(core,expected,desired) && Route(core,guard);
    }
    bool Close() {
        fOpen=false;
        bool okay=true;
        for (unsigned n=Count;n>0;n--) {
            const unsigned i=n-1;
            if (fArea[i]<0) continue;
            if (!fPlatform.Unmap(fArea[i])) { okay=false; continue; }
            fArea[i]=-1; fView[i]=nullptr; fBytes[i]=0;
        }
        if (!okay || HasMappings()) return false;
        fBarStart=fBarEnd=0; return true;
    }
};
}
#endif
