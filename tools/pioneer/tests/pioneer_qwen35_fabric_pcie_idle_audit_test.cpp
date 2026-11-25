/* SPDX-License-Identifier: MIT */
// Actual read-only PCIe idle audit over a simulated root and endpoint.
#define PIONEER_MMIO_PROBE_MODEL_ONLY
#include "pioneer_mmio_probe_test.cpp"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ35PcieIdleAudit.h"
#include <cstdio>

struct Permit {
    unsigned calls=0,failAt=0;bool revoked=false,transient=false;
    bool operator()() { if (++calls==failAt) { revoked=true;return false; }return transient || !revoked; }
};
struct IdleIO : MockIO {
    Permit* permit=nullptr;unsigned readsAfterLoss=0,pauses=0,rootAerReads=0,errorAt=0;
    uint32_t cardLink=0x8081,rootLink=0x8081,cardCaps=0x83,rootCaps=0x83;
    uint32_t cardStatus=0,rootStatus=0,rootAerHeader=0x00010001,rootAerErrors=0;
    uint32_t endpointAerHeader=0,endpointAerErrors=0;
    void Observe() { if (permit && permit->revoked) ++readsAfterLoss; }
    bool QuietCached() { Observe();return quiet; }
    void Pause() { ++pauses; }
    uint32_t CardRead(unsigned offset,unsigned width) {
        Observe();
        if (offset==0x34) return 0x40;
        if (offset==0x40) return 0x0010;
        if (offset==0x42) return 2;
        if (offset==0x4a) return cardStatus;
        if (offset==0x4c) return cardCaps;
        if (offset==0x52) return cardLink;
        if (offset==0x100) return endpointAerHeader;
        if (offset==0x104) return endpointAerErrors;
        return MockIO::CardRead(offset,width);
    }
    uint32_t RootRead(unsigned offset,unsigned width) {
        Observe();
        if (offset==0x10 || offset==0x14 || offset==0x38) return 0;
        if (offset==0x34) return 0x40;
        if (offset==0x40) return 0x0010;
        if (offset==0x42) return 0x42;
        if (offset==0x4a) return rootStatus;
        if (offset==0x4c) return rootCaps;
        if (offset==0x52) return rootLink;
        if (offset==0x100) return rootAerHeader;
        if (offset==0x104) { ++rootAerReads;return errorAt && rootAerReads>=errorAt?1:rootAerErrors; }
        if (offset==0x134) return 0;
        return MockIO::RootRead(offset,width);
    }
};
static void Prepare(IdleIO& io,tt_pioneer_mmio_result& saved) {
    tt_pioneer_mmio_transaction txn{};unsigned stage=0;
    assert(BlackholePioneerMmioBegin(io,saved,txn));
    assert(PioneerPostResetPciFinish(io,saved,stage) && stage==PioneerPostResetPciFinishComplete);
    io.rootAerReads=0;io.readsAfterLoss=0;
}
int main() {
    unsigned checks=0;
    {
        IdleIO io;tt_pioneer_mmio_result saved{};Prepare(io,saved);Permit permit;io.permit=&permit;
        PioneerQ35PcieIdleAuditResult result{};const unsigned writes=io.writes;
        assert(PioneerQ35PcieIdleAudit(io,saved,permit,result) && result.samples==3
            && result.speed==1 && result.width==8 && !result.endpointAer && io.rootAerReads==3
            && io.pauses==2 && io.writes==writes && !io.readsAfterLoss);
        checks=permit.calls;assert(checks>100);
    }
    for (unsigned mode=0;mode<2;mode++) for (unsigned loss=1;loss<=checks;loss++) {
        IdleIO io;tt_pioneer_mmio_result saved{};Prepare(io,saved);Permit permit;
        permit.failAt=loss;permit.transient=mode!=0;io.permit=&permit;
        PioneerQ35PcieIdleAuditResult result{};const unsigned writes=io.writes;
        assert(!PioneerQ35PcieIdleAudit(io,saved,permit,result) && permit.revoked
            && !io.readsAfterLoss && io.writes==writes);
    }
    for (unsigned fault=0;fault<12;fault++) {
        IdleIO io;tt_pioneer_mmio_result saved{};Prepare(io,saved);Permit permit;io.permit=&permit;
        if (fault==0) io.cardStatus=0x20;
        if (fault==1) io.rootStatus=2;
        if (fault==2) io.rootAerErrors=1;
        if (fault==3) io.errorAt=3;
        if (fault==4) io.rootAerHeader=0;
        if (fault==5) io.cardLink=0x8041;
        if (fault==6) io.rootLink|=0x0800;
        if (fault==7) io.rootCaps|=0x00100000;
        if (fault==8) io.endpointAerHeader=UINT32_MAX;
        if (fault==9) { io.endpointAerHeader=0x00010001;io.endpointAerErrors=1; }
        if (fault==10) saved.bars_before[0]=0xdeadbeef;
        if (fault==11) io.endpointAerHeader=0x10010003; // capability loop
        PioneerQ35PcieIdleAuditResult result{};const unsigned writes=io.writes;
        assert(!PioneerQ35PcieIdleAudit(io,saved,permit,result) && io.writes==writes);
    }
    {
        IdleIO io;tt_pioneer_mmio_result saved{};Prepare(io,saved);Permit permit;io.permit=&permit;
        io.endpointAerHeader=0x00010001;io.rootCaps|=0x00100000;io.rootLink|=0x2000;
        PioneerQ35PcieIdleAuditResult result{};
        assert(PioneerQ35PcieIdleAudit(io,saved,permit,result) && result.endpointAer && result.samples==3);
    }
    std::puts("PASS guarded read-only idle PCIe layout, link, Device Status and AER samples; SIMULATED ONLY");
}
