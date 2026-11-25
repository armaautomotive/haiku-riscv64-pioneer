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
template<class IO> struct PioneerQ35DefaultRoleRouteAccess {
    IO& io;
    explicit PioneerQ35DefaultRoleRouteAccess(IO& value):io(value) {}
    bool Allowed() { return true; }
    uint32_t DramTlbRead(unsigned slot,unsigned word) { return io.DramTlbRead(slot,word); }
    uint32_t ResidentNocRead(unsigned offset) { return io.ResidentNocRead(offset); }
};
template<class IO,class Scope,class Access=PioneerQ35DefaultRoleRouteAccess<IO>> class PioneerQwen35RoleRouteGuard {
    IO& fIO;
    Scope& fScope;
    Access fAccess;
    bool TlBsMatch(unsigned core) {
        const uint32_t route=PioneerQwen35::DataflowBarrierWorkerRoute(core);
        if (route==UINT32_MAX) return false;
        const unsigned x=route&63U,y=(route>>6)&63U;
        for (unsigned slot=0;slot<2;slot++) for (unsigned word=0;word<3;word++) {
            const uint32_t expected=BlackholeDramFixedTlbWord(slot?0xffa00000U:0U,x,y,false,word);
            if (fAccess.DramTlbRead(slot,word)!=expected) return false;
        }
        return true;
    }
public:
    PioneerQwen35RoleRouteGuard(IO& io,Scope& scope):fIO(io),fScope(scope),fAccess(io) {}
    template<class Selector>
    PioneerQwen35RoleRouteGuard(IO& io,Scope& scope,Selector& selector)
        :fIO(io),fScope(scope),fAccess(io,selector) {}
    bool ProvePinnedBar(uint64_t start,uint64_t end) {
        return fAccess.Allowed() && fScope.ProveExclusiveRewrite(fIO) && fAccess.Allowed() && fIO.fleetEnabled
            && PioneerMinimumBarLayoutMatches(fIO.profile)
            && fIO.profile.bar0_host_start==start && fIO.profile.bar0_host_end==end;
    }
    bool ProveReadAllowed(unsigned core,unsigned which,unsigned offset) {
        using namespace PioneerQwen35;
        if (core>=DataflowBarrierWorkers || !fAccess.Allowed() || !fScope.ProveExclusiveRewrite(fIO)
            || !DataflowRolePhysicalReadAllowed(which,offset)) return false;
        if (which<=DataflowRolePhysicalNcriscRam)
            return fScope.ProveRoleMemoryReadSafe(fIO,core,which) && fAccess.Allowed();
        return fAccess.Allowed(); // fixed status only; still not an idle proof
    }
#ifdef PIONEER_QWEN35_ROLE_WRITE_COMPILE_ONLY
#if !defined(PIONEER_QWEN35_RECOVERY_COMPILE_ONLY) || !defined(PIONEER_QWEN35_DEVICE_DISPATCH)
#error Writable role guard is a non-loadable recovery witness only
#endif
    bool ProveWriteAllowed(unsigned core,unsigned which,unsigned offset,unsigned bytes) {
        using namespace PioneerQwen35;
        return core<DataflowBarrierWorkers && which<DataflowRolePhysicalControl && fAccess.Allowed()
            && fScope.ProveExclusiveRewrite(fIO) && DataflowRolePhysicalWriteRange(which,offset,bytes)
            && fScope.ProveRoleMemoryWriteSafe(fIO,core,which,offset,bytes) && fAccess.Allowed();
    }
    bool ProveControlWriteAllowed(unsigned core,uint32_t expected,uint32_t desired) {
        using namespace PioneerQwen35;
        return core<DataflowBarrierWorkers && fAccess.Allowed() && fScope.ProveExclusiveRewrite(fIO)
            && !((expected^desired)&~uint32_t(DataflowSharedPreserveResetMask))
            && fScope.ProveRoleControlWriteSafe(fIO,core,expected,desired) && fAccess.Allowed();
    }
#endif
    bool ProveSelectedRoute(unsigned core) {
        if (core>=PioneerQwen35::DataflowBarrierWorkers
            || !ProvePinnedBar(fIO.profile.bar0_host_start,fIO.profile.bar0_host_end)
            || fIO.runtimeCore!=core || !fIO.DataflowPanelMappingStable()
            || !TlBsMatch(core)) return false;
        const uint32_t firstRaw=fAccess.ResidentNocRead(PioneerQwen35::DataflowNocLogicalIdOffset);
        fIO.Pause();
        const uint32_t secondRaw=fAccess.ResidentNocRead(PioneerQwen35::DataflowNocLogicalIdOffset);
        uint32_t first=0,second=0;
        if (!PioneerQwen35::DataflowNocDecodeLocalRoute(firstRaw,first)
            || !PioneerQwen35::DataflowNocDecodeLocalRoute(secondRaw,second)
            || first!=second || !PioneerQwen35::DataflowNocRouteMatchesWorker(first,core)) return false;
        return fIO.runtimeCore==core && fIO.DataflowPanelMappingStable() && TlBsMatch(core)
            && ProvePinnedBar(fIO.profile.bar0_host_start,fIO.profile.bar0_host_end);
    }
};
#endif
