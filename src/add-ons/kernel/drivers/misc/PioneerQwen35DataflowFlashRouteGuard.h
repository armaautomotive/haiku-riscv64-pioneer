/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_QWEN35_DATAFLOW_FLASH_ROUTE_GUARD_H
#define PIONEER_QWEN35_DATAFLOW_FLASH_ROUTE_GUARD_H
#if !defined(PIONEER_QWEN35_FLASH_CODE_OWNER_WITNESS) || !defined(PIONEER_QWEN35_ROLE_WRITE_COMPILE_ONLY) \
    || !defined(PIONEER_QWEN35_RECOVERY_COMPILE_ONLY) || !defined(PIONEER_QWEN35_DEVICE_DISPATCH) \
    || defined(PIONEER_QWEN35_RECOVERY_QUALIFY) || defined(PIONEER_QWEN35_DEVICE_DISPATCH_TRIAL)
#error Flash route integration is a non-loadable recovery witness only
#endif
#include "PioneerQwen35DataflowRoleRouteGuard.h"

// Connects flash selection to the actual role-session BAR, TLB and NoC
// route checks. Authority must retain the same session, mapping, ledger and
// snapshots and independently exclude other writers for the entire operation.
// A selection is recorded before the TLB update; an uncertain attempt is
// retained by Authority and cannot be retried merely because this object is
// reconstructed. All Authority callbacks are metadata-only;
// Authority::SelectFlashRoute must not write the route a second time. None of
// those physical ownership proofs is provided here.
template<class IO,class Scope,class Authority>
class PioneerQwen35FlashRouteGuard : public Authority {
    IO& fIO;Scope& fScope;PioneerQwen35RoleRouteGuard<IO,Scope> fRoute;
public:
    template<class... Args> PioneerQwen35FlashRouteGuard(IO& io,Scope& scope,Args&... args)
        :Authority(args...),fIO(io),fScope(scope),fRoute(io,scope) {}
    PioneerQwen35FlashRouteGuard(const PioneerQwen35FlashRouteGuard&)=delete;
    PioneerQwen35FlashRouteGuard& operator=(const PioneerQwen35FlashRouteGuard&)=delete;
    bool ProvePinnedBar(uint64_t start,uint64_t end) {
        return Authority::ProvePinnedBar(start,end) && fRoute.ProvePinnedBar(start,end)
            && Authority::ProvePinnedBar(start,end);
    }
    bool ProveSelectedRoute(unsigned core) {
        return core<PioneerQwen35::DataflowBarrierWorkers && Authority::ProveSelectedRoute(core)
            && fRoute.ProveSelectedRoute(core) && fScope.ProveRoleFleetHeldAndQuiet(fIO,core)
            && Authority::ProveSelectedRoute(core) && fRoute.ProveSelectedRoute(core);
    }
    bool ProveFlashRoleRouteSame(const void* role,const void* flash,unsigned core) {
        return ProveSelectedRoute(core) && Authority::ProveFlashRoleRouteSame(role,flash,core)
            && ProveSelectedRoute(core);
    }
    bool SelectFlashRoute(unsigned core) {
        if (core>=PioneerQwen35::DataflowBarrierWorkers || !Authority::CanSelectFlashRoute(core)
            || !fScope.CanSelectRoleHeldCore(fIO,core)
            || !ProvePinnedBar(fIO.profile.bar0_host_start,fIO.profile.bar0_host_end)
            || !Authority::BeginFlashRouteSelection(core)) return false;
        const bool okay=fIO.SelectRuntimeCore(core) && fIO.MapQ38Panel()
            && fIO.DataflowPanelMappingStable() && Authority::SelectFlashRoute(core)
            && ProveSelectedRoute(core) && Authority::ConfirmFlashRouteSelection(core)
            && ProveSelectedRoute(core);
        if (!okay) Authority::RetainUncertainFlashRouteSelection(core);
        return okay;
    }
};
#endif
