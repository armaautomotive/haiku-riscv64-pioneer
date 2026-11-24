/* SPDX-License-Identifier: MIT */
#define B_DEVICE_OP_CODES_END 1000
#include <cassert>
#include <cstdint>
#include <cstdio>
#include "../../../src/add-ons/kernel/drivers/misc/PioneerDriverReloadProtocol.h"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerRecoveryProtocol.h"
#include "../../../src/add-ons/kernel/drivers/misc/BlackholeAsicReset.h"
constexpr int B_OK=0,B_DEV_NOT_READY=-5,TT_QWEN35_DATAFLOW_DRIVER=60;
static unsigned mode=0,statusCalls=0,resetCalls=0,delays=0;
static bool forceMode=false,qualificationMode=false;
static unsigned firmwareQualification=0;
static uint64_t ExpectedTag() {
    return qualificationMode?(firmwareQualification==2?UINT64_C(0x2026112400030022)
        :firmwareQualification?UINT64_C(0x2026112300030002):UINT64_C(0x2026112300030001)):0x123;
}
static bool Call(int fd,uint32_t operation,void* output,size_t bytes) {
    assert(fd==10);
    if (operation==TT_PCI_PROBE_RELOAD_STATUS) {
        assert(bytes==sizeof(tt_pioneer_driver_reload));
        auto& r=*static_cast<tt_pioneer_driver_reload*>(output);
        assert(r.version==1 && !r.expected_tag && !r.expected_generation);
        const bool first=statusCalls++==0;
        r.driver=TT_QWEN35_DATAFLOW_DRIVER;
		r.build_tag=ExpectedTag();
		if (mode==19 && first) r.build_tag+=2;
        r.generation=first?3:4;
        r.cold=first && !forceMode;r.active_owner=first;r.open_count=1;
        if (mode==4 && !first) { r.cold=1;r.active_owner=1;r.generation=3; }
        if (mode==5 && first) r.driver=1;
        if (mode==9 && !first) r.build_tag++;
        return true;
    }
    assert(operation==(qualificationMode && !resetCalls?TT_PCI_PROBE_RECOVERY_QUALIFY
        :forceMode && !resetCalls?TT_PCI_PROBE_RECOVERY_FORCE:TT_PCI_PROBE_RECOVERY_RUN)
        && bytes==sizeof(tt_pioneer_recovery_run));
    auto& r=*static_cast<tt_pioneer_recovery_run*>(output);
    assert(r.version==1 && r.expected_tag==ExpectedTag()
        && r.expected_generation==3);
    const unsigned attempt=resetCalls++;
    if (mode==3 || mode==4) return false; // response was lost
    if ((mode==2 && !attempt) || mode==8) {
        r.operation_status=B_DEV_NOT_READY;r.stage=PIONEER_RECOVERY_STAGE_ASIC_RESET;
        r.detail=BLACKHOLE_ASIC_RESET_TIMEOUT;return true;
    }
    if (mode==7 && !attempt) {
        r.operation_status=B_DEV_NOT_READY;r.stage=PIONEER_RECOVERY_STAGE_OWNER_RELEASE;return true;
    }
    if (mode==10) { r.operation_status=B_DEV_NOT_READY;r.stage=PIONEER_RECOVERY_STAGE_FIRMWARE;return true; }
    if ((mode==11 && !attempt) || mode==12) {
        r.operation_status=B_DEV_NOT_READY;r.stage=PIONEER_RECOVERY_STAGE_FIRMWARE;
        r.detail=PIONEER_RECOVERY_DETAIL_FIRMWARE_WAIT;return true;
    }
    if ((mode==13 && !attempt) || mode==14 || mode==15) {
        r.operation_status=B_DEV_NOT_READY;r.stage=PIONEER_RECOVERY_STAGE_MAPPING;
        if (mode!=15) r.detail=PIONEER_RECOVERY_DETAIL_MAPPING_WAIT;
        return true;
    }
    if ((mode==16 && !attempt) || mode==17 || mode==18) {
        r.operation_status=B_DEV_NOT_READY;r.stage=PIONEER_RECOVERY_STAGE_DISABLE_MASTER;
        if (mode!=18) r.detail=PIONEER_RECOVERY_DETAIL_PREPARE_WAIT;
        return true;
    }
    r.operation_status=B_OK;r.stage=PIONEER_RECOVERY_STAGE_COMPLETE;
    r.build_tag=ExpectedTag();
    r.new_generation=mode==6?5:4;return true;
}
static int snooze(int delay) { assert(delay==1000000);++delays;return B_OK; }
#include "pioneer_live_reset_client.inc"
int main() {
    for (unsigned forced=0;forced<2;forced++) for (mode=1;mode<=18;mode++) {
        forceMode=forced;
        statusCalls=resetCalls=delays=0;
        const bool success=ResetStoppedRewrite(10,forceMode);
        assert(success==(mode==1 || mode==2 || mode==3 || mode==7 || mode==11 || mode==13 || mode==16));
        if (mode==2 || mode==7 || mode==11 || mode==13 || mode==16) assert(resetCalls==2 && delays==1);
        if (mode==3 || mode==4) assert(resetCalls==1 && !delays);
        if (mode==5) assert(!resetCalls);
        if (mode==8) assert(resetCalls==4 && delays==3);
        if (mode==10) assert(resetCalls==1 && !delays);
        if (mode==12) assert(resetCalls==4 && delays==3);
        if (mode==14) assert(resetCalls==4 && delays==3);
        if (mode==15) assert(resetCalls==1 && !delays);
        if (mode==17) assert(resetCalls==4 && delays==3);
        if (mode==18) assert(resetCalls==1 && !delays);
    }
    forceMode=qualificationMode=true;
    for (unsigned fw=0;fw<3;fw++) for (mode=16;mode<=19;mode++) {
		firmwareQualification=fw;
        statusCalls=resetCalls=delays=0;
        const bool success=ResetStoppedRewrite(10,true,true);
        assert(success==(mode==16));
        if (mode==16) assert(resetCalls==2 && delays==1);
        if (mode==17) assert(resetCalls==4 && delays==3);
        if (mode==18) assert(resetCalls==1 && !delays);
        if (mode==19) assert(!resetCalls);
    }
    std::puts("PASS live reset client: retained ticket retry, lost reply, generation and idle checks; SIMULATED ONLY");
}
