/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_QWEN35_DATAFLOW_CONV_PROGRAM_H
#define PIONEER_QWEN35_DATAFLOW_CONV_PROGRAM_H

#include "PioneerQwen35DataflowGraphWorkspace.h"
#include "PioneerQwen35DataflowProgram.h"

namespace PioneerQwen35 {

enum : unsigned {
	DataflowConvMagic=0x31435635,
	DataflowConvVersion=1,
	DataflowConvGroups=ConvChannels/16,
	DataflowConvBaseGroups=DataflowConvGroups/DataflowCoreCount,
	DataflowConvRemainderGroups=DataflowConvGroups%DataflowCoreCount,
	DataflowConvMaxGroupsPerCore=DataflowConvBaseGroups+1,
	DataflowConvHistoryBytes=16*3*sizeof(float),
	DataflowConvWeightBytes=16*4*sizeof(float),
	DataflowConvPanelBytes=16*sizeof(float)
};

constexpr unsigned DataflowConvFirstGroup(unsigned core)
{
	return core*DataflowConvBaseGroups
		+(core<DataflowConvRemainderGroups?core:DataflowConvRemainderGroups);
}

constexpr unsigned DataflowConvGroupsForCore(unsigned core)
{
	return DataflowConvBaseGroups+unsigned(core<DataflowConvRemainderGroups);
}

struct DataflowConvProgram {
	unsigned layer,position;
	unsigned bankRoute[2][Banks];
	RowSpace qkv,historyRead,historyWrite;
	TensorSpace weights;

	bool ShapeValid() const
	{
		if (layer>=Layers || Dense(layer) || position>=MaxPositions
			|| !DataflowBankRoutesValid(bankRoute)
			|| !qkv.Valid() || qkv.rows!=ConvChannels
			|| qkv.rowBytes!=sizeof(float) || qkv.unit!=16
			|| !weights.place.Valid() || weights.type!=0 || weights.inputs!=4
			|| weights.place.rows!=ConvChannels
			|| weights.place.rowBytes!=4*sizeof(float)
			|| weights.place.unit!=1
			|| !historyRead.Valid() || historyRead.rows!=ConvChannels
			|| historyRead.rowBytes!=3*sizeof(float) || historyRead.unit!=1
			|| !historyWrite.Valid() || historyWrite.rows!=ConvChannels
			|| historyWrite.rowBytes!=3*sizeof(float) || historyWrite.unit!=1
			|| !DataflowGraphWorkspace::Disjoint(qkv,historyRead)
			|| !DataflowGraphWorkspace::Disjoint(qkv,historyWrite)
			|| !DataflowGraphWorkspace::Disjoint(qkv,weights.place)
			|| !DataflowGraphWorkspace::Disjoint(historyRead,historyWrite)
			|| !DataflowGraphWorkspace::Disjoint(weights.place,historyRead)
			|| !DataflowGraphWorkspace::Disjoint(weights.place,historyWrite))
			return false;
		return true;
	}

	bool Valid() const
	{
		return ShapeValid() && DataflowConvRangesValid(0,DataflowConvGroups);
	}

	bool DataflowConvRangesValid(unsigned firstGroup,unsigned groups) const
	{
		if (!groups || firstGroup>=DataflowConvGroups
			|| groups>DataflowConvGroups-firstGroup) return false;
		for (unsigned group=firstGroup;group<firstGroup+groups;group++) {
			const unsigned row=group*16;
			Range input{},weight{},read{},write{};
			if (!qkv.Resolve(row,16,0,DataflowConvPanelBytes,input)
				|| !weights.place.Resolve(row,16,0,DataflowConvWeightBytes,weight)
				|| !historyRead.Resolve(row,16,0,DataflowConvHistoryBytes,read)
				|| !historyWrite.Resolve(row,16,0,DataflowConvHistoryBytes,write)
				|| !DataflowConvRangeAligned(input,DataflowConvPanelBytes)
				|| !DataflowConvRangeAligned(weight,DataflowConvWeightBytes)
				|| !DataflowConvRangeAligned(read,DataflowConvHistoryBytes)
				|| !DataflowConvRangeAligned(write,DataflowConvHistoryBytes))
				return false;
		}
		return true;
	}

	static bool DataflowConvRangeAligned(const Range& range,unsigned bytes)
	{
		return range.bank<Banks && range.bytes==bytes
			&& range.address>=BankBase && range.address<BankLimit
			&& bytes<=BankLimit-range.address && !(range.address&63)
			&& !(bytes&63);
	}
};

struct DataflowConvInvocation {
	unsigned magic,version,core,layer,position,firstGroup,groupCount;
	unsigned bankRoute[2][Banks];
	RowSpace qkv,historyRead,historyWrite;
	TensorSpace weights;

	bool Valid() const
	{
		if (magic!=DataflowConvMagic || version!=DataflowConvVersion
			|| core>=DataflowCoreCount) return false;
		DataflowConvProgram program;
		program.layer=layer;
		program.position=position;
		for (unsigned noc=0;noc<2;noc++) for (unsigned bank=0;bank<Banks;bank++)
			program.bankRoute[noc][bank]=bankRoute[noc][bank];
		program.qkv=qkv;
		program.historyRead=historyRead;
		program.historyWrite=historyWrite;
		program.weights=weights;
		return program.ShapeValid()
			&& firstGroup==DataflowConvFirstGroup(core)
			&& groupCount==DataflowConvGroupsForCore(core)
			&& program.DataflowConvRangesValid(firstGroup,groupCount);
	}
};

class DataflowConvProgramBuilder {
public:
	static bool Build(const DeviceLayout& manifest,
		const DataflowGraphWorkspace& workspace,unsigned layer,unsigned position,
		const unsigned (&bankRoute)[2][Banks],DataflowConvProgram& result)
	{
		result={};
		if (manifest.magic!=LayoutMagic || manifest.version!=LayoutVersion
			|| !manifest.context || position>=manifest.context
			|| position>=MaxPositions || layer>=Layers || Dense(layer)
			|| !workspace.Valid(manifest) || !DataflowBankRoutesValid(bankRoute))
			return false;
		const TensorSpace* weights=manifest.Weight(layer,ConvWeights);
		const unsigned readSlot=position&1;
		const unsigned writeSlot=readSlot^1;
		if (!weights || weights->type!=0 || weights->inputs!=4
			|| weights->place.rows!=ConvChannels
			|| weights->place.rowBytes!=4*sizeof(float)
			|| weights->place.unit!=1) return false;
		DataflowConvProgram candidate{};
		candidate.layer=layer;
		candidate.position=position;
		for (unsigned noc=0;noc<2;noc++) for (unsigned bank=0;bank<Banks;bank++)
			candidate.bankRoute[noc][bank]=bankRoute[noc][bank];
		candidate.qkv=workspace.space[DataflowGraphRecurrentQkvWorkspaceIndex];
		candidate.weights=*weights;
		candidate.historyRead=manifest.layers[layer].conv[readSlot];
		candidate.historyWrite=manifest.layers[layer].conv[writeSlot];
		if (!candidate.Valid()) return false;
		result=candidate;
		return true;
	}

	static bool BindCore(const DataflowConvProgram& program,unsigned core,
		DataflowConvInvocation& result)
	{
		if (!program.Valid() || core>=DataflowCoreCount) return false;
		DataflowConvInvocation candidate{};
		candidate.magic=DataflowConvMagic;
		candidate.version=DataflowConvVersion;
		candidate.core=core;
		candidate.layer=program.layer;
		candidate.position=program.position;
		candidate.firstGroup=DataflowConvFirstGroup(core);
		candidate.groupCount=DataflowConvGroupsForCore(core);
		for (unsigned noc=0;noc<2;noc++) for (unsigned bank=0;bank<Banks;bank++)
			candidate.bankRoute[noc][bank]=program.bankRoute[noc][bank];
		candidate.qkv=program.qkv;
		candidate.historyRead=program.historyRead;
		candidate.historyWrite=program.historyWrite;
		candidate.weights=program.weights;
		if (!candidate.Valid()) return false;
		result=candidate;
		return true;
	}
};

static_assert(DataflowConvGroups==512 && DataflowConvBaseGroups==4
	&& DataflowConvRemainderGroups==32,
	"convolution panels cover all 120 workers with no idle Tensix cores");
static_assert(sizeof(DataflowConvInvocation)<=DataflowProgramRegionBytes,
	"the recurrent convolution invocation fits the resident L1 command slot");

} // namespace PioneerQwen35

#endif
