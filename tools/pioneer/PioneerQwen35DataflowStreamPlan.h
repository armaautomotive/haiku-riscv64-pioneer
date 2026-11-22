/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_QWEN35_DATAFLOW_STREAM_PLAN_H
#define PIONEER_QWEN35_DATAFLOW_STREAM_PLAN_H

#include "PioneerQwen35DataflowDeviceStream.h"

namespace PioneerQwen35 {

struct DataflowStreamPlan {
	unsigned epoch=0,count=0;
	bool ready=false;
	DataflowStreamBinding binding[DataflowCoreCount]{};
	Range lease[DataflowCoreCount]{};
};

// The allocator starts after every already bound activation, graph scratch,
// and rotary range. It refuses overlap or an allocation outside the packed
// model's reserved workspace. The driver must independently verify that the
// supplied workspace and manifest belong to the live model session, upload
// and authorize every record, and hold these leases immutable during a run.
class DataflowStreamPlanBuilder {
	struct Span { unsigned bank;uint32_t begin,end; };
	static constexpr unsigned MaxSpans=1+(ActivationBuffers+DataflowGraphWorkspaceBufferCount)*Banks;
	struct State {
		PioneerQ35ActivationWorkspace workspace[Banks]{};
		uint32_t cursor[Banks]{};
		Span spans[MaxSpans]{};
		unsigned used=0;
	};

	static bool Observe(State& state,unsigned bank,uint32_t address,uint64_t bytes)
	{
		if (bank>=Banks || !bytes || (address&4095) || bytes>UINT32_MAX
			|| state.used>=MaxSpans) return false;
		const uint64_t padded=(bytes+4095)&~UINT64_C(4095);
		const auto& workspace=state.workspace[bank];
		const uint64_t end=uint64_t(address)+padded;
		if (address<workspace.address || end>uint64_t(workspace.address)+workspace.bytes
			|| end>UINT32_MAX) return false;
		for (unsigned i=0;i<state.used;i++) {
			const auto& prior=state.spans[i];
			if (prior.bank==bank && address<prior.end && prior.begin<end) return false;
		}
		state.spans[state.used++]={bank,address,uint32_t(end)};
		if (state.cursor[bank]<end) state.cursor[bank]=uint32_t(end);
		return true;
	}

	static bool ObserveRows(State& state,const RowSpace& rows)
	{
		if (!rows.Valid()) return false;
		for (unsigned i=0;i<rows.count;i++) {
			const auto& part=rows.parts[i];
			if (!Observe(state,part.bank,part.address,uint64_t(part.rows)*rows.rowBytes)) return false;
		}
		return true;
	}

	template<class PackedPlan>
	static bool Initialize(State& state,const PackedPlan& packed,const DeviceLayout& manifest,
		const DataflowGraphWorkspace& graph,const Range& rope)
	{
		if (manifest.magic!=LayoutMagic || manifest.version!=LayoutVersion
			|| manifest.reserved || !manifest.context || manifest.context>MaxPositions || !graph.Valid(manifest)
			|| rope.bank!=0 || rope.bytes!=manifest.context*Rotary*sizeof(float)) return false;
		for (unsigned bank=0;bank<Banks;bank++) {
			auto& workspace=state.workspace[bank];workspace.bank=bank;
			if (!packed.Workspace(bank,workspace.address,workspace.bytes)
				|| !workspace.bytes || (workspace.address&4095) || (workspace.bytes&4095)
				|| workspace.address<BankBase || workspace.address>=BankLimit
				|| workspace.bytes>BankLimit-workspace.address) return false;
			state.cursor[bank]=workspace.address;
		}
		if (rope.address!=state.workspace[0].address) return false;
		if (!Observe(state,rope.bank,rope.address,rope.bytes)) return false;
		for (unsigned slot=0;slot<ActivationBuffers;slot++)
			if (!ObserveRows(state,manifest.activation[slot])) return false;
		for (unsigned slot=0;slot<DataflowGraphWorkspaceBufferCount;slot++)
			if (!ObserveRows(state,graph.space[slot])) return false;
		return true;
	}

	static bool Allocate(State& state,const unsigned (&routes)[2][Banks],unsigned epoch,
		unsigned count,unsigned core,DataflowStreamBinding& binding,Range& lease)
	{
		const uint64_t bytes=uint64_t(count)*DataflowStreamRecordBytes;
		const uint64_t allocated=(bytes+4095)&~UINT64_C(4095);
		if (allocated>UINT32_MAX || core>=DataflowCoreCount) return false;
		const unsigned bank=core%Banks;
		const uint32_t address=state.cursor[bank];
		const uint64_t end=uint64_t(address)+allocated;
		if (end>uint64_t(state.workspace[bank].address)+state.workspace[bank].bytes
			|| end>UINT32_MAX) return false;
		lease={bank,address,uint32_t(allocated)};
		binding={DataflowStreamBindingMagic,DataflowStreamVersion,
			epoch,core,bank,routes[0][bank],address,count};
		if (!DataflowStreamBindingMatches(binding,lease,routes,core)) return false;
		state.cursor[bank]=uint32_t(end);return true;
	}

public:
	template<class PackedPlan>
	static bool Build(const PackedPlan& packed,const DeviceLayout& manifest,
		const DataflowGraphWorkspace& graph,const Range& rope,
		const unsigned (&routes)[2][Banks],unsigned epoch,unsigned count,
		DataflowStreamPlan& result)
	{
		result.ready=false;result.epoch=0;result.count=0;
		if (!epoch || !count || count>DataflowStreamMaxRecords || !DataflowBankRoutesValid(routes))
			return false;
		State state{};
		if (!Initialize(state,packed,manifest,graph,rope)) return false;
		for (unsigned core=0;core<DataflowCoreCount;core++) {
			if (!Allocate(state,routes,epoch,count,core,result.binding[core],result.lease[core])) return false;
		}
		result.epoch=epoch;result.count=count;result.ready=true;return true;
	}

	template<class PackedPlan>
	static bool Validate(const PackedPlan& packed,const DeviceLayout& manifest,
		const DataflowGraphWorkspace& graph,const Range& rope,
		const unsigned (&routes)[2][Banks],const DataflowStreamPlan& plan)
	{
		if (!plan.ready || !plan.epoch || !plan.count || plan.count>DataflowStreamMaxRecords
			|| !DataflowBankRoutesValid(routes)) return false;
		State state{};
		if (!Initialize(state,packed,manifest,graph,rope)) return false;
		for (unsigned core=0;core<DataflowCoreCount;core++) {
			DataflowStreamBinding expected{};Range lease{};
			if (!Allocate(state,routes,plan.epoch,plan.count,core,expected,lease)) return false;
			const auto& got=plan.binding[core];const auto& actual=plan.lease[core];
			if (got.magic!=expected.magic || got.version!=expected.version || got.epoch!=expected.epoch
				|| got.core!=expected.core || got.bank!=expected.bank || got.route!=expected.route
				|| got.base!=expected.base || got.count!=expected.count || actual.bank!=lease.bank
				|| actual.address!=lease.address || actual.bytes!=lease.bytes) return false;
		}
		return true;
	}
};
}
#endif
