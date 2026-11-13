/* SPDX-License-Identifier: MIT */
// Offline fixture bytes only: snapshots from completed simulated workers.
#ifndef PIONEER_RECURRENT_WORKER_SNAPSHOT_H
#define PIONEER_RECURRENT_WORKER_SNAPSHOT_H
#include <stdint.h>
struct PioneerRecurrentWorkerSnapshot {
 uint32_t pc,doorbell,before[8],after[8],status[16],job[64],proof[64],output[256];
};
#endif
