/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_QWEN35_DATAFLOW_L2CPU_RESET_PROBE_H
#define PIONEER_QWEN35_DATAFLOW_L2CPU_RESET_PROBE_H

#include "BlackholeArcScratchProbe.h"

inline bool PioneerQwen35L2CpuRiscsHeld(uint32_t reset)
{
    return reset!=UINT32_MAX && (reset&0xf0U)==0;
}

// The existing read-only ARC view is at BAR0 + 0x19230000. With TLB201
// configured to the ARC's 0x80000000 page, offset 0x14 is L2CPU_RESET. Bits
// 4..7 are active-low RISC resets. This transaction only observes the
// register; it does not release a processor, drain NoC traffic, identify the
// installed firmware, or establish exclusive ownership of any L1 region.
// Scope owns the driver lock and admits this only after the service has
// completed its worker-hold sequence. Guard verifies the selected route before
// and after temporarily redirecting TLB201. A failed restoration is reported
// separately so the caller can retain the session for recovery.
template<class IO,class Scope,class Guard>
bool PioneerQwen35SampleL2CpuReset(IO& io,Scope& scope,Guard& guard,
    uint32_t& reset,bool& restoreFailed)
{
    restoreFailed=false;
    const unsigned core=io.runtimeCore;
    if (!scope.ProveRoleStatusInspectionAllowed(io) || !io.SharedArcResetMapped()
        || !guard.ProveSelectedRoute(core)) return false;
    uint32_t saved[3]{};
    for (unsigned word=0;word<3;word++) saved[word]=io.DramTlbRead(1,word);
    if (!scope.ProveRoleStatusInspectionAllowed(io) || !guard.ProveSelectedRoute(core)) return false;

    // Consume the attempt before the first write. Even a failed MMIO write
    // can have changed the route, so every exit below must restore it.
    for (unsigned word=0;word<3;word++) {
        if (!scope.ProveRoleStatusInspectionAllowed(io) || io.runtimeCore!=core) {
            restoreFailed=word!=0; return false;
        }
        io.DramTlbWrite(1,word,BlackholeArcScratchTlbWord(word));
    }
    auto arcRouteMatches=[&]() {
        if (!scope.ProveRoleStatusInspectionAllowed(io) || io.runtimeCore!=core) return false;
        for (unsigned word=0;word<3;word++)
            if (io.DramTlbRead(1,word)!=BlackholeArcScratchTlbWord(word)) return false;
        return true;
    };
    bool sampled=false;
    uint32_t first=0,second=0;
    if (arcRouteMatches()) {
        const uint32_t boot=io.ScratchRead(2);
        if (arcRouteMatches()) {
            first=io.SharedArcL2CpuResetRead();
            io.Pause();
            if (arcRouteMatches()) {
                second=io.SharedArcL2CpuResetRead();
                if (arcRouteMatches()) {
                    const uint32_t bootAgain=io.ScratchRead(2);
                    sampled=boot==bootAgain && (boot&7U)==5U && first==second
                        && first!=UINT32_MAX && arcRouteMatches();
                }
            }
        }
    }

    // Do not write through a scope whose owner or generation changed.
    if (!scope.ProveRoleStatusInspectionAllowed(io) || io.runtimeCore!=core) {
        restoreFailed=true; return false;
    }
    for (unsigned word=0;word<3;word++) {
        if (!scope.ProveRoleStatusInspectionAllowed(io) || io.runtimeCore!=core) {
            restoreFailed=true; return false;
        }
        io.DramTlbWrite(1,word,saved[word]);
    }
    for (unsigned word=0;word<3;word++)
        if (io.DramTlbRead(1,word)!=saved[word]) restoreFailed=true;
    if (!scope.ProveRoleStatusInspectionAllowed(io) || !guard.ProveSelectedRoute(core))
        restoreFailed=true;
    if (restoreFailed || !sampled) return false;
    reset=second; return true;
}

#endif
