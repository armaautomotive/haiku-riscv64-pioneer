/* SPDX-License-Identifier: MIT */
#define PIONEER_HOST_PATH_MOCK_ONLY
#include "pioneer_host_path_probe_test.cpp"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQwen35GuardedHostAudits.h"
#include <cstdio>

struct CountedHost : HostMock {
    bool revoked=false; unsigned accesses=0;
    void Touch() { assert(!revoked); ++accesses; }
    uint32_t DramTlbRead(unsigned s,unsigned w) { Touch();return HostMock::DramTlbRead(s,w); }
    void DramTlbWrite(unsigned s,unsigned w,uint32_t v) { Touch();HostMock::DramTlbWrite(s,w,v); }
    uint32_t ResetRead() { Touch();return HostMock::ResetRead(); }
    uint32_t SourceRead(unsigned w) { Touch();return HostMock::SourceRead(w); }
    uint32_t ComputeNocRead(uint32_t o) { Touch();return HostMock::ComputeNocRead(o); }
    uint32_t HostInboundRead(unsigned w) { Touch();return HostMock::HostInboundRead(w); }
    uint32_t HostSiiRead(unsigned w) { Touch();return HostMock::HostSiiRead(w); }
    uint32_t HostPciRead(unsigned p,uint16_t o,uint8_t size) {
        Touch();assert(p<2);
        if (o==4) { assert(size==2);return 0x402; }
        if (o==0x34) { assert(size==1);return 0x70; }
        if (o==0x70) { assert(size==2);return 0x10; }
        if (o==0x72) { assert(size==2);return 2; }
        if (o==0x7a) { assert(size==2);return 0; }
        assert(false);return 0;
    }
    uint32_t HostHdmaRead(unsigned w) {
        Touch();assert(w<4 && resetReads>=240);
        for (unsigned i=0;i<3;i++) assert(tlb[0][i]==BlackholeHostDbiTlbWord(i));
        return 0;
    }
};

static unsigned Run(unsigned failAt) {
    CountedHost io;auto dram=io.Baseline();auto noc=io.Compute();
    tt_host_path_result path={};tt_host_dma_audit_result audit={};
    bool pathAttempted=false,auditAttempted=false;unsigned checks=0;
    auto check=[&]() { ++checks;if (checks==failAt) { io.revoked=true;return false; } return true; };
    const bool ok=PioneerQ35GuardedHostAudits(io,dram,noc,0x18611002,path,audit,pathAttempted,auditAttempted,check);
    if (!failAt) {
        assert(ok && pathAttempted && auditAttempted && path.tlb_restored && audit.tlb_restored);
        io.Restored();assert(io.accesses>100);
    } else {
        assert(!ok && io.revoked);
        if (pathAttempted) assert(!path.tlb_restored && path.recovery_required);
        if (auditAttempted) assert(!audit.tlb_restored && audit.recovery_required);
    }
    return checks;
}
int main() {
    const unsigned checks=Run(0);
    for (unsigned fail=1;fail<=checks;fail++) Run(fail);
    std::printf("PASS guarded host path and DMA audit, every authority boundary; SIMULATED ONLY (%u checks)\n",checks);
}
