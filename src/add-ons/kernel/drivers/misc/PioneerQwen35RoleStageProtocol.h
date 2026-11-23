/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_QWEN35_ROLE_STAGE_PROTOCOL_H
#define PIONEER_QWEN35_ROLE_STAGE_PROTOCOL_H
#include <stdint.h>

// Restricted fixed-image qualification. Version 1 keeps all roles held;
// version 2 performs its fixed B/NC startup/cancel sequence on worker zero.
// No user address, executable payload, model data or inference input.
#define TT_PCI_PROBE_Q35_ROLE_STAGE_QUALIFY (B_DEVICE_OP_CODES_END+0x61e8)
enum : uint32_t { TT_Q35_ROLE_STAGE_COMPLETE=8, TT_Q35_ROLE_PRESTACK_COMPLETE=9,
    TT_Q35_ROLE_LOCAL_RESTORE_COMPLETE=10 };
struct tt_q35_role_stage_qualification {
    uint32_t version,reserved[3];
    uint64_t expected_tag,expected_generation;
    int32_t operation_status;
    uint32_t stage,captured,staged,restored,mappings_closed;
    uint64_t build_tag,generation;
};
static_assert(sizeof(tt_q35_role_stage_qualification)==72,"fixed role stage qualification ABI");
static inline bool PioneerQ35RoleStageQualificationRequest(const tt_q35_role_stage_qualification& r)
{
#ifdef PIONEER_QWEN35_ROLE_LOCAL_RESTORE_QUALIFY
    const bool versionValid=r.version==3
#else
    const bool versionValid=r.version==1
#ifdef PIONEER_QWEN35_ROLE_PRESTACK_QUALIFY
            || r.version==2
#endif
#endif
        ;
    return versionValid && !r.reserved[0] && !r.reserved[1] && !r.reserved[2]
        && r.expected_tag && r.expected_generation && !r.operation_status && !r.stage
        && !r.captured && !r.staged && !r.restored && !r.mappings_closed
        && !r.build_tag && !r.generation;
}
#endif
