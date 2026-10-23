/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_PROJECTION_RESULT_LAYOUT_H
#define PIONEER_Q38_PROJECTION_RESULT_LAYOUT_H
#ifndef PIONEER_ATTENTION_FREESTANDING
#include <stdint.h>
#endif
// Fixed first-layer hybrid-attention output DATA only,not ownership or a
// producer/source/read capability. All four results survive on card until
// the real state consumer uses them. No copies through CPU vectors.
enum {TT_Q38_PROJECTION_RESULT_BASE=1048576,TT_Q38_PROJECTION_RESULT_BYTES=73728};
static inline unsigned PioneerQ38ProjectionResultSlot(unsigned role){
 return role==10?0:role==11?1:role==5?2:role==6?3:4;
}
static inline unsigned PioneerQ38ProjectionResultRows(unsigned role){
 return role==10 || role==11?48:role==5?6144:role==6?10240:0;
}
static inline uint32_t PioneerQ38ProjectionResultOffset(unsigned role){
 return role==10?uint32_t(TT_Q38_PROJECTION_RESULT_BASE):role==11?TT_Q38_PROJECTION_RESULT_BASE+4096:
  role==5?TT_Q38_PROJECTION_RESULT_BASE+8192:role==6?TT_Q38_PROJECTION_RESULT_BASE+32768:~uint32_t(0);
}
static_assert(TT_Q38_PROJECTION_RESULT_BASE+TT_Q38_PROJECTION_RESULT_BYTES<=2097152,"ALL result slots inside actual zero-initialized prefix");
static_assert(8192+6144*4==32768 && 32768+10240*4==TT_Q38_PROJECTION_RESULT_BYTES,"fixed disjoint page-aligned complete outputs");
#endif
