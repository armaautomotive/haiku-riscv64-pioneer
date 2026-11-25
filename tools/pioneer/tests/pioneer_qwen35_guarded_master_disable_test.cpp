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
static void PrepareFinish(GuardedIO& io,tt_pioneer_mmio_result& saved) {
    tt_pioneer_mmio_transaction txn{};
    assert(BlackholePioneerMmioBegin(io,saved,txn));
    io.rootPciStatus=0xf900;io.accesses=io.afterLoss=0;
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
    unsigned finishChecks=0,finishWrites=0;
    {
        GuardedIO io;tt_pioneer_mmio_result saved{};PrepareFinish(io,saved);Permit check;io.authority=&check.live;
        const unsigned previous=io.writes;unsigned stage=0;
        assert(PioneerPostResetPciFinishGuarded(io,saved,check,stage)
            && stage==PioneerPostResetPciFinishComplete && io.command[0]==0 && io.command[1]==0
            && io.rootPciStatus==0xf900 && !io.afterLoss);
        finishChecks=check.calls;finishWrites=io.writes-previous;
        assert(finishChecks>50 && finishWrites>2);
    }
    for (unsigned lose=1;lose<=finishChecks;lose++) {
        GuardedIO io;tt_pioneer_mmio_result saved{};PrepareFinish(io,saved);
        Permit check;check.loseAt=lose;io.authority=&check.live;unsigned stage=0;
        assert(!PioneerPostResetPciFinishGuarded(io,saved,check,stage) && !io.afterLoss && check.calls==lose);
    }
    for (unsigned drop=1;drop<=finishWrites;drop++) {
        GuardedIO io;tt_pioneer_mmio_result saved{};PrepareFinish(io,saved);
        const unsigned previous=io.writes;io.droppedWrite=previous+drop;
        Permit check;io.authority=&check.live;unsigned stage=0;
        assert(!PioneerPostResetPciFinishGuarded(io,saved,check,stage)
            && io.writes==previous+drop && io.rootPciStatus==0xf900);
    }
    {
        GuardedIO io;tt_pioneer_mmio_result saved{};PrepareFinish(io,saved);saved.bars_before[0]=0xdeadbeef;
        const unsigned previous=io.writes;Permit check;unsigned stage=0;
        assert(!PioneerPostResetPciFinishGuarded(io,saved,check,stage) && io.writes==previous);
    }
    std::puts("PASS guarded PCI master disable and idle restoration, ticket loss and dropped writes; SIMULATED ONLY");
}
