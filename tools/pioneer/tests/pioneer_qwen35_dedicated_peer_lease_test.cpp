/* SPDX-License-Identifier: MIT */
#include <cassert>
#include <cstdint>
#include <cstdio>

constexpr unsigned TT_COMPUTE_NOC_PASSED=1;
struct Dram {};
struct Noc { unsigned version=0,state=1;bool tlb_restored=true,recovery_required=false;
    uint32_t source[1][16]{}; };
struct MockStore { uint32_t* saved=nullptr;uint32_t tlb[2][3]{},config=0;
    bool controls_saved=false,configured=false,l1_dirty=false,pending=false,ready=false,recovery_required=false;
    bool l1_restored=true,niu_restored=true,tlb_restored=true; };
struct Node { Noc noc;MockStore store;auto& Store() { return store; } };
struct IO {
    unsigned core=0,niuWrites=0,tlbWrites=0;uint32_t reset=0x47800,config=0x42;
    uint32_t tlb[2][3]{{1,2,3},{4,5,6}},command[10]{};
    bool selectOkay=true,pciOkay=true,idle=true,dropConfig=false,dropTlb=false,sourceOkay=true;
    bool SelectRuntimeCore(unsigned c) { core=c;return selectOkay; }
    uint32_t DramTlbRead(unsigned s,unsigned w) { return tlb[s][w]; }
    void DramTlbWrite(unsigned s,unsigned w,uint32_t v) { ++tlbWrites;if (!dropTlb) tlb[s][w]=v; }
    uint32_t ResetRead() { return reset; }
    uint32_t ResidentNocRead(unsigned offset) {
        if (offset==0x178) return 0;
        if (offset==0x100) return config;
        return offset<40?command[offset/4]:0;
    }
    void ResidentNocWrite(unsigned offset,uint32_t v) {
        ++niuWrites;if (offset==0x100 && !dropConfig) config=v;
    }
};
static unsigned PioneerWorkerVersion(unsigned worker) { return worker+1; }
static bool PioneerWorkerRoute(const Dram&,const uint32_t*,unsigned) { return true; }
static bool BlackholeHostSourceRoute(const uint32_t*) { return true; }
static bool BlackholeHostReadPciQuiet(IO& io,unsigned command) { return command==0x402 && io.pciOkay; }
static bool BlackholeResidentSourceStable(IO& io,const Noc&,uint32_t config) {
    return io.sourceOkay && io.config==config;
}
static bool BlackholeResidentIdle(IO& io) { return io.idle; }
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQwen35DedicatedPeerLease.h"

static Node ReadyNode() {
    Node n;n.noc.version=PioneerWorkerVersion(7);n.noc.source[0][0]=0x42;return n;
}
int main() {
    Dram dram;
    {
        IO io;Node n=ReadyNode();
        assert(PioneerQwen35DedicatedPeerLeaseBegin(io,dram,7,n));
        assert(n.store.ready && !n.store.saved && !n.store.l1_dirty && io.config==0x10042 && io.niuWrites==1);
        assert(PioneerQwen35DedicatedPeerLeaseEnd(io,n.noc,n.store));
        assert(!n.store.ready && n.store.niu_restored && n.store.tlb_restored && io.config==0x42
            && io.niuWrites==2 && io.tlbWrites==6);
    }
    {
        IO io;Node n=ReadyNode();io.dropConfig=true;
        assert(!PioneerQwen35DedicatedPeerLeaseBegin(io,dram,7,n));
        assert(n.store.configured && n.store.recovery_required && !n.store.ready && !n.store.saved);
    }
    {
        IO io;Node n=ReadyNode();io.reset=0x46800;
        assert(!PioneerQwen35DedicatedPeerLeaseBegin(io,dram,7,n) && !io.niuWrites);
    }
    {
        IO io;Node n=ReadyNode();
        assert(PioneerQwen35DedicatedPeerLeaseBegin(io,dram,7,n));
        n.store.l1_dirty=true;const unsigned writes=io.niuWrites;
        assert(!PioneerQwen35DedicatedPeerLeaseEnd(io,n.noc,n.store) && io.niuWrites==writes);
    }
    {
        IO io;Node n=ReadyNode();
        assert(PioneerQwen35DedicatedPeerLeaseBegin(io,dram,7,n));
        io.command[2]=1;const unsigned writes=io.niuWrites;
        assert(!PioneerQwen35DedicatedPeerLeaseEnd(io,n.noc,n.store) && io.niuWrites==writes);
    }
    {
        IO io;Node n=ReadyNode();
        assert(PioneerQwen35DedicatedPeerLeaseBegin(io,dram,7,n));
        io.tlb[0][0]=99;io.dropTlb=true;
        assert(!PioneerQwen35DedicatedPeerLeaseEnd(io,n.noc,n.store));
        assert(n.store.ready && n.store.niu_restored && !n.store.tlb_restored);
    }
    std::puts("PASS dedicated peer lease uses held NoC state without L1 snapshots; SIMULATED ONLY");
}
