/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_QWEN35_DATAFLOW_SILU_PROGRAM_H
#define PIONEER_QWEN35_DATAFLOW_SILU_PROGRAM_H

#include "PioneerQwen35DataflowProgram.h"
#include "PioneerQwen35DataflowSilu.h"

namespace PioneerQwen35 {

enum : unsigned {
	DataflowSiluMagic=0x314c5335,
	DataflowSiluVersion=1,
	DataflowSiluGroups=Ffn/16,
	DataflowSiluRemainderCores=DataflowSiluGroups%DataflowCoreCount,
	DataflowSiluBaseGroups=DataflowSiluGroups/DataflowCoreCount,
	DataflowSiluMaxCoreSegments=DataflowSiluBaseGroups+1
};

template<class T>
inline void DataflowSiluZeroWords(T& value)
{
	static_assert(sizeof(T)%sizeof(uint32_t)==0,
		"SiLU invocation records contain whole words");
	uint32_t* words=reinterpret_cast<uint32_t*>(&value);
	for (unsigned i=0;i<sizeof(T)/sizeof(uint32_t);i++) words[i]=0;
}

constexpr unsigned DataflowSiluFirstGroup(unsigned core)
{
	return core*DataflowSiluBaseGroups
		+(core<DataflowSiluRemainderCores?core:DataflowSiluRemainderCores);
}

constexpr unsigned DataflowSiluGroupsForCore(unsigned core)
{
	return DataflowSiluBaseGroups
		+unsigned(core<DataflowSiluRemainderCores);
}

struct DataflowSiluProgram {
	unsigned gateSlot;
	unsigned bankRoute[2][Banks];
	RowSpace gate,up;

	bool Valid() const
	{
		return gateSlot<ActivationBuffers && DataflowBankRoutesValid(bankRoute)
			&& gate.Valid() && gate.rows==Ffn && gate.rowBytes==sizeof(float)
			&& gate.unit==16 && up.Valid() && up.rows==Ffn
			&& up.rowBytes==sizeof(float) && up.unit==16
			&& DataflowGraphWorkspace::Disjoint(gate,up);
	}
};

struct DataflowSiluInvocation {
	unsigned magic,version,core,gateSlot,segmentCount;
	unsigned bankRoute[2][Banks];
	RowSpace gate,up;
	DataflowOutputSegment segment[DataflowSiluMaxCoreSegments];

	bool Valid() const
	{
		if (magic!=DataflowSiluMagic || version!=DataflowSiluVersion
			|| core>=DataflowCoreCount || gateSlot>=ActivationBuffers
			|| !DataflowBankRoutesValid(bankRoute) || !gate.Valid()
			|| gate.rows!=Ffn || gate.rowBytes!=sizeof(float) || gate.unit!=16
			|| !up.Valid() || up.rows!=Ffn || up.rowBytes!=sizeof(float)
			|| up.unit!=16 || !DataflowGraphWorkspace::Disjoint(gate,up)
			|| segmentCount!=DataflowSiluGroupsForCore(core)) return false;
		const unsigned firstGroup=DataflowSiluFirstGroup(core);
		for (unsigned i=0;i<segmentCount;i++) {
			const auto& output=segment[i];
			const unsigned firstRow=(firstGroup+i)*16;
			Range gateRange{},upRange{};
			if (output.firstRow!=firstRow || output.rows!=16
				|| output.bytes!=16*sizeof(float)
				|| !gate.Resolve(firstRow,16,0,output.bytes,gateRange)
				|| output.bank!=gateRange.bank || output.address!=gateRange.address
				|| !up.Resolve(firstRow,16,0,output.bytes,upRange)) return false;
		}
		for (unsigned i=segmentCount;i<DataflowSiluMaxCoreSegments;i++)
			if (segment[i].bank || segment[i].address || segment[i].firstRow
				|| segment[i].rows || segment[i].bytes) return false;
		return true;
	}
};

class DataflowSiluProgramBuilder {
public:
	static bool Build(const DeviceLayout& manifest,
		const DataflowGraphWorkspace& workspace,unsigned gateSlot,
		const unsigned (&bankRoute)[2][Banks],DataflowSiluProgram& result)
	{
		DataflowSiluZeroWords(result);
		if (manifest.magic!=LayoutMagic || manifest.version!=LayoutVersion
			|| !manifest.context || gateSlot>=ActivationBuffers
			|| !workspace.Valid(manifest) || !DataflowBankRoutesValid(bankRoute))
			return false;
		const RowSpace& gate=manifest.activation[gateSlot];
		const RowSpace& up=workspace.space[DataflowGraphUpWorkspaceIndex];
		if (!gate.Valid() || gate.rows<Ffn || gate.rowBytes!=sizeof(float)
			|| gate.unit!=16 || !up.Valid() || up.rows!=Ffn
			|| up.rowBytes!=sizeof(float) || up.unit!=16
			|| !DataflowGraphWorkspace::Disjoint(gate,up)) return false;
		DataflowSiluProgram candidate;
		DataflowSiluZeroWords(candidate);
		candidate.gateSlot=gateSlot;
		for (unsigned noc=0;noc<2;noc++) for (unsigned bank=0;bank<Banks;bank++)
			candidate.bankRoute[noc][bank]=bankRoute[noc][bank];
		candidate.gate=gate;
		candidate.up=up;
		if (!candidate.Valid()) return false;
		result=candidate;
		return true;
	}

	static bool BindCore(const DataflowSiluProgram& program,unsigned core,
		DataflowSiluInvocation& result)
	{
		if (!program.Valid() || core>=DataflowCoreCount) return false;
		DataflowSiluInvocation candidate;
		DataflowSiluZeroWords(candidate);
		candidate.magic=DataflowSiluMagic;
		candidate.version=DataflowSiluVersion;
		candidate.core=core;
		candidate.gateSlot=program.gateSlot;
		candidate.segmentCount=DataflowSiluGroupsForCore(core);
		for (unsigned noc=0;noc<2;noc++) for (unsigned bank=0;bank<Banks;bank++)
			candidate.bankRoute[noc][bank]=program.bankRoute[noc][bank];
		candidate.gate=program.gate;
		candidate.up=program.up;
		const unsigned firstGroup=DataflowSiluFirstGroup(core);
		for (unsigned i=0;i<candidate.segmentCount;i++) {
			const unsigned row=(firstGroup+i)*16;
			Range output{};
			if (!program.gate.Resolve(row,16,0,16*sizeof(float),output)) return false;
			candidate.segment[i]={output.bank,output.address,row,16,16*sizeof(float)};
		}
		if (!candidate.Valid()) return false;
		result=candidate;
		return true;
	}
};

static_assert(sizeof(DataflowSiluInvocation)<=DataflowProgramRegionBytes,
	"the SiLU invocation fits the resident L1 command slot");
static_assert(DataflowSiluGroupsForCore(DataflowCoreCount-1)
	==DataflowSiluBaseGroups,
	"the contiguous 120-core schedule covers every FFN panel");

} // namespace PioneerQwen35

#endif
