/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_QWEN35_GUARDED_HOST_PAGE_H
#define PIONEER_QWEN35_GUARDED_HOST_PAGE_H
#include "PioneerHostPage.h"

// No PCI, MMIO or DMA is performed by this interface. The backing area stays
// with the fresh context if authority is lost at any point in preparation.
template<class IO,class Check> struct PioneerQ35GuardedHostPageIO {
    IO& io;Check& check;bool lost=false,allocated=false;
    bool Allowed() { if (lost || !check()) { lost=true;return false; } return true; }
    int32_t AllocateHostPage() {
        if (!Allowed()) return -1;
        const int32_t status=io.AllocateHostPage();allocated=status==0;
        return Allowed()?status:-1;
    }
    int32_t HostPageMap(uint64_t& address,uint64_t& bytes) {
        if (!Allowed()) return -1;
        const int32_t status=io.HostPageMap(address,bytes);return Allowed()?status:-1;
    }
    int32_t SetHostPageNonCached() {
        if (!Allowed()) return -1;
        const int32_t status=io.SetHostPageNonCached();return Allowed()?status:-1;
    }
    uint32_t HostPageRead(unsigned word) {
        if (!Allowed()) return UINT32_MAX;
        const uint32_t value=io.HostPageRead(word);return Allowed()?value:UINT32_MAX;
    }
    void HostPageWrite(unsigned word,uint32_t value) {
        if (Allowed()) { io.HostPageWrite(word,value);(void)Allowed(); }
    }
};
template<class IO,class Check>
bool PioneerQ35GuardedPrepareHostPage(IO& io,tt_host_page_result& result,Check& check)
{
    PioneerQ35GuardedHostPageIO<IO,Check> guarded{io,check};
    if (!guarded.Allowed()) { result={};result.released=1;return false; }
    const bool ready=PioneerPrepareHostPage(guarded,result);
    if (guarded.lost || !guarded.Allowed()) {
        if (guarded.allocated) { result.allocated=1;result.released=0; }
        result.state=TT_HOST_PAGE_PATTERN_FAILED;result.map_stable=0;return false;
    }
    return ready;
}
#endif
