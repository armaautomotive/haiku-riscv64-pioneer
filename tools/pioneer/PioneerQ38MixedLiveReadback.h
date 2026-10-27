/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_MIXED_LIVE_READBACK_H
#define PIONEER_Q38_MIXED_LIVE_READBACK_H
#include <stdint.h>
// Validation DATA only,not a driver ABI,launch grant or Work selector.
// completed means capture+full L1 restore/second proof,NOT native END,
// final ARC/host teardown,model correctness or a committed model token.
struct PioneerQ38MixedLiveReadbackResult {
 uint32_t version=1,started=0,pages=0,words=0,completed=0;
};
enum {TT_Q38_MIXED_LIVE_READBACK_PAGES=5,TT_Q38_MIXED_LIVE_READBACK_WORDS=5120};
static_assert(sizeof(PioneerQ38MixedLiveReadbackResult)==20,"fixed validation progress DATA");
#endif
