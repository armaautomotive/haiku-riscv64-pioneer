/* SPDX-License-Identifier: MIT */
// SDK compile-only witness: no real guard, owner, ioctl, driver or card use.
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQwen35DataflowRoleHaikuMap.h"
#include "../PioneerQwen35DataflowRoleReadOnlyViews.h"
struct RoleViewsCompileGuard {
    bool ProvePinnedBar(uint64_t start,uint64_t end) { return start && end>start; }
    bool ProveReadAllowed(unsigned,unsigned,unsigned) { return true; }
    bool ProveSelectedRoute(unsigned core) { return core<PioneerQwen35::DataflowBarrierWorkers; }
};
extern "C" bool pioneer_qwen35_role_haiku_views_compile(uint64_t start,uint64_t end,
    unsigned core,unsigned window,unsigned offset,uint32_t& output)
{
    PioneerQwen35RoleHaikuMap platform;
    PioneerQwen35::DataflowRoleReadOnlyViews<PioneerQwen35RoleHaikuMap> views(platform);
    RoleViewsCompileGuard guard;
    if (!views.Open(start,end,guard)) { (void)views.Close(); return false; }
    const bool read=views.Read(core,window,offset,output,guard);
    return views.Close() && read;
}
