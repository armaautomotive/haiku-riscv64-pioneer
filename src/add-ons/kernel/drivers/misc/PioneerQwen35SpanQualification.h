/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_QWEN35_SPAN_QUALIFICATION_H
#define PIONEER_QWEN35_SPAN_QUALIFICATION_H
#include "PioneerQ38SpanProtocol.h"

// Fixed, non-model transfer through the actual strict preload implementation.
// Exactly one page in each otherwise idle bank, in bank order. The caller must
// check this while holding the session lock, before making any device access.
static inline bool
PioneerQwen35SpanQualificationRequest(const tt_q38_span_upload& r, unsigned next)
{
    if (next >= 7 || r.version != 1 || r.reserved || r.bank != next
        || r.offset != 0 || r.bytes != 4096) return false;
    for (unsigned i = 0; i < 1024; ++i)
        if (r.data[i] != 0x7fc00000U + (i + next * 271U) * 0x9e3779b9U)
            return false;
    return true;
}
#endif
