/* SPDX-License-Identifier: MIT */
// Replay the observed root-status ordering against the real reset and root
// readiness algorithms. This is synthetic PCI state, not hardware evidence.
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <vector>
#include "../../../src/add-ons/kernel/drivers/misc/BlackholeAsicReset.h"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerPcieRootResetReady.h"

struct TraceReplay {
    uint32_t root[1024]{};
    uint32_t command=0x0402;
    unsigned delays=0, postTriggerCardReads=0;
    bool triggered=false;
    std::vector<uint16_t> writes;

    TraceReplay() {
        root[0]=0x20421e30;
        root[8/4]=0x06040000;
        root[0x34/4]=0x40;
        root[0x40/4]=0x00420010; // PCIe v2 root port
        root[0x4c/4]=0x83;       // up to Gen3 x8
        root[0x50/4]=0x80810000; // recorded link status
        root[0x100/4]=0x00010001; // AER v1
    }
    uint32_t RootRead(uint16_t offset,uint8_t width) {
        assert((width==1 || width==2 || width==4) && offset+width<=4096
            && (offset&3)+width<=4);
        uint32_t value=root[offset/4]>>((offset&3)*8);
        return width==1?value&0xff:width==2?value&0xffff:value;
    }
    bool RootReady() { return PioneerPcieRootResetReady(*this); }
    uint32_t CardRead(uint16_t offset,uint8_t width) {
        if (triggered) ++postTriggerCardReads;
        if (offset==0 && width==4) return 0xb1401e52;
        if (offset==4 && width==2) return command;
        assert(false);return UINT32_MAX;
    }
    void CardWrite(uint16_t offset,uint8_t width,uint32_t value) {
        writes.push_back(offset);
        if (offset==4 && width==2) command=value;
        else if (offset==0x934 && width==4) assert(value==1);
        else if (offset==0x930 && width==4) { assert(value==0x11);triggered=true; }
        else assert(false);
    }
    void RecoveryDelay() {
        ++delays;
        // The first poll sample in the real test had these root errors.
        // Marker clearance is hypothetical: the live test never proved it.
        command &= ~0x40u;
        root[0x48/4]=0x00060000;
        root[0x104/4]=1;
    }
};

int main() {
    TraceReplay io;
    BlackholeAsicResetResult result{};
    assert(!BlackholeAsicReset(io,100,result));
    assert(result.stage==BLACKHOLE_ASIC_RESET_TIMEOUT && result.polls==100);
    assert(result.marker_after==UINT32_MAX && io.postTriggerCardReads==0);
    assert(io.command==0x0402); // completion is deliberately unobservable
    for (unsigned i=0;i<3;++i) {
        assert(!BlackholeAsicResetResumeWait(io,100,result));
        assert(result.stage==BLACKHOLE_ASIC_RESET_TIMEOUT && result.polls==200+100*i);
        assert(io.postTriggerCardReads==0 && io.writes.size()==3);
    }
    assert(io.writes[0]==4 && io.writes[1]==0x934 && io.writes[2]==0x930);
    std::puts("PASS observed root-error order retains reset without endpoint polling or retrigger; SIMULATED ONLY");
}
