/* SPDX-License-Identifier: MIT */
#include <cassert>
#include <cstdio>

struct Dram {};
struct Banks { bool quarantined=false; };
struct MockStore { bool ready=false,saved=false; };
struct Image { bool saved=false,ready=false,running=false,unsafe=false; };
struct Node { unsigned noc=0; MockStore store;MockStore* master=nullptr; Image image;
    auto& Store() { return master?*master:store; } };
struct Lease {
    Node worker[120]{};unsigned prepared=0;
    bool unsafe=false,closed=false,rootRunning=false,booted=false,bootAttempted=false;
};
struct IO {
    unsigned selected=120,selects=0,maps=0,prepares=0,restores=0,audits=0;
    unsigned failSelect=120,failPrepare=120,failAudit=120,failRestore=120;
    bool fleetEnabled=false;
    bool SelectRuntimeCore(unsigned core) { selected=core;++selects;return core!=failSelect; }
    bool MapQ38Panel() { ++maps;return true; }
    bool MapRuntime() { return true; }
};
static bool PioneerQwen35DedicatedPeerLeaseBegin(IO& io,const Dram&,unsigned core,Node& n) {
    assert(io.selected==core);++io.prepares;
    n.store.ready=true;return core!=io.failPrepare;
}
static bool PioneerQ38DispatchSources(IO& io,const Dram&,Banks&,Node& n,unsigned reset) {
    assert(reset==0x47800 && io.selected<120);++io.audits;
    return n.Store().ready && io.selected!=io.failAudit;
}
static bool PioneerQwen35DedicatedPeerLeaseEnd(IO& io,unsigned&,MockStore& s) {
    assert(io.audits>=120 && s.ready);++io.restores;
    if (io.selected==io.failRestore) return false;
    s.ready=false;return true;
}
static bool PioneerQ38DispatchQuarantine(Lease& l,Banks& b) {
    l.unsafe=true;b.quarantined=true;return false;
}
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQwen35DedicatedFleet.h"

int main() {
    Dram dram;
    {
        IO io;Banks banks;Lease lease;lease.worker[0].store.ready=true;
        assert(PioneerQwen35DedicatedFleetPrepare(io,dram,banks,lease));
        assert(lease.prepared==120 && !lease.booted && !lease.bootAttempted && !lease.unsafe);
        assert(io.prepares==119 && io.audits==120 && !io.restores);
        for (const auto& n:lease.worker) assert(!n.image.saved && !n.image.ready && !n.image.running);
        io.audits=0;
        assert(PioneerQwen35DedicatedFleetClose(io,dram,banks,lease));
        assert(lease.closed && io.audits==120 && io.restores==119 && io.selected==0 && !banks.quarantined);
        assert(!PioneerQwen35DedicatedFleetClose(io,dram,banks,lease));
    }
    {
        IO io;Banks banks;Lease lease;lease.worker[0].store.ready=true;io.failPrepare=37;
        assert(!PioneerQwen35DedicatedFleetPrepare(io,dram,banks,lease));
        assert(lease.prepared==38 && lease.unsafe && banks.quarantined && io.prepares==37);
        const auto selects=io.selects;
        assert(!PioneerQwen35DedicatedFleetPrepare(io,dram,banks,lease) && io.selects==selects);
    }
    {
        IO io;Banks banks;Lease lease;lease.worker[0].store.ready=true;
        assert(PioneerQwen35DedicatedFleetPrepare(io,dram,banks,lease));
        io.audits=0;io.failAudit=83;
        assert(!PioneerQwen35DedicatedFleetClose(io,dram,banks,lease));
        assert(lease.unsafe && banks.quarantined && !io.restores);
    }
    {
        IO io;Banks banks;Lease lease;lease.worker[0].store.ready=true;
        assert(PioneerQwen35DedicatedFleetPrepare(io,dram,banks,lease));
        io.audits=0;io.failRestore=117;
        assert(!PioneerQwen35DedicatedFleetClose(io,dram,banks,lease));
        assert(lease.unsafe && banks.quarantined && io.restores==3);
        const auto restores=io.restores;
        assert(!PioneerQwen35DedicatedFleetClose(io,dram,banks,lease) && io.restores==restores);
    }
    std::puts("PASS dedicated held-fleet route preparation and pre-service close; SIMULATED ONLY");
    return 0;
}
