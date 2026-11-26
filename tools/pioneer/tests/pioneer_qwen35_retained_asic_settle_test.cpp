/* SPDX-License-Identifier: MIT */
// Real ASIC reset and retained delay adapter with simulated PCI and sleep.
#define PIONEER_QWEN35_RETAINED_GATEWAY_WITNESS 1
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <vector>
#include "../../../src/add-ons/kernel/drivers/misc/BlackholeAsicReset.h"
using uint8=uint8_t;using uint16=uint16_t;using uint32=uint32_t;
constexpr int B_OK=0,B_INTERRUPTED=-1;
struct PioneerStoreContext {};
static bool interrupted=false,settled=false,clearMarker=false;
static std::vector<int> sleeps;
struct PioneerSessionIO {
    uint32 command=0x402;unsigned writes=0,postTriggerReads=0;bool triggered=false;
    uint32 RootRead(uint16 offset,uint8 size) {
        if (triggered) {assert(settled);++postTriggerReads;}
        if (offset==0 && size==4) return 0x20421e30;
        if (offset==8 && size==4) return 0x06040000;
        if (offset==4 && size==2) return 0x402;
        return UINT32_MAX;
    }
    uint32 CardRead(uint16 offset,uint8 size) {
        if (triggered) {assert(settled);++postTriggerReads;}
        if (offset==0 && size==4) return 0xb1401e52;
        if (offset==4 && size==2) return command;
        return UINT32_MAX;
    }
    void CardWrite(uint16 offset,uint8,uint32 value) {
        ++writes;
        if (offset==4) command=value;
        if (offset==0x930) triggered=true;
    }
};
static PioneerSessionIO* active=nullptr;
static int snooze(int duration) {
    sleeps.push_back(duration);
    if (interrupted) return B_INTERRUPTED;
    settled=true;
    if (clearMarker) active->command&=~0x40U;
    return B_OK;
}
template<class IO> static bool PioneerPcieRootResetReady(IO&) {return true;}
#include "pioneer_q35_recovery_reset_io.inc"

int main() {
    PioneerSessionIO pci;active=&pci;PioneerStoreContext context;
    Qwen35RecoveryResetIO first(pci,&context);BlackholeAsicResetResult result{};
    clearMarker=true;
    assert(BlackholeAsicReset(first,3,result));
    assert((sleeps==std::vector<int>{2000000}) && result.polls==1
        && pci.writes==3 && pci.postTriggerReads>0);

    pci=PioneerSessionIO{};sleeps.clear();settled=false;clearMarker=false;interrupted=true;
    Qwen35RecoveryResetIO failed(pci,&context);
    assert(!BlackholeAsicReset(failed,3,result));
    assert(result.stage==BLACKHOLE_ASIC_RESET_TIMEOUT && pci.writes==3
        && pci.postTriggerReads==0 && (sleeps==std::vector<int>{2000000}));
    interrupted=false;clearMarker=true;Qwen35RecoveryResetIO resumed(pci,&context);
    assert(BlackholeAsicResetResumeWait(resumed,3,result));
    assert((sleeps==std::vector<int>{2000000,2000000}) && pci.writes==3 && pci.postTriggerReads>0);
    std::puts("PASS actual retained reset adapter waits before PCI polling and never replays trigger; SIMULATED ONLY");
}
