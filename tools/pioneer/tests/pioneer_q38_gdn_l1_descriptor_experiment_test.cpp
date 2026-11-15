/* SPDX-License-Identifier: MIT */
// Descriptor-extension checks only; no allocator authority, launch or device IO.
#define PIONEER_Q38_GDN_SCATTERED_STATE_EXPERIMENT 1
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38ResidentGdnShape.h"
#include <assert.h>
#include <stdio.h>

static void Fill(uint32_t* d)
{
	for (unsigned i = 0; i < 64; ++i) d[i] = 0;
	d[0] = TT_Q38_RGDN_MARKER; d[1] = 1; d[2] = 128; d[3] = 64;
	d[4] = 0; d[5] = 0x04000000; d[6] = 1; d[7] = 0xc3ff8000;
	d[8] = 1; d[9] = 0; d[10] = 0; d[11] = 0;
	d[12] = PioneerQ38SpanRoute(d[4]); d[13] = PioneerQ38SpanRoute(d[6]);
	d[14] = TT_Q38_RGDN_STATE_BYTES; d[15] = PioneerQ38ResidentGdnPeer(0); d[16] = 1;
	d[32] = d[34] = d[36] = d[38] = 1;
}

int main()
{
	uint32_t d[64]; Fill(d);
	assert(PioneerQ38ResidentGdnShapePeer(d, d[15])); // legacy tail safely spills
	assert(PioneerQ38ResidentGdnCacheMode(d) == 2);
	unsigned cases = 1;
	for (unsigned mode = 0; mode <= 1; ++mode) {
		Fill(d); d[50] = mode;
		for (unsigned i = 0; i < 8; ++i) d[42 + i] = 0x90000 + i * 0x1000;
		assert(PioneerQ38ResidentGdnCacheDescriptor(d));
		assert(PioneerQ38ResidentGdnShapePeer(d, d[15]));
		assert(PioneerQ38ResidentGdnCacheMode(d) == mode); cases++;
		for (unsigned bad = 0; bad < 5; ++bad) {
			uint32_t candidate[64];
			for (unsigned i = 0; i < 64; ++i) candidate[i] = d[i];
			if (bad == 0) candidate[42] = candidate[43];
			else if (bad == 1) candidate[42]++;
			else if (bad == 2) candidate[42] = 0x8f000;
			else if (bad == 3) candidate[42] = 0x17f001;
			else candidate[50] = 3;
			assert(!PioneerQ38ResidentGdnCacheDescriptor(candidate));
			assert(!PioneerQ38ResidentGdnShapePeer(candidate, candidate[15]));
		}
	}
	Fill(d); d[50] = 2;
	assert(PioneerQ38ResidentGdnCacheDescriptor(d));
	assert(PioneerQ38ResidentGdnCacheMode(d) == 2); cases++;
	d[42] = 0x90000; assert(!PioneerQ38ResidentGdnCacheDescriptor(d));
	Fill(d); d[51] = 1; assert(!PioneerQ38ResidentGdnShapePeer(d, d[15]));
	printf("PASS scattered GDN descriptor extension: cold/warm pages, forced legacy spill, spill mode, duplicate/unaligned/out-of-range/mode/tail refusal (%u cases); DATA ONLY\n", cases);
	return 0;
}
