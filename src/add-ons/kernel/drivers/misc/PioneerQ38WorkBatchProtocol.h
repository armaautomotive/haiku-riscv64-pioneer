/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_WORK_BATCH_PROTOCOL_H
#define PIONEER_Q38_WORK_BATCH_PROTOCOL_H
#include "PioneerQ38DispatchProtocol.h"
// Separate candidate namespace; ordinary Work clients cannot silently select it.
#define TT_PCI_PROBE_Q38_WORK_BATCH_SUPPORT (B_DEVICE_OP_CODES_END + 0x6f00)
#define TT_PCI_PROBE_Q38_WORK_BATCH_RUN (B_DEVICE_OP_CODES_END + 0x6f01)
enum { PIONEER_Q38_WORK_BATCH_COUNT=2, PIONEER_Q38_WORK_BATCH_VERSION=1 };
struct PioneerQ38WorkBatchSupport { uint32_t version,count,driver,reserved; uint64_t tag; };
struct PioneerQ38WorkBatchPacket {
 uint32_t version,count,reserved[2];
 tt_q38_dispatch_probe jobs[PIONEER_Q38_WORK_BATCH_COUNT];
};
#endif
