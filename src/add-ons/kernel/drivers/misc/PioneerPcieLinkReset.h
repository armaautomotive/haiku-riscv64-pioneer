/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_PCIE_LINK_RESET_H
#define PIONEER_PCIE_LINK_RESET_H

#include <stdint.h>

// SG2042 secondary-bus reset sequence based on Tenstorrent tt-kmd pcie.c.
// This controls only the link pulse and bounded endpoint reappearance. The
// caller must own the whole downstream bus, drain DMA, exclude MMIO/firmware
// writers, hold the root config lease and restore PCI state before device use.
enum PioneerPcieLinkResetStage : uint32_t {
    PioneerPcieLinkResetIdle=0,
    PioneerPcieLinkResetPreflight=1,
    PioneerPcieLinkResetAsserted=2,
    PioneerPcieLinkResetDeasserted=3,
    PioneerPcieLinkResetWaiting=4,
    PioneerPcieLinkResetReturned=5,
    PioneerPcieLinkResetFailed=6
};
struct PioneerPcieLinkResetResult {
    uint32_t stage=PioneerPcieLinkResetIdle;
    uint32_t bridgeControl=0,endpoint=0,polls=0,failedAt=PioneerPcieLinkResetIdle;
    bool assertAttempted=false,deassertAttempted=false,assertConfirmed=false,deassertConfirmed=false;
};

template<class IO> class PioneerPcieLinkReset {
    bool attempted=false;
    PioneerPcieLinkResetResult result{};
    static constexpr uint32_t kBusReset=0x0040;
    // PCI bridge discard-timer status may be write-one-to-clear. It is never
    // replayed when SG2042 expands the 16-bit write to a 32-bit MMIO write.
    static constexpr uint32_t kDiscardStatus=0x0400;
public:
    const PioneerPcieLinkResetResult& Result() const {return result;}
    bool Run(IO& io,unsigned pollLimit) {
        if (attempted || !pollLimit || pollLimit>101) return false;
        attempted=true;result.stage=PioneerPcieLinkResetPreflight;
        auto fail=[&] {result.failedAt=result.stage;result.stage=PioneerPcieLinkResetFailed;return false;};
        if (!io.Allowed() || io.RootRead(0,4)!=0x20421e30
            || (io.RootRead(8,4)>>8)!=0x060400 || io.RootRead(0x18,4)!=0x003f0100
            || io.RootRead(4,2)!=0x402 || io.CardRead(0,4)!=0xb1401e52
            || io.CardRead(0x2c,4)!=0x00431e52 || io.CardRead(4,2)!=0x402
            || !io.Allowed()) return fail();
        const uint32_t control=io.RootRead(0x3e,2);
        result.bridgeControl=control;
        // Reject an existing reset or discard status instead of clearing it.
        if (control>0xffff || (control&(kBusReset|kDiscardStatus)) || !io.Allowed()) return fail();
        result.assertAttempted=true;
        io.RootWrite(0x3e,2,control|kBusReset);
        result.stage=PioneerPcieLinkResetAsserted;
        if (!io.Allowed()) return fail();
        const uint32_t asserted=io.RootRead(0x3e,2);
        result.assertConfirmed=asserted==(control|kBusReset);
        // Even when the assertion readback or delay fails, attempt one
        // deassertion while the same authority remains. If authority is lost,
        // a write cannot safely be directed at an uncertain controller.
        const bool held=io.DelayUs(2000);
        if (!io.Allowed()) return fail();
        result.deassertAttempted=true;
        io.RootWrite(0x3e,2,control&~kDiscardStatus);
        result.stage=PioneerPcieLinkResetDeasserted;
        if (!io.Allowed()) return fail();
        const uint32_t deasserted=io.RootRead(0x3e,2);
        result.deassertConfirmed=deasserted==control;
        const bool settled=io.DelayUs(500000);
        if (!result.assertConfirmed || !held || !result.deassertConfirmed || !settled || !io.Allowed())
            return fail();
        result.stage=PioneerPcieLinkResetWaiting;
        for (unsigned i=0;i<pollLimit;i++) {
            if (!io.Allowed() || io.RootRead(0,4)!=0x20421e30
                || (io.RootRead(8,4)>>8)!=0x060400 || io.RootRead(4,2)!=0x402
                || io.RootRead(0x18,4)!=0x003f0100 || !io.Allowed()) return fail();
            ++result.polls;
            // Avoid endpoint config reads while the root reports that link
            // training is incomplete. Root readiness does not guarantee that
            // a later endpoint request cannot cause a PCIe error.
            if (io.RootLinkReady()) {
                if (!io.Allowed()) return fail();
                result.endpoint=io.CardRead(0,4);
                if (result.endpoint==0xb1401e52) {
                    if (!io.Allowed() || io.CardRead(0x2c,4)!=0x00431e52 || !io.Allowed()) return fail();
                    result.stage=PioneerPcieLinkResetReturned;return true;
                }
            }
            if (i+1<pollLimit && (!io.DelayUs(100000) || !io.Allowed())) return fail();
        }
        return fail();
    }
};
#endif
