/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_COMPACT_OUTPUT_H
#define PIONEER_Q38_COMPACT_OUTPUT_H
#ifndef PIONEER_ATTENTION_FREESTANDING
#include <stdint.h>
#endif
// OFFLINE layout arithmetic ONLY. Not host-address/DMA ownership authority.
// Each peer contributes ALL of its 16..256 active FP32 rows,not a padded
// 256-word slot. The complete extent stays inside the EXISTING 119KiB L1
// result range and existing separately owned1MiB host mapping.
enum { PIONEER_Q38_COMPACT_OUTPUT_MAGIC=0x324f3851,
       PIONEER_Q38_COMPACT_OUTPUT_WORDS=119*256 };
static inline bool PioneerQ38CompactOutputAppend(unsigned groups,uint32_t& words)
{
	if (!groups || groups>16 || words>PIONEER_Q38_COMPACT_OUTPUT_WORDS
		|| (words&15) || groups*16>PIONEER_Q38_COMPACT_OUTPUT_WORDS-words) return false;
	words+=groups*16; return true;
}
static inline unsigned PioneerQ38CompactOutputBytes(unsigned peers,uint32_t words)
{
	return peers && peers<=119 && !(words&15) && words>=peers*16 && words<=peers*256?words*4:0;
}
#endif
