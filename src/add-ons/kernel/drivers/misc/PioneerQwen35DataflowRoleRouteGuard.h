/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_QWEN35_DATAFLOW_ROLE_ROUTE_GUARD_H
#define PIONEER_QWEN35_DATAFLOW_ROLE_ROUTE_GUARD_H
#include "PioneerMinimumBarLayout.h"
#include "BlackholeDramSamplesProbe.h"
#include "../../../../../tools/pioneer/PioneerQwen35DataflowBarrierLayout.h"
#include "../../../../../tools/pioneer/PioneerQwen35DataflowNocProtocol.h"
#include "../../../../../tools/pioneer/PioneerQwen35DataflowRolePhysicalPlan.h"
// Read-only proof for an ALREADY selected core in an exclusively serialized
// Qwen35 session. Scope must prove the real retained rewrite owner and guard;
// this header does not select/reprogram TLBs, prove idle, or authorize writes.
// It is not currently constructed by a live driver operation.
template<class IO,class Scope> class PioneerQwen35RoleRouteGuard {
    IO& fIO;
    Scope& fScope;
    bool TlBsMatch(unsigned core) {
        const uint32_t route=PioneerQwen35::DataflowBarrierWorkerRoute(core);
        if (route==UINT32_MAX) return false;
        const unsigned x=route&63U,y=(route>>6)&63U;
        for (unsigned slot=0;slot<2;slot++) for (unsigned word=0;word<3;word++) {
            const uint32_t expected=BlackholeDramFixedTlbWord(slot?0xffa00000U:0U,x,y,false,word);
            if (fIO.DramTlbRead(slot,word)!=expected) return false;
        }
        return true;
    }
public:
    PioneerQwen35RoleRouteGuard(IO& io,Scope& scope):fIO(io),fScope(scope) {}
    bool ProvePinnedBar(uint64_t start,uint64_t end) {
        return fScope.ProveExclusiveRewrite(fIO) && fIO.fleetEnabled
            && PioneerMinimumBarLayoutMatches(fIO.profile)
            && fIO.profile.bar0_host_start==start && fIO.profile.bar0_host_end==end;
    }
    bool ProveReadAllowed(unsigned core,unsigned which,unsigned offset) {
        using namespace PioneerQwen35;
        if (core>=DataflowBarrierWorkers || !fScope.ProveExclusiveRewrite(fIO)
            || !DataflowRolePhysicalReadAllowed(which,offset)) return false;
        if (which<=DataflowRolePhysicalNcriscRam)
            return fScope.ProveRoleMemoryReadSafe(fIO,core,which);
        return true; // fixed status only; still not an idle proof
    }
    bool ProveSelectedRoute(unsigned core) {
        if (core>=PioneerQwen35::DataflowBarrierWorkers
            || !ProvePinnedBar(fIO.profile.bar0_host_start,fIO.profile.bar0_host_end)
            || fIO.runtimeCore!=core || !fIO.DataflowPanelMappingStable()
            || !TlBsMatch(core)) return false;
        const uint32_t firstRaw=fIO.ResidentNocRead(PioneerQwen35::DataflowNocLogicalIdOffset);
        fIO.Pause();
        const uint32_t secondRaw=fIO.ResidentNocRead(PioneerQwen35::DataflowNocLogicalIdOffset);
        uint32_t first=0,second=0;
        if (!PioneerQwen35::DataflowNocDecodeLocalRoute(firstRaw,first)
            || !PioneerQwen35::DataflowNocDecodeLocalRoute(secondRaw,second)
            || first!=second || !PioneerQwen35::DataflowNocRouteMatchesWorker(first,core)) return false;
        return fIO.runtimeCore==core && fIO.DataflowPanelMappingStable() && TlBsMatch(core)
            && ProvePinnedBar(fIO.profile.bar0_host_start,fIO.profile.bar0_host_end);
    }
};
#endif
