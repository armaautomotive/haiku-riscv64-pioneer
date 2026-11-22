/* SPDX-License-Identifier: MIT */
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQwen35DataflowFirmwareIdentity.h"
#include <cassert>
#include <cstdio>
int main()
{
    tt_arc_telemetry_result t{};
    t.version=1; t.state=TT_ARC_TELEMETRY_OK; t.tlb_restored=1;
    t.present_mask=3; t.values[0]=0x12050001; t.values[1]=0x000e0000;
    assert(PioneerQwen35ReportedFirmwareMatches(t));
    for (unsigned failure=0;failure<7;failure++) {
        auto bad=t;
        if (failure==0) bad.version=2;
        if (failure==1) bad.state=TT_ARC_TELEMETRY_CHANGED;
        if (failure==2) bad.tlb_restored=0;
        if (failure==3) bad.present_mask=1;
        if (failure==4) bad.present_mask=2;
        if (failure==5) bad.values[0]++;
        if (failure==6) bad.values[1]++;
        assert(!PioneerQwen35ReportedFirmwareMatches(bad));
    }
    std::puts("PASS reported firmware family precondition; SIMULATED ONLY");
}
