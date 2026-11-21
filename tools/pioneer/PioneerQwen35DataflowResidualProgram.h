/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_QWEN35_DATAFLOW_RESIDUAL_PROGRAM_H
#define PIONEER_QWEN35_DATAFLOW_RESIDUAL_PROGRAM_H

#include "PioneerQwen35DataflowProgram.h"
#include "PioneerQwen35DataflowGraphWorkspace.h"

namespace PioneerQwen35 {

enum : unsigned {
	DataflowResidualMagic=0x31535235,
	DataflowResidualVersion=1,
	DataflowResidualSave=1,
	DataflowResidualAdd=2,
	DataflowResidualGroups=Hidden/16,
	DataflowResidualSplitCores=DataflowResidualGroups-DataflowCoreCount,
	DataflowResidualMaxCoreSegments=2
};

template<class T>
inline void DataflowResidualZeroWords(T& value)
{
	static_assert(sizeof(T)%sizeof(uint32_t)==0,
		"residual invocation records contain whole words");
	uint32_t* words=reinterpret_cast<uint32_t*>(&value);
	for (unsigned i=0;i<sizeof(T)/sizeof(uint32_t);i++) words[i]=0;
}

inline unsigned DataflowResidualFirstGroup(unsigned core)
{
	return core<DataflowResidualSplitCores?core*2
		:core+DataflowResidualSplitCores;
}

inline unsigned DataflowResidualGroupsForCore(unsigned core)
{
	return core<DataflowResidualSplitCores?2U:1U;
}

struct DataflowResidualProgram {
	unsigned mode,activationSlot;
	unsigned bankRoute[2][Banks];
	RowSpace activation,residual;

	bool Valid() const
	{
		return (mode==DataflowResidualSave || mode==DataflowResidualAdd)
			&& activationSlot<ActivationBuffers
			&& DataflowBankRoutesValid(bankRoute)
			&& activation.Valid() && activation.rows>=Hidden
			&& activation.rowBytes==sizeof(float) && activation.unit==16
			&& residual.Valid() && residual.rows==Hidden
			&& residual.rowBytes==sizeof(float) && residual.unit==16
			&& DataflowGraphWorkspace::Disjoint(activation,residual);
	}
};

struct DataflowResidualInvocation {
	unsigned magic,version,core,mode,activationSlot,segmentCount;
	unsigned bankRoute[2][Banks];
	RowSpace activation,residual;
	DataflowOutputSegment segment[DataflowResidualMaxCoreSegments];

	bool Valid() const
	{
		if (magic!=DataflowResidualMagic || version!=DataflowResidualVersion
			|| core>=DataflowCoreCount) return false;
		DataflowResidualProgram program;
		DataflowResidualZeroWords(program);
	program.mode=mode;
		program.activationSlot=activationSlot;
		for (unsigned noc=0;noc<2;noc++) for (unsigned bank=0;bank<Banks;bank++)
			program.bankRoute[noc][bank]=bankRoute[noc][bank];
		program.activation=activation;
		program.residual=residual;
		if (!program.Valid()
			|| segmentCount!=DataflowResidualGroupsForCore(core)) return false;
		const RowSpace& destination=mode==DataflowResidualSave?residual:activation;
		const unsigned firstGroup=DataflowResidualFirstGroup(core);
		for (unsigned i=0;i<segmentCount;i++) {
			const auto& output=segment[i];
			const unsigned row=(firstGroup+i)*16;
			Range expected{};
			if (row>=Hidden || !destination.Resolve(row,16,0,16*sizeof(float),expected)
				|| output.bank!=expected.bank || output.address!=expected.address
				|| output.firstRow!=row || output.rows!=16
				|| output.bytes!=16*sizeof(float)) return false;
		}
		for (unsigned i=segmentCount;i<DataflowResidualMaxCoreSegments;i++)
			if (segment[i].bank || segment[i].address || segment[i].firstRow
				|| segment[i].rows || segment[i].bytes) return false;
		return true;
	}
};

class DataflowResidualProgramBuilder {
public:
	static bool Build(const DeviceLayout& manifest,
		const DataflowGraphWorkspace& workspace,unsigned mode,unsigned activationSlot,
		const unsigned (&bankRoute)[2][Banks],DataflowResidualProgram& result)
	{
		DataflowResidualZeroWords(result);
		if (manifest.magic!=LayoutMagic || manifest.version!=LayoutVersion
			|| !manifest.context || (mode!=DataflowResidualSave
				&& mode!=DataflowResidualAdd)
			|| activationSlot>=ActivationBuffers || !workspace.Valid(manifest)
			|| !DataflowBankRoutesValid(bankRoute)) return false;
		const RowSpace& activation=manifest.activation[activationSlot];
		const RowSpace& residual=workspace.space[DataflowGraphResidualWorkspaceIndex];
		if (!activation.Valid() || activation.rows<Hidden
			|| activation.rowBytes!=sizeof(float) || activation.unit!=16
			|| !residual.Valid() || residual.rows!=Hidden
			|| residual.rowBytes!=sizeof(float) || residual.unit!=16
			|| !DataflowGraphWorkspace::Disjoint(activation,residual)) return false;
		DataflowResidualProgram candidate;
		DataflowResidualZeroWords(candidate);
		candidate.mode=mode;
		candidate.activationSlot=activationSlot;
		for (unsigned noc=0;noc<2;noc++) for (unsigned bank=0;bank<Banks;bank++)
			candidate.bankRoute[noc][bank]=bankRoute[noc][bank];
		candidate.activation=activation;
		candidate.residual=residual;
		if (!candidate.Valid()) return false;
		result=candidate;
		return true;
	}

	static bool BindCore(const DataflowResidualProgram& program,unsigned core,
		DataflowResidualInvocation& result)
	{
		if (!program.Valid() || core>=DataflowCoreCount) return false;
		DataflowResidualInvocation candidate;
		DataflowResidualZeroWords(candidate);
		candidate.magic=DataflowResidualMagic;
		candidate.version=DataflowResidualVersion;
		candidate.core=core;
		candidate.mode=program.mode;
		candidate.activationSlot=program.activationSlot;
		candidate.segmentCount=DataflowResidualGroupsForCore(core);
		for (unsigned noc=0;noc<2;noc++) for (unsigned bank=0;bank<Banks;bank++)
			candidate.bankRoute[noc][bank]=program.bankRoute[noc][bank];
		candidate.activation=program.activation;
		candidate.residual=program.residual;
		const RowSpace& destination=program.mode==DataflowResidualSave
			?program.residual:program.activation;
		const unsigned firstGroup=DataflowResidualFirstGroup(core);
		for (unsigned i=0;i<candidate.segmentCount;i++) {
			const unsigned row=(firstGroup+i)*16;
			Range range{};
			if (!destination.Resolve(row,16,0,16*sizeof(float),range)) return false;
			candidate.segment[i]={range.bank,range.address,row,16,16*sizeof(float)};
		}
		if (!candidate.Valid()) return false;
		result=candidate;
		return true;
	}
};

static_assert(DataflowResidualGroups==160
	&& DataflowResidualSplitCores==40,
	"hidden residual panels divide as 40 two-panel and 80 one-panel workers");

} // namespace PioneerQwen35

#endif
