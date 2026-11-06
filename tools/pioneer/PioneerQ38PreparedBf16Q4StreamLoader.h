/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_PREPARED_BF16_Q4_STREAM_LOADER_H
#define PIONEER_Q38_PREPARED_BF16_Q4_STREAM_LOADER_H
#include "PioneerQ38PreparedBf16Q4StreamLayout.h"
#include "PioneerQ38Q4FpuCompactStreamLoader.h"
// SAME bounded queue/counter/credit/visibility state machine, DISTINCT
// three-packet prepared DATA proposal. No native/Work/source authority.
class PioneerQ38PreparedBf16Q4StreamLoader {
 using L=PioneerQ38PreparedBf16Q4StreamLayout;
#define PIONEER_Q38_Q4_STREAM_LOADER_CLASS PioneerQ38PreparedBf16Q4StreamLoader
#include "PioneerQ38Q4StreamLoaderBody.inc"
#undef PIONEER_Q38_Q4_STREAM_LOADER_CLASS
};
#endif
