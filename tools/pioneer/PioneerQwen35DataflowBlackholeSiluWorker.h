/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_QWEN35_DATAFLOW_BLACKHOLE_SILU_WORKER_H
#define PIONEER_QWEN35_DATAFLOW_BLACKHOLE_SILU_WORKER_H

#include "PioneerQwen35DataflowBlackholeIO.h"
#include "PioneerQwen35DataflowSiluProgram.h"

namespace PioneerQwen35 {

template<class IO>
bool PioneerQwen35DataflowSiluWorker(const DataflowSiluInvocation& invocation,
	float* gatePage,float* upPage,float* outputPage,IO& io)
{
	if (!invocation.Valid() || !gatePage || !upPage || !outputPage) return false;
	for (unsigned segment=0;segment<invocation.segmentCount;segment++) {
		const auto& output=invocation.segment[segment];
		if (!io.ReadGatePanel(output.firstRow,gatePage)
			|| !io.ReadUpPanel(output.firstRow,upPage)) return false;
		for (unsigned lane=0;lane<16;lane++)
			if (!DataflowSiluProduct(gatePage[lane],upPage[lane],outputPage[lane]))
				return false;
		if (!io.WriteGatePanel(output,outputPage)) return false;
	}
	return true;
}

class DataflowBlackholeSiluOps {
	const DataflowSiluInvocation* fProgram=nullptr;
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
		if (!fProgram || !output || row>=Ffn || !Segment(row)) return false;
		Range range{};
		return space.Resolve(row,16,0,16*sizeof(float),range)
			&& fReader.Read(range.address,fProgram->bankRoute[0][range.bank],
				output,16*sizeof(float));
	}

public:
	bool Bind(const DataflowSiluInvocation& program)
	{
		fProgram=nullptr;
		if (!program.Valid()) return false;
		fProgram=&program;
		return true;
	}

	bool ReadGatePanel(unsigned row,float* output) const
	{
		return fProgram && ReadPanel(fProgram->gate,row,
			reinterpret_cast<uint8_t*>(output));
	}

	bool ReadUpPanel(unsigned row,float* output) const
	{
		return fProgram && ReadPanel(fProgram->up,row,
			reinterpret_cast<uint8_t*>(output));
	}

	bool WriteGatePanel(const DataflowOutputSegment& output,const float* values) const
	{
		if (!fProgram || !values || output.bytes!=16*sizeof(float)
			|| !output.rows || output.rows!=16 || (output.firstRow&15)) return false;
		const DataflowOutputSegment* expected=Segment(output.firstRow);
		if (!expected || expected->bank!=output.bank
			|| expected->address!=output.address || expected->bytes!=output.bytes)
			return false;
		Range range{};
		return fProgram->gate.Resolve(output.firstRow,16,0,output.bytes,range)
			&& range.bank==output.bank && range.address==output.address
			&& fWriter.Write(output.address,fProgram->bankRoute[1][output.bank],
				reinterpret_cast<const uint8_t*>(values),output.bytes);
	}
};

} // namespace PioneerQwen35

#endif
