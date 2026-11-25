/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_QWEN35_GUARDED_MMIO_BEGIN_H
#define PIONEER_QWEN35_GUARDED_MMIO_BEGIN_H
#include "BlackholePioneerMmioProbe.h"

// The wrapped Map only creates CPU virtual mappings; it must not access any
// device register. A partial map remains owned by the retained context if
// the ticket expires while Map is running. The check itself is metadata-only.
template<class IO,class Check> struct PioneerQ35GuardedMmioBeginIO {
    IO& io;Check& check;bool lost=false;
    bool Allowed() { if (lost || !check()) { lost=true;return false; } return true; }
    const tt_pioneer_profile& Profile() { return io.Profile(); }
    uint64_t Size(unsigned bar) {
        if (!Allowed()) return UINT64_MAX;
        const uint64_t value=io.Size(bar);return Allowed()?value:UINT64_MAX;
    }
    uint32_t CardRead(uint16_t offset,uint8_t size) {
        if (!Allowed()) return UINT32_MAX;
        const uint32_t value=io.CardRead(offset,size);return Allowed()?value:UINT32_MAX;
    }
    void CardWrite(uint16_t offset,uint8_t size,uint32_t value) {
        if (Allowed()) { io.CardWrite(offset,size,value);(void)Allowed(); }
    }
    uint32_t RootRead(uint16_t offset,uint8_t size) {
        if (!Allowed()) return UINT32_MAX;
        const uint32_t value=io.RootRead(offset,size);return Allowed()?value:UINT32_MAX;
    }
    void RootWrite(uint16_t offset,uint8_t size,uint32_t value) {
        if (Allowed()) { io.RootWrite(offset,size,value);(void)Allowed(); }
    }
    bool Quiet() {
        if (RootRead(0x10,4)!=0 || RootRead(0x14,4)!=0 || (RootRead(0x38,4)&1)!=0
            || !Allowed()) return false;
        const bool quiet=io.QuietCached();return Allowed() && quiet;
    }
    bool Map() {
        if (!Allowed()) return false;
        const bool mapped=io.Map();return Allowed() && mapped;
    }
    uint32_t ReadNodeId() {
        if (!Allowed()) return UINT32_MAX;
        const uint32_t value=io.ReadNodeId();return Allowed()?value:UINT32_MAX;
    }
};

template<class IO,class Check>
bool PioneerQ35GuardedMmioBegin(IO& io,tt_pioneer_mmio_result& result,
    tt_pioneer_mmio_transaction& transaction,Check& check)
{
    PioneerQ35GuardedMmioBeginIO<IO,Check> guarded{io,check};
    if (!guarded.Allowed()) { result={};transaction={};return false; }
    const bool begun=BlackholePioneerMmioBegin(guarded,result,transaction);
    if (guarded.lost || !guarded.Allowed()) { result.restored=0;return false; }
    return begun;
}
#endif
