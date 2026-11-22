/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_RECOVERY_PROTOCOL_H
#define PIONEER_RECOVERY_PROTOCOL_H
#include <stdint.h>

#define TT_PCI_PROBE_RECOVERY_STATUS (B_DEVICE_OP_CODES_END+0x61e2)

// Read-only owner-lifetime status. This operation never authorizes or issues
// a reset, and accepts no user-provided hardware addresses or recovery proof.
struct tt_pioneer_recovery_status {
	uint32_t version,reserved[3];
	uint64_t expected_tag,expected_generation;
	uint32_t cold,quiesced,open_count,owner_present,owner_final_free,owner_retirement_eligible;
	uint64_t build_tag,generation;
};
static_assert(sizeof(tt_pioneer_recovery_status)==72,"fixed recovery status ABI");

static inline bool PioneerRecoveryStatusRequest(const tt_pioneer_recovery_status& r)
{
	return r.version==1 && !r.reserved[0] && !r.reserved[1] && !r.reserved[2]
		&& r.expected_tag && r.expected_generation && !r.cold && !r.quiesced
		&& !r.open_count && !r.owner_present && !r.owner_final_free
		&& !r.owner_retirement_eligible && !r.build_tag && !r.generation;
}
#endif
