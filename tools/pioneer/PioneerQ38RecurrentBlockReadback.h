/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_RECURRENT_BLOCK_READBACK_H
#define PIONEER_Q38_RECURRENT_BLOCK_READBACK_H
#include <stdint.h>
// Validation progress DATA only. completed proves capture+full L1 restore,
// NOT final native END,model correctness,state/token commit or Work.
struct PioneerQ38RecurrentBlockReadbackResult {
 uint32_t version=1,started=0,pages=0,words=0,completed=0;
};
enum {TT_Q38_RECURRENT_BLOCK_READBACK_PAGES=5,TT_Q38_RECURRENT_BLOCK_READBACK_WORDS=5120};
static_assert(sizeof(PioneerQ38RecurrentBlockReadbackResult)==20,"fixed private validation DATA");
#endif
