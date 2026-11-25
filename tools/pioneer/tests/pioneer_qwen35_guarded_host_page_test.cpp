/* SPDX-License-Identifier: MIT */
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQwen35GuardedHostPage.h"
#include <cassert>
#include <cstdio>

struct HostPage {
    uint32_t words[TT_HOST_PAGE_WORDS]{};
    bool revoked=false,allocated=false;unsigned accesses=0;
    void Touch() { assert(!revoked);++accesses; }
    int32_t AllocateHostPage() { Touch();assert(!allocated);allocated=true;return 0; }
    int32_t HostPageMap(uint64_t& address,uint64_t& bytes) {
        Touch();assert(allocated);address=UINT64_C(0x100000000);bytes=TT_HOST_PAGE_BYTES;return 0;
    }
    int32_t SetHostPageNonCached() { Touch();assert(allocated);return 0; }
    uint32_t HostPageRead(unsigned word) { Touch();assert(allocated && word<TT_HOST_PAGE_WORDS);return words[word]; }
    void HostPageWrite(unsigned word,uint32_t value) { Touch();assert(allocated && word<TT_HOST_PAGE_WORDS);words[word]=value; }
};
static unsigned Run(unsigned failAt,bool& lostAfterAllocation) {
    HostPage io;tt_host_page_result result{};unsigned checks=0;
    auto check=[&]() { ++checks;if (checks==failAt) { io.revoked=true;return false; }return true; };
    const bool okay=PioneerQ35GuardedPrepareHostPage(io,result,check);
    if (!failAt) {
        assert(okay && result.state==TT_HOST_PAGE_READY && result.allocated && !result.released
            && result.patterns==2 && result.map_stable);
    } else {
        assert(!okay && io.revoked && result.state!=TT_HOST_PAGE_READY && !result.map_stable);
        if (io.allocated) { assert(result.allocated && !result.released);lostAfterAllocation=true; }
        else assert(!result.allocated && result.released);
    }
    return checks;
}
int main() {
    bool lostAfterAllocation=false;const unsigned checks=Run(0,lostAfterAllocation);
    for (unsigned failure=1;failure<=checks;failure++) Run(failure,lostAfterAllocation);
    assert(lostAfterAllocation);
    std::printf("PASS guarded CPU host-page preparation: every ticket boundary retains allocation; SIMULATED ONLY (%u checks)\n",checks);
}
