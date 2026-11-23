/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_QWEN35_FLEET_INSPECTION_PROTOCOL_H
#define PIONEER_QWEN35_FLEET_INSPECTION_PROTOCOL_H
#include <stdint.h>

// Available only on the separately tagged, restricted qualification driver.
// It accepts no addresses, model data, firmware commands or reset request.
#define TT_PCI_PROBE_Q35_FLEET_INSPECT (B_DEVICE_OP_CODES_END+0x61e7)
enum : uint32_t { TT_Q35_FLEET_INSPECTION_COMPLETE=4 };
struct tt_q35_fleet_inspection {
    uint32_t version,reserved[3];
    uint64_t expected_tag,expected_generation;
    int32_t operation_status;
    uint32_t stage,failed_core,l2cpu_reset,restore_failed,prepared;
    uint64_t build_tag,generation;
};
static_assert(sizeof(tt_q35_fleet_inspection)==72,"fixed fleet inspection ABI");
static inline bool PioneerQ35FleetInspectionRequest(const tt_q35_fleet_inspection& r)
{
    return r.version==1 && !r.reserved[0] && !r.reserved[1] && !r.reserved[2]
        && r.expected_tag && r.expected_generation && !r.operation_status && !r.stage
        && !r.failed_core && !r.l2cpu_reset && !r.restore_failed && !r.prepared
        && !r.build_tag && !r.generation;
}
#endif
