/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_ATTENTION_FIXTURE_H
#define PIONEER_ATTENTION_FIXTURE_H
#include "PioneerAttentionMath.h"
// Little-endian public-test fixture, not a kernel/device ABI. Payload order:
// Q float32[16][128], K/V float16[tokens][8][128], result float32[16][128].
struct PioneerAttentionFixture {
	uint32_t magic, version, layer, tokens, width, heads, kvHeads;
	uint32_t queryCount, kvCount, resultCount, scaleBits, reserved;
};
static_assert(sizeof(PioneerAttentionFixture) == 48, "fixture layout");
static inline bool PioneerAttentionFixtureValid(const PioneerAttentionFixture& h)
{
	return h.magic == 0x4e545450 && h.version == 1 && h.layer < PIONEER_ATTN_LAYERS
		&& h.tokens >= 1 && h.tokens <= PIONEER_ATTN_TOKENS && h.width == PIONEER_ATTN_WIDTH
		&& h.heads == PIONEER_ATTN_HEADS && h.kvHeads == PIONEER_ATTN_KV_HEADS
		&& h.queryCount == PIONEER_ATTN_HEADS * PIONEER_ATTN_WIDTH
		&& h.kvCount == h.tokens * PIONEER_ATTN_KV_HEADS * PIONEER_ATTN_WIDTH
		&& h.resultCount == h.queryCount && h.scaleBits == PioneerAttentionBits(0.0883883461356163f) && !h.reserved;
}
#endif
