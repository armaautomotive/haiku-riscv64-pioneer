/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_QWEN35_GUARDED_HOST_AUDITS_H
#define PIONEER_QWEN35_GUARDED_HOST_AUDITS_H
#include "BlackholeHostDmaAudit.h"

// The scope check is metadata-only. Once authority is lost, even cleanup
// accesses are suppressed; the caller must retain the mapping and snapshots.
template<class IO,class Check> struct PioneerQ35GuardedHostAuditIO {
    IO& io; Check& check; bool lost=false;
    bool Allowed() { if (lost || !check()) { lost=true; return false; } return true; }
    uint32_t DramTlbRead(unsigned s,unsigned w) {
        if (!Allowed()) return UINT32_MAX;
        uint32_t v=io.DramTlbRead(s,w); return Allowed()?v:UINT32_MAX;
    }
    void DramTlbWrite(unsigned s,unsigned w,uint32_t v) {
        if (Allowed()) { io.DramTlbWrite(s,w,v); (void)Allowed(); }
    }
    uint32_t ResetRead() { if (!Allowed()) return UINT32_MAX; uint32_t v=io.ResetRead(); return Allowed()?v:UINT32_MAX; }
    uint32_t SourceRead(unsigned w) { if (!Allowed()) return UINT32_MAX; uint32_t v=io.SourceRead(w); return Allowed()?v:UINT32_MAX; }
    uint32_t ComputeNocRead(uint32_t o) { if (!Allowed()) return UINT32_MAX; uint32_t v=io.ComputeNocRead(o); return Allowed()?v:UINT32_MAX; }
    uint32_t HostInboundRead(unsigned w) { if (!Allowed()) return UINT32_MAX; uint32_t v=io.HostInboundRead(w); return Allowed()?v:UINT32_MAX; }
    uint32_t HostSiiRead(unsigned w) { if (!Allowed()) return UINT32_MAX; uint32_t v=io.HostSiiRead(w); return Allowed()?v:UINT32_MAX; }
    uint32_t HostHdmaRead(unsigned w) { if (!Allowed()) return UINT32_MAX; uint32_t v=io.HostHdmaRead(w); return Allowed()?v:UINT32_MAX; }
    uint32_t HostPciRead(unsigned p,uint16_t o,uint8_t s) {
        if (!Allowed()) return UINT32_MAX;
        uint32_t v=io.HostPciRead(p,o,s); return Allowed()?v:UINT32_MAX;
    }
};

template<class IO,class Check>
bool PioneerQ35GuardedHostAudits(IO& io,tt_dram_samples_result& dram,
    const tt_compute_noc_result& noc,uint32_t node,tt_host_path_result& path,
    tt_host_dma_audit_result& audit,bool& pathAttempted,bool& auditAttempted,Check& check)
{
    PioneerQ35GuardedHostAuditIO<IO,Check> guarded{io,check};
    bool okay=false;
    if (guarded.Allowed()) {
        pathAttempted=true;
        if (BlackholeHostPathProbe(guarded,dram,noc,node,path) && guarded.Allowed()) {
            auditAttempted=true;
            okay=BlackholeHostDmaAudit(guarded,dram,noc,path,audit) && guarded.Allowed();
        }
    }
    if (guarded.lost) {
        // Neither result may claim successful restoration after authority loss.
        if (pathAttempted) { path.tlb_restored=0; path.recovery_required=1; }
        if (auditAttempted) { audit.tlb_restored=0; audit.recovery_required=1; }
    }
    return okay && !guarded.lost;
}
#endif
