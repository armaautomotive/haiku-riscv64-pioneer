/* SPDX-License-Identifier: MIT */
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQwen35PostResetL2Audit.h"
#include <cassert>
#include <cstdio>

struct IO {
    uint32_t tlb[3]={17,19,21},initial[3]={17,19,21};
    uint32_t boot=5,reset=15;
    unsigned allowedCalls=0,loseAt=~0U,writes=0,resetReads=0,bootReads=0,dropWrite=0;
    bool changingReset=false,changingBoot=false;
    bool L2AuditAllowed() { return ++allowedCalls<loseAt; }
    uint32_t TlbRead(unsigned i) { return tlb[i]; }
    void TlbWrite(unsigned i,uint32_t v) { if (++writes!=dropWrite) tlb[i]=v; }
    bool ArcRoute() const {
        for (unsigned i=0;i<3;i++) if (tlb[i]!=BlackholeArcScratchTlbWord(i)) return false;
        return true;
    }
    uint32_t L2BootRead() { assert(ArcRoute());return changingBoot && bootReads++?boot+1:boot; }
    uint32_t L2ResetRead() { assert(ArcRoute());return changingReset && resetReads++?reset+1:reset; }
    void L2Pause() {}
    bool Restored() const { return tlb[0]==initial[0] && tlb[1]==initial[1] && tlb[2]==initial[2]; }
};
int main() {
    PioneerQ35PostResetL2Result result;
    {
        IO io;assert(PioneerQ35PostResetL2Audit(io,result) && result.sampled && result.reset==15
            && !result.restoreFailed && io.Restored() && io.writes==6);
    }
    {
        IO io;io.reset=0x1f;
        assert(!PioneerQ35PostResetL2Audit(io,result) && result.sampled && result.reset==0x1f
            && !result.restoreFailed && io.Restored());
    }
    for (unsigned kind=0;kind<4;kind++) {
        IO io;
        if (kind==0) io.changingReset=true;
        if (kind==1) io.changingBoot=true;
        if (kind==2) io.reset=UINT32_MAX;
        if (kind==3) io.dropWrite=1;
        assert(!PioneerQ35PostResetL2Audit(io,result) && !result.sampled && !result.restoreFailed && io.Restored());
    }
    // Losing permission before the first write is harmless; afterward no
    // competing TLB restoration is attempted and failure must be retained.
    for (unsigned point=1;point<24;point++) {
        IO io;io.loseAt=point;
        const bool okay=PioneerQ35PostResetL2Audit(io,result);
        if (io.allowedCalls>=point) {
            assert(!okay);
            if (io.writes) assert(result.restoreFailed);
            else assert(!result.restoreFailed && io.Restored());
        } else assert(okay && io.Restored());
    }
    {
        IO io;io.dropWrite=4; // first restoration write fails readback
        assert(!PioneerQ35PostResetL2Audit(io,result) && result.restoreFailed && !io.Restored());
    }
    std::puts("PASS post-reset L2 reset observation and TLB retention; SIMULATED ONLY");
}
