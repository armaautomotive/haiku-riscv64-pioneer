/* SPDX-License-Identifier: MIT */
#define PIONEER_QWEN35_FABRIC_GUARD_WITNESS 1
#include "../PioneerQwen35DataflowRoleWritableViews.h"
#include <cassert>
#include <cstdio>
using namespace PioneerQwen35;
static constexpr uint64_t Start=0x4900000000ULL,End=0x491fffffffULL;
static constexpr unsigned Count=DataflowRolePhysicalControl+1;
struct BarGuard {
    bool route=true;
    bool ProvePinnedBar(uint64_t start,uint64_t end) { return start==Start && end==End; }
    bool ProveSelectedRoute(unsigned core) { return route && core==3; }
    bool ProveWriteAllowed(unsigned,unsigned,unsigned,unsigned) { return true; }
};
struct Platform {
    uint32_t dummy=0;unsigned maps=0,unmaps=0;int fail=-1,duplicate=-1,duplicateValue=0,failMap=-1,nullMap=-1;
    BarGuard* dropAfterWrite=nullptr;
    bool MapReadWrite(uint64_t,unsigned,int32_t& area,volatile uint32_t*& view) {
        const unsigned index=maps++;
        if (index==unsigned(failMap)) return false;
        area=index==unsigned(duplicate)?duplicateValue:int32_t(index);
        view=index==unsigned(nullMap)?nullptr:&dummy;return true;
    }
    bool Unmap(int32_t area) { unmaps++;return area!=fail; }
    void WriteWord(volatile uint32_t* view,uint32_t value) { *view=value;if (dropAfterWrite) dropAfterWrite->route=false; }
    uint32_t ReadWord(const volatile uint32_t* view) { return *view; }
};
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
        if (views.FabricCloseAttempted()) {
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
    unsigned openChecks=0,closeChecks=0;
    {
        Platform platform;BarGuard bar;Check oldCheck,openCheck,closeCheck;Protected protectedArea;
        DataflowRoleWritableViews<Platform> views(platform);
        assert(views.Open(Start,End,bar) && views.CloseForFabricRecovery(oldCheck,protectedArea));
        assert(views.OpenForFabricRecovery(bar,openCheck,protectedArea) && views.FabricFreshOpenComplete());
        assert(views.CloseFabricRecoveryViews(closeCheck,protectedArea) && views.FabricFreshCloseComplete()
            && !views.HasMappings() && !views.Close() && !views.OpenForFabricRecovery(bar,openCheck,protectedArea));
        openChecks=openCheck.calls;closeChecks=closeCheck.calls;
    }
    for (unsigned failure=1;failure<=openChecks;failure++) {
        Platform platform;BarGuard bar;Check oldCheck,openCheck;Protected protectedArea;
        DataflowRoleWritableViews<Platform> views(platform);
        assert(views.Open(Start,End,bar) && views.CloseForFabricRecovery(oldCheck,protectedArea));
        openCheck.fail=failure;assert(!views.OpenForFabricRecovery(bar,openCheck,protectedArea));
        const unsigned maps=platform.maps,unmaps=platform.unmaps;
        assert(!views.FabricFreshOpenComplete());
        if (views.FabricFreshOpenAttempted())
            assert(!views.OpenForFabricRecovery(bar,openCheck,protectedArea)
                && !views.Close() && platform.maps==maps && platform.unmaps==unmaps);
    }
    for (unsigned i=0;i<Count;i++) for (unsigned fault=0;fault<4;fault++) {
        Platform platform;BarGuard bar;Check oldCheck,openCheck;Protected protectedArea;
        DataflowRoleWritableViews<Platform> views(platform);
        assert(views.Open(Start,End,bar) && views.CloseForFabricRecovery(oldCheck,protectedArea));
        const unsigned fresh=Count+i;
        if (fault==0) platform.failMap=int(fresh);
        if (fault==1) platform.nullMap=int(fresh);
        if (fault==2) protectedArea.area=int32_t(fresh);
        if (fault==3 && i) { platform.duplicate=int(fresh);platform.duplicateValue=int(Count); }
        if (fault==3 && !i) continue;
        assert(!views.OpenForFabricRecovery(bar,openCheck,protectedArea) && views.FabricFreshOpenAttempted());
        const unsigned maps=platform.maps,unmaps=platform.unmaps;
        assert(!views.OpenForFabricRecovery(bar,openCheck,protectedArea) && !views.Close()
            && platform.maps==maps && platform.unmaps==unmaps);
    }
    for (unsigned failure=1;failure<=closeChecks;failure++) {
        Platform platform;BarGuard bar;Check oldCheck,openCheck,closeCheck;Protected protectedArea;
        DataflowRoleWritableViews<Platform> views(platform);
        assert(views.Open(Start,End,bar) && views.CloseForFabricRecovery(oldCheck,protectedArea)
            && views.OpenForFabricRecovery(bar,openCheck,protectedArea));
        closeCheck.fail=failure;assert(!views.CloseFabricRecoveryViews(closeCheck,protectedArea));
        const unsigned unmaps=platform.unmaps;
        if (views.FabricFreshCloseAttempted())
            assert(!views.CloseFabricRecoveryViews(closeCheck,protectedArea) && !views.Close() && platform.unmaps==unmaps);
    }
    for (unsigned i=0;i<Count;i++) {
        Platform platform;BarGuard bar;Check oldCheck,openCheck,closeCheck;Protected protectedArea;
        DataflowRoleWritableViews<Platform> views(platform);
        assert(views.Open(Start,End,bar) && views.CloseForFabricRecovery(oldCheck,protectedArea)
            && views.OpenForFabricRecovery(bar,openCheck,protectedArea));
        protectedArea.area=int32_t(Count+i);
        assert(!views.CloseFabricRecoveryViews(closeCheck,protectedArea) && !views.FabricFreshCloseAttempted());
        protectedArea.area=-1;platform.fail=int(Count+i);
        assert(!views.CloseFabricRecoveryViews(closeCheck,protectedArea) && views.FabricFreshCloseAttempted());
        const unsigned unmaps=platform.unmaps;
        assert(!views.CloseFabricRecoveryViews(closeCheck,protectedArea) && !views.Close() && platform.unmaps==unmaps);
    }
    {
        Platform platform;BarGuard bar;Check oldCheck,openCheck,closeCheck;Protected protectedArea;
        DataflowRoleWritableViews<Platform> views(platform);const uint32_t value=0x87654321;
        assert(views.Open(Start,End,bar));platform.dropAfterWrite=&bar;
        assert(!views.WriteWords(3,DataflowRolePhysicalBriscBoot,0,&value,4,bar)
            && !views.ImageWritesConfirmed());
        platform.dropAfterWrite=nullptr;bar.route=true;
        assert(views.CloseForFabricRecovery(oldCheck,protectedArea)
            && views.OpenForFabricRecovery(bar,openCheck,protectedArea)
            && !views.ImageWritesConfirmed());
        assert(views.CloseFabricRecoveryViews(closeCheck,protectedArea));
    }
    std::puts("PASS guarded retirement of role mappings; SIMULATED ONLY");
}
