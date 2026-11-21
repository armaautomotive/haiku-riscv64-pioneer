/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_QWEN35_DATAFLOW_BLACKHOLE_RESIDUAL_WORKER_H
#define PIONEER_QWEN35_DATAFLOW_BLACKHOLE_RESIDUAL_WORKER_H

#include "PioneerQwen35DataflowBlackholeIO.h"
#include "PioneerQwen35DataflowResidualProgram.h"

namespace PioneerQwen35 {

union DataflowResidualFloatBits {
	float value;
	uint32_t bits;
};

inline bool DataflowResidualFinite(float value)
{
	DataflowResidualFloatBits encoded{};
	encoded.value=value;
	return (encoded.bits&0x7f800000U)!=0x7f800000U;
}

template<class IO>
bool PioneerQwen35DataflowResidualWorker(
	const DataflowResidualInvocation& invocation,float* activationPage,
	float* residualPage,float* outputPage,IO& io)
{
	if (!invocation.Valid() || !activationPage || !residualPage || !outputPage)
		return false;
	for (unsigned i=0;i<invocation.segmentCount;i++) {
		const auto& output=invocation.segment[i];
		if (!io.ReadActivationPanel(output.firstRow,activationPage)) return false;
		if (invocation.mode==DataflowResidualSave) {
			for (unsigned lane=0;lane<16;lane++) {
				if (!DataflowResidualFinite(activationPage[lane])) return false;
				outputPage[lane]=activationPage[lane];
			}
			if (!io.WriteResidualPanel(output,outputPage)) return false;
		} else if (invocation.mode==DataflowResidualAdd) {
			if (!io.ReadResidualPanel(output.firstRow,residualPage)) return false;
			for (unsigned lane=0;lane<16;lane++) {
				if (!DataflowResidualFinite(activationPage[lane])
					|| !DataflowResidualFinite(residualPage[lane])) return false;
				outputPage[lane]=activationPage[lane]+residualPage[lane];
				if (!DataflowResidualFinite(outputPage[lane])) return false;
			}
			if (!io.WriteActivationPanel(output,outputPage)) return false;
		} else return false;
	}
	return true;
}

class DataflowBlackholeResidualOps {
	const DataflowResidualInvocation* fProgram=nullptr;
	DataflowBlackholeNoc<0> fReader;
	DataflowBlackholeNoc<1> fWriter;

	const DataflowOutputSegment* Segment(unsigned row) const
	{
		if (!fProgram || (row&15)) return nullptr;
		for (unsigned i=0;i<fProgram->segmentCount;i++)
			if (fProgram->segment[i].firstRow==row) return &fProgram->segment[i];
		return nullptr;
	}

	bool ReadPanel(const RowSpace& space,unsigned row,uint8_t* output) const
	{
		if (!fProgram || !output || row>=Hidden || !Segment(row)) return false;
		Range range{};
		return space.Resolve(row,16,0,16*sizeof(float),range)
			&& fReader.Read(range.address,fProgram->bankRoute[0][range.bank],
				output,16*sizeof(float));
	}

	bool WritePanel(const RowSpace& space,const DataflowOutputSegment& output,
		const float* values) const
	{
		if (!fProgram || !values || output.bytes!=16*sizeof(float)
			|| output.rows!=16 || (output.firstRow&15)) return false;
		const DataflowOutputSegment* expected=Segment(output.firstRow);
		if (!expected || expected->bank!=output.bank
			|| expected->address!=output.address || expected->bytes!=output.bytes)
			return false;
		Range range{};
		return space.Resolve(output.firstRow,16,0,output.bytes,range)
			&& range.bank==output.bank && range.address==output.address
			&& fWriter.Write(output.address,
				fProgram->bankRoute[1][output.bank],
				reinterpret_cast<const uint8_t*>(values),output.bytes);
	}

public:
	bool Bind(const DataflowResidualInvocation& program)
	{
		fProgram=nullptr;
		if (!program.Valid()) return false;
		fProgram=&program;
		return true;
	}

	bool ReadActivationPanel(unsigned row,float* output) const
	{
		return fProgram && ReadPanel(fProgram->activation,row,
			reinterpret_cast<uint8_t*>(output));
	}

	bool ReadResidualPanel(unsigned row,float* output) const
	{
		return fProgram && ReadPanel(fProgram->residual,row,
			reinterpret_cast<uint8_t*>(output));
	}

	bool WriteResidualPanel(const DataflowOutputSegment& output,
		const float* values) const
	{
		return fProgram && fProgram->mode==DataflowResidualSave
			&& WritePanel(fProgram->residual,output,values);
	}

	bool WriteActivationPanel(const DataflowOutputSegment& output,
		const float* values) const
	{
		return fProgram && fProgram->mode==DataflowResidualAdd
			&& WritePanel(fProgram->activation,output,values);
	}
};

} // namespace PioneerQwen35

#endif
