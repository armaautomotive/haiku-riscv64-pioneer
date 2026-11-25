/* SPDX-License-Identifier: MIT */
#define PIONEER_RESIDENT_STORE_MOCK_ONLY
#include "pioneer_resident_store_test.cpp"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQwen35GuardedLeaseEnd.h"
#include <cstdio>

struct CountedIO : StoreMock {
    unsigned accesses=0;
    uint32_t* HostReadBackup(unsigned words) { ++accesses;return StoreMock::HostReadBackup(words); }
    uint32_t ResidentNocRead(uint32_t o) { ++accesses;return StoreMock::ResidentNocRead(o); }
    void ResidentNocWrite(uint32_t o,uint32_t v) { ++accesses;StoreMock::ResidentNocWrite(o,v); }
    uint32_t ResidentL1Read(unsigned w) { ++accesses;return StoreMock::ResidentL1Read(w); }
    void ResidentL1Write(unsigned w,uint32_t v) { ++accesses;StoreMock::ResidentL1Write(w,v); }
    uint32_t DramTlbRead(unsigned s,unsigned w) { ++accesses;return StoreMock::DramTlbRead(s,w); }
    void DramTlbWrite(unsigned s,unsigned w,uint32_t v) { ++accesses;StoreMock::DramTlbWrite(s,w,v); }
    uint32_t ResetRead() { ++accesses;return StoreMock::ResetRead(); }
    uint32_t HostPciRead(unsigned p,uint16_t o,uint8_t s) { ++accesses;return StoreMock::HostPciRead(p,o,s); }
};
struct Admission {
    CountedIO& io;unsigned calls=0,failAt=0,accessesAtFailure=0;
    bool operator()() {
        if (++calls==failAt) { accessesAtFailure=io.accesses;return false; }
        return true; // transient revocation must still remain sticky
    }
};
int main() {
    unsigned totalChecks=0;
    unsigned idleChecks=0,beginChecks=0;
    {
        CountedIO io;tt_resident_session_idle_result result{};Admission check{io};
        assert(PioneerQ35GuardedSessionIdle(io,io.Compute(),result,check) && result.ready && result.tlb_restored);
        idleChecks=check.calls;
    }
    for (unsigned failAt=1;failAt<=idleChecks;failAt++) {
        CountedIO io;tt_resident_session_idle_result result{};Admission check{io};check.failAt=failAt;
        assert(!PioneerQ35GuardedSessionIdle(io,io.Compute(),result,check)
            && result.recovery_required && !result.ready && check.calls==failAt && io.accesses==check.accessesAtFailure);
    }
    {
        CountedIO io;PioneerResidentLease lease;Admission check{io};
        assert(PioneerQ35GuardedLeaseBegin(io,io.Compute(),lease,check) && lease.ready && !lease.recovery_required);
        beginChecks=check.calls;assert(beginChecks>4000);
    }
    for (unsigned failAt=1;failAt<=beginChecks;failAt++) {
        CountedIO io;PioneerResidentLease lease;Admission check{io};check.failAt=failAt;
        assert(!PioneerQ35GuardedLeaseBegin(io,io.Compute(),lease,check)
            && lease.recovery_required && !lease.ready && check.calls==failAt && io.accesses==check.accessesAtFailure);
    }
    {
        CountedIO io;tt_host_page_result host;PioneerResidentLease lease;
        assert(Prepared(io,host,lease));lease.l1_dirty=true;lease.l1_restored=false;
        Admission check{io};
        assert(PioneerQ35GuardedLeaseEnd(io,io.Compute(),lease,check));
        assert(!lease.recovery_required && lease.l1_restored && lease.niu_restored && lease.tlb_restored);
        totalChecks=check.calls;assert(totalChecks>8000);
        assert(io.ReleaseHostPage()==0);io.Safe();
    }
    for (unsigned failAt=1;failAt<=totalChecks;failAt++) {
        CountedIO io;tt_host_page_result host;PioneerResidentLease lease;
        assert(Prepared(io,host,lease));lease.l1_dirty=true;lease.l1_restored=false;
        Admission check{io};check.failAt=failAt;
        assert(!PioneerQ35GuardedLeaseEnd(io,io.Compute(),lease,check));
        assert(check.calls==failAt && io.accesses==check.accessesAtFailure && lease.recovery_required);
        if (failAt>1) assert(!lease.ready && !lease.l1_restored && !lease.niu_restored && !lease.tlb_restored);
    }
    puts("PASS guarded session idle and lease begin/end stop after every simulated revocation");
}
