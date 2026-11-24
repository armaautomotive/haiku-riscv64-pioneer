/* SPDX-License-Identifier: MIT */
#define B_DEVICE_OP_CODES_END 1000
#define PIONEER_QWEN35_RECOVERY_QUALIFY 1
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include "../../../src/add-ons/kernel/drivers/misc/PioneerDriverReloadProtocol.h"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerRecoveryProtocol.h"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38BankProtocol.h"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQwen35FleetInspectionProtocol.h"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQwen35RiscResetProtocol.h"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQwen35RoleStageProtocol.h"
#ifdef PIONEER_QWEN35_SPAN_QUALIFY
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38SpanProtocol.h"
#endif

constexpr int B_NOT_ALLOWED=-1;
static int DriverReloadControl(void*,uint32_t,void*,size_t) { return 1; }
static int Q38BankControl(void*,uint32_t,void*,size_t) { return 2; }
#ifdef PIONEER_QWEN35_SPAN_QUALIFY
static int Q38SpanControl(void*,uint32_t,void*,size_t) { return 10; }
#elif defined(PIONEER_QWEN35_FLEET_INSPECTION_QUALIFY)
static int Qwen35FleetInspectionControl(void*,void*,size_t) { return 8; }
#ifdef PIONEER_QWEN35_RISC_RESET_SAMPLE_QUALIFY
static int Qwen35RiscResetSampleControl(void*,void*,size_t) { return 11; }
#endif
#ifdef PIONEER_QWEN35_ROLE_STAGE_QUALIFY
static int Qwen35RoleStageControl(void*,void*,size_t) { return 9; }
#endif
#else
static int Qwen35RecoveryStatus(void*,void*,size_t) { return 3; }
static int Qwen35RecoveryRun(void*,void*,size_t) { return 4; }
static int Qwen35RecoveryForce(void*,void*,size_t) { return 5; }
static int Qwen35RecoveryQualify(void*,void*,size_t) { return 6; }
static int Qwen35RecoveryPcieEvidence(void*,void*,size_t) { return 7; }
#endif
static int Route(uint32_t operation) {
    void* instance=nullptr;void* data=nullptr;size_t length=0;
#include "pioneer_recovery_qualify_routing.inc"
    return -2; // qualification must never fall through
}
int main() {
    for (uint32_t op=B_DEVICE_OP_CODES_END;op<B_DEVICE_OP_CODES_END+0x9000;op++) {
        int expected=B_NOT_ALLOWED;
        if (op==TT_PCI_PROBE_RELOAD_STATUS || op==TT_PCI_PROBE_RELOAD_QUIESCE) expected=1;
        if (op==TT_PCI_PROBE_Q38_BANK_SUPPORT || op==TT_PCI_PROBE_Q38_BANK_BEGIN
            || op==TT_PCI_PROBE_Q38_BANK_END) expected=2;
#ifdef PIONEER_QWEN35_SPAN_QUALIFY
        if (op==TT_PCI_PROBE_Q38_SPAN_SUPPORT || op==TT_PCI_PROBE_Q38_SPAN_UPLOAD
            || op==TT_PCI_PROBE_Q38_SPAN_END) expected=10;
#elif defined(PIONEER_QWEN35_FLEET_INSPECTION_QUALIFY)
        if (op==TT_PCI_PROBE_Q35_FLEET_INSPECT) expected=8;
#ifdef PIONEER_QWEN35_RISC_RESET_SAMPLE_QUALIFY
        if (op==TT_PCI_PROBE_Q35_RISC_RESET_SAMPLE) expected=11;
#endif
#ifdef PIONEER_QWEN35_ROLE_STAGE_QUALIFY
        if (op==TT_PCI_PROBE_Q35_ROLE_STAGE_QUALIFY) expected=9;
#endif
#else
        if (op==TT_PCI_PROBE_RECOVERY_STATUS) expected=3;
        if (op==TT_PCI_PROBE_RECOVERY_RUN) expected=4;
        if (op==TT_PCI_PROBE_RECOVERY_FORCE) expected=5;
        if (op==TT_PCI_PROBE_RECOVERY_QUALIFY) expected=6;
        if (op==TT_PCI_PROBE_RECOVERY_PCIE_EVIDENCE) expected=7;
#endif
        assert(Route(op)==expected);
    }
#ifdef PIONEER_QWEN35_SPAN_QUALIFY
    std::puts("PASS span qualification routes only strict upload, bank and reload operations; SIMULATED ONLY");
#elif defined(PIONEER_QWEN35_FLEET_INSPECTION_QUALIFY)
    std::puts("PASS fleet inspection qualification routes only bank, reload and inspection operations; SIMULATED ONLY");
#else
    std::puts("PASS recovery qualification routes only bank, reload and recovery operations; SIMULATED ONLY");
#endif
}
