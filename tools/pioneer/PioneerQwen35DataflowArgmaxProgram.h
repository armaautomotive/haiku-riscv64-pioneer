/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_QWEN35_DATAFLOW_ARGMAX_PROGRAM_H
#define PIONEER_QWEN35_DATAFLOW_ARGMAX_PROGRAM_H

#include "PioneerQwen35DataflowProgram.h"

namespace PioneerQwen35 {

enum : unsigned {
	DataflowArgmaxMagic=0x314d4135,
	DataflowArgmaxVersion=1,
	DataflowArgmaxResultMagic=0x31524135
};

template<class T>
inline void DataflowArgmaxZeroWords(T& value)
{
	static_assert(sizeof(T)%sizeof(uint32_t)==0,
		"argmax records contain whole words");
	uint32_t* words=reinterpret_cast<uint32_t*>(&value);
	for (unsigned i=0;i<sizeof(T)/sizeof(uint32_t);i++) words[i]=0;
}

struct DataflowArgmaxProgram {
	unsigned magic,version;
	unsigned bankRoute[2][Banks];
	RowSpace logits;

	bool Valid() const
	{
		return magic==DataflowArgmaxMagic && version==DataflowArgmaxVersion
			&& DataflowBankRoutesValid(bankRoute) && logits.Valid()
			&& logits.rows==Vocabulary && logits.rowBytes==sizeof(float)
			&& logits.unit==16;
	}
};

struct DataflowArgmaxInvocation {
	unsigned magic,version,core,active;
	DataflowArgmaxProgram program;

	bool Valid() const
	{
		return magic==DataflowArgmaxMagic && version==DataflowArgmaxVersion
			&& core<DataflowCoreCount && active==unsigned(core==0)
			&& program.Valid();
	}
};

inline bool DataflowArgmaxMatchesWorkspace(const DeviceLayout& manifest,
	const DataflowGraphWorkspace& workspace,
	const unsigned (&auditedRoutes)[2][Banks],
	const DataflowArgmaxProgram& program)
{
	if (manifest.magic!=LayoutMagic || manifest.version!=LayoutVersion
		|| !manifest.context || !workspace.Valid(manifest) || !program.Valid()
		|| !DataflowBankRoutesValid(auditedRoutes)
		|| !DataflowRowSpaceMatches(
			workspace.space[DataflowGraphVocabularyWorkspaceIndex],program.logits))
		return false;
	for (unsigned noc=0;noc<2;noc++) for (unsigned bank=0;bank<Banks;bank++)
		if (program.bankRoute[noc][bank]!=auditedRoutes[noc][bank]) return false;
	return true;
}

class DataflowArgmaxProgramBuilder {
public:
	static bool Build(const DeviceLayout& manifest,
		const DataflowGraphWorkspace& workspace,
		const unsigned (&bankRoute)[2][Banks],DataflowArgmaxProgram& result)
	{
		DataflowArgmaxZeroWords(result);
		if (manifest.magic!=LayoutMagic || manifest.version!=LayoutVersion
			|| !manifest.context || !workspace.Valid(manifest)
			|| !DataflowBankRoutesValid(bankRoute)) return false;
		DataflowArgmaxProgram candidate;
		DataflowArgmaxZeroWords(candidate);
		candidate.magic=DataflowArgmaxMagic;
		candidate.version=DataflowArgmaxVersion;
		for (unsigned noc=0;noc<2;noc++) for (unsigned bank=0;bank<Banks;bank++)
			candidate.bankRoute[noc][bank]=bankRoute[noc][bank];
		candidate.logits=workspace.space[DataflowGraphVocabularyWorkspaceIndex];
		if (!candidate.Valid()) return false;
		result=candidate;
		return true;
	}

	static bool BindCore(const DataflowArgmaxProgram& program,unsigned core,
		DataflowArgmaxInvocation& result)
	{
		if (!program.Valid() || core>=DataflowCoreCount) return false;
		DataflowArgmaxInvocation candidate;
		DataflowArgmaxZeroWords(candidate);
		candidate.magic=DataflowArgmaxMagic;
		candidate.version=DataflowArgmaxVersion;
		candidate.core=core;
		candidate.active=unsigned(core==0);
		candidate.program=program;
		if (!candidate.Valid()) return false;
		result=candidate;
		return true;
	}
};

struct DataflowArgmaxResult {
	unsigned token,scoreBits,sequence,checksum;
};

inline unsigned DataflowArgmaxResultChecksum(unsigned token,unsigned scoreBits,
	unsigned sequence)
{
	return DataflowArgmaxResultMagic^token^(scoreBits<<7)^(scoreBits>>25)
		^(sequence*0x9e3779b9U);
}

inline bool DataflowArgmaxResultValid(const DataflowArgmaxResult& result)
{
	if (result.token>=Vocabulary || !result.sequence
		|| result.sequence>DataflowWorkerSequenceLimit
		|| (result.scoreBits&0x7f800000U)==0x7f800000U) return false;
	return result.checksum==DataflowArgmaxResultChecksum(result.token,
		result.scoreBits,result.sequence);
}

inline bool DataflowArgmaxReadResult(const DataflowWorkerMailbox& mailbox,
	unsigned sequence,DataflowArgmaxResult& result)
{
	const unsigned token=mailbox.reserved[0];
	const unsigned scoreBits=mailbox.reserved[1];
	const unsigned resultSequence=mailbox.reserved[2];
	const unsigned magic=mailbox.reserved[3];
	const unsigned tokenComplement=mailbox.reserved[4];
	const unsigned checksum=mailbox.reserved[5];
	if (magic!=DataflowArgmaxResultMagic || tokenComplement!=~token
		|| resultSequence!=sequence || mailbox.reserved[6]) return false;
	const DataflowArgmaxResult candidate{token,scoreBits,resultSequence,checksum};
	if (!DataflowArgmaxResultValid(candidate)) return false;
	result=candidate;
	return true;
}

static_assert(sizeof(DataflowArgmaxInvocation)<=DataflowProgramRegionBytes,
	"the argmax invocation fits the resident L1 command slot");

} // namespace PioneerQwen35

#endif
