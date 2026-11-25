/* SPDX-License-Identifier: MIT */
#define PIONEER_QWEN35_DEVICE_DISPATCH 1
#define PIONEER_BANKS_MOCK_ONLY
#include "pioneer_resident_banks_test.cpp"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQwen35GuardedStorePage.h"
#include <cstdio>
#include <vector>

struct CountedStore : BanksMock {
    unsigned accesses=0,checksAtDanger[16]{},dangerCount=0;
    const unsigned* checks=nullptr;
    uint32_t ResidentNocRead(uint32_t o) { ++accesses;return BanksMock::ResidentNocRead(o); }
    void ResidentNocWrite(uint32_t o,uint32_t v) {
        ++accesses;if (o==0x40 && checks) checksAtDanger[dangerCount++]=*checks;
        BanksMock::ResidentNocWrite(o,v);
    }
    uint32_t ResidentL1Read(unsigned w) { ++accesses;return BanksMock::ResidentL1Read(w); }
    uint32_t DramTlbRead(unsigned s,unsigned w) { ++accesses;return BanksMock::DramTlbRead(s,w); }
    uint32_t ResetRead() { ++accesses;return BanksMock::ResetRead(); }
    uint32_t SourceRead(unsigned w) { ++accesses;return BanksMock::SourceRead(w); }
    uint32_t HostPciRead(unsigned p,uint16_t o,uint8_t s) { ++accesses;return BanksMock::HostPciRead(p,o,s); }
    void HostPciWrite(unsigned p,uint16_t o,uint8_t s,uint32_t v) {
        ++accesses;if (checks) checksAtDanger[dangerCount++]=*checks;
        BanksMock::HostPciWrite(p,o,s,v);
    }
    int32_t HostPageMap(uint64_t& a,uint64_t& n) { ++accesses;return BanksMock::HostPageMap(a,n); }
    uint32_t HostPageRead(unsigned w) { ++accesses;return BanksMock::HostPageRead(w); }
    void HostPageWrite(unsigned w,uint32_t v) { ++accesses;BanksMock::HostPageWrite(w,v); }
    void Pause() { ++accesses;BanksMock::Pause(); }
};
struct Admission {
    CountedStore& io;unsigned calls=0,failAt=0,accessesAtFailure=0;
    bool operator()() {
        if (++calls==failAt) { accessesAtFailure=io.accesses;return false; }
        return true;
    }
};
static std::vector<uint8_t> initialized(PioneerResidentBanks::Pages,0);
static bool Run(bool write,unsigned failAt,unsigned& checks,std::vector<unsigned>* dangers=nullptr) {
    CountedStore io;auto dram=BankProof(io);auto noc=io.Compute();
    tt_host_page_result host{};PioneerResidentLease lease;PioneerResidentBanks banks;
    assert(banks.Bind(dram) && Prepared(io,host,lease));
    PioneerQ35StreamBankRange ranges[120]{};int owner=0;
    for (unsigned i=0;i<120;i++) ranges[i]={0,i*4096,4096};
    assert(banks.ReserveQ35Stream(dram,&owner,ranges,120,initialized.data(),initialized.size()));
    for (unsigned w=0;w<1024;w++) io.memory[io.Key(18|(12<<6),PioneerResidentBanks::Base)][w]=0x12340000+w;
    uint32_t page[1024];for (unsigned w=0;w<1024;w++) page[w]=0x12340000+w;
    tt_resident_store_result result{};PioneerResidentStoreTrace trace{};Admission check{io};check.failAt=failAt;
    io.checks=&check.calls;
    bool passed=PioneerQ35GuardedStorePage(io,check,dram,noc,host,lease,0,write,page,result,banks,0,&owner,trace);
    checks=check.calls;
    if (!failAt) {
        assert(passed && !lease.recovery_required && result.state==TT_STORE_PASSED);
        if (dangers) for (unsigned n=0;n<io.dangerCount;n++) dangers->push_back(io.checksAtDanger[n]);
    } else {
        assert(!passed && lease.recovery_required && result.recovery_required && io.HostReadMock::quarantined);
        assert(check.calls==failAt && io.accesses==check.accessesAtFailure);
    }
    return passed;
}
int main() {
    (void)&Input;(void)&Prepare;(void)&Upload;(void)&Close;
    for (bool write:{false,true}) {
        unsigned total=0;std::vector<unsigned> danger;
        assert(Run(write,0,total,&danger) && total>2000 && !danger.empty());
        for (unsigned failure=1;failure<=total;failure+=31) { unsigned seen=0;Run(write,failure,seen); }
        for (unsigned index:danger) for (unsigned failure:{index,index+1}) {
            unsigned seen=0;Run(write,failure,seen);
        }
        unsigned seen=0;Run(write,total,seen);
    }
    puts("PASS guarded scratch page transfer revocation around PCI writes and NoC doorbells; SIMULATED ONLY");
}
