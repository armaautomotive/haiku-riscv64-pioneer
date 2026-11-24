/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_QWEN35_PROJECTION_QUALIFICATION_PROTOCOL_H
#define PIONEER_QWEN35_PROJECTION_QUALIFICATION_PROTOCOL_H
#include <stdint.h>

// Fixed all-worker Q4_K diagnostic. No user pointer, model, code image,
// address or shape is accepted; this is not an inference or Work command.
#define TT_PCI_PROBE_Q35_PROJECTION_QUALIFY (B_DEVICE_OP_CODES_END+0x61e9)
struct tt_q35_projection_qualification {
    uint32_t version,reserved;
    uint64_t expected_tag,expected_generation;
    int32_t operation_status;
    uint32_t stage,route_stage,route_core,output_verified,cleanup_complete;
    uint64_t build_tag,generation;
};
static_assert(sizeof(tt_q35_projection_qualification)==64,"fixed projection qualification ABI");
static inline bool PioneerQ35ProjectionQualificationRequest(const tt_q35_projection_qualification& r)
{
    return r.version==1 && !r.reserved && r.expected_tag && r.expected_generation
        && !r.operation_status && !r.stage && !r.route_stage && !r.route_core
        && !r.output_verified && !r.cleanup_complete && !r.build_tag && !r.generation;
}
#endif
