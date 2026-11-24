/* SPDX-License-Identifier: MIT */
#include "PioneerQwen35DataflowDeviceGraph.h"
#include "PioneerQwen35DataflowDynamicToken.h"
#include "PioneerQwen35DataflowGraphRunProtocol.h"
#include "PioneerQwen35DataflowGraphAuthorization.h"
#include "PioneerQwen35DataflowGraphTrace.h"
#include "PioneerQwen35DataflowGraphProfile.h"
#include "PioneerQwen35DataflowResidentDispatch.h"
#ifdef PIONEER_QWEN35_SHARED_ROLE_GRAPH_WITNESS
#include "PioneerQwen35DataflowResidentStreamCache.h"
static_assert(unsigned(PioneerQwen35::DataflowResidentStreamCacheEntries)>=unsigned(PioneerQwen35::DataflowGraphDecodeSteps),
	"the resident length cache covers the complete decode graph");
extern "C" __attribute__((noinline)) bool qwen35_resident_stream_cache_bind(
	const PioneerQwen35::DataflowStreamBinding* binding) {
	if (!binding) return false;
	auto* cache=reinterpret_cast<PioneerQwen35::DataflowResidentStreamCache*>(static_cast<uintptr_t>(
		PioneerQwen35::DataflowResidentStreamCacheAddress));
	return PioneerQwen35::DataflowResidentStreamCacheBind(*cache,*binding);
}
#endif

// The graph dispatcher remains non-loadable until the kernel owns its sealed
// model and session lifetimes and the device path has been qualified.
extern "C" __attribute__((noinline)) bool qwen35_graph_authorize(
	const PioneerQwen35::DataflowGraphRunDescriptor* run,unsigned index,unsigned readBytes) {
	using namespace PioneerQwen35;
	if (!run) return false;
	const auto* record=reinterpret_cast<const DataflowStreamRecord*>(static_cast<uintptr_t>(DataflowStreamStagingAddress));
	const auto* program=reinterpret_cast<const volatile uint32_t*>(static_cast<uintptr_t>(DataflowProgramAddress));
	return DataflowGraphFetchedCommandMatches(*run,index,*record,program,DataflowProgramRegionBytes,readBytes);
}
extern "C" bool qwen35_graph_execute(unsigned core,unsigned sequence,
	PioneerQwen35::DataflowWorkerMailbox* mailbox,unsigned* detail);
extern "C" bool qwen35_graph_reset_queues();

// BabyRISCV exposes cycle/cycleh. The high-low-high read handles rollover;
// these values are diagnostic only and never prove completion or release.
// https://github.com/tenstorrent/tt-isa-documentation/blob/main/BlackholeA0/TensixTile/BabyRISCV/CSRs.md
static inline uint64_t DataflowProfileCycle()
{
	for (unsigned attempt=0;attempt<4;attempt++) {
		uint32_t high,low,again;
		asm volatile("csrr %0, 0xc80\n\tcsrr %1, 0xc00\n\tcsrr %2, 0xc80"
			: "=&r"(high),"=&r"(low),"=&r"(again) :: "memory");
		if (high==again) return (uint64_t(high)<<32)|low;
	}
	return ~uint64_t(0); // bounded diagnostic failure; never retain inference
}

struct GraphWitnessExecution {
	const PioneerQwen35::DataflowGraphRunDescriptor& run;
	PioneerQwen35::DataflowWorkerMailbox* mailbox;
	unsigned fetchStage=0;
	unsigned index=0;
	unsigned readBytes=PioneerQwen35::DataflowStreamRecordBytes;
	PioneerQwen35::DataflowNocDiagnostic fetchNoC{};
	PioneerQwen35::DataflowGraphProfile* profile=nullptr;
	void Mark(unsigned phase) {
		auto& progress=*reinterpret_cast<volatile uint32_t*>(static_cast<uintptr_t>(
			PioneerQwen35::DataflowGraphProgressAddress));
		PioneerQwen35::DataflowStoreRelease(progress,
			PioneerQwen35::DataflowGraphProgressWord(index,phase));
	}
	bool Fetch(unsigned step) {
		const uint64_t start=profile?DataflowProfileCycle():0;
		auto& operation=*reinterpret_cast<volatile uint32_t*>(static_cast<uintptr_t>(
			PioneerQwen35::DataflowGraphOperationProgressAddress));
		PioneerQwen35::DataflowStoreRelease(operation,0);
		index=step;Mark(PioneerQwen35::DataflowGraphProgressFetch);
		if (run.core==0 && step==3) {
			using namespace PioneerQwen35;
			for (unsigned slot=DataflowGraphProjectionTraceFirst;slot<DataflowGraphTraceSlots;slot++) {
				auto* trace=reinterpret_cast<volatile uint32_t*>(static_cast<uintptr_t>(
					DataflowGraphTraceAddress+slot*sizeof(DataflowGraphTraceRecord)));
				DataflowStoreRelease(trace[0],0);
			}
		}
		readBytes=PioneerQwen35::DataflowStreamRecordBytes;
#ifdef PIONEER_QWEN35_SHARED_ROLE_GRAPH_WITNESS
		auto* cache=reinterpret_cast<PioneerQwen35::DataflowResidentStreamCache*>(static_cast<uintptr_t>(
			PioneerQwen35::DataflowResidentStreamCacheAddress));
		const unsigned hint=PioneerQwen35::DataflowResidentStreamCacheHint(*cache,index);
		if (hint==UINT32_MAX) { fetchStage=PioneerQwen35::DataflowStreamFetchBinding;return false; }
		if (hint) readBytes=hint;
#endif
		bool okay=PioneerQwen35::DataflowBlackholeStreamFetchDynamic(run.binding,run.lease,run.routes,run.core,index,run.token,&fetchStage,&fetchNoC,readBytes);
#ifdef PIONEER_QWEN35_SHARED_ROLE_GRAPH_WITNESS
		if (okay) {
			const auto* record=reinterpret_cast<const PioneerQwen35::DataflowStreamRecord*>(static_cast<uintptr_t>(
				PioneerQwen35::DataflowStreamStagingAddress));
			okay=PioneerQwen35::DataflowResidentStreamCacheRecord(*cache,index,*record,readBytes);
			if (!okay) fetchStage=PioneerQwen35::DataflowStreamFetchRecord;
		}
#endif
		if (profile) profile->Add(PioneerQwen35::DataflowProfileFetch,start,DataflowProfileCycle());
		return okay;
	}
	bool Authorize(unsigned step) {
		const uint64_t start=profile?DataflowProfileCycle():0;
		Mark(PioneerQwen35::DataflowGraphProgressAuthorize);
		const bool okay=qwen35_graph_authorize(&run,step,readBytes);
		if (profile) profile->Add(PioneerQwen35::DataflowProfileAuthorize,start,DataflowProfileCycle());
		return okay;
	}
	bool Execute(unsigned sequence,unsigned& status,unsigned& detail) {
		Mark(PioneerQwen35::DataflowGraphProgressExecute);
		status=detail=0;
#ifdef PIONEER_QWEN35_SHARED_ROLE_GRAPH_WITNESS
		auto* reuse=reinterpret_cast<PioneerQwen35::DataflowActivationReuse*>(static_cast<uintptr_t>(
			PioneerQwen35::DataflowActivationReuseAddress));
		PioneerQwen35::DataflowActivationReuseSetStep(*reuse,run.ticket.epoch,index+1);
#endif
		const auto* program=reinterpret_cast<const volatile uint32_t*>(static_cast<uintptr_t>(
			PioneerQwen35::DataflowProgramAddress));
		const unsigned operation=profile?PioneerQwen35::DataflowGraphProfileOperation(*program):0;
		const uint64_t start=profile?DataflowProfileCycle():0;
		const bool result=qwen35_graph_execute(run.core,sequence,mailbox,&detail);
		if (profile) profile->Add(operation,start,DataflowProfileCycle());
		if (result && run.core==0) {
			using namespace PioneerQwen35;
			for (unsigned slot=0;slot<DataflowGraphTraceSlots;slot++) {
				if (DataflowGraphTraceStep(slot)!=index || slot>=DataflowGraphProfileTraceFirst) continue;
				auto* trace=reinterpret_cast<volatile uint32_t*>(static_cast<uintptr_t>(
					DataflowGraphTraceAddress+slot*sizeof(DataflowGraphTraceRecord)));
				if (slot>=DataflowGraphProjectionTraceFirst) {
					DataflowGraphTracePublishPending(trace,sequence);
					continue;
				}
				const auto* page=reinterpret_cast<const volatile uint32_t*>(static_cast<uintptr_t>(DataflowOutputBufferAddress));
				const auto sample=DataflowGraphTraceSample(sequence,page);
				DataflowStoreRelease(trace[0],0);
				DataflowStoreRelease(trace[1],sample.first);
				DataflowStoreRelease(trace[2],sample.last);
				DataflowStoreRelease(trace[3],sample.magnitudeOr);
				DataflowStoreRelease(trace[0],sample.sequence);
			}
		}
		if (result) Mark(PioneerQwen35::DataflowGraphProgressBarrier);
		return result;
	}
	bool ResetQueues() {
		Mark(PioneerQwen35::DataflowGraphProgressResetQueues);
		return qwen35_graph_reset_queues();
	}
};
extern "C" unsigned PioneerQwen35DeviceGraphRun(
	const PioneerQwen35::DataflowGraphRunDescriptor& run,
	PioneerQwen35::DataflowWorkerMailbox* mailbox)
{
	if (!mailbox || !PioneerQwen35::DataflowGraphRunValid(run,mailbox->logicalCore)) return PioneerQwen35::DataflowGraphInvalid;
#ifdef PIONEER_QWEN35_SHARED_ROLE_GRAPH_WITNESS
	if (!qwen35_resident_stream_cache_bind(&run.binding)) return PioneerQwen35::DataflowGraphInvalid;
	PioneerQwen35::DataflowActivationReuseReset(*reinterpret_cast<PioneerQwen35::DataflowActivationReuse*>(
		static_cast<uintptr_t>(PioneerQwen35::DataflowActivationReuseAddress)));
#endif
	PioneerQwen35::DataflowGraphProfile profile;
	GraphWitnessExecution execution{run,mailbox};
	if (run.core==0 && run.ticket.count==PioneerQwen35::DataflowGraphDecodeSteps) {
		using namespace PioneerQwen35;
		execution.profile=&profile;
		for (unsigned slot=DataflowGraphProfileTraceFirst;slot<DataflowGraphTraceSlots;slot++) {
			auto* record=reinterpret_cast<volatile uint32_t*>(static_cast<uintptr_t>(
				DataflowGraphTraceAddress+slot*sizeof(DataflowGraphTraceRecord)));
			DataflowStoreRelease(record[0],0);
		}
	}
	const uint64_t profileStart=execution.profile?DataflowProfileCycle():0;
	PioneerQwen35::DataflowBlackholeBarrierIO transport(run.core);
	PioneerQwen35::DataflowDeviceGraph<GraphWitnessExecution,
		PioneerQwen35::DataflowBlackholeBarrierIO> graph(execution,transport);
	if (!graph.Start(run.binding,run.core,run.pollLimit,run.ticket.epoch,run.ticket.firstSequence,run.ticket.count))
		return PioneerQwen35::DataflowGraphInvalid;
	for (;;) {
		const auto state=graph.Tick();
		if (state==PioneerQwen35::DataflowGraphRetain) {
			using namespace PioneerQwen35;
			DataflowStoreRelease(mailbox->reserved[1],graph.Fault());
			DataflowStoreRelease(mailbox->reserved[2],graph.CompletedSteps());
			DataflowStoreRelease(mailbox->reserved[3],graph.FailureCore());
			const unsigned status=graph.Fault()==DataflowGraphFetchFault
				?DataflowStreamFetchDiagnosticCode(execution.fetchStage,execution.fetchNoC)
				:graph.FailureStatus();
			DataflowStoreRelease(mailbox->reserved[4],status);
			DataflowFence();
			DataflowStoreRelease(mailbox->reserved[0],DataflowGraphFaultMagic);
		}
		if (state==PioneerQwen35::DataflowGraphLocalComplete) {
			auto& progress=*reinterpret_cast<volatile uint32_t*>(static_cast<uintptr_t>(
				PioneerQwen35::DataflowGraphProgressAddress));
			PioneerQwen35::DataflowStoreRelease(progress,0);
			if (execution.profile && profile.Finish(profileStart,DataflowProfileCycle())) {
				using namespace PioneerQwen35;
				auto* records=reinterpret_cast<volatile uint32_t*>(static_cast<uintptr_t>(
					DataflowGraphTraceAddress+DataflowGraphProfileTraceFirst*sizeof(DataflowGraphTraceRecord)));
				profile.Publish(records,run.ticket.firstSequence+run.ticket.count-1);
			}
			return state;
		}
		if (state==PioneerQwen35::DataflowGraphRetain)
			return state;
	}
}

extern "C" bool qwen35_graph_run_descriptor(
	const PioneerQwen35::DataflowGraphRunDescriptor* descriptor,
	PioneerQwen35::DataflowWorkerMailbox* mailbox)
{
	if (!descriptor || !mailbox || !PioneerQwen35::DataflowGraphRunValid(*descriptor,mailbox->logicalCore)
		|| descriptor->triggerSequence!=PioneerQwen35::DataflowLoadAcquire(mailbox->commandSequence)) return false;
	return PioneerQwen35DeviceGraphRun(*descriptor,mailbox)==PioneerQwen35::DataflowGraphLocalComplete;
}
