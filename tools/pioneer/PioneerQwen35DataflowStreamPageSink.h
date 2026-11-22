/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_QWEN35_DATAFLOW_STREAM_PAGE_SINK_H
#define PIONEER_QWEN35_DATAFLOW_STREAM_PAGE_SINK_H

#include "PioneerQwen35DataflowStreamPlan.h"

namespace PioneerQwen35 {

// The IO adapter owns an exclusive, sticky bank claim. WriteVerifiedPage
// must prove exact device readback and retain ownership on an uncertain DMA.
// This object belongs on the heap: it buffers one page for each core.
template<class IO> class DataflowStreamPageSink {
	IO& io;
	DataflowStreamPlan plan{};
	uint32_t page[DataflowCoreCount][1024]{};
	unsigned nextCore=0,nextStep=0;
	bool begun=false,sealed=false,unsafe=false;
	bool Same(const DataflowStreamBinding& b,const Range& r,unsigned core) const {
		const auto& p=plan.binding[core];const auto& q=plan.lease[core];
		return b.magic==p.magic && b.version==p.version && b.epoch==p.epoch
			&& b.core==p.core && b.bank==p.bank && b.route==p.route
			&& b.base==p.base && b.count==p.count
			&& r.bank==q.bank && r.address==q.address && r.bytes==q.bytes;
	}
public:
	explicit DataflowStreamPageSink(IO& storage):io(storage) {}
	DataflowStreamPageSink(const DataflowStreamPageSink&)=delete;
	DataflowStreamPageSink& operator=(const DataflowStreamPageSink&)=delete;
	bool Begin(const DataflowStreamPlan& p) {
		if (begun || unsafe || !p.ready || !p.epoch || !p.count
			|| p.count>DataflowStreamMaxRecords) return false;
		// The caller must first validate this complete plan against the bound
		// model and packed workspace; Reserve must verify actual ownership.
		if (!io.Reserve(p,this)) { unsafe=true;return false; }
		plan=p;begun=true;return true;
	}
	bool StageRecord(const DataflowStreamBinding& binding,const Range& lease,
		unsigned index,const DataflowStreamRecord& record) {
		if (!begun || sealed || unsafe || nextStep>=plan.count || index!=nextStep
			|| binding.core!=nextCore || !Same(binding,lease,nextCore)
			|| !DataflowStreamRecordStructureValid(record,plan.epoch,index+1,nextCore)) {
			unsafe=true;return false;
		}
		auto* words=reinterpret_cast<const uint32_t*>(&record);
		uint32_t* target=page[nextCore];
		if (!(index&1)) for (unsigned i=0;i<1024;i++) target[i]=0;
		for (unsigned i=0;i<DataflowStreamRecordBytes/4;i++)
			target[(index&1)*(DataflowStreamRecordBytes/4)+i]=words[i];
		if ((index&1) || index+1==plan.count) {
			const uint64_t offset=uint64_t(binding.base-BankBase)+uint64_t(index/2)*4096;
			if (!io.WriteVerifiedPage(this,binding.bank,offset,target)) {
				unsafe=true;io.Quarantine();return false;
			}
		}
		if (++nextCore==DataflowCoreCount) { nextCore=0;nextStep++; }
		return true;
	}
	bool Seal(const DataflowStreamPlan& p) {
		if (!begun || sealed || unsafe || nextCore || nextStep!=plan.count
			|| !p.ready || p.epoch!=plan.epoch || p.count!=plan.count) return false;
		for (unsigned core=0;core<DataflowCoreCount;core++)
			if (!Same(p.binding[core],p.lease[core],core)) return false;
		if (!io.Seal(this)) { unsafe=true;io.Quarantine();return false; }
		sealed=true;return true;
	}
	void Abort() { if (begun && !unsafe) io.Quarantine();unsafe=true; }
	bool Sealed() const { return sealed && !unsafe; }
	bool SealedMatches(const DataflowStreamPlan& expected) const {
		if (!Sealed() || !expected.ready || expected.epoch!=plan.epoch || expected.count!=plan.count) return false;
		for (unsigned core=0;core<DataflowCoreCount;core++)
			if (!Same(expected.binding[core],expected.lease[core],core)) return false;
		return io.SealedMatches(expected,this);
	}
	bool Unsafe() const { return unsafe; }
};
}
#endif
