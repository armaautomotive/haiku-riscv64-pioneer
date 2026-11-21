/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_QWEN35_DATAFLOW_BLACKHOLE_RMS_WORKER_H
#define PIONEER_QWEN35_DATAFLOW_BLACKHOLE_RMS_WORKER_H

#include "PioneerQwen35DataflowBlackholeIO.h"
#include "PioneerQwen35DataflowRmsProgram.h"

namespace PioneerQwen35 {

enum : unsigned {
	DataflowRmsScaleRecordMagic=0x31535235,
	DataflowRmsScaleRecordVersion=1,
	DataflowRmsScalePollLimit=4096
};

struct DataflowRmsScaleRecord {
	uint32_t word[16];

	bool Valid(unsigned sequence,const DataflowRmsInvocation& invocation) const
	{
		if (word[0]!=DataflowRmsScaleRecordMagic
			|| word[1]!=DataflowRmsScaleRecordVersion || word[2]!=sequence
			|| word[5]!=invocation.outputSlot || word[6]!=invocation.layer
			|| word[7]!=invocation.role || word[8]!=Checksum(word)
			|| !sequence || sequence>DataflowWorkerSequenceLimit) return false;
		for (unsigned i=9;i<16;i++) if (word[i]) return false;
		const float scale=PioneerAttentionFloat(word[3]);
		return DataflowRmsDomain(scale) && scale>0 && word[4]==~word[3];
	}

	static uint32_t Checksum(const uint32_t (&words)[16])
	{
		uint32_t value=2166136261U;
		for (unsigned i=0;i<8;i++) value=(value^words[i])*16777619U;
		return value;
	}

	static bool Build(unsigned sequence,const DataflowRmsInvocation& invocation,
		float scale,DataflowRmsScaleRecord& result)
	{
		if (!sequence || sequence>DataflowWorkerSequenceLimit
			|| !invocation.Valid() || !(scale>0) || !DataflowRmsDomain(scale))
			return false;
		for (unsigned i=0;i<16;i++) result.word[i]=0;
		result.word[0]=DataflowRmsScaleRecordMagic;
		result.word[1]=DataflowRmsScaleRecordVersion;
		result.word[2]=sequence;
		result.word[3]=PioneerAttentionBits(scale);
		result.word[4]=~result.word[3];
		result.word[5]=invocation.outputSlot;
		result.word[6]=invocation.layer;
		result.word[7]=invocation.role;
		result.word[8]=Checksum(result.word);
		return result.Valid(sequence,invocation);
	}
};

static_assert(sizeof(DataflowRmsScaleRecord)==64,
	"RMS scale barrier is one aligned NoC page");

template<class IO>
bool PioneerQwen35DataflowRmsWorker(const DataflowRmsInvocation& invocation,
	unsigned sequence,float* inputCache,float* inputPage,float* weightPage,
	float* outputPage,IO& io)
{
	if (!invocation.Valid() || !sequence
		|| sequence>DataflowWorkerSequenceLimit || !inputCache || !inputPage
		|| !weightPage || !outputPage) return false;
	float scale=0;
	if (invocation.core==0) {
		if (!io.ReadHiddenInput(inputCache)) return false;
		if (!DataflowHiddenRmsScale(inputCache,scale)) return false;
		DataflowRmsScaleRecord record;
		if (!DataflowRmsScaleRecord::Build(sequence,invocation,scale,record)
			|| !io.WriteScaleRecord(record)) return false;
	} else {
		bool observed=false;
		DataflowRmsScaleRecord record;
		for (unsigned poll=0;poll<DataflowRmsScalePollLimit;poll++) {
			if (io.ReadScaleRecord(record)
				&& record.Valid(sequence,invocation)) {
				scale=PioneerAttentionFloat(record.word[3]);
				observed=true;
				break;
			}
		}
		if (!observed) return false;
	}
	for (unsigned segment=0;segment<invocation.segmentCount;segment++) {
		const auto& output=invocation.segment[segment];
		if (!io.ReadInputPanel(output.firstRow,inputPage)
			|| !io.ReadWeightPanel(output.firstRow,weightPage)) return false;
		for (unsigned lane=0;lane<16;lane++) {
			const float value=(inputPage[lane]*scale)*weightPage[lane];
			if ((PioneerAttentionBits(value)&0x7f800000U)==0x7f800000U)
				return false;
			outputPage[lane]=value;
		}
		if (!io.WriteOutput(output.bank,output.address,
			reinterpret_cast<const uint8_t*>(outputPage),16*sizeof(float)))
			return false;
	}
	return true;
}

class DataflowBlackholeRmsOps {
	const DataflowRmsInvocation* fInvocation=nullptr;
	unsigned fSequence=0;
	DataflowBlackholeNoc<0> fReader;
	DataflowBlackholeNoc<1> fWriter;

	bool InputRange(unsigned row,unsigned rows,Range& range) const
	{
		return fInvocation && rows && (row|rows)%16==0
			&& row<Hidden && rows<=Hidden-row
			&& fInvocation->input.Resolve(row,rows,0,rows*sizeof(float),range);
	}

public:
	bool Bind(const DataflowRmsInvocation& invocation,unsigned sequence)
	{
		fInvocation=nullptr;
		fSequence=0;
		if (!invocation.Valid() || !sequence
			|| sequence>DataflowWorkerSequenceLimit) return false;
		fInvocation=&invocation;
		fSequence=sequence;
		return true;
	}

	bool ReadHiddenInput(float* output) const
	{
		if (!fInvocation || fInvocation->core!=0 || !output) return false;
		unsigned covered=0;
		for (unsigned i=0;i<fInvocation->input.count;i++) {
			const Fragment& part=fInvocation->input.parts[i];
			if (part.firstRow>=Hidden) break;
			const unsigned rows=part.rows<Hidden-part.firstRow
				?part.rows:Hidden-part.firstRow;
			Range expected{};
			if (!rows || !InputRange(part.firstRow,rows,expected)
				|| expected.bank!=part.bank || expected.address!=part.address
				|| !fReader.Read(expected.address,
					fInvocation->bankRoute[0][expected.bank],
					reinterpret_cast<uint8_t*>(output+part.firstRow),
					rows*sizeof(float))) return false;
			covered+=rows;
		}
		return covered==Hidden;
	}

	bool ReadInputPanel(unsigned row,float* output) const
	{
		if (!fInvocation || !output) return false;
		Range range{};
		return InputRange(row,16,range)
			&& fReader.Read(range.address,fInvocation->bankRoute[0][range.bank],
				reinterpret_cast<uint8_t*>(output),16*sizeof(float));
	}

	bool ReadWeightPanel(unsigned firstRow,float* output) const
	{
		if (!fInvocation || !output || firstRow>=Hidden || (firstRow&15))
			return false;
		Range range{};
		return fInvocation->weight.Resolve(0,1,firstRow*sizeof(float),
			16*sizeof(float),range)
			&& fReader.Read(range.address,fInvocation->bankRoute[0][range.bank],
				reinterpret_cast<uint8_t*>(output),16*sizeof(float));
	}

	bool WriteScaleRecord(const DataflowRmsScaleRecord& record) const
	{
		if (!fInvocation || fInvocation->core!=0
			|| !record.Valid(fSequence,*fInvocation)) return false;
		Range scratch{};
		return fInvocation->output.Resolve(fInvocation->scratchRow,16,0,
			sizeof(record),scratch)
			&& fWriter.Write(scratch.address,
				fInvocation->bankRoute[1][scratch.bank],
				reinterpret_cast<const uint8_t*>(&record),sizeof(record));
	}

	bool ReadScaleRecord(DataflowRmsScaleRecord& record) const
	{
		if (!fInvocation || fInvocation->core==0) return false;
		Range scratch{};
		return fInvocation->output.Resolve(fInvocation->scratchRow,16,0,
			sizeof(record),scratch)
			&& fReader.Read(scratch.address,
				fInvocation->bankRoute[0][scratch.bank],
				reinterpret_cast<uint8_t*>(&record),sizeof(record));
	}

	bool WriteOutput(unsigned bank,uint32_t address,const uint8_t* input,
		unsigned bytes) const
	{
		if (!fInvocation || !input || bytes!=16*sizeof(float)) return false;
		for (unsigned i=0;i<fInvocation->segmentCount;i++) {
			const auto& output=fInvocation->segment[i];
			if (output.bank==bank && output.address==address
				&& output.bytes==bytes)
				return fWriter.Write(address,fInvocation->bankRoute[1][bank],
					input,bytes);
		}
		return false;
	}
};

} // namespace PioneerQwen35

#endif
