/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_QWEN35_DATAFLOW_BLACKHOLE_CONV_WORKER_H
#define PIONEER_QWEN35_DATAFLOW_BLACKHOLE_CONV_WORKER_H

#include "PioneerQwen35DataflowBlackholeIO.h"
#include "PioneerQwen35DataflowConvProgram.h"
#include "PioneerQwen35DataflowSilu.h"

namespace PioneerQwen35 {

template<class IO>
bool PioneerQwen35DataflowConvWorker(const DataflowConvInvocation& invocation,
	float* qkvPage,float* weightPage,float* historyPage,float* nextHistoryPage,
	float* outputPage,IO& io)
{
	if (!invocation.Valid() || !qkvPage || !weightPage || !historyPage
		|| !nextHistoryPage || !outputPage || !io.Bind(invocation)) return false;
	for (unsigned group=0;group<invocation.groupCount;group++) {
		const unsigned row=(invocation.firstGroup+group)*16;
		if (!io.ReadQkvPanel(row,qkvPage)
			|| !io.ReadWeightPanel(row,weightPage)) return false;
		if (invocation.position) {
			if (!io.ReadHistoryPanel(row,historyPage)) return false;
		} else {
			for (unsigned i=0;i<48;i++) historyPage[i]=0.0f;
		}
		for (unsigned channel=0;channel<16;channel++) {
			const float current=qkvPage[channel];
			const float* weight=weightPage+channel*4;
			const float* history=historyPage+channel*3;
			float sum=current*weight[3];
			const float first=history[0]*weight[0];
			const float second=history[1]*weight[1];
			const float third=history[2]*weight[2];
			sum=((first+second)+third)+sum;
			float activated=0.0f;
			if (!DataflowSiluFinite(current) || !DataflowSiluFinite(sum)
				|| !DataflowSiluProduct(sum,1.0f,activated)) return false;
			nextHistoryPage[channel*3]=history[1];
			nextHistoryPage[channel*3+1]=history[2];
			nextHistoryPage[channel*3+2]=current;
			outputPage[channel]=activated;
		}
		if (!io.WriteHistoryPanel(row,nextHistoryPage)
			|| !io.WriteQkvPanel(row,outputPage)) return false;
	}
	return true;
}

class DataflowBlackholeConvOps {
	const DataflowConvInvocation* fProgram=nullptr;
	DataflowBlackholeNoc<0> fReader;
	DataflowBlackholeNoc<1> fWriter;

	bool ReadPanel(const RowSpace& space,unsigned row,unsigned bytes,
		uint8_t* output) const
	{
		if (!fProgram || !output) return false;
		Range range{};
		return space.Resolve(row,16,0,bytes,range)
			&& DataflowConvProgram::DataflowConvRangeAligned(range,bytes)
			&& fReader.Read(range.address,fProgram->bankRoute[0][range.bank],
				output,bytes);
	}

	bool WritePanel(const RowSpace& space,unsigned row,unsigned bytes,
		const uint8_t* input) const
	{
		if (!fProgram || !input) return false;
		Range range{};
		return space.Resolve(row,16,0,bytes,range)
			&& DataflowConvProgram::DataflowConvRangeAligned(range,bytes)
			&& fWriter.Write(range.address,fProgram->bankRoute[1][range.bank],
				input,bytes);
	}

public:
	bool Bind(const DataflowConvInvocation& program)
	{
		fProgram=nullptr;
		if (!program.Valid()) return false;
		fProgram=&program;
		return true;
	}

	bool ReadQkvPanel(unsigned row,float* output) const
	{
		return fProgram && ReadPanel(fProgram->qkv,row,DataflowConvPanelBytes,
			reinterpret_cast<uint8_t*>(output));
	}

	bool ReadWeightPanel(unsigned row,float* output) const
	{
		return fProgram && ReadPanel(fProgram->weights.place,row,
			DataflowConvWeightBytes,reinterpret_cast<uint8_t*>(output));
	}

	bool ReadHistoryPanel(unsigned row,float* output) const
	{
		return fProgram && ReadPanel(fProgram->historyRead,row,
			DataflowConvHistoryBytes,reinterpret_cast<uint8_t*>(output));
	}

	bool WriteHistoryPanel(unsigned row,const float* input) const
	{
		return fProgram && WritePanel(fProgram->historyWrite,row,
			DataflowConvHistoryBytes,reinterpret_cast<const uint8_t*>(input));
	}

	bool WriteQkvPanel(unsigned row,const float* input) const
	{
		return fProgram && WritePanel(fProgram->qkv,row,DataflowConvPanelBytes,
			reinterpret_cast<const uint8_t*>(input));
	}
};

} // namespace PioneerQwen35

#endif
