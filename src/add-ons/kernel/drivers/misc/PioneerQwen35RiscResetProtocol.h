/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_QWEN35_RISC_RESET_PROTOCOL_H
#define PIONEER_QWEN35_RISC_RESET_PROTOCOL_H
#include <stdint.h>

// Fixed read-only reset-unit observation, restricted to its own tagged
// empty-session qualification driver. No address or value is user supplied.
#define TT_PCI_PROBE_Q35_RISC_RESET_SAMPLE (B_DEVICE_OP_CODES_END+0x61ea)
struct tt_q35_risc_reset_sample {
    uint32_t version,reserved[3];
    uint64_t expected_tag,expected_generation;
    int32_t operation_status;
    uint32_t sampled,restore_failed,reserved_output;
    uint32_t words[8];
    uint64_t build_tag,generation;
};
static_assert(sizeof(tt_q35_risc_reset_sample)==96,"fixed reset-word observation ABI");
inline bool PioneerQ35RiscResetSampleRequest(const tt_q35_risc_reset_sample& r)
{
    if (r.version!=1 || r.reserved[0] || r.reserved[1] || r.reserved[2]
        || !r.expected_tag || !r.expected_generation || r.operation_status || r.sampled
        || r.restore_failed || r.reserved_output || r.build_tag || r.generation) return false;
    for (unsigned i=0;i<8;i++) if (r.words[i]) return false;
    return true;
}
#endif
