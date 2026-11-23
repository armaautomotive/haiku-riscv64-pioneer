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

constexpr int B_NOT_ALLOWED=-1;
static int DriverReloadControl(void*,uint32_t,void*,size_t) { return 1; }
static int Q38BankControl(void*,uint32_t,void*,size_t) { return 2; }
static int Qwen35RecoveryStatus(void*,void*,size_t) { return 3; }
static int Qwen35RecoveryRun(void*,void*,size_t) { return 4; }
static int Qwen35RecoveryForce(void*,void*,size_t) { return 5; }
static int Qwen35RecoveryQualify(void*,void*,size_t) { return 6; }
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
        if (op==TT_PCI_PROBE_RECOVERY_STATUS) expected=3;
        if (op==TT_PCI_PROBE_RECOVERY_RUN) expected=4;
        if (op==TT_PCI_PROBE_RECOVERY_FORCE) expected=5;
        if (op==TT_PCI_PROBE_RECOVERY_QUALIFY) expected=6;
        assert(Route(op)==expected);
    }
    std::puts("PASS recovery qualification routes only bank, reload and recovery operations; SIMULATED ONLY");
}
