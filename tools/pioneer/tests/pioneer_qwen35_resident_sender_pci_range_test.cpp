/* SPDX-License-Identifier: MIT */
#define PIONEER_QWEN35_DEVICE_DISPATCH 1
#define PIONEER_QWEN35_RECOVERY_COMPILE_ONLY 1
#define PIONEER_QWEN35_SHARED_ROLE_OWNER_WITNESS 1
#define PIONEER_QWEN35_SNAPSHOT_TAKEOVER_WITNESS 1
#define PIONEER_QWEN35_RESIDENT_SERVICE_WITNESS 1
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQwen35DataflowPciMemory.h"
#include <cassert>
#include <cstdio>
using namespace PioneerQwen35;
int main() {
    assert(DataflowPciL1Range(DataflowResidentSenderReceiptAddress,sizeof(DataflowResidentSenderReceipt)));
    assert(!DataflowPciL1Range(DataflowResidentSenderReceiptAddress-4,4));
    assert(!DataflowPciL1Range(DataflowResidentSenderReceiptAddress+sizeof(DataflowResidentSenderReceipt),4));
    assert(!DataflowPciRunningWriteRange(DataflowResidentSenderReceiptAddress,sizeof(DataflowResidentSenderReceipt)));
    puts("PASS resident sender receipt read range without running host writes; OFFLINE ONLY");
}
