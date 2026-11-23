/* SPDX-License-Identifier: MIT */
#define B_DEVICE_OP_CODES_END 1000
#include <cassert>
#include <cstdint>
#include <cstdio>
#include "../../../src/add-ons/kernel/drivers/misc/PioneerDriverReloadProtocol.h"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerRecoveryProtocol.h"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38BankProtocol.h"

constexpr unsigned TT_QWEN35_DATAFLOW_DRIVER=60;
static unsigned mode=0,statusCalls=0,beginCalls=0,endCalls=0,resetCalls=0,closeCalls=0,evidenceCalls=0;
static bool lastQualification=false;
static int OpenExclusiveDataflowDriver() { return mode==7?-1:10; }
static int close(int fd) { assert(fd==10);++closeCalls;return mode==8?-1:0; }
static bool ResetStoppedRewrite(int fd,bool force,bool qualification=false) {
    assert(fd==10 && force);++resetCalls;lastQualification=qualification;return mode!=5 && mode!=10 && mode!=11;
}
static bool Call(int fd,uint32_t op,void* data,size_t) {
    assert(fd==10);
    if (op==TT_PCI_PROBE_RELOAD_STATUS) {
        auto& r=*static_cast<tt_pioneer_driver_reload*>(data);
        const bool first=statusCalls++==0;
        r.driver=60;r.build_tag=mode==2?UINT64_C(0x111):mode==12?UINT64_C(0x2026112300030002):UINT64_C(0x2026112300030001);
        r.generation=1;r.open_count=1;r.guard_version=1;
        if (mode==9 && first) r.guard_version=0;
        if (!first && (mode==3 || mode==4 || mode==10 || mode==11)) {
            r.active_owner=1;r.cold=mode==4 || mode==11;
        }
        return true;
    }
    if (op==TT_PCI_PROBE_Q38_BANK_BEGIN) {
        ++beginCalls;
        if (mode==3 || mode==4) return false;
        auto& r=*static_cast<tt_q38_bank_session*>(data);
        r.result.state=TT_RESIDENT_SESSION_READY;r.result.active=1;r.result.host_ready=1;
        return true;
    }
    if (op==TT_PCI_PROBE_RECOVERY_PCIE_EVIDENCE) {
        auto& r=*static_cast<tt_pioneer_pcie_evidence*>(data);
        assert(r.version==1 && r.expected_tag==UINT64_C(0x2026112300030001) && r.expected_generation==1);
        ++evidenceCalls;r.samples[0].valid=1;return true;
    }
    assert(op==TT_PCI_PROBE_Q38_BANK_END);++endCalls;return true;
}
#include "pioneer_qualify_client.inc"
int main() {
    for (mode=1;mode<=12;mode++) {
        statusCalls=beginCalls=endCalls=resetCalls=closeCalls=evidenceCalls=0;lastQualification=false;
        assert(QualifyRecovery()==(mode==1 || mode==6 || mode==12));
        if (mode==1 || mode==6 || mode==8 || mode==12)
            assert(beginCalls==1 && resetCalls==1 && lastQualification && !endCalls);
        if (mode==2 || mode==9) assert(!beginCalls && !resetCalls && !endCalls);
        if (mode==3) assert(beginCalls==1 && endCalls==1 && !resetCalls);
        if (mode==4) assert(beginCalls==1 && !endCalls && resetCalls==1 && !lastQualification);
        if (mode==5) assert(beginCalls==1 && resetCalls==1 && lastQualification && !endCalls);
        if (mode==10) assert(beginCalls==1 && resetCalls==1 && lastQualification && endCalls==1);
        if (mode==11) assert(beginCalls==1 && resetCalls==1 && lastQualification && !endCalls && evidenceCalls==1);
        if (mode!=11) assert(!evidenceCalls);
        assert(closeCalls==(mode==7?0U:1U));
    }
    std::puts("PASS qualification client gates driver identity and handles failed bank setup; SIMULATED ONLY");
}
