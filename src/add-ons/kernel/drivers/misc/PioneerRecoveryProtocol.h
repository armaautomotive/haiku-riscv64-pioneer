/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_RECOVERY_PROTOCOL_H
#define PIONEER_RECOVERY_PROTOCOL_H
#include <stdint.h>

#define TT_PCI_PROBE_RECOVERY_STATUS (B_DEVICE_OP_CODES_END+0x61e2)
#define TT_PCI_PROBE_RECOVERY_RUN (B_DEVICE_OP_CODES_END+0x61e3)
// Explicitly abandon a faulted exclusive rewrite session and quarantine it
// before attempting the same guarded reset as RECOVERY_RUN.
#define TT_PCI_PROBE_RECOVERY_FORCE (B_DEVICE_OP_CODES_END+0x61e4)
// Present only in the restricted idle-session qualification build.
#define TT_PCI_PROBE_RECOVERY_QUALIFY (B_DEVICE_OP_CODES_END+0x61e5)
// Fixed, read-only evidence from a restricted qualification session.
#define TT_PCI_PROBE_RECOVERY_PCIE_EVIDENCE (B_DEVICE_OP_CODES_END+0x61e6)
// Non-loadable snapshot witness only: start the one-shot Tensix fabric path.
#define TT_PCI_PROBE_RECOVERY_FABRIC (B_DEVICE_OP_CODES_END+0x61eb)

enum : uint32_t {
	PIONEER_PCIE_TRACE_BEFORE_RESET=0,
	PIONEER_PCIE_TRACE_AFTER_RESET=1,
	PIONEER_PCIE_TRACE_AFTER_PCI=2,
	PIONEER_PCIE_TRACE_AFTER_MAPPING=3,
	PIONEER_PCIE_TRACE_AFTER_FIRMWARE=4,
	PIONEER_PCIE_TRACE_ON_FAILURE=5,
	PIONEER_PCIE_TRACE_AFTER_TRIGGER=6,
	PIONEER_PCIE_TRACE_BEFORE_FIRST_POLL=7,
	PIONEER_PCIE_TRACE_AFTER_FIRST_VENDOR=8,
	PIONEER_PCIE_TRACE_AFTER_FIRST_MARKER=9,
	PIONEER_PCIE_TRACE_SAMPLES=10
};
struct tt_pioneer_pcie_trace_sample {
	uint32_t valid,device_status,link_status,uncorrectable,uncorrectable_mask,
		severity,root_status,source,header[4];
};
struct tt_pioneer_pcie_evidence {
	uint32_t version,reserved[3];
	uint64_t expected_tag,expected_generation;
	tt_pioneer_pcie_trace_sample samples[PIONEER_PCIE_TRACE_SAMPLES];
};
static_assert(sizeof(tt_pioneer_pcie_trace_sample)==48,"fixed trace sample ABI");
static_assert(sizeof(tt_pioneer_pcie_evidence)==512,"fixed trace evidence ABI");
static inline bool PioneerPcieEvidenceRequest(const tt_pioneer_pcie_evidence& r)
{
	if (r.version!=1 || r.reserved[0] || r.reserved[1] || r.reserved[2]
		|| !r.expected_tag || !r.expected_generation) return false;
	for (const auto& s:r.samples) {
		if (s.valid || s.device_status || s.link_status || s.uncorrectable
			|| s.uncorrectable_mask || s.severity || s.root_status || s.source) return false;
		for (auto word:s.header) if (word) return false;
	}
	return true;
}

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
// resumable pre-reset preparation, firmware poll or cleaned-up mapping
// allocation failure; a stage alone does not imply that it can be retried.
enum : uint32_t { PIONEER_RECOVERY_DETAIL_FIRMWARE_WAIT=0x46570002,
	PIONEER_RECOVERY_DETAIL_MAPPING_WAIT=0x4d500002,
	PIONEER_RECOVERY_DETAIL_PREPARE_WAIT=0x50520002 };
// The fabric sequence was audited, but the retained session is not restored.
enum : uint32_t { PIONEER_RECOVERY_STAGE_FABRIC_PREPARE=14,
	PIONEER_RECOVERY_STAGE_FABRIC_SEQUENCE=15,
	PIONEER_RECOVERY_DETAIL_FABRIC_AUDITED=0x46410001 };
// Failure diagnostics, not retry authorizations. The low 24 bits identify
// the read-only audit or guarded firmware operation that failed.
enum : uint32_t { PIONEER_RECOVERY_DETAIL_FIRMWARE_PROBE=0x46000000,
	PIONEER_RECOVERY_DETAIL_FIRMWARE_DBI=0x44000000,
	PIONEER_RECOVERY_DETAIL_FIRMWARE_QUEUE=0x51000000,
	PIONEER_RECOVERY_DETAIL_RESET_PREMAP=0x52010000,
	PIONEER_RECOVERY_DETAIL_RESET_PREPROBE=0x52020000,
	PIONEER_RECOVERY_DETAIL_RESET_SUBMIT=0x52030000 };
struct tt_pioneer_recovery_run {
	uint32_t version,reserved[3];
	uint64_t expected_tag,expected_generation;
	uint32_t stage,detail;
	int32_t operation_status;
	// Qualification only: pre-reset root/card PCIe Device Status, packed
	// as root:16 | card:16. Unknown is 0xffff. Must be zero on input.
	uint32_t output_reserved;
	uint64_t build_tag;
	// On a failed qualification only, this packs the pre-reset (upper 32) and
	// post-reset (lower 32) root AER Uncorrectable Error Status. Either half is
	// UINT32_MAX if unavailable. PCI Device Status admission refusal is zero.
	// On success it is the new generation, as in the ordinary recovery ABI.
	uint64_t new_generation;
};
static_assert(sizeof(tt_pioneer_recovery_run)==64,"fixed recovery run ABI");
static inline bool PioneerRecoveryRunRequest(const tt_pioneer_recovery_run& r)
{
	return r.version==1 && !r.reserved[0] && !r.reserved[1] && !r.reserved[2]
		&& r.expected_tag && r.expected_generation && !r.stage && !r.detail
		&& !r.operation_status && !r.output_reserved && !r.build_tag && !r.new_generation;
}
#endif
