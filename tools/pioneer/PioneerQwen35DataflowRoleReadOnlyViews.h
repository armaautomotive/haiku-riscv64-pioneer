/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_QWEN35_DATAFLOW_ROLE_READ_ONLY_VIEWS_H
#define PIONEER_QWEN35_DATAFLOW_ROLE_READ_ONLY_VIEWS_H
#include "PioneerQwen35DataflowRolePhysicalPlan.h"
namespace PioneerQwen35 {
// Non-owning route guard and platform provide only fixed read-only mappings.
// Guard must prove the pinned device/BAR and a stable selected tile before and
// after each read; these checks do not establish idle, NoC quiescence or write
// authority. No write, reset, launch, card command or live driver binding is
// provided. Failed partial maps remain tracked until an explicit Close.
template<class Platform> class DataflowRoleReadOnlyViews {
    Platform& fPlatform;
    int32_t fArea[DataflowRolePhysicalWindowCount];
    const volatile uint32_t* fView[DataflowRolePhysicalWindowCount]{};
    unsigned fBytes[DataflowRolePhysicalWindowCount]{};
    uint64_t fBarStart=0,fBarEnd=0;
    bool fAttempted=false,fOpen=false;
public:
    explicit DataflowRoleReadOnlyViews(Platform& platform):fPlatform(platform) {
        for (auto& area:fArea) area=-1;
    }
    DataflowRoleReadOnlyViews(const DataflowRoleReadOnlyViews&)=delete;
    DataflowRoleReadOnlyViews& operator=(const DataflowRoleReadOnlyViews&)=delete;
    bool Opened() const { return fOpen; }
    bool HasMappings() const {
        for (auto area:fArea) if (area>=0) return true;
        return false;
    }
    template<class Guard> bool Open(uint64_t barStart,uint64_t barEnd,Guard& guard) {
        if (fAttempted || fOpen || HasMappings() || barEnd<barStart
            || barEnd-barStart==UINT64_MAX || !guard.ProvePinnedBar(barStart,barEnd)) return false;
        uint64_t addresses[DataflowRolePhysicalWindowCount]{};
        unsigned lengths[DataflowRolePhysicalWindowCount]{};
        const uint64_t barBytes=barEnd-barStart+1;
        for (unsigned i=0;i<DataflowRolePhysicalWindowCount;i++)
            if (!DataflowRolePhysicalAddress(barStart,barBytes,i,addresses[i],lengths[i])) return false;
        fAttempted=true;
        for (unsigned i=0;i<DataflowRolePhysicalWindowCount;i++) {
            int32_t area=-1; const volatile uint32_t* view=nullptr;
            const bool okay=fPlatform.MapReadOnly(addresses[i],lengths[i],area,view);
            if (area>=0) { fArea[i]=area; fView[i]=view; fBytes[i]=lengths[i]; }
            if (!okay || area<0 || !view) return false;
        }
        if (!guard.ProvePinnedBar(barStart,barEnd)) return false;
        fBarStart=barStart; fBarEnd=barEnd; fOpen=true; return true;
    }
    template<class Guard> bool Read(unsigned core,unsigned which,unsigned byteOffset,
        uint32_t& output,Guard& guard) {
        if (!fOpen || core>=DataflowBarrierWorkers || which>=DataflowRolePhysicalWindowCount
            || fArea[which]<0 || !fView[which] || (byteOffset&3)
            || byteOffset>fBytes[which] || 4>fBytes[which]-byteOffset
            || !DataflowRolePhysicalReadAllowed(which,byteOffset)
            || !guard.ProvePinnedBar(fBarStart,fBarEnd) || !guard.ProveSelectedRoute(core)
            || !guard.ProveReadAllowed(core,which,byteOffset)) return false;
        const uint32_t value=fPlatform.ReadWord(fView[which]+byteOffset/4);
        if (!guard.ProveReadAllowed(core,which,byteOffset) || !guard.ProveSelectedRoute(core)
            || !guard.ProvePinnedBar(fBarStart,fBarEnd)) return false;
        output=value; return true;
    }
    bool Close() {
        fOpen=false; fBarStart=0; fBarEnd=0;
        bool okay=true;
        for (unsigned n=DataflowRolePhysicalWindowCount;n>0;n--) {
            const unsigned i=n-1;
            if (fArea[i]<0) continue;
            if (!fPlatform.Unmap(fArea[i])) { okay=false; continue; }
            fArea[i]=-1; fView[i]=nullptr; fBytes[i]=0;
        }
        return okay && !HasMappings();
    }
};
}
#endif
