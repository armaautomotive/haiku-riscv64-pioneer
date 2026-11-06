/* SPDX-License-Identifier: MIT */
#include "PioneerQ38Bf16FullClientRole.h"
// Reuse vocabulary/channel/lifecycle only. No llama inference context or
// CPU model products. The actual session refuses every non-tag105 driver.
#include "pioneer_assistant_qwen27_full_offload.cpp"
