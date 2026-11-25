/* SPDX-License-Identifier: MIT */
#define PIONEER_QWEN35_FABRIC_GUARD_WITNESS 1
#include "../PioneerQwen35DataflowRoleWritableViews.h"
#include <cassert>
#include <cstdio>
using namespace PioneerQwen35;
static constexpr uint64_t Start=0x4900000000ULL,End=0x491fffffffULL;
static constexpr unsigned Count=DataflowRolePhysicalControl+1;
struct Platform {
    uint32_t dummy=0;unsigned maps=0,unmaps=0;int fail=-1,duplicate=-1;
    bool MapReadWrite(uint64_t,unsigned,int32_t& area,volatile uint32_t*& view) {
        area=maps==unsigned(duplicate)?0:int32_t(maps);maps++;view=&dummy;return true;
    }
    bool Unmap(int32_t area) { unmaps++;return area!=fail; }
};
struct BarGuard { bool ProvePinnedBar(uint64_t start,uint64_t end) { return start==Start && end==End; } };
struct Check {
    unsigned calls=0,fail=0;
    bool operator()() { return ++calls!=fail; } // transient failure
};
struct Protected { int32_t area=-1;bool operator()(int32_t actual) { return actual==area; } };
int main() {
    unsigned totalChecks=0;
    {
        Platform platform;BarGuard bar;Check check;Protected protectedArea;
        DataflowRoleWritableViews<Platform> views(platform);
        assert(views.Open(Start,End,bar) && views.HasMappings());
        for (unsigned i=0;i<Count;i++) assert(views.ReferencesArea(i));
        assert(!views.ReferencesArea(-1) && !views.ReferencesArea(Count));
        assert(views.CloseForFabricRecovery(check,protectedArea));
        assert(views.FabricCloseAttempted() && views.FabricCloseComplete() && !views.Opened() && !views.HasMappings()
            && platform.unmaps==Count && !views.Close() && !views.Open(Start,End,bar));
        totalChecks=check.calls;
    }
    for (unsigned failure=1;failure<=totalChecks;failure++) {
        Platform platform;BarGuard bar;Check check;Protected protectedArea;
        DataflowRoleWritableViews<Platform> views(platform);
        assert(views.Open(Start,End,bar));check.fail=failure;
        assert(!views.CloseForFabricRecovery(check,protectedArea) && !views.FabricCloseComplete());
        const unsigned unmaps=platform.unmaps;
        if (failure>2) {
            assert(views.FabricCloseAttempted() && !views.CloseForFabricRecovery(check,protectedArea) && !views.Close()
                && !views.ReopenForRecovery(bar) && platform.unmaps==unmaps);
        } else {
            // Rejection before the first unmap does not consume the attempt.
            assert(!views.FabricCloseAttempted());check.fail=0;assert(views.CloseForFabricRecovery(check,protectedArea));
        }
    }
    for (unsigned i=0;i<Count;i++) {
        Platform platform;BarGuard bar;Check check;Protected protectedArea;
        DataflowRoleWritableViews<Platform> views(platform);
        assert(views.Open(Start,End,bar));protectedArea.area=i;
        assert(!views.CloseForFabricRecovery(check,protectedArea) && !platform.unmaps && views.Opened());
        protectedArea.area=-1;platform.fail=i;
        assert(!views.CloseForFabricRecovery(check,protectedArea));
        const unsigned unmaps=platform.unmaps;
        assert(!views.CloseForFabricRecovery(check,protectedArea) && !views.Close() && platform.unmaps==unmaps);
    }
    {
        Platform platform;BarGuard bar;Check check;Protected protectedArea;
        DataflowRoleWritableViews<Platform> views(platform);platform.duplicate=1;
        assert(views.Open(Start,End,bar));
        assert(!views.CloseForFabricRecovery(check,protectedArea) && !platform.unmaps && views.Opened());
    }
    {
        Platform platform;BarGuard bar;Check check;Protected protectedArea;
        DataflowRoleWritableViews<Platform> views(platform);
        assert(views.CloseForFabricRecovery(check,protectedArea) && views.FabricCloseComplete());
        assert(!views.Open(Start,End,bar));
    }
    std::puts("PASS guarded retirement of role mappings; SIMULATED ONLY");
}
