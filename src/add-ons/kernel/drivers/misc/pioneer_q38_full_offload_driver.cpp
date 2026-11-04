/* SPDX-License-Identifier: MIT */
// Separate full-card candidate. Building is not a physical/numerical release.
// No hybrid Work selector or CPU learned/reference backend is linked.
#define PIONEER_Q38_FULL_OFFLOAD_NATIVE 1
#define PIONEER_Q38_TOKEN_NATIVE_CONTEXT 1
#define PIONEER_Q38_TOKEN_EMIT_CONTEXT_BODY 1
#define PIONEER_Q38_TOKEN_PUSHED_NATIVE_CONTEXT 1
#define PIONEER_Q38_TOKEN_PUSHED_PROGRAMS 1
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
#include "tenstorrent_pci_probe.cpp"
