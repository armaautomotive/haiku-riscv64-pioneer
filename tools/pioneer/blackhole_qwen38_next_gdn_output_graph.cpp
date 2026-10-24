/* SPDX-License-Identifier: MIT */
// Distinct QOP2/nonzero recurrent-layer graph; offline instructions ONLY.
#include "PioneerQ38NextGdnOutputImageOnly.h"
#if !defined(PIONEER_Q38_NEXT_GDN_OUTPUT_GRAPH_ROOT) || PIONEER_Q38_NEXT_GDN_OUTPUT_GRAPH_ROOT != 1 || defined(PIONEER_Q38_GDN_OUTPUT_GRAPH_ROOT) || defined(PIONEER_Q38_GDN_OUTPUT_INPUT_IMAGE) || defined(PIONEER_Q38_NEXT_GDN_OUTPUT_INPUT_IMAGE) || defined(PIONEER_Q38_NEXT_GDN_OUTPUT_GATHER_IMAGE)
#error Exact standalone QOP2 root required, never an externally aliased role
#endif
#define PIONEER_ATTENTION_FREESTANDING
typedef __UINT32_TYPE__ uint32_t;
#include "../../src/add-ons/kernel/drivers/misc/PioneerQ38GdnOutputGraphShape.h"
#include "../../src/add-ons/kernel/drivers/misc/PioneerQ38NextGdnOutputGraphShape.h"
// Both old parsers are already defined, strictly unchanged. These aliases
// select ONLY common instruction-template branches in this translation
// unit, not native ownership or a layer-zero producer.
#define PIONEER_Q38_GDN_OUTPUT_GRAPH_ROOT 1
#define TT_Q38_PROJECTION_GRAPH_MARKER TT_Q38_NEXT_GDN_OUTPUT_GRAPH_MARKER
#define TT_Q38_PROJECTION_GRAPH_FAULT TT_Q38_NEXT_GDN_OUTPUT_GRAPH_FAULT
#define TT_Q38_PROJECTION_GRAPH_SOURCES TT_Q38_NEXT_GDN_OUTPUT_GRAPH_SOURCES
#define PioneerQ38ProjectionGraphManifest PioneerQ38NextGdnOutputGraphManifest
#define TT_Q38_PROJECTION_MATRIX_MARKER TT_Q38_NEXT_GDN_OUTPUT_MATRIX_MARKER
#define PioneerQ38ProjectionMatrixShape PioneerQ38NextGdnOutputMatrixShape
#define PioneerQ38ProjectionOutputOffset PioneerQ38NextGdnOutputResultOffset
#define TT_Q38_GDN_OUTPUT_GRAPH_SOURCE_SHADOW TT_Q38_NEXT_GDN_OUTPUT_GRAPH_SOURCE_SHADOW
#define TT_Q38_GDN_OUTPUT_INPUT_MARKER TT_Q38_NEXT_GDN_OUTPUT_INPUT_MARKER
#define PioneerQ38GdnOutputInputProof PioneerQ38NextGdnOutputInputProof
#define TT_Q38_GDN_OUTPUT_GATHER_MARKER TT_Q38_NEXT_GDN_OUTPUT_GATHER_MARKER
#define TT_Q38_GDN_OUTPUT_GATHER_SOURCES TT_Q38_NEXT_GDN_OUTPUT_GATHER_SOURCES
#include "blackhole_qwen38_projection_graph.cpp"
