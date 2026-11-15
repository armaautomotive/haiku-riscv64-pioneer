/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_GDN_REDISTRIBUTED_RECURRENT_PLAN_H
#define PIONEER_Q38_GDN_REDISTRIBUTED_RECURRENT_PLAN_H
#include "PioneerQ38GdnBanks.h"
#include "PioneerQ38GdnInputPlan.h"
#include "PioneerQ38ResidentGdnRedistributedShape.h"

// DATA-only recurrent descriptor candidate for the alternate 119-peer map.
// It binds one logical half-head to the same physical peer for input and
// state, across any recurrent layer. It grants no launch, L1 allocation,
// state mutation, native-owner, or Work-mode authority.
class PioneerQ38GdnRedistributedRecurrentPlan {
	PioneerQ38GdnInputPlan input;
	const PioneerQ38CanonicalModelPlan* plan = nullptr;
	const PioneerQ38GdnBanks* state = nullptr;
	uint32_t descriptor[64] = {};
	uint32_t gatedDescriptor[64] = {};
	uint32_t rendezvousDescriptor[64] = {};
	bool attempted = false;
	bool ready = false;

	static bool Describe(const PioneerQ38GdnBanks& owner,
		const PioneerQ38GdnIdentity& id, const uint32_t* inputDescriptor,
		uint32_t* output)
	{
		if (!inputDescriptor || !PioneerQ38GdnRedistributedInputShape(inputDescriptor)
			|| !owner.InitializationPublished(id))
			return false;

		const unsigned layer = inputDescriptor[5];
		const unsigned logicalWorker = inputDescriptor[7];
		PioneerQ38GdnBankExtent current, next;
		if (!owner.InitializeHalf(id, layer, 0, logicalWorker, current)
			|| !owner.InitializeHalf(id, layer, 1, logicalWorker, next)
			|| current.bytes != TT_Q38_RGDN_STATE_BYTES || current.pages != 8
			|| next.bytes != TT_Q38_RGDN_STATE_BYTES || next.pages != 8)
			return false;
		return BuildDescriptorData(inputDescriptor, current, next, output);
	}

	static const PioneerQ38CanonicalPart* Norm(
		const PioneerQ38CanonicalModelPlan& canonical, unsigned layer)
	{
		if (!canonical.Ready() || layer >= 64 || layer % 4 == 3)
			return nullptr;
		const PioneerQ38CanonicalPart* norm = nullptr;
		for (unsigned i = 0; i < canonical.Count(); ++i) {
			const auto* part = canonical.Part(i);
			if (part->kind != canonical.WeightKind || part->layer != layer
				|| part->role != 9)
				continue;
			if (norm || part->type || part->firstRow || part->rows != 1
				|| part->rowBytes != 512 || part->bytes != 512
				|| part->allocated != 4096)
				return nullptr;
			norm = part;
		}
		return norm;
	}

public:
	// Pure descriptor constructor shared by the owner-bound planner and
	// exhaustive host tests. Extents remain inert metadata; this function does
	// not prove that they are reserved, initialized, mapped, or safe to access.
	static bool BuildDescriptorData(const uint32_t* inputDescriptor,
		const PioneerQ38GdnBankExtent& current,
		const PioneerQ38GdnBankExtent& next, uint32_t* output)
	{
		if (!inputDescriptor || !output
			|| !PioneerQ38GdnRedistributedInputShape(inputDescriptor)
			|| current.bytes != TT_Q38_RGDN_STATE_BYTES || current.pages != 8
			|| next.bytes != TT_Q38_RGDN_STATE_BYTES || next.pages != 8)
			return false;
		const unsigned layer = inputDescriptor[5];
		const unsigned logicalWorker = inputDescriptor[7];
		const unsigned phase = inputDescriptor[6];

		for (unsigned i = 0; i < 64; ++i)
			output[i] = 0;
		output[0] = TT_Q38_RGDN_MARKER;
		output[1] = 1;
		output[2] = 128;
		output[3] = 64;
		output[4] = current.bank;
		output[5] = current.address;
		output[6] = next.bank;
		output[7] = next.address;
		output[8] = phase;
		output[9] = logicalWorker;
		output[10] = layer;
		output[11] = 0; // candidate current slot; no token/slot flip
		output[12] = current.route;
		output[13] = next.route;
		output[14] = TT_Q38_RGDN_STATE_BYTES;
		output[15] = inputDescriptor[9];
		for (unsigned i = 16; i < 40; ++i)
			output[i] = inputDescriptor[i];
		output[40] = inputDescriptor[40];

		return PioneerQ38ResidentGdnRedistributedShape(output)
			&& PioneerQ38GdnRedistributedPairing(inputDescriptor, output);
	}

	static bool BuildGatedDescriptorData(
		const PioneerQ38CanonicalModelPlan& canonical,
		const uint32_t* inputDescriptor, const uint32_t* stateDescriptor,
		uint32_t* output)
	{
		if (!output || !PioneerQ38GdnRedistributedPairing(inputDescriptor,
			stateDescriptor))
			return false;
		const auto* workspace = canonical.Workspace(inputDescriptor[3]);
		const auto* norm = Norm(canonical, inputDescriptor[5]);
		uint32_t ordinal = 0;
		PioneerQ38GdnTaskPlacement::Route sibling = {};
		const unsigned siblingLogical = inputDescriptor[7] ^ 1;
		if (!workspace || canonical.Base + workspace->offset != inputDescriptor[4]
			|| !norm
			|| !PioneerQ38GdnTaskPlacement::LayerOrdinal(inputDescriptor[5], ordinal)
			|| !PioneerQ38GdnTaskPlacement::Map(ordinal, siblingLogical, sibling))
			return false;

		for (unsigned i = 0; i < 64; ++i)
			output[i] = inputDescriptor[i];
		output[0] = TT_Q38_GDN_GATED_MARKER;
		output[11] = norm->bank;
		output[12] = canonical.Base + norm->offset;
		output[13] = PioneerQ38ProjectionRoute(norm->bank);
		output[14] = siblingLogical + 1;
		output[15] = sibling.peer;
		output[41] = stateDescriptor[11];
		output[42] = PioneerQ38ProjectionResultOffset(5)
			+ (inputDescriptor[7] >> 1) * 512;
		output[43] = TT_Q38_GDN_GATED_OUTPUT + inputDescriptor[7] * 256;
		output[44] = TT_Q38_GDN_GATED_READY;
		output[45] = 0x8e800;
		output[46] = 128;
		output[47] = 64;
		output[48] = 512;
		output[49] = 512;
		output[50] = 256;
		output[51] = TT_Q38_GDN_GATED_POLLS;
		for (unsigned i = 52; i < 64; ++i)
			output[i] = 0;
		return PioneerQ38GdnRedistributedGatedShape(output, inputDescriptor,
			stateDescriptor);
	}

	static bool BuildRendezvousDescriptorData(const uint32_t* gated,
		const uint32_t* inputDescriptor, const uint32_t* stateDescriptor,
		uint32_t* output)
	{
		if (!gated || !output)
			return false;
		for (unsigned i = 0; i < 64; ++i)
			output[i] = gated[i];
		output[0] = TT_Q38_GDN_RENDEZVOUS_MARKER;
		output[51] = TT_Q38_GDN_RENDEZVOUS_POLLS;
		output[52] = uint32_t(PioneerQ38GdnRendezvousBudget());
		output[53] = uint32_t(PioneerQ38GdnRendezvousBudget() >> 32);
		output[54] = 4;
		output[55] = TT_Q38_GDN_RENDEZVOUS_CLOCK;
		return PioneerQ38GdnRedistributedRendezvousShape(output,
			inputDescriptor, stateDescriptor);
	}

	PioneerQ38GdnRedistributedRecurrentPlan() = default;
	PioneerQ38GdnRedistributedRecurrentPlan(
		const PioneerQ38GdnRedistributedRecurrentPlan&) = delete;
	PioneerQ38GdnRedistributedRecurrentPlan& operator=(
		const PioneerQ38GdnRedistributedRecurrentPlan&) = delete;

	bool Build(const PioneerQ38CanonicalModelPlan& canonical,
		const PioneerQ38GdnBanks& owner, unsigned layer, unsigned logicalWorker,
		unsigned position, unsigned bank, const PioneerQ38GdnIdentity& id,
		const uint32_t* source, unsigned phase)
	{
		if (attempted)
			return false;
		attempted = true;
		plan = &canonical;
		state = &owner;
		ready = input.BuildRedistributed(canonical, layer, logicalWorker, position,
			bank, id, source, phase)
			&& Describe(owner, id, input.Descriptor(), descriptor)
			&& BuildGatedDescriptorData(canonical, input.Descriptor(),
				descriptor, gatedDescriptor)
			&& BuildRendezvousDescriptorData(gatedDescriptor,
				input.Descriptor(), descriptor, rendezvousDescriptor);
		return ready;
	}

	bool Matches(const PioneerQ38CanonicalModelPlan& canonical,
		const PioneerQ38GdnBanks& owner, unsigned layer,
		unsigned logicalWorker, unsigned position, unsigned bank,
		const PioneerQ38GdnIdentity& id, const uint32_t* source,
		unsigned phase) const
	{
		if (!ready || state != &owner
			|| !input.MatchesRedistributed(canonical, layer, logicalWorker,
				position, bank, id, source, phase))
			return false;
		uint32_t current[64];
		if (!Describe(owner, id, input.Descriptor(), current))
			return false;
		for (unsigned i = 0; i < 64; ++i)
			if (current[i] != descriptor[i])
				return false;
		uint32_t currentGated[64], currentRendezvous[64];
		return BuildGatedDescriptorData(canonical, input.Descriptor(), current,
			currentGated)
			&& BuildRendezvousDescriptorData(currentGated, input.Descriptor(),
				current, currentRendezvous)
			&& [&]() {
				for (unsigned i = 0; i < 64; ++i)
					if (currentGated[i] != gatedDescriptor[i]
						|| currentRendezvous[i] != rendezvousDescriptor[i])
						return false;
				return true;
			}();
	}

	bool LearnedPagesPresent(const uint8_t* pages, unsigned count) const
	{
		if (!ready || !plan || !pages || count != PioneerResidentBanks::Pages
			|| !input.LearnedPagesPresent(pages, count))
			return false;
		if (plan->Capacity < 4096 || plan->Capacity % 4096 != 0
			|| gatedDescriptor[11] >= TT_DRAM_BANKS
			|| gatedDescriptor[12] < plan->Base)
			return false;
		const uint64_t offset = uint64_t(gatedDescriptor[12]) - plan->Base;
		if (offset >= plan->Capacity || offset % 4096 != 0)
			return false;
		const uint64_t page = uint64_t(gatedDescriptor[11])
			* (plan->Capacity / 4096) + offset / 4096;
		if (page >= count)
			return false;
		return pages[page] == 1;
	}

	const uint32_t* InputDescriptor() const
	{
		return ready ? input.Descriptor() : nullptr;
	}

	const uint32_t* StateDescriptor() const
	{
		return ready ? descriptor : nullptr;
	}

	const uint32_t* GatedDescriptor() const
	{
		return ready ? gatedDescriptor : nullptr;
	}

	const uint32_t* RendezvousDescriptor() const
	{
		return ready ? rendezvousDescriptor : nullptr;
	}
};
#endif
