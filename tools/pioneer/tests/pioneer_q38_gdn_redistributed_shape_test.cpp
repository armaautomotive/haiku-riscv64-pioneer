/* SPDX-License-Identifier: MIT */
// Candidate descriptor geometry only. No launch, ownership, initialization,
// dispatch, native capability, emitted-image, or performance qualification.
#include <assert.h>
#include <stdint.h>
#define PIONEER_ATTENTION_FREESTANDING
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38ResidentGdnRedistributedShape.h"

int main()
{
#ifdef PIONEER_Q38_RGDN_ROOT_QUALIFICATION
	uint32_t rootDescriptor[64] = {};
	assert(!PioneerQ38ResidentGdnRedistributedShape(rootDescriptor));
	return 0;
#else
	unsigned stateUse[PioneerQ38GdnTaskPlacement::PhysicalWorkers] = {};
	for (uint32_t layer = 0; layer < PioneerQ38GdnTaskPlacement::RecurrentLayers; ++layer) {
		uint32_t peers[PioneerQ38GdnTaskPlacement::LogicalWorkers] = {};
		for (uint32_t logical = 0; logical < PioneerQ38GdnTaskPlacement::LogicalWorkers; ++logical) {
			PioneerQ38GdnTaskPlacement::Route route = {};
			assert(PioneerQ38GdnTaskPlacement::Map(layer, logical, route));
			assert(route.layerOrdinal == layer && route.actualLayer == PioneerQ38GdnTaskPlacement::ActualLayer(layer));
			assert(route.logicalWorker == logical && route.physicalWorker >= 1);
			assert(route.physicalWorker <= PioneerQ38GdnTaskPlacement::PhysicalWorkers);
			assert(route.stateIndex < PioneerQ38GdnTaskPlacement::TotalTasks);
			++stateUse[route.physicalWorker - 1];
			for (uint32_t previous = 0; previous < logical; ++previous)
				assert(peers[previous] != route.peer);
			peers[logical] = route.peer;
			uint32_t recovered = ~uint32_t(0);
			assert(PioneerQ38GdnTaskPlacement::LogicalWorker(route.actualLayer,
				route.physicalWorker, recovered));
			assert(recovered == logical);

			uint32_t d[64] = {};
			d[0] = TT_Q38_RGDN_MARKER; d[1] = 1; d[2] = 128; d[3] = 64;
			d[4] = 0; d[5] = 0x04000000; d[6] = 6;
			d[7] = 0xc4000000 - TT_Q38_RGDN_STATE_BYTES;
			d[8] = 1; d[9] = logical; d[10] = route.actualLayer;
			d[12] = PioneerQ38SpanRoute(d[4]); d[13] = PioneerQ38SpanRoute(d[6]);
			d[14] = TT_Q38_RGDN_STATE_BYTES; d[15] = route.peer; d[16] = 1;
			for (unsigned epoch = 32; epoch < 40; epoch += 2)
				d[epoch] = 1;
			assert(PioneerQ38ResidentGdnRedistributedShape(d));

			uint32_t savedPeer = d[15];
			d[15] = PioneerQ38ResidentGdnPeer(logical);
			assert(PioneerQ38ResidentGdnRedistributedShape(d)
				== (route.physicalWorker == logical + 1));
			d[15] = savedPeer;
			d[10] = route.actualLayer + 1;
			assert(!PioneerQ38ResidentGdnRedistributedShape(d));
			d[10] = route.actualLayer;
			d[15] ^= 1;
			assert(!PioneerQ38ResidentGdnRedistributedShape(d));
		}
	}
	for (unsigned peer = 0; peer < PioneerQ38GdnTaskPlacement::PhysicalWorkers; ++peer)
		assert(stateUse[peer] == PioneerQ38GdnTaskPlacement::StateSlots(peer + 1));
	assert(PioneerQ38GdnTaskPlacement::Peer(0) == ~uint32_t(0));
	assert(PioneerQ38GdnTaskPlacement::Peer(120) == ~uint32_t(0));
	PioneerQ38GdnTaskPlacement::Route invalid = {};
	assert(!PioneerQ38GdnTaskPlacement::Map(48, 0, invalid));
	assert(!PioneerQ38GdnTaskPlacement::Map(0, 96, invalid));
	return 0;
#endif
}
