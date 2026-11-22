/* SPDX-License-Identifier: MIT */
#include "PioneerQwen35DataflowDeviceGraph.h"
#include "PioneerQwen35DataflowDynamicToken.h"
#include "PioneerQwen35DataflowGraphRunProtocol.h"
#include "PioneerQwen35DataflowGraphAuthorization.h"
#include "PioneerQwen35DataflowGraphTrace.h"

// The graph dispatcher remains non-loadable until the kernel owns its sealed
// model and session lifetimes and the device path has been qualified.
extern "C" __attribute__((noinline)) bool qwen35_graph_authorize(
	const PioneerQwen35::DataflowGraphRunDescriptor* run,unsigned index) {
	using namespace PioneerQwen35;
	if (!run) return false;
	const auto* record=reinterpret_cast<const DataflowStreamRecord*>(static_cast<uintptr_t>(DataflowStreamStagingAddress));
	const auto* program=reinterpret_cast<const volatile uint32_t*>(static_cast<uintptr_t>(DataflowProgramAddress));
	return DataflowGraphFetchedCommandMatches(*run,index,*record,program,DataflowProgramRegionBytes);
}
extern "C" bool qwen35_graph_execute(unsigned core,unsigned sequence,
	PioneerQwen35::DataflowWorkerMailbox* mailbox,unsigned* detail);
extern "C" bool qwen35_graph_reset_queues();

struct GraphWitnessExecution {
	const PioneerQwen35::DataflowGraphRunDescriptor& run;
	PioneerQwen35::DataflowWorkerMailbox* mailbox;
	unsigned fetchStage=0;
	unsigned index=0;
	PioneerQwen35::DataflowNocDiagnostic fetchNoC{};
	void Mark(unsigned phase) {
		auto& progress=*reinterpret_cast<volatile uint32_t*>(static_cast<uintptr_t>(
			PioneerQwen35::DataflowGraphProgressAddress));
		PioneerQwen35::DataflowStoreRelease(progress,
			PioneerQwen35::DataflowGraphProgressWord(index,phase));
	}
	bool Fetch(unsigned step) {
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
		return PioneerQwen35::DataflowBlackholeStreamFetchDynamic(run.binding,run.lease,run.routes,run.core,index,run.token,&fetchStage,&fetchNoC);
	}
	bool Authorize(unsigned step) {
		Mark(PioneerQwen35::DataflowGraphProgressAuthorize);
		return qwen35_graph_authorize(&run,step);
	}
	bool Execute(unsigned sequence,unsigned& status,unsigned& detail) {
		Mark(PioneerQwen35::DataflowGraphProgressExecute);
		status=detail=0;
		const bool result=qwen35_graph_execute(run.core,sequence,mailbox,&detail);
		if (result && run.core==0) {
			using namespace PioneerQwen35;
			for (unsigned slot=0;slot<DataflowGraphTraceSlots;slot++) {
				if (DataflowGraphTraceStep(slot)!=index) continue;
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
	GraphWitnessExecution execution{run,mailbox};
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
