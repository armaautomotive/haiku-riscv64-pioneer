/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_QWEN35_DATAFLOW_MODEL_PROTOCOL_H
#define PIONEER_QWEN35_DATAFLOW_MODEL_PROTOCOL_H

#include "../../../../../tools/pioneer/PioneerQwen35DeviceLayout.h"

#define TT_PCI_PROBE_QWEN35_DATAFLOW_MODEL_BIND (B_DEVICE_OP_CODES_END + 0x8838)

enum : uint32_t {
	TT_QWEN35_DATAFLOW_MODEL_ABI=1
};

struct tt_qwen35_dataflow_model_bind {
	uint32_t version,reserved;
	PioneerQwen35::Range rope;
	PioneerQwen35::DeviceLayout manifest;
};

static_assert(sizeof(tt_qwen35_dataflow_model_bind)==81972,
	"fixed manifest binding ABI");

#endif
