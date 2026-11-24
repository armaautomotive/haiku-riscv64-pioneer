/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_QWEN35_SPAN_SUMMARY_H
#define PIONEER_QWEN35_SPAN_SUMMARY_H

#include "PioneerQ38SpanShape.h"
#ifndef PIONEER_Q38_SPAN_FAST
#error Qwen3.5 span summary requires the fast-span protocol
#endif

// Candidate summary for the separate Q35 preload image. The read-data-word
// counter is unsupported on Blackhole, so its four recorded copies must be
// zero; all other response, request, write-flit and identity checks remain.
static inline bool PioneerQwen35SpanSummary(const uint32_t* s,const uint32_t* d,
    const uint32_t* before,const uint32_t* after)
{
    if (!s || !PioneerQ38SpanShape(d) || !before || !after) return false;
    const unsigned bytes=d[4],chunks=(bytes+16383)/16384;
    const bool fast=d[2]==2;
    const unsigned reads=(fast?1:4)*chunks,writes=(fast?1:3)*chunks;
    const unsigned readFlits=(fast?1:4)*(bytes/64),writeFlits=(fast?1:3)*(bytes/64);
    if (s[0]!=reads || s[1]!=readFlits || s[2]!=writes || s[3]!=writeFlits
        || s[4]!=bytes/4096 || s[5]!=bytes || s[6]!=d[6] || s[7]!=TT_Q38_SPAN_MARKER
        || s[24]!=d[10] || s[25]!=d[11] || s[26]!=d[5] || s[27]!=d[7]
        || s[28]!=d[8] || s[29]!=d[9] || s[30]!=(fast?2U:1U) || s[31]
        || before[1] || after[1] || s[9] || s[17]
        || before[2]!=before[6] || after[2]!=after[6]) return false;
    const unsigned delta[]={reads,0,reads+writes,reads,writes,writeFlits,reads+writes,writes};
    for (unsigned i=0;i<8;i++) if (i!=1 && (s[8+i]!=before[i] || s[16+i]!=after[i]
        || uint32_t(after[i]-before[i])!=delta[i])) return false;
    return true;
}
#endif
