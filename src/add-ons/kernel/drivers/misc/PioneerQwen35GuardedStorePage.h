/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_QWEN35_GUARDED_STORE_PAGE_H
#define PIONEER_QWEN35_GUARDED_STORE_PAGE_H
#include "PioneerQwen35GuardedLeaseEnd.h"

// Guard a recovery transfer, including the operations that enable BME and
// ring a NoC doorbell. A revoked scope never resumes even if it later admits.
// Host-page quarantine is CPU bookkeeping and remains available after loss.
template<class IO,class Check> struct PioneerQ35GuardedStoreIO : PioneerQ35GuardedLeaseIO<IO,Check> {
    using Base=PioneerQ35GuardedLeaseIO<IO,Check>;
    PioneerQ35GuardedStoreIO(IO& io,Check& check):Base{io,check} {}
    uint32_t SourceRead(unsigned word) {
        if (!this->Allowed()) return UINT32_MAX;
        uint32_t value=this->io.SourceRead(word);return this->Allowed()?value:UINT32_MAX;
    }
    int32_t HostPageMap(uint64_t& address,uint64_t& bytes) {
        if (!this->Allowed()) return -1;
        int32_t status=this->io.HostPageMap(address,bytes);return this->Allowed()?status:-1;
    }
    uint32_t HostPageRead(unsigned word) {
        if (!this->Allowed()) return UINT32_MAX;
        uint32_t value=this->io.HostPageRead(word);return this->Allowed()?value:UINT32_MAX;
    }
    void HostPageWrite(unsigned word,uint32_t value) {
        if (!this->Allowed()) return;
        this->io.HostPageWrite(word,value);(void)this->Allowed();
    }
    void HostPciWrite(unsigned peer,uint16_t offset,uint8_t width,uint32_t value) {
        if (!this->Allowed()) return;
        this->io.HostPciWrite(peer,offset,width,value);(void)this->Allowed();
    }
    void Pause() {
        if (!this->Allowed()) return;
        this->io.Pause();(void)this->Allowed();
    }
    void QuarantineHostPage() { this->io.QuarantineHostPage(); }
};

template<class IO,class Check>
bool PioneerQ35GuardedStorePage(IO& io,Check& check,const tt_dram_samples_result& dram,
    const tt_compute_noc_result& noc,const tt_host_page_result& host,PioneerResidentLease& lease,
    uint64_t offset,bool write,uint32_t* data,tt_resident_store_result& result,
    const PioneerResidentBanks& banks,unsigned bank,const void* owner,PioneerResidentStoreTrace& trace)
{
    PioneerQ35GuardedStoreIO<IO,Check> guarded(io,check);
    if (!guarded.Allowed()) {
        result={};result.version=1;trace={};
        lease.recovery_required=true;result.recovery_required=1;io.QuarantineHostPage();return false;
    }
    const bool passed=PioneerResidentStorePage(guarded,dram,noc,host,lease,PioneerResidentBanks::Capacity,
        offset,write,data,result,&banks,bank,owner,&trace);
    if (!guarded.Allowed()) {
        lease.recovery_required=true;result.recovery_required=1;
        io.QuarantineHostPage();return false;
    }
    return passed;
}
#endif
