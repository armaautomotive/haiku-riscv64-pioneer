/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_QWEN35_FLEET_PLAN_H
#define PIONEER_QWEN35_FLEET_PLAN_H

#include "PioneerQwen35PackedPlan.h"
#include "../../src/add-ons/kernel/drivers/misc/PioneerQwen35ProjectionShape.h"
#include <array>

struct PioneerQ35ProjectionBand {
	unsigned fragment, bank, firstRow, groups, worker, wave;
	uint32_t address, bytes;
};

// Construct the width-specific jobs for a projection from the same packed
// fragments used by the uploader. Jobs describe placement only. The owning
// ROOT must still prove channel, initialized range, and receipt per job.
inline bool PioneerQ35BuildProjection(const PioneerQwen35Gguf& source,
	const PioneerQwen35PackedPlan& plan, unsigned tensor, unsigned workers,
	std::vector<PioneerQ35ProjectionBand>& output)
{
	if (!plan.MatchesSource(source) || tensor>=source.Tensors().size()
		|| !workers || workers>119)
		return false;
	const auto& t=source.Tensors()[tensor];
	if (!PioneerQwen35Projection::Supports(t.type,t.inputs) || (t.outputs&15))
		return false;
	const unsigned rowBytes=PioneerQ35TiledRowBytes(t.type,t.inputs);
	const unsigned groupBytes=rowBytes*16;
	if (!groupBytes || (groupBytes&63))
		return false;

	struct Fragment {
		unsigned index, groups, jobs, stagedGroups=0, stagedJobs=0;
	};
	std::array<Fragment,7> fragments{};
	std::array<bool,7> present{};
	const auto& parts=plan.Parts();
	unsigned firstRow=0, totalJobs=0;
	for (unsigned index=0;index<parts.size();index++) {
		const auto& part=parts[index];
		if (part.kind!=PIONEER_Q35_WEIGHT || part.tensor!=tensor)
			continue;
		if (part.bank>=7 || present[part.bank] || part.firstRow!=firstRow
			|| !part.rows || (part.rows&15) || part.rowBytes!=rowBytes
			|| uint64_t(part.rows)*rowBytes!=part.bytes || part.bytes>part.allocated
			|| part.proposedAddress<plan.Base || (part.proposedAddress&4095)
			|| uint64_t(part.proposedAddress)-plan.Base+part.allocated>plan.Capacity)
			return false;
		const unsigned groups=part.rows/16;
		const unsigned jobs=(groups+15)/16;
		fragments[part.bank]={index,groups,jobs};
		present[part.bank]=true;
		firstRow+=part.rows;
		totalJobs+=jobs;
	}
	if (!totalJobs || firstRow!=t.outputs)
		return false;

	// Use idle workers for smaller bands, biased towards the bank with
	// the most groups per job. A single job never crosses a bank boundary.
	const unsigned active=std::min(workers,firstRow/16);
	while (totalJobs<active) {
		Fragment* best=nullptr;
		for (auto& fragment:fragments) {
			if (fragment.groups==fragment.jobs)
				continue;
			if (!best || fragment.groups*best->jobs>best->groups*fragment.jobs)
				best=&fragment;
		}
		if (!best)
			return false;
		best->jobs++;
		totalJobs++;
	}

	std::vector<PioneerQ35ProjectionBand> staged;
	staged.reserve(totalJobs);
	while (staged.size()<totalJobs) {
		for (unsigned bank=0;bank<fragments.size();bank++) {
			auto& fragment=fragments[bank];
			if (fragment.stagedJobs==fragment.jobs)
				continue;
			const auto& part=parts[fragment.index];
			const unsigned groups=fragment.groups/fragment.jobs
				+ unsigned(fragment.stagedJobs<fragment.groups%fragment.jobs);
			const unsigned offset=fragment.stagedGroups*groupBytes;
			const unsigned bytes=groups*groupBytes;
			if (!groups || groups>16 || offset>=part.bytes || bytes>part.bytes-offset)
				return false;
			const unsigned slot=unsigned(staged.size());
			staged.push_back({fragment.index, bank, part.firstRow+fragment.stagedGroups*16,
				groups, slot%workers, slot/workers, part.proposedAddress+offset, bytes});
			fragment.stagedGroups+=groups;
			fragment.stagedJobs++;
		}
	}
	for (const auto& fragment:fragments)
		if (fragment.stagedGroups!=fragment.groups)
			return false;
	if (!plan.MatchesSource(source))
		return false;
	output.swap(staged);
	return true;
}
#endif
