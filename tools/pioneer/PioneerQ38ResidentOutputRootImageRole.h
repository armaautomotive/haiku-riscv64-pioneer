/* SPDX-License-Identifier: MIT */
#ifdef PIONEER_Q38_MIXED_RECURRENT_PROJECTION_ROOT_IMAGE
#error Separate recurrent projection image cannot select an existing image role
#endif
// No guard: late role changes must not select a different owner.
#include "PioneerQ38ResidentCohortImageRole.h"
#if !defined(PIONEER_Q38_RESIDENT_OUTPUT_ROOT_IMAGE) || PIONEER_Q38_RESIDENT_OUTPUT_ROOT_IMAGE != 1
#error Exact separate OFFLINE Conv GDN output full-matrix ROOT image required
#endif
#if defined(PIONEER_Q38_GDN_OUTPUT_GRAPH_ROOT) || defined(PIONEER_Q38_PROJECTION_RESULT_STASH) || defined(PIONEER_Q38_PROJECTION_NEXT_ROOT) || defined(PIONEER_Q38_OUTPUT_TRANSFER_ROOT) || defined(PIONEER_Q38_WORK_ROOT_COMMAND_REUSE) || defined(PIONEER_Q38_WORK_BATCH4_FETCH) || defined(PIONEER_Q38_PEER_PHASE_ROOT)
#error Combined output ROOT is not a legacy graph/layout/host DMA/command cache experiment
#endif
