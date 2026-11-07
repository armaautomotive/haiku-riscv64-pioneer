/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_EARLY_JOB_SNAPSHOT_H
#define PIONEER_Q38_EARLY_JOB_SNAPSHOT_H
// DATA comparison only: no native, device, source-ownership or Work grant.
// Receipt validation separately checks all reserved words and worker boot.
// Compare every field that MatrixJob derives from the live descriptor/part,
// and every fixed ordinary-matrix field omitted by the receipt job grammar.
namespace PioneerQ38EarlyJobSnapshot {
using Word = __UINT32_TYPE__;
static inline bool Matches(const volatile Word* job, const Word* d,
 const volatile Word* part, unsigned worker, unsigned sequence,
 unsigned magic, unsigned route, unsigned tile) {
 if (!job || !d || !part) return false;
 if (job[0] != magic || job[1] != sequence || job[2] != 1 || job[3] != worker + 1
  || job[4] != route || job[5] != d[42] || job[6] != d[40] || job[7] != d[6]) return false;
 for (unsigned i = 0; i < 8; ++i) if (job[8+i] != d[32+i]) return false;
 return job[16] == 0x31483851 && job[17] == d[15] && job[18] == tile
  && !job[19] && !job[20] && job[21] == d[14] && job[22] == part[0]
  && job[23] == part[2] && job[24] == part[4] && job[25] == 1 && job[26] == 1;
}
}
#endif
