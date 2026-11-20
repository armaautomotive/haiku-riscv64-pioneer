/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_POLL_SENTINEL_H
#define PIONEER_Q38_POLL_SENTINEL_H
#ifndef PIONEER_ATTENTION_FREESTANDING
#include <stdint.h>
#endif
// CPU metadata ONLY. A likely late worker is a scheduling hint,NEVER proof
// that ANY other worker completed. Every original status/PC/reset audit is
// still required. Ties choose the LAST-launched maximum-group worker.
static inline bool PioneerQ38SelectPollSentinel(const uint32_t (*parts)[8],unsigned workers,
	unsigned& output,unsigned maxGroups=16) {
 if(!parts || !workers || workers>119 || !maxGroups)return false;
 unsigned selected=0;
 for(unsigned w=0;w<workers;w++) {
  if(!parts[w][2] || parts[w][2]>maxGroups)return false;
  if(parts[w][2]>=parts[selected][2])selected=w;
 }
 output=selected;return true;
}
// Keep the existing 65,536-attempt window for 1..16-group work. Larger
// reduction shards need proportionally more time to finish, while remaining
// bounded by the caller's shared all-worker poll budget.
static inline unsigned PioneerQ38PollSentinelBudget(unsigned groups,unsigned groupsPerBudget=16) {
 if(!groups || !groupsPerBudget || groups>0xffffffffU-(groupsPerBudget-1))return 0;
 unsigned scale=(groups+groupsPerBudget-1)/groupsPerBudget;
 if(scale>0xffffffffU/65536U)return 0;
 return scale*65536U;
}
static inline unsigned PioneerQ38PollBackoffNext(unsigned value) {
 return value<64 || value>2048 || (value&(value-1))?0:value<2048?value*2:2048;
}
#endif
