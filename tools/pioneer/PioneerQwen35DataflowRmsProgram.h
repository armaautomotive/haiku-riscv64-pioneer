/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_QWEN35_DATAFLOW_RMS_PROGRAM_H
#define PIONEER_QWEN35_DATAFLOW_RMS_PROGRAM_H

#include "PioneerQwen35DataflowProgram.h"
#include "PioneerQwen35DataflowRms.h"

namespace PioneerQwen35 {

enum : unsigned {
	DataflowRmsMagic=0x314d5235,
	DataflowRmsVersion=1,
	DataflowRmsGroups=Hidden/16,
	DataflowRmsMaxCoreSegments=2,
	DataflowRmsSplitCores=DataflowRmsGroups-DataflowCoreCount
};

template<class T>
inline void DataflowRmsZeroWords(T& value)
{
	static_assert(sizeof(T)%sizeof(uint32_t)==0,
		"device RMS records contain whole words");
	uint32_t* words=reinterpret_cast<uint32_t*>(&value);
	for (unsigned i=0;i<sizeof(T)/sizeof(uint32_t);i++) words[i]=0;
}

inline bool DataflowRmsRoleValid(unsigned layer,unsigned role)
{
	return layer==Layers?role==FinalNorm
		:layer<Layers && (role==AttentionNorm || role==PostAttentionNorm);
}

inline bool DataflowRmsSpacesDisjoint(const RowSpace& a,const RowSpace& b)
{
	if (!a.Valid() || !b.Valid()) return false;
	for (unsigned i=0;i<a.count;i++) for (unsigned j=0;j<b.count;j++) {
		const auto& x=a.parts[i];
		const auto& y=b.parts[j];
		if (x.bank!=y.bank) continue;
		const uint64_t xEnd=uint64_t(x.address)+uint64_t(x.rows)*a.rowBytes;
		const uint64_t yEnd=uint64_t(y.address)+uint64_t(y.rows)*b.rowBytes;
		if (x.address<yEnd && y.address<xEnd) return false;
	}
	return true;
}

struct DataflowRmsProgram {
	unsigned layer,role,inputSlot,outputSlot,scratchRow;
	unsigned bankRoute[2][Banks];
	RowSpace input,weight,output;

	bool Valid() const
	{
		if (!DataflowRmsRoleValid(layer,role)
			|| inputSlot>=ActivationBuffers || outputSlot>=ActivationBuffers
			|| inputSlot==outputSlot || !DataflowBankRoutesValid(bankRoute)
			|| !input.Valid() || input.rows<Hidden || input.rowBytes!=sizeof(float)
			|| input.unit!=16 || !weight.Valid() || weight.rows!=1
			|| weight.rowBytes!=Hidden*sizeof(float) || weight.unit!=1
			|| !output.Valid() || output.rows<Ffn
			|| output.rowBytes!=sizeof(float) || output.unit!=16
			|| scratchRow<Hidden || (scratchRow&15)
			|| scratchRow>output.rows-16
			|| !DataflowRmsSpacesDisjoint(input,output)
			|| !DataflowRmsSpacesDisjoint(weight,input)
			|| !DataflowRmsSpacesDisjoint(weight,output)) return false;
		Range scratch{};
		return output.Resolve(scratchRow,16,0,16*sizeof(float),scratch);
	}
};

struct DataflowRmsInvocation {
	unsigned magic,version,layer,role,core,inputSlot,outputSlot;
	unsigned scratchRow,segmentCount;
	unsigned bankRoute[2][Banks];
	RowSpace input,weight,output;
	DataflowOutputSegment segment[DataflowRmsMaxCoreSegments];

	bool Valid() const
	{
		if (magic!=DataflowRmsMagic || version!=DataflowRmsVersion
			|| core>=DataflowCoreCount) return false;
		DataflowRmsProgram program;
		DataflowRmsZeroWords(program);
		program.layer=layer;
		program.role=role;
		program.inputSlot=inputSlot;
		program.outputSlot=outputSlot;
		program.scratchRow=scratchRow;
		for (unsigned noc=0;noc<2;noc++) for (unsigned bank=0;bank<Banks;bank++)
			program.bankRoute[noc][bank]=bankRoute[noc][bank];
		program.input=input;
		program.weight=weight;
		program.output=output;
		if (!program.Valid()
			|| segmentCount!=(core<DataflowRmsSplitCores?2U:1U)) return false;
		const unsigned firstGroup=core<DataflowRmsSplitCores
			?core*2:core+DataflowRmsSplitCores;
		for (unsigned i=0;i<segmentCount;i++) {
			const auto& outputPart=segment[i];
			const unsigned row=(firstGroup+i)*16;
			Range expected{};
			if (row>=Hidden || !output.Resolve(row,16,0,16*sizeof(float),expected)
				|| outputPart.bank!=expected.bank
				|| outputPart.address!=expected.address
				|| outputPart.firstRow!=row || outputPart.rows!=16
				|| outputPart.bytes!=16*sizeof(float)) return false;
		}
		for (unsigned i=segmentCount;i<DataflowRmsMaxCoreSegments;i++)
			if (segment[i].bank || segment[i].address || segment[i].firstRow
				|| segment[i].rows || segment[i].bytes) return false;
		return true;
	}
};

class DataflowRmsProgramBuilder {
public:
	static bool Build(const DeviceLayout& manifest,unsigned layer,unsigned role,
		unsigned inputSlot,unsigned outputSlot,const unsigned (&bankRoute)[2][Banks],
		DataflowRmsProgram& result)
	{
		DataflowRmsZeroWords(result);
		if (manifest.magic!=LayoutMagic || manifest.version!=LayoutVersion
			|| !manifest.context || !DataflowRmsRoleValid(layer,role)
			|| inputSlot>=ActivationBuffers || outputSlot>=ActivationBuffers
			|| inputSlot==outputSlot || !DataflowBankRoutesValid(bankRoute)) return false;
		const TensorSpace* weight=manifest.Weight(layer,role);
		const RowSpace& input=manifest.activation[inputSlot];
		const RowSpace& output=manifest.activation[outputSlot];
		if (!weight || weight->type!=0 || weight->inputs!=Hidden
			|| !weight->place.Valid() || weight->place.rows!=1
			|| weight->place.rowBytes!=Hidden*sizeof(float)
			|| weight->place.unit!=1 || !input.Valid() || input.rows<Hidden
			|| input.rowBytes!=sizeof(float) || input.unit!=16
			|| !output.Valid() || output.rows<Ffn
			|| output.rowBytes!=sizeof(float) || output.unit!=16
			|| !DataflowRmsSpacesDisjoint(input,output)
			|| !DataflowRmsSpacesDisjoint(weight->place,input)
			|| !DataflowRmsSpacesDisjoint(weight->place,output)) return false;
		unsigned scratchRow=Hidden;
		Range scratch{};
		for (;scratchRow<=output.rows-16;scratchRow+=16)
			if (output.Resolve(scratchRow,16,0,16*sizeof(float),scratch)) break;
		if (scratchRow>output.rows-16) return false;
		DataflowRmsProgram candidate;
		DataflowRmsZeroWords(candidate);
		candidate.layer=layer;
		candidate.role=role;
		candidate.inputSlot=inputSlot;
		candidate.outputSlot=outputSlot;
		candidate.scratchRow=scratchRow;
		for (unsigned noc=0;noc<2;noc++) for (unsigned bank=0;bank<Banks;bank++)
			candidate.bankRoute[noc][bank]=bankRoute[noc][bank];
		candidate.input=input;
		candidate.weight=weight->place;
		candidate.output=output;
		if (!candidate.Valid()) return false;
		result=candidate;
		return true;
	}

	static bool BindCore(const DataflowRmsProgram& program,unsigned core,
		DataflowRmsInvocation& result)
	{
		if (!program.Valid() || core>=DataflowCoreCount) return false;
		DataflowRmsInvocation candidate;
		DataflowRmsZeroWords(candidate);
		candidate.magic=DataflowRmsMagic;
		candidate.version=DataflowRmsVersion;
		candidate.layer=program.layer;
		candidate.role=program.role;
		candidate.core=core;
		candidate.inputSlot=program.inputSlot;
		candidate.outputSlot=program.outputSlot;
		candidate.scratchRow=program.scratchRow;
		candidate.segmentCount=core<DataflowRmsSplitCores?2:1;
		for (unsigned noc=0;noc<2;noc++) for (unsigned bank=0;bank<Banks;bank++)
			candidate.bankRoute[noc][bank]=program.bankRoute[noc][bank];
		candidate.input=program.input;
		candidate.weight=program.weight;
		candidate.output=program.output;
		const unsigned firstGroup=core<DataflowRmsSplitCores
			?core*2:core+DataflowRmsSplitCores;
		for (unsigned i=0;i<candidate.segmentCount;i++) {
			const unsigned firstRow=(firstGroup+i)*16;
			Range destination{};
			if (!program.output.Resolve(firstRow,16,0,16*sizeof(float),destination))
				return false;
			candidate.segment[i]={destination.bank,destination.address,
				firstRow,16,16*sizeof(float)};
		}
		if (!candidate.Valid()) return false;
		result=candidate;
		return true;
	}
};

static_assert(sizeof(DataflowRmsInvocation)<=DataflowProgramRegionBytes,
	"the RMS invocation fits the resident L1 command slot");

} // namespace PioneerQwen35

#endif
