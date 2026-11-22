/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_QWEN35_DATAFLOW_GRAPH_PROFILE_H
#define PIONEER_QWEN35_DATAFLOW_GRAPH_PROFILE_H

#include "PioneerQwen35DataflowGraphTrace.h"

namespace PioneerQwen35 {
// Diagnostics only: aggregate core0 cycle deltas for one completed decode.
// The remainder includes barriers, queue reset, loop and profiling overhead;
// it is not proof of a particular worker's idle time or release safety.
enum DataflowGraphProfileBin : unsigned {
	DataflowProfileEmbedding=0,DataflowProfileResidual,DataflowProfileRms,
	DataflowProfileProjection,DataflowProfileConv,DataflowProfileDelta,
	DataflowProfileSilu,DataflowProfileDense,DataflowProfileArgmax,
	DataflowProfileFetch,DataflowProfileAuthorize,DataflowProfileOther,
	DataflowProfileTotal,DataflowProfileBins
};
static_assert(DataflowGraphProfileTraceFirst+DataflowProfileBins==DataflowGraphTraceSlots,
	"all profile bins fit the saved, bounded graph trace region");
inline unsigned DataflowGraphProfileOperation(unsigned magic)
{
	switch (magic) {
		case DataflowEmbeddingMagic:return DataflowProfileEmbedding;
		case DataflowResidualMagic:return DataflowProfileResidual;
		case DataflowRmsMagic:return DataflowProfileRms;
		case DataflowProgramMagic:return DataflowProfileProjection;
		case DataflowConvMagic:return DataflowProfileConv;
		case DataflowDeltaMagic:return DataflowProfileDelta;
		case DataflowSiluMagic:return DataflowProfileSilu;
		case DataflowDenseAttentionMagic:return DataflowProfileDense;
		case DataflowArgmaxMagic:return DataflowProfileArgmax;
		default:return DataflowProfileOther;
	}
}
class DataflowGraphProfile {
	uint64_t fCycles[DataflowProfileBins]{};
	bool fValid=true;
public:
	void Add(unsigned bin,uint64_t start,uint64_t end) {
		if (!fValid) return;
		if (bin>DataflowProfileOther || start==~uint64_t(0)
			|| end==~uint64_t(0) || end<start) { fValid=false;return; }
		if (bin==DataflowProfileOther) return; // counted in total remainder
		const uint64_t delta=end-start,old=fCycles[bin];
		fCycles[bin]+=delta;
		if (fCycles[bin]<old) fValid=false;
	}
	bool Finish(uint64_t start,uint64_t end) {
		if (!fValid || start==~uint64_t(0) || end==~uint64_t(0) || end<start) return false;
		const uint64_t total=end-start;
		uint64_t sum=0;
		for (unsigned bin=0;bin<DataflowProfileOther;bin++) {
			const uint64_t prior=sum;sum+=fCycles[bin];
			if (sum<prior || sum>total) return false;
		}
		fCycles[DataflowProfileOther]=total-sum;
		fCycles[DataflowProfileTotal]=total;
		return true;
	}
	uint64_t Cycles(unsigned bin) const { return bin<DataflowProfileBins?fCycles[bin]:0; }
	void Publish(volatile uint32_t* records,unsigned sequence) const {
		if (!fValid || !records || !sequence) return;
		for (unsigned bin=0;bin<DataflowProfileBins;bin++) {
			volatile uint32_t* record=records+4*bin;
			DataflowStoreRelease(record[0],0);
			DataflowStoreRelease(record[1],uint32_t(fCycles[bin]));
			DataflowStoreRelease(record[2],uint32_t(fCycles[bin]>>32));
			DataflowStoreRelease(record[3],unsigned(fCycles[bin]!=0));
			DataflowStoreRelease(record[0],sequence);
		}
	}
};
}
#endif
