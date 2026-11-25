/* SPDX-License-Identifier: MIT */
// Exercise the actual fleet adapter with the guarded recovery selector.
#define PIONEER_Q35_FLEET_TEST_NO_MAIN 1
#include "pioneer_qwen35_dataflow_role_fleet_pci_io_test.cpp"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQwen35GuardedRoleSelect.h"

struct GuardedFleetIO : PioneerSessionIO {
    uint32_t tlbs[2][3]{};
    unsigned accesses=0;
    GuardedFleetIO() {
        const unsigned route=DataflowBarrierWorkerRoute(runtimeCore);
        for (unsigned slot=0;slot<2;slot++) for (unsigned word=0;word<3;word++)
            tlbs[slot][word]=BlackholeDramFixedTlbWord(slot?0xffa00000U:0,
                route&63U,(route>>6)&63U,false,word);
    }
    uint32_t DramTlbRead(unsigned slot,unsigned word) { ++accesses;return tlbs[slot][word]; }
    void DramTlbWrite(unsigned slot,unsigned word,uint32_t value) { ++accesses;tlbs[slot][word]=value; }
    uint32_t ResidentNocRead(unsigned offset) { ++accesses;return PioneerSessionIO::ResidentNocRead(offset); }
    uint32_t ResetRead() { ++accesses;return PioneerSessionIO::ResetRead(); }
};
struct FleetCheck {
    GuardedFleetIO& io;unsigned calls=0,failAt=0,accessesAtFailure=0;
    bool operator()() {
        if (++calls==failAt) { accessesAtFailure=io.accesses;return false; }
        return true;
    }
};
int main() {
    gLocked=true;
    GuardedFleetIO device;FleetPciTestScope scope;PioneerQwen35RoleHaikuMap platform;
    DataflowRoleWritableViews<PioneerQwen35RoleHaikuMap> views(platform);
    FleetCheck check{device};
    using Selector=PioneerQ35GuardedRoleSelector<GuardedFleetIO,FleetCheck>;
    PioneerQwen35RoleFleetPciIO<GuardedFleetIO,FleetPciTestScope,decltype(views),Selector>
        fleet(device,scope,views,check);
    assert(fleet.AllRolesHeld(119) && device.runtimeCore==119 && device.accesses>12);
    const unsigned before=device.accesses;
    check.failAt=check.calls+2; // lose authority immediately before first TLB write
    assert(!fleet.AllRolesHeld(0) && device.runtimeCore==119 && device.accesses==before);
    const unsigned calls=check.calls;
    assert(!fleet.RoleSelectionAuthorityValid() && !fleet.AllRolesHeld(0)
        && check.calls==calls && device.accesses==before);
    GuardedFleetIO countedDevice;FleetCheck countedCheck{countedDevice};
    PioneerQwen35RoleFleetPciIO<GuardedFleetIO,FleetPciTestScope,decltype(views),Selector>
        counted(countedDevice,scope,views,countedCheck);
    assert(counted.AllRolesHeld(119));
    const unsigned boundaries=countedCheck.calls;
    assert(boundaries>200);
    for (unsigned failure=1;failure<=boundaries;failure++) {
        GuardedFleetIO trialDevice;FleetCheck trialCheck{trialDevice};trialCheck.failAt=failure;
        PioneerQwen35RoleFleetPciIO<GuardedFleetIO,FleetPciTestScope,decltype(views),Selector>
            trial(trialDevice,scope,views,trialCheck);
        assert(!trial.AllRolesHeld(119) && trialCheck.calls==failure
            && trialDevice.accesses==trialCheck.accessesAtFailure);
        assert(!trial.RoleSelectionAuthorityValid() && trialDevice.accesses==trialCheck.accessesAtFailure);
    }
    gLocked=false;
    std::printf("PASS actual held-fleet adapter stops on all %u simulated authority boundaries\n",boundaries);
}
