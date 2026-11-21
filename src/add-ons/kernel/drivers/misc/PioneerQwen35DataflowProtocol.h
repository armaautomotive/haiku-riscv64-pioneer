/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_QWEN35_DATAFLOW_PROTOCOL_H
#define PIONEER_QWEN35_DATAFLOW_PROTOCOL_H

#include <Drivers.h>
#include <stdint.h>

#define TT_PCI_PROBE_QWEN35_DATAFLOW_SUPPORT (B_DEVICE_OP_CODES_END + 0x8836)
#define TT_PCI_PROBE_QWEN35_DATAFLOW_CONTROL (B_DEVICE_OP_CODES_END + 0x8837)

enum : uint32_t {
	TT_QWEN35_DATAFLOW_DRIVER=60,
	TT_QWEN35_DATAFLOW_ABI=1,
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
	TT_QWEN35_DATAFLOW_RESULT_CLOSED=17
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
	uint32_t reserved[2];
};

static inline bool
PioneerQwen35DataflowRequestValid(const tt_qwen35_dataflow_request& request)
{
	return request.version==TT_QWEN35_DATAFLOW_ABI
		&& request.action<=TT_QWEN35_DATAFLOW_ACTION_RECOVER
		&& request.reserved[0]==0 && request.reserved[1]==0;
}

#endif
