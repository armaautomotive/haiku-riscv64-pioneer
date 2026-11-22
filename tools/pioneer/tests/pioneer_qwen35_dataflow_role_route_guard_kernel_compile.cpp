/* SPDX-License-Identifier: MIT */
// SDK compile-only guard witness; there is no real session or card access.
#include <KernelExport.h>
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQwen35DataflowRoleRouteGuard.h"
struct RoleRouteCompileIO {
    tt_pioneer_profile profile;
    unsigned runtimeCore;
    bool fleetEnabled;
    bool DataflowPanelMappingStable();
    uint32_t DramTlbRead(unsigned,unsigned);
    uint32_t ResidentNocRead(unsigned);
    void Pause();
};
struct RoleRouteCompileScope {
    bool ProveExclusiveRewrite(RoleRouteCompileIO&);
    bool ProveRoleMemoryReadSafe(RoleRouteCompileIO&,unsigned,unsigned);
};
extern "C" bool pioneer_qwen35_role_route_guard_compile(RoleRouteCompileIO& io,
    RoleRouteCompileScope& scope,unsigned core)
{
    PioneerQwen35RoleRouteGuard<RoleRouteCompileIO,RoleRouteCompileScope> guard(io,scope);
    return guard.ProveSelectedRoute(core)
        && guard.ProveReadAllowed(core,PioneerQwen35::DataflowRolePhysicalBriscRam,0);
}
