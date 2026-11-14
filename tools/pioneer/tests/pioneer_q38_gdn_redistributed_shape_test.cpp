/* SPDX-License-Identifier: MIT */
// Candidate descriptor geometry only. No launch, ownership, initialization,
// dispatch, native capability, emitted-image, or performance qualification.
#include <assert.h>
#include <stdint.h>
#define PIONEER_ATTENTION_FREESTANDING
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38ResidentGdnRedistributedShape.h"

#ifndef PIONEER_Q38_RGDN_ROOT_QUALIFICATION
static void FillInput(uint32_t* d, uint32_t layer, uint32_t logical, uint32_t peer)
{
	for (unsigned i = 0; i < 64; ++i) d[i] = 0;
	d[0] = TT_Q38_GDN_INPUT_MARKER; d[1] = 1; d[2] = 67108864;
	d[3] = 1; d[4] = 0x04000000; d[5] = layer; d[6] = 1;
	d[7] = logical; d[8] = PioneerQ38ProjectionRoute(d[3]); d[9] = peer;
	d[10] = logical + 1; d[11] = 2; d[12] = 0x0c000000;
	d[13] = PioneerQ38ProjectionRoute(d[11]); d[14] = 3; d[15] = 0x10000000;
	d[16] = 1; d[40] = 0; d[41] = PioneerQ38ProjectionRoute(d[14]);
	const unsigned h = logical / 2, k = h & 15, block = (h >> 4) * 64;
	d[42] = TT_Q38_CONV_OUTPUT + k * 512;
	d[43] = TT_Q38_CONV_OUTPUT + 8192 + k * 512;
	d[44] = TT_Q38_CONV_OUTPUT + 16384 + h * 512 + (logical & 1) * 256;
	d[45] = PioneerQ38ProjectionResultOffset(10) + block;
	d[46] = PioneerQ38ProjectionResultOffset(11) + block;
	d[47] = h & 15; d[48] = 512; d[49] = 512; d[50] = 256; d[51] = 64;
	d[52] = 7; d[53] = 24; d[54] = 1536; d[55] = 322;
	d[32] = d[34] = d[36] = d[38] = 1;
}

static void FillState(uint32_t* d, uint32_t layer, uint32_t logical, uint32_t peer)
{
	for (unsigned i = 0; i < 64; ++i) d[i] = 0;
	d[0] = TT_Q38_RGDN_MARKER; d[1] = 1; d[2] = 128; d[3] = 64;
	d[4] = 0; d[5] = 0x04000000; d[6] = 6; d[7] = 0xc3ff8000;
	d[8] = 1; d[9] = logical; d[10] = layer; d[11] = 0;
	d[12] = PioneerQ38SpanRoute(d[4]); d[13] = PioneerQ38SpanRoute(d[6]);
	d[14] = TT_Q38_RGDN_STATE_BYTES; d[15] = peer; d[16] = 1;
	d[32] = d[34] = d[36] = d[38] = 1;
}

static void FillGated(uint32_t* d, const uint32_t* input, const uint32_t* state,
	uint32_t logical, uint32_t peer, uint32_t siblingPeer)
{
	for (unsigned i = 0; i < 64; ++i) d[i] = 0;
	d[0] = TT_Q38_GDN_GATED_MARKER; d[1] = 1;
	for (unsigned i = 2; i <= 10; ++i) d[i] = input[i];
	d[9] = peer; d[11] = 4; d[12] = 0x14000000;
	d[13] = PioneerQ38ProjectionRoute(d[11]); d[14] = (logical ^ 1) + 1;
	d[15] = siblingPeer; d[40] = input[40]; d[41] = state[11];
	d[42] = PioneerQ38ProjectionResultOffset(5) + (logical / 2) * 512;
	d[43] = TT_Q38_GDN_GATED_OUTPUT + logical * 256;
	d[44] = TT_Q38_GDN_GATED_READY; d[45] = 0x8e800;
	d[46] = 128; d[47] = 64; d[48] = d[49] = 512;
	d[50] = 256; d[51] = TT_Q38_GDN_GATED_POLLS;
	for (unsigned i = 16; i < 40; ++i) d[i] = input[i];
}
#endif

int main()
{
#ifdef PIONEER_Q38_RGDN_ROOT_QUALIFICATION
	uint32_t rootDescriptor[64] = {};
	assert(!PioneerQ38ResidentGdnRedistributedShape(rootDescriptor));
	assert(!PioneerQ38GdnRedistributedInputShape(rootDescriptor));
	assert(!PioneerQ38GdnRedistributedPairing(rootDescriptor, rootDescriptor));
	assert(!PioneerQ38GdnRedistributedGatedShape(rootDescriptor, rootDescriptor, rootDescriptor));
	assert(!PioneerQ38GdnRedistributedRendezvousShape(rootDescriptor, rootDescriptor, rootDescriptor));
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
			uint32_t input[64], state[64];
			FillInput(input, route.actualLayer, logical, route.peer);
			FillState(state, route.actualLayer, logical, route.peer);
			assert(PioneerQ38GdnRedistributedInputShape(input));
			assert(PioneerQ38ResidentGdnRedistributedShape(state));
			assert(PioneerQ38GdnRedistributedPairing(input, state));
			uint32_t siblingRouteOrdinal = 0;
			assert(PioneerQ38GdnTaskPlacement::LayerOrdinal(route.actualLayer, siblingRouteOrdinal));
			PioneerQ38GdnTaskPlacement::Route sibling = {};
			assert(PioneerQ38GdnTaskPlacement::Map(siblingRouteOrdinal, logical ^ 1, sibling));
			uint32_t gated[64]; FillGated(gated, input, state, logical, route.peer, sibling.peer);
			assert(PioneerQ38GdnRedistributedGatedShape(gated, input, state));
			gated[0] = TT_Q38_GDN_RENDEZVOUS_MARKER;
			gated[51] = TT_Q38_GDN_RENDEZVOUS_POLLS;
			gated[52] = uint32_t(PioneerQ38GdnRendezvousBudget());
			gated[53] = uint32_t(PioneerQ38GdnRendezvousBudget() >> 32);
			gated[54] = 4; gated[55] = TT_Q38_GDN_RENDEZVOUS_CLOCK;
			assert(PioneerQ38GdnRedistributedRendezvousShape(gated, input, state));
			gated[15] ^= 1;
			assert(!PioneerQ38GdnRedistributedGatedShape(gated, input, state));
			gated[15] ^= 1;
			input[9] ^= 1;
			assert(!PioneerQ38GdnRedistributedInputShape(input));
			assert(!PioneerQ38GdnRedistributedPairing(input, state));
			input[9] ^= 1;

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
