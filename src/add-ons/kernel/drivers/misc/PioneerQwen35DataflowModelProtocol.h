/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_QWEN35_DATAFLOW_MODEL_PROTOCOL_H
#define PIONEER_QWEN35_DATAFLOW_MODEL_PROTOCOL_H

#include "../../../../../tools/pioneer/PioneerQwen35DeviceLayout.h"

#define TT_PCI_PROBE_QWEN35_DATAFLOW_MODEL_BIND (B_DEVICE_OP_CODES_END + 0x8838)
#define TT_PCI_PROBE_QWEN35_DATAFLOW_PROJECT (B_DEVICE_OP_CODES_END + 0x8839)
#define TT_PCI_PROBE_QWEN35_DATAFLOW_ACTIVATION_SEED (B_DEVICE_OP_CODES_END + 0x883a)
#define TT_PCI_PROBE_QWEN35_DATAFLOW_ACTIVATION_READ (B_DEVICE_OP_CODES_END + 0x883b)

enum : uint32_t {
	TT_QWEN35_DATAFLOW_MODEL_ABI=1
};

struct tt_qwen35_dataflow_model_bind {
	uint32_t version,reserved;
	PioneerQwen35::Range rope;
	PioneerQwen35::DeviceLayout manifest;
};

struct tt_qwen35_dataflow_projection {
	uint32_t version,layer,role,input_slot,output_slot;
	uint32_t result,next_sequence,state,recovery_required;
};

struct tt_qwen35_dataflow_activation_seed {
	uint32_t version,slot,rows,reserved;
	uint64_t source;
	uint32_t result,activation_mask;
};

struct tt_qwen35_dataflow_activation_read {
	uint32_t version,slot,rows,reserved;
	uint64_t destination;
	uint32_t result,activation_mask;
};

static_assert(sizeof(tt_qwen35_dataflow_activation_seed)==32,
	"fixed activation seed ABI");
static_assert(sizeof(tt_qwen35_dataflow_activation_read)==32,
	"fixed activation readback ABI");

static_assert(sizeof(tt_qwen35_dataflow_model_bind)==81972,
	"fixed manifest binding ABI");

#endif
