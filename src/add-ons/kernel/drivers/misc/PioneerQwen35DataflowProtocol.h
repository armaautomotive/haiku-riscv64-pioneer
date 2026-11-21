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
	TT_QWEN35_DATAFLOW_ACTION_QUERY=0,
	TT_QWEN35_DATAFLOW_ACTION_INSTALL=1,
	TT_QWEN35_DATAFLOW_ACTION_CLOSE=2,
	TT_QWEN35_DATAFLOW_STATE_EMPTY=0,
	TT_QWEN35_DATAFLOW_STATE_READY=1,
	TT_QWEN35_DATAFLOW_STATE_FAULTED=2,
	TT_QWEN35_DATAFLOW_STATE_CLOSED=3
};

// The support ABI separates service lifecycle from model execution.
// Projection and token capabilities remain absent until their Tensix programs
// and native owner paths are complete.
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
		&& request.action<=TT_QWEN35_DATAFLOW_ACTION_CLOSE
		&& request.reserved[0]==0 && request.reserved[1]==0;
}

#endif
