/* SPDX-License-Identifier: MIT */
#include <cassert>
#include <cstdint>
#include <cstdio>

constexpr unsigned TT_COMPUTE_NOC_PASSED=1;
struct Dram {};
struct Banks { bool quarantined=false; };
struct Noc { unsigned version=0,state=1;bool tlb_restored=true,recovery_required=false;
    uint32_t source[1][16]{}; };
struct Store { uint32_t* saved=nullptr;uint32_t tlb[2][3]{},config=0;
    bool controls_saved=false,configured=false,l1_dirty=false,pending=false,ready=false,recovery_required=false;
    bool l1_restored=true,niu_restored=true,tlb_restored=true; };
struct Image { bool saved=false,ready=false,running=false,unsafe=false; };
struct Node { Noc noc;::Store store;Image image;auto& Store() { return store; } };
struct Lease { Node worker[120]{};unsigned prepared=0;
    bool unsafe=false,closed=false,rootRunning=false,booted=false,bootAttempted=false; };
struct IO {
    unsigned core=0,niuWrites=0,tlbWrites=0,dropConfigCore=120;
    uint32_t config[120]{},tlb[120][2][3]{},command[120][10]{};
    IO() { for (auto& c:config) c=0x42; }
    bool SelectRuntimeCore(unsigned c) { core=c;return c<120; }
    bool MapQ38Panel() { return true; }
    uint32_t DramTlbRead(unsigned s,unsigned w) { return tlb[core][s][w]; }
    void DramTlbWrite(unsigned s,unsigned w,uint32_t v) { ++tlbWrites;tlb[core][s][w]=v; }
    uint32_t ResetRead() { return 0x47800; }
    uint32_t ResidentNocRead(unsigned off) {
        if (off==0x178) return 0;
        if (off==0x100) return config[core];
        return off<40?command[core][off/4]:0;
    }
    void ResidentNocWrite(unsigned off,uint32_t v) {
        ++niuWrites;if (off==0x100 && core!=dropConfigCore) config[core]=v;
    }
};
static unsigned PioneerWorkerVersion(unsigned w) { return w+1; }
static bool PioneerWorkerRoute(const Dram&,const uint32_t*,unsigned) { return true; }
static bool BlackholeHostSourceRoute(const uint32_t*) { return true; }
static bool BlackholeHostReadPciQuiet(IO&,unsigned command) { return command==0x402; }
static bool BlackholeResidentSourceStable(IO& io,const Noc&,uint32_t config) { return io.config[io.core]==config; }
static bool BlackholeResidentIdle(IO&) { return true; }
static bool PioneerQ38DispatchSources(IO& io,const Dram&,Banks&,Node& n,unsigned reset) {
    return reset==0x47800 && n.Store().ready && io.config[io.core]==(n.Store().config|0x10000);
}
static bool PioneerQ38DispatchQuarantine(Lease& l,Banks& b) { l.unsafe=true;b.quarantined=true;return false; }
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQwen35DedicatedPeerLease.h"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQwen35DedicatedFleet.h"

static void Initialize(Lease& lease,IO& io) {
    lease.worker[0].store.ready=true;lease.worker[0].store.config=0x42;io.config[0]=0x10042;
    for (unsigned core=1;core<120;core++) {
        lease.worker[core].noc.version=PioneerWorkerVersion(core);
        lease.worker[core].noc.source[0][0]=0x42;
    }
}
int main() {
    Dram dram;
    {
        IO io;Banks banks;Lease lease;Initialize(lease,io);
        assert(PioneerQwen35DedicatedFleetPrepare(io,dram,banks,lease));
        assert(lease.prepared==120 && io.niuWrites==119);
        for (unsigned core=1;core<120;core++)
            assert(lease.worker[core].store.ready && !lease.worker[core].store.saved && io.config[core]==0x10042);
        assert(PioneerQwen35DedicatedFleetClose(io,dram,banks,lease));
        assert(lease.closed && !banks.quarantined && io.niuWrites==238 && io.tlbWrites==119*6);
        for (unsigned core=1;core<120;core++)
            assert(!lease.worker[core].store.ready && io.config[core]==0x42);
    }
    {
        IO io;Banks banks;Lease lease;Initialize(lease,io);io.dropConfigCore=57;
        assert(!PioneerQwen35DedicatedFleetPrepare(io,dram,banks,lease));
        assert(lease.prepared==58 && lease.unsafe && banks.quarantined
            && lease.worker[57].store.recovery_required && io.niuWrites==57);
    }
    std::puts("PASS composed dedicated fleet and route-only peer leases; SIMULATED ONLY");
}
