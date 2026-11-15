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
	const PioneerQ38GdnBanks* state = nullptr;
	uint32_t descriptor[64] = {};
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
		state = &owner;
		ready = input.BuildRedistributed(canonical, layer, logicalWorker, position,
			bank, id, source, phase)
			&& Describe(owner, id, input.Descriptor(), descriptor);
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
		return true;
	}

	bool LearnedPagesPresent(const uint8_t* pages, unsigned count) const
	{
		return ready && input.LearnedPagesPresent(pages, count);
	}

	const uint32_t* InputDescriptor() const
	{
		return ready ? input.Descriptor() : nullptr;
	}

	const uint32_t* StateDescriptor() const
	{
		return ready ? descriptor : nullptr;
	}
};
#endif
