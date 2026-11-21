/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_QWEN35_DATAFLOW_MODEL_PROTOCOL_H
#define PIONEER_QWEN35_DATAFLOW_MODEL_PROTOCOL_H

#include "../../../../../tools/pioneer/PioneerQwen35DataflowGraphWorkspace.h"

#define TT_PCI_PROBE_QWEN35_DATAFLOW_MODEL_BIND (B_DEVICE_OP_CODES_END + 0x8838)
#define TT_PCI_PROBE_QWEN35_DATAFLOW_PROJECT (B_DEVICE_OP_CODES_END + 0x8839)
#define TT_PCI_PROBE_QWEN35_DATAFLOW_ACTIVATION_SEED (B_DEVICE_OP_CODES_END + 0x883a)
#define TT_PCI_PROBE_QWEN35_DATAFLOW_ACTIVATION_READ (B_DEVICE_OP_CODES_END + 0x883b)
#define TT_PCI_PROBE_QWEN35_DATAFLOW_EMBED_TOKEN (B_DEVICE_OP_CODES_END + 0x883c)
#define TT_PCI_PROBE_QWEN35_DATAFLOW_RMS_NORM (B_DEVICE_OP_CODES_END + 0x883d)
#define TT_PCI_PROBE_QWEN35_DATAFLOW_SILU (B_DEVICE_OP_CODES_END + 0x883e)
#define TT_PCI_PROBE_QWEN35_DATAFLOW_RESIDUAL (B_DEVICE_OP_CODES_END + 0x883f)
#define TT_PCI_PROBE_QWEN35_DATAFLOW_FFN_SEQUENCE (B_DEVICE_OP_CODES_END + 0x8840)

enum : uint32_t {
	TT_QWEN35_DATAFLOW_MODEL_ABI=2,
	TT_QWEN35_DATAFLOW_RESIDUAL_SAVE=1,
	TT_QWEN35_DATAFLOW_RESIDUAL_ADD=2,
	TT_QWEN35_DATAFLOW_CAP_FFN_SEQUENCE=256U
};

struct tt_qwen35_dataflow_model_bind {
	uint32_t version,reserved;
	PioneerQwen35::Range rope;
	PioneerQwen35::DeviceLayout manifest;
	PioneerQwen35::DataflowGraphWorkspace graphWorkspace;
};

struct tt_qwen35_dataflow_projection {
	// output_slot 0/1 selects a ping-pong buffer; DataflowGraphUpOutputSlot
	// selects the validated Ffn-sized graph scratch and admits only Up weights.
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

struct tt_qwen35_dataflow_embed_token {
	uint32_t version,token,output_slot,result;
	uint32_t next_sequence,state,recovery_required,reserved;
};

struct tt_qwen35_dataflow_rms_norm {
	uint32_t version,layer,role,input_slot,output_slot;
	uint32_t result,next_sequence,state,recovery_required;
};

struct tt_qwen35_dataflow_silu {
	uint32_t version,gate_slot;
	uint32_t result,next_sequence,state,recovery_required;
};

struct tt_qwen35_dataflow_residual {
	uint32_t version,mode,activation_slot;
	uint32_t result,next_sequence,state,recovery_required;
};

struct tt_qwen35_dataflow_ffn_sequence {
	uint32_t version,layer,input_slot,output_slot;
	uint32_t result,stage,next_sequence,state,recovery_required;
};

static_assert(sizeof(tt_qwen35_dataflow_activation_seed)==32,
	"fixed activation seed ABI");
static_assert(sizeof(tt_qwen35_dataflow_activation_read)==32,
	"fixed activation readback ABI");
static_assert(sizeof(tt_qwen35_dataflow_embed_token)==32,
	"fixed token embedding ABI");
static_assert(sizeof(tt_qwen35_dataflow_rms_norm)==36,
	"fixed RMS normalization ABI");
static_assert(sizeof(tt_qwen35_dataflow_silu)==24,
	"fixed SiLU multiplication ABI");
static_assert(sizeof(tt_qwen35_dataflow_residual)==28,
	"fixed residual update ABI");
static_assert(sizeof(tt_qwen35_dataflow_ffn_sequence)==36,
	"fixed feed-forward sequence ABI");

static_assert(sizeof(tt_qwen35_dataflow_model_bind)==82228,
	"fixed manifest binding ABI");

#endif
