/* SPDX-License-Identifier: MIT */
// Exercise the real root-only readiness check; no PCI or hardware access.
#include <cassert>
#include <cstdint>
#include "../../../src/add-ons/kernel/drivers/misc/PioneerPcieRootResetReady.h"
#include <cstdio>

struct Root {
    uint32_t words[1024]{};
    unsigned cardReads=0,rootReads=0;
    uint32_t CardRead(uint16_t,uint8_t) { ++cardReads;return UINT32_MAX; }
    uint32_t RootRead(uint16_t offset,uint8_t width) {
        assert((width==1 || width==2 || width==4) && offset+width<=4096 && (offset&3)+width<=4);
        ++rootReads;
        const uint32_t value=words[offset/4]>>((offset&3)*8);
        return width==1?value&0xff:width==2?value&0xffff:value;
    }
};
static Root ReadyRoot() {
    Root io;
    io.words[0]=0x20421e30;
    io.words[0x34/4]=0x40;
    io.words[0x40/4]=0x00420010; // PCIe v2 root port
    io.words[0x4c/4]=0x83; // up to Gen3 x8, DLL-active reporting not advertised
    io.words[0x50/4]=0x80810000; // Gen1 x8, not training
    io.words[0x100/4]=0x00010001; // AER v1
    return io;
}
int main() {
    {
        Root io=ReadyRoot();
        assert(PioneerPcieRootResetReady(io) && io.rootReads>0 && io.cardReads==0);
    }
    for (unsigned fault=0;fault<11;fault++) {
        Root io=ReadyRoot();
        if (fault==0) io.words[0]=UINT32_MAX;
        if (fault==1) io.words[0x40/4]=0x00020010; // endpoint, not root
        if (fault==2) io.words[0x50/4]=0x88810000; // training
        if (fault==3) io.words[0x50/4]=0x80800000; // speed zero
        if (fault==4) io.words[0x50/4]=0x80010000; // width zero
        if (fault==5) io.words[0x104/4]=1; // root uncorrectable AER
        if (fault==6) io.words[0x48/4]=0x00060000; // nonfatal/fatal device status
        if (fault==7) io.words[0x100/4]=0; // AER absent
        if (fault==8) io.words[0x40/4]=0x00424010; // capability loop
        if (fault==9) io.words[0x4c/4]|=0x00100000; // DLL active advertised but absent
        if (fault==10) io.words[0x104/4]=UINT32_MAX;
        assert(!PioneerPcieRootResetReady(io) && io.cardReads==0);
    }
    {
        Root io=ReadyRoot();io.words[0x4c/4]|=0x00100000;io.words[0x50/4]|=0x20000000;
        assert(PioneerPcieRootResetReady(io) && io.cardReads==0);
    }
    std::puts("PASS reset root readiness rejects link, status and AER faults without endpoint reads; SIMULATED ONLY");
}
