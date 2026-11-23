/* SPDX-License-Identifier: MIT */
#define B_DEVICE_OP_CODES_END 1000
#include <cassert>
#include <cstdint>
#include <cstdio>
#include "../../../src/add-ons/kernel/drivers/misc/PioneerDriverReloadProtocol.h"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38BankProtocol.h"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQwen35FleetInspectionProtocol.h"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQwen35RoleStageProtocol.h"

constexpr int B_OK=0;
constexpr unsigned TT_QWEN35_DATAFLOW_DRIVER=60;
static unsigned mode=0,statusCalls=0,beginCalls=0,inspectCalls=0,stageCalls=0,endCalls=0,closeCalls=0;
static int OpenExclusiveDataflowDriver() { return mode==12?-1:10; }
static int close(int fd) { assert(fd==10);++closeCalls;return mode==11?-1:0; }
static bool Call(int fd,uint32_t op,void* data,size_t) {
    assert(fd==10);
    if (op==TT_PCI_PROBE_RELOAD_STATUS) {
        auto& r=*static_cast<tt_pioneer_driver_reload*>(data);
        const bool first=statusCalls++==0;
        r.driver=60;r.build_tag=mode==2?UINT64_C(0x111)
            :mode>=16?UINT64_C(0x2026112300030010)
            :mode>=13?UINT64_C(0x2026112300030005):UINT64_C(0x2026112300030004);
        r.generation=(!first && mode==10)?2:1;r.open_count=1;r.guard_version=1;
        if (!first && mode!=7) r.active_owner=1;
        if (!first && (mode==4 || mode==6 || mode==14 || mode==17)) r.cold=1;
        return true;
    }
    if (op==TT_PCI_PROBE_Q38_BANK_BEGIN) {
        ++beginCalls;if (mode==3 || mode==4 || mode==7) return false;
        auto& r=*static_cast<tt_q38_bank_session*>(data);
        r.result.state=TT_RESIDENT_SESSION_READY;r.result.active=1;r.result.host_ready=1;return true;
    }
    if (op==TT_PCI_PROBE_Q35_FLEET_INSPECT) {
        ++inspectCalls;if (mode==9) return false;
        auto& r=*static_cast<tt_q35_fleet_inspection*>(data);
        assert(r.version==1 && r.expected_tag==(mode>=16?UINT64_C(0x2026112300030010)
            :mode>=13?UINT64_C(0x2026112300030005):UINT64_C(0x2026112300030004)) && r.expected_generation==1);
        r.build_tag=r.expected_tag;r.generation=1;r.prepared=120;r.failed_core=120;
        r.l2cpu_reset=15;r.stage=TT_Q35_FLEET_INSPECTION_COMPLETE;
        if (mode==5 || mode==6) r.operation_status=-4;
        if (mode==6) r.restore_failed=1;
        return true;
    }
    if (op==TT_PCI_PROBE_Q35_ROLE_STAGE_QUALIFY) {
        ++stageCalls;
        auto& r=*static_cast<tt_q35_role_stage_qualification*>(data);
        assert(r.version==(mode>=16?2U:1U) && r.expected_tag==(mode>=16?UINT64_C(0x2026112300030010)
            :UINT64_C(0x2026112300030005)) && r.expected_generation==1);
        r.build_tag=r.expected_tag;r.generation=1;
        r.stage=mode>=16?TT_Q35_ROLE_PRESTACK_COMPLETE:TT_Q35_ROLE_STAGE_COMPLETE;
        r.captured=r.staged=r.restored=r.mappings_closed=1;
        if (mode==14 || mode==17) r.operation_status=-4;
        if (mode==15 || mode==18) r.restored=0;
        return true;
    }
    assert(op==TT_PCI_PROBE_Q38_BANK_END);++endCalls;
    if (mode==8) return false;
    auto& r=*static_cast<tt_q38_bank_session*>(data);
    r.result.state=TT_RESIDENT_SESSION_CLOSED;return true;
}
#include "pioneer_fleet_inspection_client.inc"
int main() {
    for (mode=1;mode<=12;mode++) {
        statusCalls=beginCalls=inspectCalls=stageCalls=endCalls=closeCalls=0;
        assert(QualifyHeldFleet()==(mode==1));
        if (mode==2 || mode==12) assert(!beginCalls && !inspectCalls && !endCalls);
        if (mode==3) assert(beginCalls==1 && !inspectCalls && endCalls==1);
        if (mode==4 || mode==7) assert(beginCalls==1 && !inspectCalls && !endCalls);
        if (mode==5 || mode==8 || mode==9 || mode==11) assert(inspectCalls==1 && endCalls==1);
        if (mode==6 || mode==10) assert(inspectCalls==1 && !endCalls);
        assert(closeCalls==(mode==12?0U:1U));
    }
    for (mode=13;mode<=15;mode++) {
        statusCalls=beginCalls=inspectCalls=stageCalls=endCalls=closeCalls=0;
        assert(QualifyHeldFleet(true)==(mode==13));
        assert(beginCalls==1 && inspectCalls==1 && stageCalls==1 && closeCalls==1);
        assert(endCalls==(mode==14?0U:1U));
    }
    for (mode=16;mode<=18;mode++) {
        statusCalls=beginCalls=inspectCalls=stageCalls=endCalls=closeCalls=0;
        assert(QualifyHeldFleet(true,true)==(mode==16));
        assert(beginCalls==1 && inspectCalls==1 && stageCalls==1 && closeCalls==1);
        assert(endCalls==(mode==17?0U:1U));
    }
    std::puts("PASS fleet inspection client checks tag and ends only known non-cold owners; SIMULATED ONLY");
}
