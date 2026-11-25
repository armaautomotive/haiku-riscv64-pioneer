/* SPDX-License-Identifier: MIT */
#define PIONEER_MMIO_PROBE_MODEL_ONLY
#include "pioneer_mmio_probe_test.cpp"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQwen35GuardedMmioBegin.h"
#include <cstdio>

struct CountedMmio : MockIO {
    bool revoked=false;unsigned accesses=0;
    void Touch() { assert(!revoked);++accesses; }
    uint64_t Size(unsigned bar) { Touch();return MockIO::Size(bar); }
    bool QuietCached() { Touch();return quiet; }
    uint32_t CardRead(uint16_t offset,uint8_t size) { Touch();return MockIO::CardRead(offset,size); }
    void CardWrite(uint16_t offset,uint8_t size,uint32_t value) { Touch();MockIO::CardWrite(offset,size,value); }
    uint32_t RootRead(uint16_t offset,uint8_t size) {
        Touch();if (offset==0x10 || offset==0x14 || offset==0x38) { assert(size==4);return 0; }
        return MockIO::RootRead(offset,size);
    }
    void RootWrite(uint16_t offset,uint8_t size,uint32_t value) { Touch();MockIO::RootWrite(offset,size,value); }
    bool Map() { Touch();return MockIO::Map(); }
    uint32_t ReadNodeId() { Touch();return MockIO::ReadNodeId(); }
};

static unsigned Run(unsigned failAt,bool& mappedLoss) {
    CountedMmio io;tt_pioneer_mmio_result result{};tt_pioneer_mmio_transaction transaction{};
    unsigned checks=0;
    auto check=[&]() { ++checks;if (checks==failAt) { io.revoked=true;return false; }return true; };
    const bool okay=PioneerQ35GuardedMmioBegin(io,result,transaction,check);
    if (!failAt) {
        assert(okay && result.state==TT_PIONEER_MMIO_READ_OK && io.mapped && io.mmioReads==3);
        assert(transaction.config_attempted && transaction.map_attempted);
    } else {
        assert(!okay && io.revoked && !result.restored);
        if (io.writes) assert(transaction.config_attempted);
        if (io.mapped) { assert(transaction.map_attempted);mappedLoss=true; }
    }
    return checks;
}
int main() {
    bool mappedLoss=false;const unsigned checks=Run(0,mappedLoss);
    for (unsigned fail=1;fail<=checks;fail++) Run(fail,mappedLoss);
    assert(mappedLoss);
    CountedMmio partial;partial.partialMapFails=true;
    tt_pioneer_mmio_result result{};tt_pioneer_mmio_transaction transaction{};
    auto allowed=[]() { return true; };
    assert(!PioneerQ35GuardedMmioBegin(partial,result,transaction,allowed)
        && partial.mapped && transaction.config_attempted && transaction.map_attempted && !result.restored);
    std::printf("PASS guarded fresh MMIO begin: every ticket boundary and retained partial maps; SIMULATED ONLY (%u checks)\n",checks);
}
