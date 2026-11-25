/* SPDX-License-Identifier: MIT */
#define PIONEER_MMIO_PROBE_MODEL_ONLY
#include "pioneer_mmio_probe_test.cpp"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerPostResetPci.h"
#include <cstdio>
struct Authority {
    unsigned checks=0,loseAt=0;bool lost=false;
    bool operator()(){if(++checks==loseAt)lost=true;return !lost;}
};
struct GuardedMockIO:MockIO {
    Authority& authority;
    explicit GuardedMockIO(Authority& a):authority(a){mapped=true;}
    bool QuietCached(){assert(!authority.lost);return quiet;}
    uint64_t Size(unsigned bar){assert(!authority.lost);return MockIO::Size(bar);}
    uint32_t CardRead(unsigned o,unsigned w){assert(!authority.lost);return MockIO::CardRead(o,w);}
    uint32_t RootRead(unsigned o,unsigned w){assert(!authority.lost);if(o==0x10 || o==0x14 || o==0x38)return 0;return MockIO::RootRead(o,w);}
    void CardWrite(unsigned o,unsigned w,uint32_t v){assert(!authority.lost);MockIO::CardWrite(o,w,v);}
    void RootWrite(unsigned o,unsigned w,uint32_t v){assert(!authority.lost);MockIO::RootWrite(o,w,v);}
};
static bool Restore(GuardedMockIO& io,Authority& check){
    PioneerGuardedMasterDisableIO<GuardedMockIO,Authority> guarded(io,check);
    PioneerPostResetPciResult result{};
    return PioneerPostResetPciRestore(guarded,result) && guarded.Valid();
}
int main(){
    Authority allowed;GuardedMockIO baseline(allowed);
    assert(Restore(baseline,allowed) && baseline.command[0]==0x402 && baseline.command[1]==0x402);
    for(unsigned at=1;at<=allowed.checks;at++){
        Authority check;check.loseAt=at;GuardedMockIO io(check);
        assert(!Restore(io,check) && check.lost);
        assert(!(io.command[0]&4) && !(io.command[1]&4));
    }
    std::puts("PASS actual guarded post-link PCI restoration, authority loss at every check; SIMULATED ONLY");
}
