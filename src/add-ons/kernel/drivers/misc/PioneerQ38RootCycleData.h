/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_ROOT_CYCLE_DATA_H
#define PIONEER_Q38_ROOT_CYCLE_DATA_H
#ifndef PIONEER_ATTENTION_FREESTANDING
#include <stdint.h>
#endif
// DATA-only diagnostic framing,NEVER a replacement for ANY existing
// source/counter/PC/nonce/owner proof. No IO,clock conversion or authority.
static inline uint64_t PioneerQ38RootCyclePhase(const uint32_t* stats,unsigned phase) {
	return !stats || phase>=3?0:(uint64_t(stats[59+phase*2])<<32)|stats[58+phase*2];
}
static inline bool PioneerQ38RootCycleDataValid(const uint32_t* stats) {
	return stats && stats[57]==0x31435951 && PioneerQ38RootCyclePhase(stats,0)
		&& PioneerQ38RootCyclePhase(stats,1) && PioneerQ38RootCyclePhase(stats,2);
}
#endif
