/* SPDX-License-Identifier: MIT */
// Compile and execute the driver's actual preparation function using fake IO.
#define PIONEER_QWEN35_DEDICATED_FLEET_WITNESS 1
#ifdef TEST_SNAPSHOT_TAKEOVER
#define PIONEER_QWEN35_SNAPSHOT_TAKEOVER_WITNESS 1
#else
#define PIONEER_QWEN35_EXCLUSIVE_TAKEOVER_WITNESS 1
#endif
#define main dedicated_fleet_test_main
#include "pioneer_qwen35_dedicated_fleet_test.cpp"
#undef main
#include <new>

using PioneerQ38DispatchLease=Lease;
struct Baseline4 { Dram dram; };
struct Baseline3 { Baseline4 baseline;unsigned noc=0; };
struct Baseline2 { Baseline3 baseline; };
struct Baseline1 { Baseline2 baseline; };
struct PioneerStoreContext {
    Baseline1 baseline;IO io;MockStore lease;Banks banks;Lease* q38Dispatch=nullptr;
    bool q38DispatchRoutes=false,qwen35DedicatedFleet=false;
    ~PioneerStoreContext() { delete q38Dispatch; }
};
static bool PioneerQ38DispatchUnsafe(const Lease& l) { return l.unsafe || l.rootRunning; }
static bool PioneerResidentLeaseBegin(IO&,unsigned&,MockStore& s) { s.ready=true;return true; }
static bool BlackholeDramAllCoresHeld(IO&,const Dram&) { return true; }
static bool PioneerWorkerNocProbe(IO&,const Dram&,unsigned,unsigned&) { return true; }
#include "pioneer_dedicated_prepare.inc"

int main() {
    {
        PioneerStoreContext context;context.lease.ready=true;
        assert(PrepareQ38Dispatch(context));
        assert(context.qwen35DedicatedFleet && context.q38DispatchRoutes
            && context.q38Dispatch && context.q38Dispatch->prepared==120
            && !context.q38Dispatch->booted && !context.q38Dispatch->bootAttempted
            && context.io.prepares==119 && context.io.audits==121 && context.io.selected==0);
        const unsigned selects=context.io.selects;
        assert(PrepareQ38Dispatch(context) && context.io.selects==selects);
    }
    {
        PioneerStoreContext context;context.lease.ready=true;context.io.failPrepare=12;
        assert(!PrepareQ38Dispatch(context));
        assert(context.qwen35DedicatedFleet && context.q38Dispatch->unsafe
            && context.q38Dispatch->prepared==13 && context.banks.quarantined);
        const unsigned selects=context.io.selects;
        assert(!PrepareQ38Dispatch(context) && context.io.selects==selects);
    }
    {
        PioneerStoreContext context;context.q38Dispatch=new Lease;
        context.q38Dispatch->booted=true; // old dispatcher must never be accepted
        assert(!PrepareQ38Dispatch(context) && !context.io.selects);
    }
    std::puts("PASS actual dedicated preparation never boots old dispatcher; SIMULATED ONLY");
}
