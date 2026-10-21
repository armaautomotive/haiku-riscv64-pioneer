/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_MODEL_THREAD_SWEEP_H
#define PIONEER_Q38_MODEL_THREAD_SWEEP_H
#ifndef PIONEER_Q38_MODEL_THREAD_SWEEP
#error Isolated public THREE32 host-thread diagnostic ONLY
#endif
#include "../../src/add-ons/kernel/drivers/misc/PioneerDriverReloadProtocol.h"
// No tensor data,device address,IO or completion/cleanup authority here.
// Matrices remain entirely on one exact original-TC Work driver. The
// separate raw-cache trial is NOT a production/Assistant selector.
// Native raw-cache builds are BLOCKED after tag22's first Q4 failure;
// this private metadata/CPU-test role remains for offline investigation.
#ifdef PIONEER_Q38_MODEL_THREAD_RAW_CACHE
#if !defined(PIONEER_Q38_MODEL_THREAD_SWEEP_NARROW) || !defined(PIONEER_Q38_MODEL_THREAD_SWEEP_FOUR)
#error Isolated raw-cache model trial requires fixed8/4/8 public Work role
#endif
#endif
#ifdef PIONEER_Q38_MODEL_THREAD_SWEEP_NARROW
#ifdef PIONEER_Q38_MODEL_THREAD_SWEEP_FOUR
enum {PioneerQ38ModelWideThreads=8,PioneerQ38ModelNarrowThreads=4};
#else
enum {PioneerQ38ModelWideThreads=8,PioneerQ38ModelNarrowThreads=1};
#endif
#else
#ifdef PIONEER_Q38_MODEL_THREAD_SWEEP_FOUR
#error Four-thread comparison requires its separate narrow Work role
#endif
enum {PioneerQ38ModelWideThreads=32,PioneerQ38ModelNarrowThreads=8};
#endif
static inline unsigned PioneerQ38ModelSequenceThreads(unsigned sequence) {
 return sequence<3?unsigned(sequence==1?PioneerQ38ModelNarrowThreads:PioneerQ38ModelWideThreads):0U;
}
static inline bool PioneerQ38ModelThreadSweepTag(const tt_pioneer_driver_reload& r) {
 return r.version==1 && r.driver==55 && r.guard_version==1 && r.generation
  && r.build_tag==
#ifdef PIONEER_Q38_MODEL_THREAD_RAW_CACHE
  UINT64_C(0x2026102100010022)
#else
  UINT64_C(0x2026102100010012)
#endif
  && !r.quiesced && !r.cold
  && !r.active_owner && r.open_count==1;
}
#endif
