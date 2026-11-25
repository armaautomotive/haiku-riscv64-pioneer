/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_QWEN35_GUARDED_BASELINE_H
#define PIONEER_QWEN35_GUARDED_BASELINE_H
#include "PioneerQwen35GuardedHostAudits.h"
#include "BlackholeArcTelemetryProbe.h"
#include "BlackholeTensixResetProbe.h"

// The same sticky check covers all real baseline probes, including their
// temporary TLB changes and cleanup. Lost authority suppresses later IO;
// the caller retains the mapping and snapshots rather than retrying cleanup.
template<class IO,class Check>
struct PioneerQ35GuardedBaselineIO : PioneerQ35GuardedHostAuditIO<IO,Check> {
    using Base=PioneerQ35GuardedHostAuditIO<IO,Check>;
    PioneerQ35GuardedBaselineIO(IO& io,Check& check):Base{io,check} {}
    uint32_t TlbRead(unsigned i) {
        if (!this->Allowed()) return UINT32_MAX;
        uint32_t v=this->io.TlbRead(i);return this->Allowed()?v:UINT32_MAX;
    }
    void TlbWrite(unsigned i,uint32_t v) {
        if (this->Allowed()) { this->io.TlbWrite(i,v);(void)this->Allowed(); }
    }
    uint32_t ScratchRead(unsigned i) {
        if (!this->Allowed()) return UINT32_MAX;
        uint32_t v=this->io.ScratchRead(i);return this->Allowed()?v:UINT32_MAX;
    }
    uint32_t CsmRead(uint32_t address) {
        if (!this->Allowed()) return UINT32_MAX;
        uint32_t v=this->io.CsmRead(address);return this->Allowed()?v:UINT32_MAX;
    }
    uint32_t NiuRead() {
        if (!this->Allowed()) return UINT32_MAX;
        uint32_t v=this->io.NiuRead();return this->Allowed()?v:UINT32_MAX;
    }
    uint32_t DramIdRead(unsigned i) {
        if (!this->Allowed()) return UINT32_MAX;
        uint32_t v=this->io.DramIdRead(i);return this->Allowed()?v:UINT32_MAX;
    }
    uint32_t DramRead(unsigned i) {
        if (!this->Allowed()) return UINT32_MAX;
        uint32_t v=this->io.DramRead(i);return this->Allowed()?v:UINT32_MAX;
    }
    void DramWrite(unsigned i,uint32_t v) {
        if (this->Allowed()) { this->io.DramWrite(i,v);(void)this->Allowed(); }
    }
};

template<class IO,class Check>
bool PioneerQ35GuardedBaseline(IO& io,uint32_t node,bool operating,bool writeSamples,unsigned targetY,
    tt_arc_scratch_result& arc,tt_arc_telemetry_result& telemetry,tt_tensix_reset_result& tensix,
    tt_dram_samples_result& dram,tt_compute_noc_result& noc,tt_host_path_result& path,
    tt_host_dma_audit_result& audit,bool& telemetryAttempted,bool& resetAttempted,
    bool& dramAttempted,bool& nocAttempted,bool& pathAttempted,bool& auditAttempted,Check& check)
{
    PioneerQ35GuardedBaselineIO<IO,Check> guarded(io,check);
    bool arcAttempted=false,okay=false;
    do {
        // This recovery path may not turn the read-only DRAM audit into a
        // sample-writing probe or select a different firmware baseline.
        if ((node&0xfff)!=2 || operating || writeSamples || targetY!=2 || !guarded.Allowed()) break;
        arcAttempted=true;
        if (!BlackholeArcScratchProbe(guarded,arc) || !guarded.Allowed()) break;
        telemetryAttempted=true;
        if (!BlackholeArcTelemetryProbe(guarded,arc,telemetry,false) || !guarded.Allowed()) break;
        resetAttempted=true;
        if (!BlackholeTensixResetProbe(guarded,telemetry,tensix) || !guarded.Allowed()) break;
        dramAttempted=true;
        if (!BlackholeDramSamplesProbe(guarded,telemetry,tensix,false,dram) || !guarded.Allowed()) break;
        nocAttempted=true;
        if (!BlackholeComputeNocProbe(guarded,dram,noc,targetY) || !guarded.Allowed()) break;
        pathAttempted=true;
        if (!BlackholeHostPathProbe(guarded,dram,noc,node,path) || !guarded.Allowed()) break;
        auditAttempted=true;
        if (!BlackholeHostDmaAudit(guarded,dram,noc,path,audit) || !guarded.Allowed()) break;
        okay=true;
    } while (false);
    if (guarded.lost) {
        if (arcAttempted) arc.tlb_restored=0;
        if (telemetryAttempted) telemetry.tlb_restored=0;
        if (resetAttempted) tensix.tlb_restored=0;
        if (dramAttempted) { dram.tlb_restored=0;dram.recovery_required=1; }
        if (nocAttempted) { noc.tlb_restored=0;noc.recovery_required=1; }
        if (pathAttempted) { path.tlb_restored=0;path.recovery_required=1; }
        if (auditAttempted) { audit.tlb_restored=0;audit.recovery_required=1; }
    }
    return okay && !guarded.lost;
}
#endif
