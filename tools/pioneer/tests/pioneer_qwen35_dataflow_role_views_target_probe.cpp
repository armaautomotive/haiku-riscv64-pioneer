/* SPDX-License-Identifier: MIT */
#include "../PioneerQwen35DataflowRoleReadOnlyViews.h"
using namespace PioneerQwen35;
struct RoleViewProbePlatform {
    const volatile uint32_t* data;
    bool MapReadOnly(uint64_t,unsigned,int32_t& area,const volatile uint32_t*& view) {
        area=1; view=data; return data!=nullptr;
    }
    bool Unmap(int32_t) { return true; }
    uint32_t ReadWord(const volatile uint32_t* p) { return *p; }
};
struct RoleViewProbeGuard {
    bool ProvePinnedBar(uint64_t start,uint64_t end) { return start && end>start; }
    bool ProveReadAllowed(unsigned,unsigned,unsigned) { return true; }
    bool ProveSelectedRoute(unsigned core) { return core<DataflowBarrierWorkers; }
};
bool PioneerQwen35RoleViewsTargetProbe(RoleViewProbePlatform& platform,uint64_t start,
    uint64_t end,unsigned core,unsigned window,unsigned offset,uint32_t& output)
{
    DataflowRoleReadOnlyViews<RoleViewProbePlatform> views(platform);
    RoleViewProbeGuard guard;
    if (!views.Open(start,end,guard)) { (void)views.Close(); return false; }
    const bool read=views.Read(core,window,offset,output,guard);
    return views.Close() && read;
}
