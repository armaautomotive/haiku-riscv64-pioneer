/* SPDX-License-Identifier: MIT */
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQwen35GuardedMmioEnd.h"
#include <cassert>
#include <cstdio>
#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wreturn-type"
#elif defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wreturn-type"
#endif
#define main q35_dispose_fixture_main
#include "pioneer_post_reset_dispose_test.cpp"
#undef main
#if defined(__clang__)
#pragma clang diagnostic pop
#elif defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

struct CountedPci {
    uint32_t card[64]{},root[64]{};bool revoked=false;unsigned accesses=0,writes=0,deleted=0;
    uint16_t rootStatus=0xa5a5;
    CountedPci() {
        card[1]=root[1]=0x402;
        const uint32_t bars[]={0xc,0x49,0x2000000c,0x49,0xc,0xfffffff8};
        for (unsigned i=0;i<6;i++) card[4+i]=bars[i];
        root[9]=0x20010001;root[10]=root[11]=0x49;
    }
    void Touch() { assert(!revoked);++accesses; }
    uint32_t CardRead(unsigned address,unsigned) { Touch();return card[address/4]; }
    uint32_t RootRead(unsigned address,unsigned) { Touch();return root[address/4]; }
    void CardWrite(unsigned address,unsigned,uint32_t value) { Touch();++writes;card[address/4]=value; }
    void RootWrite(unsigned address,unsigned size,uint32_t value) {
        Touch();++writes;
        if (address==4) { rootStatus &= ~(size==2?rootStatus:uint16_t(value>>16));root[address/4]=value&0xffffU; }
        else root[address/4]=value;
    }
};
static tt_pioneer_mmio_result InitialResult() {
    tt_pioneer_mmio_result r{};r.version=1;r.state=TT_PIONEER_MMIO_READ_OK;
    for (unsigned i=0;i<6;i++) r.bars_before[i]=i%2?0:0xc;
    r.bridge_before[0]=0x1fff1;return r;
}
static unsigned Run(unsigned failAt) {
    CountedPci io;auto result=InitialResult();tt_pioneer_mmio_transaction transaction{true,true};
    unsigned checks=0;auto check=[&]() { ++checks;if (checks==failAt) { io.revoked=true;return false; }return true; };
    auto unmap=[&]() {
        for (unsigned i=0;i<3;i++) {
            if (!check()) return false;
            io.Touch();++io.deleted;
            if (!check()) return false;
        }
        return true;
    };
    const bool okay=PioneerQ35GuardedMmioEnd(io,result,transaction,check,unmap);
    if (!failAt) assert(okay && result.restored && io.deleted==3 && io.writes==11 && io.rootStatus==0xa5a5);
    else {
        assert(!okay && io.revoked && !result.restored
            && result.state==TT_PIONEER_MMIO_RESTORE_FAILED && !transaction.config_attempted);
        const unsigned accesses=io.accesses;
        auto later=[]() { return true; };auto noUnmap=[]() { assert(false);return false; };
        assert(!PioneerQ35GuardedMmioEnd(io,result,transaction,later,noUnmap) && io.accesses==accesses);
    }
    return checks;
}
// Compose the guarded PCI transaction with the actual area-disposal logic.
static unsigned RunActualDisposal(unsigned failAt,bool alias=false,bool protectedArea=false) {
    deleted.clear();deleteAfterLoss=0;failArea=-1;
    CountedPci io;FakeIO mapping;FabricAreas(mapping);
    if (alias) mapping.runtimeAreas[1]=mapping.PioneerHostPageIO::area;
    auto result=InitialResult();tt_pioneer_mmio_transaction transaction{true,true};
    bool live=true;authorityLive=&live;unsigned checks=0;
    auto check=[&]() { ++checks;if (checks==failAt) { live=false;io.revoked=true;return false; }return true; };
    auto protect=[&](area_id id) { return protectedArea && id==12; };
    auto unmap=[&]() { return mapping.DisposeFabricOldMmio(check,protect)==B_OK; };
    const bool okay=PioneerQ35GuardedMmioEnd(io,result,transaction,check,unmap);
    assert(!deleteAfterLoss && mapping.PioneerHostPageIO::area==20 && mapping.outputArea==21 && mapping.wideArea==22);
    if (!failAt && !alias && !protectedArea) {
        assert(okay && result.restored && mapping.fabricOldMmioDisposeComplete && deleted.size()==3 && io.rootStatus==0xa5a5);
    } else {
        assert(!okay && !result.restored && !transaction.config_attempted);
        if (alias || protectedArea) assert(deleted.empty() && !io.writes);
        const auto deletedCount=deleted.size();const unsigned accessCount=io.accesses;
        auto later=[]() { return true; };auto noUnmap=[]() { assert(false);return false; };
        assert(!PioneerQ35GuardedMmioEnd(io,result,transaction,later,noUnmap)
            && deleted.size()==deletedCount && io.accesses==accessCount);
    }
    authorityLive=nullptr;return checks;
}
int main() {
    (void)&q35_dispose_fixture_main;
    const unsigned checks=Run(0);
    for (unsigned fail=1;fail<=checks;fail++) Run(fail);
    CountedPci io;auto result=InitialResult();tt_pioneer_mmio_transaction transaction{true,true};
    auto check=[]() { return true; };auto refuse=[]() { return false; };
    assert(!PioneerQ35GuardedMmioEnd(io,result,transaction,check,refuse)
        && !io.accesses && !result.restored && !transaction.config_attempted);
    const unsigned composedChecks=RunActualDisposal(0);
    for (unsigned fail=1;fail<=composedChecks;fail++) RunActualDisposal(fail);
    RunActualDisposal(0,true);RunActualDisposal(0,false,true);
    std::printf("PASS guarded PCI close: every ticket boundary, actual simulated area disposal, no replay; SIMULATED ONLY (%u/%u checks)\n",
        checks,composedChecks);
}
