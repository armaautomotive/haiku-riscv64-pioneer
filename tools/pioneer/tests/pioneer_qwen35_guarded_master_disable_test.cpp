/* SPDX-License-Identifier: MIT */
// Exercise the actual guarded PCI disable against the existing PCI model.
#define PIONEER_MMIO_PROBE_MODEL_ONLY
#include "pioneer_mmio_probe_test.cpp"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerPostResetPci.h"
#include <cstdio>

struct DisableMasterIO : MockIO {
    void CardWrite(unsigned offset,unsigned width,uint32_t value) {
        if (offset==4 && value==0x402 && command[0]==0x406) {
            assert(width==2);if (++writes!=droppedWrite) command[0]=value;return;
        }
        MockIO::CardWrite(offset,width,value);
    }
    void RootWrite(unsigned offset,unsigned width,uint32_t value) {
        if (offset==4 && value==0x402 && command[1]==0x406) {
            assert(width==4 && command[0]==0x402);
            if (++writes!=droppedWrite) command[1]=value;return;
        }
        MockIO::RootWrite(offset,width,value);
    }
};

struct GuardedIO : DisableMasterIO {
    unsigned accesses=0,afterLoss=0;
    bool* authority=nullptr;
    void Touch() { ++accesses; if (authority && !*authority) ++afterLoss; }
    bool QuietCached() { Touch();return quiet; }
    uint32_t CardRead(unsigned offset,unsigned width) {
        Touch();return DisableMasterIO::CardRead(offset,width);
    }
    uint32_t RootRead(unsigned offset,unsigned width) {
        Touch();
        if (offset==0x10 || offset==0x14 || offset==0x38) { assert(width==4);return 0; }
        return DisableMasterIO::RootRead(offset,width);
    }
    void CardWrite(unsigned offset,unsigned width,uint32_t value) {
        Touch();DisableMasterIO::CardWrite(offset,width,value);
    }
    void RootWrite(unsigned offset,unsigned width,uint32_t value) {
        Touch();DisableMasterIO::RootWrite(offset,width,value);
    }
};
struct Permit {
    unsigned calls=0,loseAt=0;bool live=true;
    bool operator()() { if (++calls==loseAt) live=false;return live; }
};
static void Prepare(GuardedIO& io) {
    tt_pioneer_mmio_result saved{};tt_pioneer_mmio_transaction txn{};
    assert(BlackholePioneerMmioBegin(io,saved,txn));
    io.command[0]=io.command[1]=0x406;io.rootPciStatus=0xf900;
    io.accesses=io.afterLoss=0;
}
int main() {
    GuardedIO good;Prepare(good);Permit permit;good.authority=&permit.live;
    assert(PioneerPreResetDisableMasterGuarded(good,permit)
        && good.command[0]==0x402 && good.command[1]==0x402
        && good.rootPciStatus==0xf900 && !good.afterLoss);
    const unsigned checks=permit.calls;
    for (unsigned lose=1;lose<=checks;lose++) {
        GuardedIO io;Prepare(io);Permit denied;denied.loseAt=lose;io.authority=&denied.live;
        assert(!PioneerPreResetDisableMasterGuarded(io,denied) && !io.afterLoss
            && denied.calls==lose);
    }
    for (unsigned drop=1;drop<=2;drop++) {
        GuardedIO io;Prepare(io);Permit allowed;io.authority=&allowed.live;
        io.droppedWrite=io.writes+drop;
        assert(!PioneerPreResetDisableMasterGuarded(io,allowed));
        assert(io.command[1]==0x406);
    }
    GuardedIO identity;Prepare(identity);identity.cardID=0xffffffff;
    const unsigned writes=identity.writes;Permit allowed;
    assert(!PioneerPreResetDisableMasterGuarded(identity,allowed) && identity.writes==writes);
    std::puts("PASS guarded one-shot PCI bus-master disable, ticket loss and dropped writes; SIMULATED ONLY");
}
