/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_QWEN35_MANIFEST_VALIDATION_H
#define PIONEER_QWEN35_MANIFEST_VALIDATION_H

#include "PioneerQwen35DeviceLayout.h"
#include "PioneerQwen35DataflowGraphWorkspace.h"
#include "PioneerQwen35Roles.h"

namespace PioneerQwen35 {

// Validate the root's entire writable address space before it can see a
// manifest. Each bank starts empty and advances monotonically through
// immutable weights, uninitialized sequence state and (in bank 0) RoPE.
template<class IsInitialized>
bool ValidateManifest(const DeviceLayout& manifest, Range rope, IsInitialized&& initialized,
	unsigned uploadedPages,const RowSpace* dataflowScratch=nullptr,
	unsigned dataflowScratchCount=0)
{
	constexpr unsigned page=4096;
	if (manifest.magic!=LayoutMagic || manifest.version!=LayoutVersion || manifest.reserved
		|| !manifest.context || manifest.context>MaxPositions)
		return false;
	unsigned end[Banks]={BankBase,BankBase,BankBase,BankBase,BankBase,BankBase,BankBase};
	unsigned immutablePages=0;
	auto extent=[&](unsigned bank, unsigned address, unsigned bytes, bool immutable) {
		const unsigned allocated=(bytes+page-1)&~(page-1);
		if (bank>=Banks || address!=end[bank] || !allocated || allocated>BankLimit-address)
			return false;
		for (unsigned at=0; at<allocated; at+=page)
			if (initialized(bank,(address-BankBase+at)/page)!=immutable) return false;
		end[bank]+=allocated;
		if (immutable) immutablePages+=allocated/page;
		return true;
	};
	auto rows=[&](const RowSpace& space, unsigned n, unsigned rowBytes, unsigned unit,
		bool immutable) {
		if (space.rows!=n || space.rowBytes!=rowBytes || space.unit!=unit || !space.Valid())
			return false;
		for (unsigned i=0; i<space.count; i++) {
			const auto& part=space.parts[i];
			if (!extent(part.bank,part.address,part.rows*space.rowBytes,immutable))
				return false;
		}
		return true;
	};
	unsigned formats[15]={};
	for (unsigned ordinal=0; ordinal<=Layers; ordinal++) {
		const unsigned layer=ordinal?ordinal-1:unsigned(Layers);
		for (unsigned role=0;; role++) {
			const auto* expected=PioneerQ35Role(layer,role);
			if (!expected) break;
			const auto* weight=manifest.Weight(layer,role);
			if (!weight || weight->inputs!=expected->inputs
				|| (weight->type!=expected->type && weight->type!=expected->alternate))
				return false;
			unsigned rowBytes=expected->inputs*4, unit=1;
			if (weight->type) {
				if (!PioneerQwen35Projection::Supports(weight->type,weight->inputs)) return false;
				const unsigned blocks=weight->inputs>>(weight->type==8?5:8);
				rowBytes=blocks*(PioneerQwen35Projection::TileBytes(weight->type)/16);
				unit=16;
			}
			if (!rows(weight->place,expected->outputs,rowBytes,unit,true)) return false;
			formats[weight->type]++;
		}
	}
	if (formats[0]!=177 || formats[8]!=48 || formats[12]!=131 || formats[13]!=48 || formats[14]!=22)
		return false;
	for (unsigned layer=0; layer<Layers; layer++) {
		const auto& space=manifest.layers[layer];
		if (Dense(layer)) {
			if (!rows(space.kv,2*DenseKeyHeads,manifest.context*DenseHeadWidth*2,1,false))
				return false;
			continue;
		}
		for (unsigned slot=0; slot<2; slot++)
			if (!rows(space.delta[slot],DeltaHeads,DeltaStateWords*4,1,false)
				|| !rows(space.conv[slot],ConvChannels,3*4,1,false))
				return false;
	}
	unsigned workspaceStart[Banks];
	for (unsigned bank=0;bank<Banks;bank++) workspaceStart[bank]=end[bank];
	if (rope.bank!=0 || rope.bytes!=manifest.context*Rotary*4
		|| !extent(rope.bank,rope.address,rope.bytes,true)) return false;
	for (unsigned slot=0;slot<ActivationBuffers;slot++)
		if (!rows(manifest.activation[slot],Ffn,sizeof(float),16,false)) return false;
	if (dataflowScratchCount && (!dataflowScratch
		|| dataflowScratchCount!=DataflowGraphWorkspaceBufferCount))
		return false;
	if (dataflowScratchCount)
		for (unsigned i=0;i<DataflowGraphWorkspaceBufferCount;i++)
			if (!rows(dataflowScratch[i],DataflowGraphWorkspaceRows(i),
				sizeof(float),16,false)) return false;
	for (unsigned bank=0;bank<Banks;bank++)
		if (end[bank]<workspaceStart[bank]
			|| end[bank]-workspaceStart[bank]>ActivationWorkspaceBytes) return false;
	return uploadedPages==immutablePages;
}
}
#endif
