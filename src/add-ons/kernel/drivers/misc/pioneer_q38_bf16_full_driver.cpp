/* SPDX-License-Identifier: MIT */
// Separate complete-model BF16/FPU native subject. Build/inspect only until
// exclusive lifecycle,numerical and controlled hardware tests qualify it.
#define PIONEER_Q38_BF16_FULL_NATIVE 1
#define PIONEER_Q38_FULL_OFFLOAD_NATIVE 1
#define PIONEER_Q38_TOKEN_BF16_PROGRAMS 1
#define PIONEER_Q38_TOKEN_NATIVE_CONTEXT 1
#define PIONEER_Q38_TOKEN_EMIT_CONTEXT_BODY 1
#define PIONEER_Q38_DECODER_RUNTIME 1
#define PIONEER_Q38_GDN_MUTABLE_STATE 1
#define PIONEER_Q38_FFN_NATIVE_GRAPH 1
#define PIONEER_Q38_FFN_PERSISTENT_GRAPH 1
#define PIONEER_Q38_FFN_GRAPH_INPUT_ROOT 1
#define PIONEER_Q38_FFN_MODEL_INIT 1
#define PIONEER_Q38_NORMAL_CLOCK 1
#define PIONEER_Q38_SPAN_PRELOAD 1
#define PIONEER_Q38_SPAN_FAST 1
#define PIONEER_Q38_LOCAL_DELTA_PROBE 1
#include "PioneerQ38Bf16FullNativeRole.h"
#include "tenstorrent_pci_probe.cpp"
