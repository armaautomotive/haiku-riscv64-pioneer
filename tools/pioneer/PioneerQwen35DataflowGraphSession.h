/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_QWEN35_DATAFLOW_GRAPH_SESSION_H
#define PIONEER_QWEN35_DATAFLOW_GRAPH_SESSION_H

#include "PioneerQwen35DataflowBoundWorkspace.h"
#include "PioneerQwen35DataflowGraphModelTransaction.h"
#include "PioneerQwen35DataflowServiceOwner.h"

#ifdef PIONEER_QWEN35_DEVICE_DISPATCH
namespace PioneerQwen35 {

enum DataflowGraphSessionResult { DataflowGraphSessionInvalid=0,DataflowGraphSessionPreflight=1,
	DataflowGraphSessionRetained=2,DataflowGraphSessionComplete=3 };

// Heap-owned by the session caller. The candidate is separately allocated;
// after commit it becomes the live authority and the caller must keep it.
// A retained run and both authorities must not be destroyed until graph fault
// recovery proves the fleet and resources safe.
struct DataflowGraphSessionRun {
	DataflowGraphPublisher publisher;
	DataflowGraphCompletion completion;
	DataflowGraphModelTransaction model;
	DataflowGraphRunDescriptor scratch{};
	DataflowGraphRunTicket ticket{};
	DataflowDynamicTokenContext token{};
	bool attempted=false;
};

class DataflowGraphSession {
	DataflowStreamPlan plan{};
	unsigned routes[2][Banks]{};
	unsigned inputSlot=ActivationBuffers;
	DataflowProjectionAuthority** live=nullptr;
	DataflowServiceOwner* service=nullptr;
	DataflowGraphRunIssuer issuer;
	DataflowGraphSessionRun* active=nullptr;
	bool bound=false,faulted=false;

	template<class IO> bool Matches(DataflowGraphSessionRun& run,const DataflowStreamPlan& requested,
		const DataflowDynamicTokenContext& token,const DataflowGraphRunTicket& ticket,unsigned slot,IO& io) {
		return bound && !faulted && active==&run && &requested==&plan && service
			&& service->OwnsGraph(run.publisher) && io.GraphSessionSafe(*service) && slot==inputSlot
			&& io.OwnsGraphSession(this,plan,inputSlot) && run.model.Matches(token,ticket,slot);
	}
	template<class IO> struct Adapter {
		DataflowGraphSession& session;DataflowGraphSessionRun& run;IO& io;
		bool GraphOwnershipMatches(const DataflowStreamPlan& p,const DataflowDynamicTokenContext& t,
			const DataflowGraphRunTicket& ticket,unsigned slot) { return session.Matches(run,p,t,ticket,slot,io); }
		bool CommitGraphModelState(const DataflowGraphPublisher& publisher,const DataflowGraphCompletion& completion) {
			return session.Matches(run,session.plan,run.token,run.ticket,session.inputSlot,io)
				&& run.model.Commit(publisher,completion);
		}
		bool ReadWorkerSnapshot(unsigned c,DataflowWorkerSnapshot& s) { return io.ReadWorkerSnapshot(c,s); }
		bool CoreReady(unsigned c,const DataflowWorkerSnapshot& s) { return io.CoreReady(c,s); }
		bool StageGraphRun(unsigned c,const DataflowGraphRunDescriptor& d) { return io.StageGraphRun(c,d); }
		bool GraphRunMatches(unsigned c,const DataflowGraphRunDescriptor& d) { return io.GraphRunMatches(c,d); }
		bool PublishGraphCommand(unsigned c,unsigned sequence) { return io.PublishGraphCommand(c,sequence); }
		bool WaitForCompletion(unsigned c,unsigned sequence,DataflowWorkerSnapshot& s) { return io.WaitForCompletion(c,sequence,s); }
		bool AuditGraphCore(unsigned c,const DataflowGraphRunDescriptor& d,const DataflowWorkerSnapshot& s,bool decode) {
			return io.AuditGraphCore(c,d,s,decode);
		}
		bool ReadArgmaxResult(unsigned sequence,DataflowArgmaxResult& result) { return io.ReadArgmaxResult(sequence,result); }
		bool Acknowledge(unsigned c,unsigned sequence) { return io.Acknowledge(c,sequence); }
	};
	template<class IO> struct FaultAdapter {
		IO& io;
		bool FenceGraphHostWrites() { return io.FenceGraphHostWrites(); }
		void Pause() { io.Pause(); }
		bool ReadWorkerSnapshot(unsigned c,DataflowWorkerSnapshot& s) { return io.ReadWorkerSnapshot(c,s); }
		bool AuditGraphRecoveryCore(unsigned c,const DataflowWorkerSnapshot& s) {
			return io.AuditGraphRecoveryCore(c,s);
		}
		bool Acknowledge(unsigned c,unsigned sequence) { return io.AcknowledgeFault(c,sequence); }
	};

	DataflowGraphSessionResult Retain() {
		faulted=true;if (service) service->RetainFaultedGraph();return DataflowGraphSessionRetained;
	}
public:
	DataflowGraphSession()=default;
	DataflowGraphSession(const DataflowGraphSession&)=delete;
	DataflowGraphSession& operator=(const DataflowGraphSession&)=delete;
	bool Faulted() const { return faulted; }
	DataflowGraphSessionRun* RetainedRun() const { return faulted?active:nullptr; }

	// A settled fault permits service close only; it never promotes the
	// candidate or makes this session usable for inference again. The caller
	// keeps the audit and retained run alive through close and final release.
	template<class IO> DataflowGraphFaultAuditResult SettleFaultForClose(
		DataflowGraphFaultAudit& audit,unsigned pollLimit,IO& io) {
		if (!bound || !faulted || !active || !service
			|| !service->OwnsFaultedGraph(active->publisher)
			|| !io.GraphFaultSessionSafe(this,*service,plan,inputSlot))
			return DataflowGraphFaultAuditInvalid;
		FaultAdapter<IO> adapter{io};
		const auto result=audit.Settle(active->publisher,pollLimit,adapter);
		if (result!=DataflowGraphFaultAuditSettled) return result;
		return service->PermitFaultedGraphClose(active->publisher,audit)
			?DataflowGraphFaultAuditSettled:DataflowGraphFaultAuditInvalid;
	}

	// The supplied stream is already recorded and sealed. The IO check must
	// bind it to this session's immutable bank claim, slot and audited routes.
	template<class IO> bool Bind(const DataflowStreamPlan& sealed,const unsigned (&bankRoutes)[2][Banks],
		unsigned slot,DataflowProjectionAuthority*& liveAuthority,DataflowServiceOwner& resident,IO& io) {
		if (bound || faulted || !liveAuthority || !resident.Ready() || !io.GraphSessionSafe(resident) || slot>=ActivationBuffers
			|| sealed.count!=DataflowGraphDecodeSteps || !io.SealedStreamMatches(sealed,slot)
			|| !liveAuthority->PlanningRoutesMatch(bankRoutes)) return false;
		const auto* manifest=liveAuthority->Manifest();const auto* graph=liveAuthority->GraphWorkspace();
		DataflowBoundWorkspace workspace;
		if (!manifest || !graph || !workspace.Initialize(*liveAuthority)
			|| !DataflowStreamPlanBuilder::Validate(workspace,*manifest,*graph,liveAuthority->PlanningRope(),bankRoutes,sealed)
			|| !io.ReserveGraphSession(this,sealed,slot)) return false;
		plan=sealed;for (unsigned n=0;n<2;n++) for (unsigned b=0;b<Banks;b++) routes[n][b]=bankRoutes[n][b];
		inputSlot=slot;live=&liveAuthority;service=&resident;bound=true;return true;
	}

	// Caller holds the global session lock and owns run and candidate on heap.
	// Only a complete return transfers candidate to *live and returns the old
	// authority for deferred deletion. No output is exposed on any failure.
	template<class IO> DataflowGraphSessionResult Run(DataflowGraphSessionRun& run,
		DataflowProjectionAuthority& candidate,unsigned position,unsigned token,bool decode,unsigned pollLimit,
		DataflowProjectionProgram& projectionScratch,IO& io,DataflowArgmaxResult& output,
		DataflowProjectionAuthority*& retired,unsigned& stage) {
		output={Vocabulary,0,0,0};retired=nullptr;stage=DataflowDecoderSequenceInvalidRequest;
		if (!bound || faulted || active || run.attempted || !live || !*live || !service
			|| !service->Ready() || !pollLimit || pollLimit>DataflowGraphRunMaxPolls) return DataflowGraphSessionInvalid;
		run.attempted=true;
		unsigned trigger=0;
		if (!service->BeginGraph(run.publisher,trigger)) return DataflowGraphSessionPreflight;
		active=&run;
		const auto* manifest=(*live)->Manifest();
		const unsigned count=decode?DataflowGraphDecodeSteps:DataflowGraphPromptSteps;
		if (!manifest || !issuer.Issue(count,run.ticket)
			|| !DataflowDynamicTokenContext::Build(*manifest,token,position,run.token)
			|| !run.model.Prepare(*live,candidate,run.publisher,run.ticket,run.token,inputSlot,projectionScratch,stage)) {
			if (!service->CancelGraphBeforePublication(run.publisher)) return Retain();
			active=nullptr;return DataflowGraphSessionPreflight;
		}
		Adapter<IO> adapter{*this,run,io};
		const auto published=run.publisher.Start(plan,routes,run.token,run.ticket,trigger,pollLimit,inputSlot,adapter,run.scratch);
		if (published!=DataflowGraphPublished) {
			if (!run.publisher.Unpublished() || !service->CancelGraphBeforePublication(run.publisher)) return Retain();
			active=nullptr;return DataflowGraphSessionPreflight;
		}
		DataflowArgmaxResult result{};
		if (run.completion.Finish(run.publisher,plan,routes,run.token,adapter,run.scratch,result)!=DataflowGraphCompletionAudited
			|| !service->CommitGraph(run.publisher,run.completion,adapter)) return Retain();
		retired=run.model.Retired();
		if (!retired || *live!=&candidate) return Retain();
		output=result;active=nullptr;return DataflowGraphSessionComplete;
	}
};
}
#endif
#endif
