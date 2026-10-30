/* SPDX-License-Identifier: MIT */
// Rechecked BUILD-only experiment. Not native/worker/DATA/Work authority.
#include "PioneerQ38SingleClosedAuditRole.h"
#if !defined(PIONEER_Q38_GLOBAL_CHAIN_WORKER_SEALED_ADMISSION) || PIONEER_Q38_GLOBAL_CHAIN_WORKER_SEALED_ADMISSION != 1 \
 || !defined(PIONEER_Q38_GLOBAL_CHAIN_SINGLE_CLOSED_AUDIT) || PIONEER_Q38_GLOBAL_CHAIN_SINGLE_CLOSED_AUDIT != 1
#error Exact opt-in QGW2 single-audit worker-sealed admission BUILD required
#endif
