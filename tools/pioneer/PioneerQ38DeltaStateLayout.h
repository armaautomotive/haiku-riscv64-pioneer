/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_DELTA_STATE_LAYOUT_H
#define PIONEER_Q38_DELTA_STATE_LAYOUT_H
#include <stdint.h>
// CPU-only checkpoint/debug adapter,NOT a transport or a per-token backend.
// Exact pinned GGML K=1/one-sequence state: [value_head][value][key].
// Proposed resident format: [value_head][half][key][value_within_half].
// Each half is contiguous32KiB for the local vector program. A future
// native owner must prove/init/reserve its bank extents independently;
// these ordinary pointers/indices confer NO NoC/physical/DMA authority.
enum { PIONEER_Q38_GDN_CHECKPOINT_WORDS=48*128*128,PIONEER_Q38_GDN_HALF_WORDS=128*64 };
static inline unsigned PioneerQ38GdnKeyHead(unsigned valueHead) { return valueHead<48?valueHead%16:~0U; }
static inline bool PioneerQ38GdnStateSeparate(const uint32_t* a,unsigned an,const uint32_t* b,unsigned bn)
{
	uintptr_t x=reinterpret_cast<uintptr_t>(a),y=reinterpret_cast<uintptr_t>(b),limit=~uintptr_t(0);
	return a && b && an<=limit/4 && bn<=limit/4 && x<=limit-uintptr_t(an)*4 && y<=limit-uintptr_t(bn)*4
		&& (x+uintptr_t(an)*4<=y || y+uintptr_t(bn)*4<=x);
}
static inline bool PioneerQ38GdnCheckpointValue(uint32_t bits)
{
	return (bits&0x7f800000)!=0x7f800000 && (!(bits&0x7fffffff) || (bits&0x7f800000));
}
static inline bool PioneerQ38GdnImportHalf(const uint32_t* ggml,unsigned count,unsigned head,unsigned half,
	uint32_t* local,unsigned capacity)
{
	if (count!=PIONEER_Q38_GDN_CHECKPOINT_WORDS || head>=48 || half>=2 || capacity!=PIONEER_Q38_GDN_HALF_WORDS
		|| !PioneerQ38GdnStateSeparate(ggml,count,local,capacity)) return false;
	// Validate the entire selected half BEFORE exposing even its first word.
	for (unsigned key=0;key<128;key++) for (unsigned value=0;value<64;value++)
		if (!PioneerQ38GdnCheckpointValue(ggml[head*16384+(half*64+value)*128+key])) return false;
	for (unsigned key=0;key<128;key++) for (unsigned value=0;value<64;value++)
		local[key*64+value]=ggml[head*16384+(half*64+value)*128+key];
	return true;
}
static inline bool PioneerQ38GdnExportHalf(const uint32_t* local,unsigned count,unsigned head,unsigned half,
	uint32_t* ggml,unsigned capacity)
{
	if (count!=PIONEER_Q38_GDN_HALF_WORDS || head>=48 || half>=2 || capacity!=PIONEER_Q38_GDN_CHECKPOINT_WORDS
		|| !PioneerQ38GdnStateSeparate(local,count,ggml,capacity)) return false;
	for (unsigned i=0;i<count;i++) if (!PioneerQ38GdnCheckpointValue(local[i])) return false;
	for (unsigned key=0;key<128;key++) for (unsigned value=0;value<64;value++)
		ggml[head*16384+(half*64+value)*128+key]=local[key*64+value];
	return true;
}
#endif
