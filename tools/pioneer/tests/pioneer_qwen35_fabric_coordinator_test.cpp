/* SPDX-License-Identifier: MIT */
// Test actual orchestration using simulated stage owners and lifetimes.
#include <cassert>
#include <cstdio>
#include <vector>
enum { B_OK=0,B_NOT_ALLOWED=-1,B_NO_MEMORY=-2,B_DEV_NOT_READY=-3 };
struct Session { bool retained=false; };
struct Operations {
    unsigned failure=0;bool attached=false,detached=false,discarded=false,retained=false,contextRetained=false,contextDestroyed=false,partialPrepare=false;
    Session* live=nullptr;std::vector<unsigned> calls;
    bool Step(unsigned value) { calls.push_back(value);return value!=failure; }
    bool Admission() { return Step(1); }
    Session* AllocateSession() { if (!Step(2)) return nullptr;live=new Session;return live; }
    bool PrepareSession(Session&) { attached=failure!=3 || partialPrepare;return Step(3); }
    void RetainOrDiscard(Session& s) {
        calls.push_back(10);
        if (attached) { s.retained=true;retained=true; }
        else { delete &s;live=nullptr;discarded=true; }
    }
    bool SessionAttached(Session&) { return Step(4) && attached; }
    bool RestoreSession(Session&) { return Step(5); }
    bool DetachSession(Session& s) {
        if (!Step(6)) return false;
        delete &s;live=nullptr;attached=false;detached=true;return true;
    }
    void RetainSession(Session& s) { calls.push_back(11);s.retained=true;retained=true; }
    bool SessionDetached() { return Step(7) && detached; }
    bool ReleaseRoleOwner() { return Step(8); }
    bool RecordGraphRetirement() { return Step(9); }
    void RetainContext() { contextRetained=true; }
    int Finish() { calls.push_back(12);if (failure==12) return B_DEV_NOT_READY;contextDestroyed=true;return B_OK; }
    ~Operations() { delete live; }
};
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQwen35FabricRecoveryCoordinator.h"
int main() {
    {
        Operations ops;assert(PioneerQ35CoordinateFabricRecovery(ops)==B_OK && ops.detached && ops.contextDestroyed
            && !ops.live && ops.calls==std::vector<unsigned>({1,2,3,4,5,6,7,8,9,12}));
    }
    for (unsigned failure:{1,2,3,4,5,6,7,8,9,12}) {
        Operations ops;ops.failure=failure;const int result=PioneerQ35CoordinateFabricRecovery(ops);
        assert(result==(failure==1?B_NOT_ALLOWED:failure==2?B_NO_MEMORY:B_DEV_NOT_READY));
        assert(!ops.contextDestroyed);
        if (failure==3) assert(ops.discarded && !ops.live && !ops.attached);
        if (failure>=4 && failure<=6) assert(ops.retained && ops.live && ops.live->retained);
        if (failure>=7) assert(ops.detached && !ops.live);
        if (failure>=7 && failure<=9) assert(ops.contextRetained);
        if (failure!=12) for (unsigned call:ops.calls) assert(call!=12);
    }
    {
        Operations ops;ops.failure=3;ops.partialPrepare=true;
        assert(PioneerQ35CoordinateFabricRecovery(ops)==B_DEV_NOT_READY
            && ops.attached && ops.retained && ops.live && ops.live->retained && !ops.discarded);
    }
    puts("PASS full fabric recovery coordinator: ordered stages, retained failures, no stale session; SIMULATED ONLY");
}
