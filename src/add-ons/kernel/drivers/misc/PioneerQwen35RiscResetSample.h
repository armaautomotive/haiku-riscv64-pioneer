/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_QWEN35_RISC_RESET_SAMPLE_H
#define PIONEER_QWEN35_RISC_RESET_SAMPLE_H
#include "BlackholeArcScratchProbe.h"

// Fixed read-only observation of ARC reset-unit words 0x40..0x5c. The
// caller must own the session, serialize its TLB and retain it if restoration
// cannot be confirmed. Stable samples are evidence, not reset authority.
template<class IO,class Scope,class Guard>
bool PioneerQ35SampleRiscResetWords(IO& io,Scope& scope,Guard& guard,
    uint32_t (&result)[8],bool& restoreFailed)
{
    restoreFailed=false;
    const unsigned core=io.runtimeCore;
    if (!scope.ProveRoleStatusInspectionAllowed(io) || !io.SharedArcResetMapped()
        || !guard.ProveSelectedRoute(core)) return false;
    uint32_t saved[3]{},values[8]{};
    for (unsigned word=0;word<3;word++) {
        saved[word]=io.DramTlbRead(1,word);
        if (io.DramTlbRead(1,word)!=saved[word]) return false;
    }
    if (!scope.ProveRoleStatusInspectionAllowed(io) || !guard.ProveSelectedRoute(core)) return false;
    for (unsigned word=0;word<3;word++) {
        if (!scope.ProveRoleStatusInspectionAllowed(io) || io.runtimeCore!=core) {
            restoreFailed=word!=0;return false;
        }
        io.DramTlbWrite(1,word,BlackholeArcScratchTlbWord(word));
        if (io.DramTlbRead(1,word)!=BlackholeArcScratchTlbWord(word)) {
            restoreFailed=true;return false;
        }
    }
    auto route=[&]() {
        if (!scope.ProveRoleStatusInspectionAllowed(io) || io.runtimeCore!=core) return false;
        for (unsigned word=0;word<3;word++)
            if (io.DramTlbRead(1,word)!=BlackholeArcScratchTlbWord(word)) return false;
        return true;
    };
    if (!route()) { restoreFailed=true;return false; }
    const uint32_t boot=io.ScratchRead(2);
    bool stable=(boot&7U)==5U;
    for (unsigned pass=0;pass<2;pass++) for (unsigned word=0;word<8;word++) {
        if (!route()) { restoreFailed=true;return false; }
        const uint32_t value=io.SharedArcRiscResetRead(word);
        if (!pass) values[word]=value;else if (value!=values[word]) stable=false;
    }
    if (!route()) { restoreFailed=true;return false; }
    if (io.ScratchRead(2)!=boot) stable=false;
    if (!route()) { restoreFailed=true;return false; }
    for (unsigned word=0;word<3;word++) {
        if (!scope.ProveRoleStatusInspectionAllowed(io) || io.runtimeCore!=core) {
            restoreFailed=true;return false;
        }
        io.DramTlbWrite(1,word,saved[word]);
        if (io.DramTlbRead(1,word)!=saved[word]) { restoreFailed=true;return false; }
    }
    if (!scope.ProveRoleStatusInspectionAllowed(io) || !guard.ProveSelectedRoute(core)) {
        restoreFailed=true;return false;
    }
    if (!stable) return false;
    for (unsigned word=0;word<8;word++) result[word]=values[word];
    return true;
}
#endif
