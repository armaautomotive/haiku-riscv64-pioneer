/* SPDX-License-Identifier: MIT */
// Distinct offline QOP1 complete ssm_out parent;NO native install selector.
#if !defined(PIONEER_Q38_GDN_OUTPUT_GRAPH_ROOT) || PIONEER_Q38_GDN_OUTPUT_GRAPH_ROOT != 1
#error Exact standalone GDN output parent required
#endif
#if defined(_KERNEL_MODE) || defined(PIONEER_Q38_CPU_VALIDATION) || defined(PIONEER_Q38_WORK_INFERENCE) || defined(PIONEER_Q38_PROJECTION_RESULT_STASH) || defined(PIONEER_Q38_GDN_OUTPUT_PRODUCER_NATIVE) || defined(PIONEER_Q38_GDN_RENDEZVOUS_NATIVE) || defined(PIONEER_Q38_RESIDENT_DECODER_NATIVE) || defined(PIONEER_Q38_PROJECTION_EXECUTION_OWNER)
#error Offline parent never shares kernel, reference, Work or old result owners
#endif
#if defined(PIONEER_Q38_GDN_GATED_NATIVE) || defined(PIONEER_Q38_GDN_RECURRENT_NATIVE) || defined(PIONEER_Q38_GDN_STATE_INIT_NATIVE) || defined(PIONEER_Q38_GDN_INPUT_OWNER) || defined(PIONEER_Q38_RESIDENT_DECODER_OWNER) || defined(PIONEER_Q38_EMBEDDING_NATIVE) || defined(PIONEER_Q38_LEARNED_RMS_NATIVE) || defined(PIONEER_Q38_GDN_RENDEZVOUS_VALIDATION_NATIVE)
#error Standalone parent never shares native producer or DEBUG roles
#endif
#define PIONEER_ATTENTION_FREESTANDING
typedef __UINT32_TYPE__ uint32_t;
#include "../../src/add-ons/kernel/drivers/misc/PioneerQ38GdnOutputGraphShape.h"
// The old parsers are already defined above. Only this translation unit's
// separately built dispatcher/parent uses the stricter new namespace.
#define TT_Q38_PROJECTION_GRAPH_MARKER TT_Q38_GDN_OUTPUT_GRAPH_MARKER
#define TT_Q38_PROJECTION_GRAPH_FAULT TT_Q38_GDN_OUTPUT_GRAPH_FAULT
#define TT_Q38_PROJECTION_GRAPH_SOURCES TT_Q38_GDN_OUTPUT_GRAPH_SOURCES
#define PioneerQ38ProjectionGraphManifest PioneerQ38GdnOutputGraphManifest
#define TT_Q38_PROJECTION_MATRIX_MARKER TT_Q38_GDN_OUTPUT_MATRIX_MARKER
#define PioneerQ38ProjectionMatrixShape PioneerQ38GdnOutputMatrixShape
#define PioneerQ38ProjectionOutputOffset PioneerQ38GdnOutputResultOffset
#include "blackhole_qwen38_projection_graph.cpp"
