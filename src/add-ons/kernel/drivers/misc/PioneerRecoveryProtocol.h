/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_RECOVERY_PROTOCOL_H
#define PIONEER_RECOVERY_PROTOCOL_H
#include <stdint.h>

#define TT_PCI_PROBE_RECOVERY_STATUS (B_DEVICE_OP_CODES_END+0x61e2)
#define TT_PCI_PROBE_RECOVERY_RUN (B_DEVICE_OP_CODES_END+0x61e3)
// Explicitly abandon a faulted exclusive rewrite session and quarantine it
// before attempting the same guarded reset as RECOVERY_RUN.
#define TT_PCI_PROBE_RECOVERY_FORCE (B_DEVICE_OP_CODES_END+0x61e4)

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

enum : uint32_t {
	PIONEER_RECOVERY_STAGE_ADMISSION=0,
	PIONEER_RECOVERY_STAGE_DISABLE_MASTER=1,
	PIONEER_RECOVERY_STAGE_ASIC_RESET=2,
	PIONEER_RECOVERY_STAGE_PCI_RESTORE=3,
	PIONEER_RECOVERY_STAGE_MAPPING=4,
	PIONEER_RECOVERY_STAGE_FIRMWARE=5,
	PIONEER_RECOVERY_STAGE_FLEET=6,
	PIONEER_RECOVERY_STAGE_HOST_DMA=7,
	PIONEER_RECOVERY_STAGE_CLOSE_MAPPING=8,
	PIONEER_RECOVERY_STAGE_PCI_IDLE=9,
	PIONEER_RECOVERY_STAGE_OWNER_RELEASE=10,
	PIONEER_RECOVERY_STAGE_UNRESERVE=11,
	PIONEER_RECOVERY_STAGE_GUARD_COMPLETE=12,
	PIONEER_RECOVERY_STAGE_COMPLETE=13
};
// The same reset ticket remains owned. These details identify a safely
// resumable firmware poll or cleaned-up mapping allocation failure; a stage
// alone does not imply that an arbitrary failure can be retried.
enum : uint32_t { PIONEER_RECOVERY_DETAIL_FIRMWARE_WAIT=0x46570002,
	PIONEER_RECOVERY_DETAIL_MAPPING_WAIT=0x4d500002 };
struct tt_pioneer_recovery_run {
	uint32_t version,reserved[3];
	uint64_t expected_tag,expected_generation;
	uint32_t stage,detail;
	int32_t operation_status;
	uint32_t output_reserved;
	uint64_t build_tag,new_generation;
};
static_assert(sizeof(tt_pioneer_recovery_run)==64,"fixed recovery run ABI");
static inline bool PioneerRecoveryRunRequest(const tt_pioneer_recovery_run& r)
{
	return r.version==1 && !r.reserved[0] && !r.reserved[1] && !r.reserved[2]
		&& r.expected_tag && r.expected_generation && !r.stage && !r.detail
		&& !r.operation_status && !r.output_reserved && !r.build_tag && !r.new_generation;
}
#endif
