/* SPDX-License-Identifier: MIT */
// Offline fixture bytes only: snapshots from completed simulated workers.
#ifndef PIONEER_RECURRENT_WORKER_SNAPSHOT_H
#define PIONEER_RECURRENT_WORKER_SNAPSHOT_H
#include <stdint.h>
struct PioneerRecurrentWorkerStage {
 uint32_t pc,doorbell,counters[8],status[16],job[64],proof[64],descriptor[64],inner[16],line[32],native[128];
};
struct PioneerRecurrentWorkerSnapshot {
 uint32_t pc,doorbell,before[8],after[8],status[16],job[64],proof[64],output[256];
 PioneerRecurrentWorkerStage initial,producer;
};
#endif
