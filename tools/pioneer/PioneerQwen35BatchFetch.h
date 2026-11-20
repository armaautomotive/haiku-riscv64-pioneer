/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_QWEN35_BATCH_FETCH_H
#define PIONEER_QWEN35_BATCH_FETCH_H
#ifndef PIONEER_ATTENTION_FREESTANDING
#include <stdint.h>
#else
typedef __UINT32_TYPE__ uint32_t;
#endif
#include "PioneerQwen35Projection.h"

// Bounded metadata for an offline Qwen3.5 Work candidate. This describes
// original-layout reads only; it grants no source, ownership, NoC or
// completion authority. The caller must retain its existing source seal,
// idle/response proof and byte counters.
struct PioneerQwen35BatchFetchPlan {
	uint32_t source=0, destination=0, bytes=0, copySource=0, copyWords=0, tileBytes=0;
	bool issue=false;
};

static inline unsigned PioneerQwen35FetchBatch(unsigned type)
{
	return type==8?4U:type==12?3U:(type==13 || type==14)?2U:0U;
}

static inline unsigned PioneerQwen35FetchOrdinal(unsigned type,unsigned block)
{
	return type==8?block>>2:type==12?(block*0xaaabU)>>17:block>>1;
}

static inline unsigned PioneerQwen35FetchCount(unsigned type,unsigned blocks)
{
	const unsigned batch=PioneerQwen35FetchBatch(type);
	return batch?PioneerQwen35FetchOrdinal(type,blocks+batch-1):0;
}

static inline bool PioneerQwen35BatchFetch(unsigned type,unsigned inputs,unsigned groups,
	unsigned group,unsigned block,uint32_t first,PioneerQwen35BatchFetchPlan& output)
{
	const unsigned batch=PioneerQwen35FetchBatch(type);
	const unsigned tile=PioneerQwen35Projection::TileBytes(type);
	if (!batch || !PioneerQwen35Projection::Supports(type,inputs) || !tile
		|| !groups || groups>16 || group>=groups
		|| first<0x04000000 || first>=0xc4000000 || (first&63)) return false;
	const unsigned blocks=inputs>>(type==8?5:8);
	if (block>=blocks) return false;
	const uint32_t total=groups*blocks*tile;
	if (total>0xc4000000-first) return false;
	PioneerQwen35BatchFetchPlan p;
	const unsigned ordinal=PioneerQwen35FetchOrdinal(type,block);
	const unsigned firstBlock=ordinal*batch;
	const unsigned batchBlocks=blocks-firstBlock<batch?blocks-firstBlock:batch;
	p.source=first+(group*blocks+firstBlock)*tile;
	p.destination=0x98000;
	p.bytes=tile*batchBlocks;
	p.copySource=p.destination+(block-firstBlock)*tile;
	p.copyWords=tile/4;
	p.tileBytes=tile;
	p.issue=block==firstBlock;
	// Keep each new NoC read <=8 KiB and inside the fixed 0x98000 scratch
	// window. The caller copies one completed original tile to 0x90000 before
	// running the unchanged checked matrix path.
	if (p.bytes>8192 || p.destination+p.bytes>0x9a000
		|| p.copySource+tile>p.destination+p.bytes
		|| p.source<first || p.source>0xc4000000-p.bytes
		|| p.source+p.bytes>first+total) return false;
	output=p;
	return true;
}
#endif
