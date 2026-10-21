/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_MODEL_THREAD_SWEEP_H
#define PIONEER_Q38_MODEL_THREAD_SWEEP_H
#ifndef PIONEER_Q38_MODEL_THREAD_SWEEP
#error Isolated public THREE32 host-thread diagnostic ONLY
#endif
#include "../../src/add-ons/kernel/drivers/misc/PioneerDriverReloadProtocol.h"
// No tensor data,device address,IO or completion/cleanup authority here.
// Matrices remain entirely on the SAME qualified041/tag12 Work driver.
static inline unsigned PioneerQ38ModelSequenceThreads(unsigned sequence) {
 return sequence<3?(sequence==1?8U:32U):0U;
}
static inline bool PioneerQ38ModelThreadSweepTag(const tt_pioneer_driver_reload& r) {
 return r.version==1 && r.driver==55 && r.guard_version==1 && r.generation
  && r.build_tag==UINT64_C(0x2026102100010012) && !r.quiesced && !r.cold
  && !r.active_owner && r.open_count==1;
}
#endif
