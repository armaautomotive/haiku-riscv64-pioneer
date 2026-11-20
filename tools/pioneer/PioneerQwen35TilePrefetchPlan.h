/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_QWEN35_TILE_PREFETCH_PLAN_H
#define PIONEER_QWEN35_TILE_PREFETCH_PLAN_H

#include "PioneerQwen35Projection.h"

// Data-only plan for the checked Qwen3.5-4B peer. The caller still issues
// each NoC read and proves its exact completion before consuming that buffer.
struct PioneerQ35TilePrefetchPlan {
	__UINT32_TYPE__ currentSource=0,currentRaw=0,nextSource=0,nextRaw=0,bytes=0;
	bool next=false;

	static bool Describe(unsigned type,unsigned inputs,unsigned groups,unsigned group,
		unsigned block,__UINT32_TYPE__ first,PioneerQ35TilePrefetchPlan& output)
	{
		if (!PioneerQwen35Projection::Supports(type,inputs) || !groups || groups>16
			|| group>=groups || first<0x04000000 || (first&63) || first>=0xc4000000)
			return false;
		const unsigned bytes=PioneerQwen35Projection::TileBytes(type);
		const unsigned blocks=inputs>>(type==8?5:8);
		if (!bytes || !blocks || block>=blocks) return false;
		const __UINT32_TYPE__ total=groups*blocks*bytes;
		if (total>0xc4000000-first) return false;
		PioneerQ35TilePrefetchPlan plan;
		plan.currentSource=first+(group*blocks+block)*bytes;
		plan.currentRaw=(block&1)?0x98000:0x90000;
		plan.bytes=bytes;
		plan.next=block+1<blocks;
		if (plan.next) {
			plan.nextSource=plan.currentSource+bytes;
			plan.nextRaw=(block&1)?0x90000:0x98000;
		}
		output=plan;
		return true;
	}
};

#endif
