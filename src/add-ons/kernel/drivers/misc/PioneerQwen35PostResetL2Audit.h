/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_QWEN35_POST_RESET_L2_AUDIT_H
#define PIONEER_QWEN35_POST_RESET_L2_AUDIT_H
#include "BlackholeArcScratchProbe.h"

struct PioneerQ35PostResetL2Result {
    uint32_t reset=UINT32_MAX;
    bool sampled=false,restoreFailed=false;
};

// Sample the four active-low L2CPU reset bits using the already-owned fresh
// post-reset mapping. The caller must serialize TLB access, retain the
// mapping on uncertainty and independently control later device activity.
// This is an observation, not proof that firmware cannot release a core.
template<class IO>
bool PioneerQ35PostResetL2Audit(IO& io,PioneerQ35PostResetL2Result& result)
{
    result={};
    if (!io.L2AuditAllowed()) return false;
    uint32_t saved[3]{};
    for (unsigned i=0;i<3;i++) saved[i]=io.TlbRead(i);
    if ((saved[2]&0xffffc000U) || !io.L2AuditAllowed()) return false;
    bool touched=false;
    for (unsigned i=0;i<3;i++) {
        if (!io.L2AuditAllowed()) { result.restoreFailed=touched;return false; }
        touched=true;io.TlbWrite(i,BlackholeArcScratchTlbWord(i));
    }
    auto route=[&]() {
        if (!io.L2AuditAllowed()) return false;
        for (unsigned i=0;i<3;i++) if (io.TlbRead(i)!=BlackholeArcScratchTlbWord(i)) return false;
        return true;
    };
    uint32_t first=UINT32_MAX,second=UINT32_MAX;
    if (route()) {
        const uint32_t boot=io.L2BootRead();
        if (route()) {
            first=io.L2ResetRead();io.L2Pause();
            if (route()) {
                second=io.L2ResetRead();
                if (route()) {
                    const uint32_t bootAgain=io.L2BootRead();
                    result.sampled=boot!=UINT32_MAX && boot==bootAgain && (boot&7U)==5U
                        && first!=UINT32_MAX && first==second && route();
                }
            }
        }
    }
    // Do not issue a compensating TLB write after the mapping/ownership
    // evidence disappears. The caller must retain the live mapping instead.
    if (!io.L2AuditAllowed()) { result.restoreFailed=true;return false; }
    for (unsigned i=0;i<3;i++) {
        if (!io.L2AuditAllowed()) { result.restoreFailed=true;return false; }
        io.TlbWrite(i,saved[i]);
    }
    for (unsigned i=0;i<3;i++) if (io.TlbRead(i)!=saved[i]) result.restoreFailed=true;
    if (!io.L2AuditAllowed()) result.restoreFailed=true;
    if (result.restoreFailed || !result.sampled) return false;
    result.reset=second;
    return (second&0xf0U)==0;
}
#endif
