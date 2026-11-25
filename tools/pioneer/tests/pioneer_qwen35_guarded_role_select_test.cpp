/* SPDX-License-Identifier: MIT */
#define PIONEER_RESIDENT_STORE_MOCK_ONLY
#include "pioneer_resident_store_test.cpp"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQwen35GuardedRoleSelect.h"
#include <cstdio>

struct SelectIO : StoreMock {
    bool fleetEnabled=true;
    bool dropWrite=false;
    unsigned runtimeCore=120,accesses=0;
    uint32_t DramTlbRead(unsigned slot,unsigned word) {
        ++accesses;return StoreMock::DramTlbRead(slot,word);
    }
    void DramTlbWrite(unsigned slot,unsigned word,uint32_t value) {
        ++accesses;if (!dropWrite || slot!=0 || word!=0) StoreMock::DramTlbWrite(slot,word,value);
    }
};
struct SelectCheck {
    SelectIO& io;
    unsigned calls=0,failAt=0,accessesAtFailure=0;
    bool operator()() {
        if (++calls==failAt) { accessesAtFailure=io.accesses;return false; }
        return true;
    }
};
int main() {
    unsigned checks=0;
    for (unsigned core=0;core<120;core++) {
        SelectIO io;SelectCheck check{io};PioneerQ35GuardedRoleSelector<SelectIO,SelectCheck> selector(io,check);
        assert(selector.Select(core) && io.runtimeCore==core && io.accesses==12);
        const unsigned route=PioneerQwen35::DataflowBarrierWorkerRoute(core);
        for (unsigned slot=0;slot<2;slot++) for (unsigned word=0;word<3;word++)
            assert(io.DramTlbRead(slot,word)==BlackholeDramFixedTlbWord(slot?0xffa00000U:0,
                route&63U,(route>>6)&63U,false,word));
        checks=check.calls;
    }
    assert(checks==26);
    for (unsigned failAt=1;failAt<=checks;failAt++) {
        SelectIO io;SelectCheck check{io};check.failAt=failAt;
        PioneerQ35GuardedRoleSelector<SelectIO,SelectCheck> selector(io,check);
        assert(!selector.Select(119) && io.runtimeCore==120 && check.calls==failAt
            && io.accesses==check.accessesAtFailure);
        const unsigned access=io.accesses;
        assert(!selector.Select(0) && !selector.Allowed()
            && io.accesses==access && check.calls==failAt);
    }
    SelectIO io;SelectCheck check{io};PioneerQ35GuardedRoleSelector<SelectIO,SelectCheck> selector(io,check);
    assert(!selector.Select(120) && io.accesses==0);
    io.fleetEnabled=false;
    assert(!selector.Select(0) && io.accesses==0);
    SelectIO dropped;dropped.dropWrite=true;SelectCheck droppedCheck{dropped};
    PioneerQ35GuardedRoleSelector<SelectIO,SelectCheck> droppedSelector(dropped,droppedCheck);
    assert(!droppedSelector.Select(119) && dropped.runtimeCore==120 && dropped.accesses==6);
    puts("PASS guarded role selection: 120 routes and every simulated revocation boundary");
}
