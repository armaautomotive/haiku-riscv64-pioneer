/* SPDX-License-Identifier: MIT */
#define B_DEVICE_OP_CODES_END 1000
#include <cassert>
#include <cerrno>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include "../../../src/add-ons/kernel/drivers/misc/PioneerDriverReloadProtocol.h"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38BankProtocol.h"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQwen35ProjectionQualificationProtocol.h"

constexpr int B_OK=0;
constexpr unsigned TT_QWEN35_DATAFLOW_DRIVER=60;
constexpr uint64_t Tag=UINT64_C(0x2026112400000001),Generation=42;
static unsigned mode,statusCalls,beginCalls,projectionCalls,endCalls,closeCalls;
static int OpenExclusiveDataflowDriver() { return mode==16?-1:10; }
static int close(int fd) { assert(fd==10);++closeCalls;return mode==15?-1:0; }
static bool Call(int fd,uint32_t op,void* data,size_t) {
    assert(fd==10);
    if (op==TT_PCI_PROBE_RELOAD_STATUS) {
        auto& r=*static_cast<tt_pioneer_driver_reload*>(data);
        bool first=statusCalls++==0;
        r.driver=60;r.build_tag=mode==2?Tag+1:Tag;r.generation=mode==3?Generation+1:Generation;
        r.open_count=1;r.guard_version=1;r.active_owner=!first;
        if (!first && mode==12) r.cold=1;
        if (!first && mode==13) r.generation++;
        return true;
    }
    if (op==TT_PCI_PROBE_Q38_BANK_BEGIN) {
        ++beginCalls;if (mode==4) return false;
        auto& r=*static_cast<tt_q38_bank_session*>(data);
        r.result.state=TT_RESIDENT_SESSION_READY;r.result.active=1;r.result.host_ready=1;
        if (mode==5) r.result.uploaded_pages=1;
        return true;
    }
    if (op==TT_PCI_PROBE_Q35_PROJECTION_QUALIFY) {
        ++projectionCalls;if (mode==6) return false;
        auto& r=*static_cast<tt_q35_projection_qualification*>(data);
        assert(PioneerQ35ProjectionQualificationRequest(r));
        assert(r.expected_tag==Tag && r.expected_generation==Generation);
        r.build_tag=Tag;r.generation=Generation;r.stage=15;r.output_verified=1;r.cleanup_complete=1;
        if (mode==7) { r.operation_status=-1;r.output_verified=0; }
        if (mode==8) r.cleanup_complete=0;
        if (mode==9) r.generation++;
        if (mode==10) r.stage=14;
        if (mode==11) r.output_verified=2;
        if (mode==17) r.reserved=1;
        return true;
    }
    assert(op==TT_PCI_PROBE_Q38_BANK_END);++endCalls;
    if (mode==14) return false;
    auto& r=*static_cast<tt_q38_bank_session*>(data);
    r.result.state=TT_RESIDENT_SESSION_CLOSED;r.result.pci_restored=1;r.result.host_released=1;
    return true;
}
#include "pioneer_projection_client.inc"
int main() {
    for (mode=1;mode<=17;mode++) {
        statusCalls=beginCalls=projectionCalls=endCalls=closeCalls=0;
        assert(QualifyFixedProjection(Tag,Generation)==(mode==1));
        if (mode==2 || mode==3 || mode==16) assert(!beginCalls && !projectionCalls && !endCalls);
        if (mode==4 || mode==5) assert(beginCalls==1 && !projectionCalls && endCalls==1);
        if (mode>=6 && mode<=11) assert(projectionCalls==1 && endCalls==(mode==7?1U:0U));
        if (mode==12 || mode==13 || mode==17) assert(projectionCalls==1 && !endCalls);
        if (mode==14 || mode==15) assert(projectionCalls==1 && endCalls==1);
        assert(closeCalls==(mode==16?0U:1U));
    }
    statusCalls=beginCalls=projectionCalls=endCalls=closeCalls=0;
    assert(!QualifyFixedProjection(0,Generation) && !QualifyFixedProjection(Tag,0));
    assert(!statusCalls && !beginCalls && !projectionCalls && !endCalls && !closeCalls);
    uint64_t tag=0,generation=0;
    assert(ParseProjectionIdentity("2026112400000001","42",tag,generation)
        && tag==Tag && generation==Generation);
    assert(!ParseProjectionIdentity("0x26112400000001","42",tag,generation));
    assert(!ParseProjectionIdentity("2026112400000001","-42",tag,generation));
    assert(!ParseProjectionIdentity("2026112400000001","18446744073709551616",tag,generation));
    assert(!ParseProjectionIdentity("0000000000000000","42",tag,generation));
    assert(!ParseProjectionIdentity("2026112400000001","0",tag,generation));
    std::puts("PASS fixed projection client identity, output, cleanup and failure retention; SIMULATED ONLY");
}
