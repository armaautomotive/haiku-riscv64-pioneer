/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_QWEN35_DEVICE_LAYOUT_BUILD_H
#define PIONEER_QWEN35_DEVICE_LAYOUT_BUILD_H

#include "PioneerQwen35DeviceLayout.h"
#include "PioneerQwen35PackedPlan.h"
#include <memory>

namespace PioneerQwen35 {

// Serialize the same authenticated source layout that drives weight upload.
// Addresses are offsets in the dedicated reservation. The native owner
// still has to prove the corresponding ranges have been allocated and loaded.
inline bool BuildDeviceLayout(const PioneerQwen35Gguf& source,
	const PioneerQwen35PackedPlan& plan, unsigned context, DeviceLayout& output)
{
	if (!plan.MatchesSource(source) || !context || context>MaxPositions) return false;
	auto model=std::make_unique<DeviceLayout>();
	model->magic=LayoutMagic;
	model->version=LayoutVersion;
	model->context=context;
	for (const auto& part:plan.Parts()) {
		RowSpace* space=nullptr;
		unsigned rowCount=0, rowBytes=0, unit=1;
		switch (part.kind) {
			case PIONEER_Q35_WORKSPACE:
				continue;
			case PIONEER_Q35_WEIGHT: {
				if (part.tensor>=source.Tensors().size() || part.layer>Layers
					|| !model->Weight(part.layer,part.role)) return false;
				const auto& t=source.Tensors()[part.tensor];
				auto& target=part.layer==Layers
					? (part.role==TiedOutput?model->tied:model->finalNorm)
					: model->layers[part.layer].weights[part.role];
				target.type=t.type;
				target.inputs=unsigned(t.inputs);
				space=&target.place;
				rowCount=unsigned(t.outputs);
				if (t.type) {
					if (!PioneerQwen35Projection::Supports(t.type,t.inputs)) return false;
					rowBytes=PioneerQ35TiledRowBytes(t.type,t.inputs);
					unit=16;
				} else
					rowBytes=unsigned(t.inputs*4);
				break;
			}
			case PIONEER_Q35_CONV_STATE:
				if (part.layer>=Layers || Dense(part.layer) || part.role>=2) return false;
				space=&model->layers[part.layer].conv[part.role];
				rowCount=ConvChannels;
				rowBytes=3*4;
				break;
			case PIONEER_Q35_DELTA_STATE:
				if (part.layer>=Layers || Dense(part.layer) || part.role>=2) return false;
				space=&model->layers[part.layer].delta[part.role];
				rowCount=DeltaHeads;
				rowBytes=DeltaStateWords*4;
				break;
			case PIONEER_Q35_KV_STATE:
				if (!Dense(part.layer) || part.role) return false;
				space=&model->layers[part.layer].kv;
				rowCount=DenseKeyHeads*2;
				rowBytes=context*DenseHeadWidth*2;
				break;
			default:
				return false;
		}
		if (!rowBytes || part.rowBytes!=rowBytes || space->count>=Banks
			|| !part.rows || part.firstRow>=rowCount || part.rows>rowCount-part.firstRow
			|| part.bytes!=uint64_t(part.rows)*rowBytes || part.bytes>part.allocated
			|| part.allocated-part.bytes>=4096 || (part.allocated&4095)
			|| part.proposedAddress<BankBase || part.proposedAddress>=BankLimit
			|| part.allocated>BankLimit-part.proposedAddress)
			return false;
		space->rows=rowCount;
		space->rowBytes=rowBytes;
		space->unit=unit;
		space->parts[space->count++]={part.bank,part.proposedAddress,part.firstRow,part.rows};
	}
	if (!model->tied.place.Valid() || !model->finalNorm.place.Valid()) return false;
	for (unsigned layer=0; layer<Layers; layer++) {
		const auto& layout=model->layers[layer];
		for (unsigned role=0; role<(Dense(layer)?11U:LayerRoles); role++)
			if (!layout.weights[role].place.Valid()) return false;
		if (Dense(layer)) {
			if (!layout.kv.Valid()) return false;
		} else
			for (unsigned slot=0; slot<2; slot++)
				if (!layout.conv[slot].Valid() || !layout.delta[slot].Valid()) return false;
	}
	if (!plan.MatchesSource(source)) return false;
	output=*model;
	return true;
}
}
#endif
