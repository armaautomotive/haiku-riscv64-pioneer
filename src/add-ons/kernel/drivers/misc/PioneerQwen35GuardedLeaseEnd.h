/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_QWEN35_GUARDED_LEASE_END_H
#define PIONEER_QWEN35_GUARDED_LEASE_END_H
#include "PioneerResidentStore.h"

// Admission is metadata-only. It does not establish physical exclusion.
// Revocation is sticky even if a subsequent admission check would succeed.
template<class IO,class Check> struct PioneerQ35GuardedLeaseIO {
    IO& io;Check& check;bool lost=false;
    bool Allowed() { if (lost || !check()) { lost=true;return false; } return true; }
    uint32_t ResidentNocRead(uint32_t offset) {
        if (!Allowed()) return UINT32_MAX;
        uint32_t value=io.ResidentNocRead(offset);return Allowed()?value:UINT32_MAX;
    }
    void ResidentNocWrite(uint32_t offset,uint32_t value) {
        if (!Allowed()) return;
        io.ResidentNocWrite(offset,value);(void)Allowed();
    }
    uint32_t ResidentL1Read(unsigned word) {
        if (!Allowed()) return UINT32_MAX;
        uint32_t value=io.ResidentL1Read(word);return Allowed()?value:UINT32_MAX;
    }
    void ResidentL1Write(unsigned word,uint32_t value) {
        if (!Allowed()) return;
        io.ResidentL1Write(word,value);(void)Allowed();
    }
    uint32_t DramTlbRead(unsigned slot,unsigned word) {
        if (!Allowed()) return UINT32_MAX;
        uint32_t value=io.DramTlbRead(slot,word);return Allowed()?value:UINT32_MAX;
    }
    void DramTlbWrite(unsigned slot,unsigned word,uint32_t value) {
        if (!Allowed()) return;
        io.DramTlbWrite(slot,word,value);(void)Allowed();
    }
    uint32_t ResetRead() {
        if (!Allowed()) return UINT32_MAX;
        uint32_t value=io.ResetRead();return Allowed()?value:UINT32_MAX;
    }
    uint32_t HostPciRead(unsigned peer,uint16_t offset,uint8_t size) {
        if (!Allowed()) return UINT32_MAX;
        uint32_t value=io.HostPciRead(peer,offset,size);return Allowed()?value:UINT32_MAX;
    }
};

template<class IO,class Check>
bool PioneerQ35GuardedLeaseEnd(IO& io,const tt_compute_noc_result& noc,
    PioneerResidentLease& lease,Check& check)
{
    PioneerQ35GuardedLeaseIO<IO,Check> guarded{io,check};
    if (!guarded.Allowed()) { lease.recovery_required=true;return false; }
    const bool clean=PioneerResidentLeaseEnd(guarded,noc,lease);
    if (!guarded.Allowed()) {
        // The underlying helper may update its bookkeeping after an ignored
        // access. Never preserve a restoration claim after authority is lost.
        lease.ready=false;lease.recovery_required=true;
        lease.l1_restored=lease.niu_restored=lease.tlb_restored=false;
        return false;
    }
    return clean;
}
#endif
