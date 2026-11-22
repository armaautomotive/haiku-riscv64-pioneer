/* SPDX-License-Identifier: MIT */
#include "../PioneerQwen35DataflowRoleReadOnlyViews.h"
#include <cassert>
#include <cstdio>
#include <vector>
using namespace PioneerQwen35;
struct Platform {
    std::vector<std::vector<uint32_t>> memory;
    int failMap=-1,failUnmap=-1;
    unsigned maps=0,unmaps=0,reads=0;
    Platform():memory(DataflowRolePhysicalWindowCount) {
        for (unsigned i=0;i<DataflowRolePhysicalWindowCount;i++) {
            DataflowRolePhysicalWindow plan{}; assert(DataflowRolePhysicalWindowPlan(i,plan));
            memory[i].assign(plan.bytes/4,0x12340000U+i);
        }
    }
    bool MapReadOnly(uint64_t physical,unsigned bytes,int32_t& area,const volatile uint32_t*& output) {
        const unsigned index=maps++;
        assert(index<DataflowRolePhysicalWindowCount);
        DataflowRolePhysicalWindow plan{}; assert(DataflowRolePhysicalWindowPlan(index,plan));
        assert(physical==0x4900000000ULL+plan.offset && bytes==plan.bytes);
        if (int(index)==failMap) return false;
        area=int32_t(index); output=memory[index].data(); return true;
    }
    bool Unmap(int32_t area) {
        ++unmaps;
        if (area==failUnmap) { failUnmap=-1; return false; }
        return true;
    }
    uint32_t ReadWord(const volatile uint32_t* address) { ++reads; return *address; }
};
struct Guard {
    bool pinned=true,route=true,failSecond=false;
    unsigned routeCalls=0;
    bool ProvePinnedBar(uint64_t start,uint64_t end) const {
        return pinned && start==0x4900000000ULL && end==0x491fffffffULL;
    }
    bool ProveReadAllowed(unsigned,unsigned,unsigned) const { return true; }
    bool ProveSelectedRoute(unsigned core) {
        ++routeCalls;
        if (failSecond && routeCalls==2) return false;
        return route && core==7;
    }
};
int main()
{
    {
        Platform platform; Guard guard; DataflowRoleReadOnlyViews<Platform> views(platform);
        guard.pinned=false; assert(!views.Open(0x4900000000ULL,0x491fffffffULL,guard));
        assert(!views.HasMappings() && !platform.maps);
        guard.pinned=true; assert(views.Open(0x4900000000ULL,0x491fffffffULL,guard));
        assert(views.Opened() && views.HasMappings() && platform.maps==DataflowRolePhysicalWindowCount);
        uint32_t value=0;
        assert(views.Read(7,DataflowRolePhysicalNoc1,0x1840,value,guard));
        assert(value==0x12340000U+DataflowRolePhysicalNoc1);
        const unsigned reads=platform.reads;
        guard.pinned=false;
        assert(!views.Read(7,DataflowRolePhysicalNoc1,0,value,guard));
        guard.pinned=true;
        assert(!views.Read(7,DataflowRolePhysicalNoc1,0x2000,value,guard));
        assert(!views.Read(7,DataflowRolePhysicalNoc1,0x20c,value,guard));
        assert(!views.Read(7,DataflowRolePhysicalControl,0,value,guard));
        assert(!views.Read(7,DataflowRolePhysicalBriscBoot,4,value,guard));
        assert(!views.Read(7,DataflowRolePhysicalNoc1,1,value,guard));
        assert(!views.Read(8,DataflowRolePhysicalNoc1,0,value,guard));
        assert(platform.reads==reads);
        guard.routeCalls=0; guard.failSecond=true; value=0x55;
        assert(!views.Read(7,DataflowRolePhysicalNoc1,0,value,guard) && value==0x55);
        platform.failUnmap=3;
        assert(!views.Close() && !views.Opened() && views.HasMappings());
        assert(views.Close() && !views.HasMappings());
        assert(!views.Open(0x4900000000ULL,0x491fffffffULL,guard));
    }
    {
        Platform platform; Guard guard; platform.failMap=4;
        DataflowRoleReadOnlyViews<Platform> views(platform);
        assert(!views.Open(0x4900000000ULL,0x491fffffffULL,guard));
        assert(!views.Opened() && views.HasMappings() && platform.maps==5);
        assert(views.Close() && !views.HasMappings() && platform.unmaps==4);
    }
    std::puts("PASS read-only role view lifecycle, route guards and partial-map cleanup; SIMULATED ONLY");
}
