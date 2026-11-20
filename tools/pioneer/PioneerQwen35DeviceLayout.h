/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_QWEN35_DEVICE_LAYOUT_H
#define PIONEER_QWEN35_DEVICE_LAYOUT_H

#include "PioneerQwen35Geometry.h"
#include "../../src/add-ons/kernel/drivers/misc/PioneerQwen35ProjectionShape.h"

namespace PioneerQwen35 {

enum : unsigned {
	LayerRoles=14, Banks=7, ActivationBuffers=2, ActivationWorkspaceBytes=64*1024*1024,
	BankBase=0x04000000, BankLimit=0xc4000000,
	LayoutMagic=0x314d3542, LayoutVersion=2
};
struct Range { unsigned bank, address, bytes; };
struct Fragment { unsigned bank, address, firstRow, rows; };
struct RowTransfer { Range range; unsigned localRow, rows; };

struct RowSpace {
	unsigned rows, rowBytes, count, unit;
	Fragment parts[Banks];

	bool Bounded() const
	{
		// Covers large vocabulary tables as well as 4 MiB K/V rows at
		// the maximum context. Products stay in a 32-bit word.
		return rows && rows<=Vocabulary && rowBytes && rowBytes<=MaxPositions*DenseHeadWidth*2
			&& (rowBytes<=16384 || rows<=32) && (unit==1 || unit==16)
			&& count && count<=Banks;
	}

	bool Valid() const
	{
		if (!Bounded() || (rows&(unit-1)))
			return false;
		unsigned first=0, seen=0;
		for (unsigned i=0; i<count; i++) {
			const auto& part=parts[i];
			if (part.bank>=Banks || seen&(1U<<part.bank) || (part.address&4095)
				|| part.address<BankBase || part.address>=BankLimit || part.firstRow!=first
				|| !part.rows || part.rows>rows-first || (part.rows&(unit-1))
				|| part.rows*rowBytes>BankLimit-part.address)
				return false;
			first+=part.rows;
			seen|=1U<<part.bank;
		}
		for (unsigned i=count; i<Banks; i++)
			if (parts[i].bank || parts[i].address || parts[i].firstRow || parts[i].rows) return false;
		return first==rows;
	}

	// A transfer never crosses a physical fragment. Projection callers
	// request whole 16-row groups. Scalar/state callers use one or more
	// logical rows and may request a slice within that interval.
	bool Resolve(unsigned row, unsigned n, unsigned offset, unsigned bytes, Range& range) const
	{
		if (!Bounded() || !n || ((row|n)&(unit-1)) || row>=rows || n>rows-row
			|| !bytes || offset>n*rowBytes || bytes>n*rowBytes-offset)
			return false;
		for (unsigned i=0; i<count; i++) {
			const auto& part=parts[i];
			if (row<part.firstRow || row-part.firstRow>=part.rows) continue;
			unsigned local=row-part.firstRow;
			if (n>part.rows-local || part.bank>=Banks || part.address<BankBase
				|| part.address>=BankLimit || part.rows>rows || part.rows*rowBytes>BankLimit-part.address)
				return false;
			range={part.bank,part.address+local*rowBytes+offset,bytes};
			return true;
		}
		return false;
	}
};

struct TensorSpace {
	unsigned type, inputs;
	RowSpace place;

	bool ResolveBand(unsigned first, unsigned groups, Range& range) const
	{
		if (!PioneerQwen35Projection::Supports(type,inputs) || place.unit!=16
			|| !groups || groups>16)
			return false;
		unsigned blocks=inputs>>(type==8?5:8);
		unsigned rowBytes=blocks*(PioneerQwen35Projection::TileBytes(type)/16);
		return place.rowBytes==rowBytes && place.Resolve(first,groups*16,0,groups*16*rowBytes,range);
	}
};
struct LayerSpace {
	TensorSpace weights[LayerRoles];
	RowSpace conv[2], delta[2], kv;
};
struct DeviceLayout {
	unsigned magic, version, context, reserved;
	TensorSpace tied, finalNorm;
	RowSpace activation[ActivationBuffers];
	LayerSpace layers[Layers];

	const TensorSpace* Weight(unsigned layer, unsigned role) const
	{
		if (layer==Layers) return role==TiedOutput?&tied:role==FinalNorm?&finalNorm:nullptr;
		if (layer>=Layers || role>=(Dense(layer)?11U:LayerRoles)) return nullptr;
		return &layers[layer].weights[role];
	}

	bool ResolveEmbedding(unsigned token, Range& range, unsigned& lane) const
	{
		if (tied.type!=14 || tied.inputs!=Hidden || tied.place.rows!=Vocabulary
			|| tied.place.rowBytes!=2120 || tied.place.unit!=16 || token>=Vocabulary)
			return false;
		if (!tied.place.Resolve(token&~15U,16,0,tied.place.rowBytes*16,range)) return false;
		lane=token&15;
		return true;
	}

	bool ResolveConv(unsigned layer, unsigned slot, unsigned channel, Range& range) const
	{
		if (layer>=Layers || Dense(layer) || slot>=2 || (channel&15)) return false;
		return layers[layer].conv[slot].Resolve(channel,16,0,16*3*4,range);
	}

	bool ResolveDelta(unsigned layer, unsigned slot, unsigned head, Range& range) const
	{
		if (layer>=Layers || Dense(layer) || slot>=2 || head>=DeltaHeads) return false;
		return layers[layer].delta[slot].Resolve(head,1,0,DeltaStateWords*4,range);
	}

	bool ResolveKv(unsigned layer, unsigned head, unsigned position, bool value, Range& range) const
	{
		constexpr unsigned bytes=DenseHeadWidth*2;
		if (!Dense(layer) || head>=DenseKeyHeads || position>=context || context>MaxPositions)
			return false;
		return layers[layer].kv.Resolve(head+(value?unsigned(DenseKeyHeads):0U),1,position*bytes,bytes,range);
	}

	bool ResolveActivationTransfers(unsigned slot,unsigned firstRow,unsigned rows,
		RowTransfer (&transfers)[Banks],unsigned& count) const
	{
		count=0;
		if (slot>=ActivationBuffers || !rows || ((firstRow|rows)&15)
			|| !activation[slot].Valid() || activation[slot].rowBytes!=sizeof(float)
			|| activation[slot].unit!=16 || firstRow>=activation[slot].rows
			|| rows>activation[slot].rows-firstRow) return false;
		const RowSpace& space=activation[slot];
		const unsigned requestEnd=firstRow+rows;
		unsigned covered=0;
		for (unsigned index=0;index<space.count;index++) {
			const Fragment& part=space.parts[index];
			const unsigned begin=firstRow>part.firstRow?firstRow:part.firstRow;
			const unsigned partEnd=part.firstRow+part.rows;
			const unsigned end=requestEnd<partEnd?requestEnd:partEnd;
			if (begin>=end) continue;
			if (((begin|end)&15) || count>=Banks) return false;
			transfers[count++]={{part.bank,
				part.address+(begin-part.firstRow)*space.rowBytes,
				(end-begin)*space.rowBytes},begin-firstRow,end-begin};
			covered+=end-begin;
		}
		return count && covered==rows;
	}
};
static_assert(sizeof(Fragment)==16 && sizeof(RowSpace)==128, "fixed model transport ABI");
static_assert(sizeof(DeviceLayout)==81952, "bounded immutable device manifest with two activation shards");
}
#endif
