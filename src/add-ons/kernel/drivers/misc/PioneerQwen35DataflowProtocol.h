/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_QWEN35_DATAFLOW_PROTOCOL_H
#define PIONEER_QWEN35_DATAFLOW_PROTOCOL_H

#include <Drivers.h>
#include <stdint.h>

#define TT_PCI_PROBE_QWEN35_DATAFLOW_SUPPORT (B_DEVICE_OP_CODES_END + 0x8836)
#define TT_PCI_PROBE_QWEN35_DATAFLOW_CONTROL (B_DEVICE_OP_CODES_END + 0x8837)

static_assert(TT_PCI_PROBE_QWEN35_DATAFLOW_CONTROL
	-TT_PCI_PROBE_QWEN35_DATAFLOW_SUPPORT==1,
	"Qwen35 support and control ioctls must be contiguous");

enum : uint32_t {
	TT_QWEN35_DATAFLOW_DRIVER=60,
	TT_QWEN35_DATAFLOW_ABI=3,
	TT_QWEN35_DATAFLOW_CORES=120,
	TT_QWEN35_DATAFLOW_CAP_RESIDENT_SERVICE=1U,
	TT_QWEN35_DATAFLOW_CAP_MANIFEST_BIND=2U,
	TT_QWEN35_DATAFLOW_CAP_ACTIVATION_SEED=4U,
	TT_QWEN35_DATAFLOW_CAP_ACTIVATION_READ=8U,
	TT_QWEN35_DATAFLOW_CAP_TOKEN_EMBEDDING=16U,
	TT_QWEN35_DATAFLOW_CAP_RMS_NORM=32U,
	TT_QWEN35_DATAFLOW_CAP_SILU=64U,
	TT_QWEN35_DATAFLOW_CAP_RESIDUAL=128U,
	TT_QWEN35_DATAFLOW_PROJECTION_MATRIX=1U,
	TT_QWEN35_DATAFLOW_TOKEN_NONE=0U,
	TT_QWEN35_DATAFLOW_TOKEN_EMBEDDING=1U,
	TT_QWEN35_DATAFLOW_ACTION_QUERY=0,
	TT_QWEN35_DATAFLOW_ACTION_INSTALL=1,
	TT_QWEN35_DATAFLOW_ACTION_CLOSE=2,
	TT_QWEN35_DATAFLOW_ACTION_RECOVER=3,
	TT_QWEN35_DATAFLOW_STATE_EMPTY=0,
	TT_QWEN35_DATAFLOW_STATE_READY=1,
	TT_QWEN35_DATAFLOW_STATE_FAULTED=2,
	TT_QWEN35_DATAFLOW_STATE_CLOSED=3,
	TT_QWEN35_DATAFLOW_RESULT_COMPLETE=0,
	TT_QWEN35_DATAFLOW_RESULT_RECOVERY_PENDING=13,
	TT_QWEN35_DATAFLOW_RESULT_RECOVERED=14,
	TT_QWEN35_DATAFLOW_RESULT_STOP_AUDIT_FAILED=15,
	TT_QWEN35_DATAFLOW_RESULT_CLOSED=17,
	TT_QWEN35_DATAFLOW_RESULT_RESUME_FAILED=18
};

// The protocol exposes lower-level input/layer primitives plus complete
// decoder and generation requests. Generation is available only when the
// driver advertises the corresponding model-protocol capability.
struct tt_qwen35_dataflow_support {
	uint32_t version,driver,cores,capabilities;
	uint32_t projection_capabilities,token_capabilities;
};

struct tt_qwen35_dataflow_request {
	uint32_t version,action,state,result,next_sequence,recovery_required;
	// Input must be zero. On INSTALL start/readiness failures and CLOSE stop
	// audit failures, output packs reserved[0] as stage:8 | core:24 and
	// reserved[1] as state:8 | failure:8 | phase:8 | zero:8.
	// reserved[2] contains action-specific detail: a raw local route on a
	// phase-2 worker startup fault, or the resume detail on CLOSE stage9.
	// Resume detail 1 means reset mismatch, 2 generic idle-proof failure, and
	// 0x101..0x124 identify the failing restored-core idle subproof. QUERY and
	// RECOVER return the last decoder-operation detail in reserved[0] as
	// owner_result:8 | core:16 | operation:8, reserved[1] as worker
	// state:8 | failure:8 | phase:8 | zero:8, and reserved[2] as the pending
	// worker sequence. A core value of 0xffff means no worker was isolated.
	uint32_t reserved[3];
};

static inline bool
PioneerQwen35DataflowRequestValid(const tt_qwen35_dataflow_request& request)
{
	return request.version==TT_QWEN35_DATAFLOW_ABI
		&& request.action<=TT_QWEN35_DATAFLOW_ACTION_RECOVER
		&& request.reserved[0]==0 && request.reserved[1]==0
		&& request.reserved[2]==0;
}

#endif
