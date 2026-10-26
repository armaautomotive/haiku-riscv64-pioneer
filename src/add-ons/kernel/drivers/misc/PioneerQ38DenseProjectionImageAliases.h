/* SPDX-License-Identifier: MIT */
#if !defined(PIONEER_Q38_DENSE_PROJECTION_IMAGE) || PIONEER_Q38_DENSE_PROJECTION_IMAGE != 1 \
 || defined(PIONEER_Q38_PROJECTION_NEXT_ROOT) || defined(PIONEER_Q38_PROJECTION_RESULT_STASH) || defined(PIONEER_Q38_GDN_OUTPUT_GRAPH_ROOT) \
 || defined(PIONEER_Q38_DENSE_PARTITION_IMAGE) || defined(PIONEER_Q38_CPU_VALIDATION) || defined(PIONEER_Q38_WORK_INFERENCE) || defined(_KERNEL_MODE)
#error Separate dense projection IMAGE/emulator adapter only,not existing native or Work grammar
#endif
#ifndef PIONEER_Q38_DENSE_PROJECTION_IMAGE_ALIASES_H
#define PIONEER_Q38_DENSE_PROJECTION_IMAGE_ALIASES_H
// Shared arithmetic/transport core,GENUINELY NEW QDM1 grammar and slots.
// Canonical/native owners include independent Dense names without adapters.
#define TT_Q38_PROJECTION_MATRIX_MARKER TT_Q38_DENSE_MATRIX_MARKER
#define TT_Q38_PROJECTION_MATRIX_FAULT TT_Q38_DENSE_MATRIX_FAULT
#define TT_Q38_PROJECTION_MATRIX_FRAME TT_Q38_DENSE_MATRIX_FRAME
#define TT_Q38_PROJECTION_MATRIX_ROWS TT_Q38_DENSE_MATRIX_ROWS
#define TT_Q38_PROJECTION_MATRIX_INPUT TT_Q38_DENSE_MATRIX_INPUT
#define TT_Q38_PROJECTION_OUTPUT_OFFSET TT_Q38_DENSE_QG
#define TT_Q38_PROJECTION_MATRIX_SHADOW TT_Q38_DENSE_MATRIX_SHADOW
#define TT_Q38_PROJECTION_ROWS_SHADOW TT_Q38_DENSE_ROWS_SHADOW
#define PioneerQ38ProjectionOutputOffset PioneerQ38DenseProjectionOffset
#define PioneerQ38ProjectionJobCount PioneerQ38DenseProjectionJobCount
#define PioneerQ38ProjectionMatrixShape PioneerQ38DenseProjectionMatrixShape
#define PioneerQ38ProjectionMatrixOrdered PioneerQ38DenseProjectionMatrixOrdered
#define PioneerQ38ProjectionMatrixInputReads PioneerQ38DenseProjectionInputReads
#define PioneerQ38ProjectionMatrixInputFlits PioneerQ38DenseProjectionInputFlits
#endif
