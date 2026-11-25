/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_QWEN35_DATAFLOW_GRAPH_MODEL_TRANSACTION_H
#define PIONEER_QWEN35_DATAFLOW_GRAPH_MODEL_TRANSACTION_H

#include "PioneerQwen35DataflowGraphMetadata.h"
#include "PioneerQwen35DataflowGraphCompletion.h"

#ifdef PIONEER_QWEN35_DEVICE_DISPATCH
namespace PioneerQwen35 {

// Session-owned, one-run transaction. The session supplies a distinct heap-owned
// candidate and holds its exclusive lock through Prepare, publication,
// audit and Commit. The stream owner separately proves a sealed immutable plan,
// service state and ticket freshness. On failure it retains all objects; this
// class neither executes tensors nor makes an uncertain device state reusable.
class DataflowGraphModelTransaction {
	enum Phase { Empty,Prepared,Committed,Rejected } phase=Empty;
	DataflowProjectionAuthority** liveSlot=nullptr;
	DataflowProjectionAuthority* original=nullptr;
	DataflowProjectionAuthority* candidate=nullptr;
	DataflowProjectionAuthority* retired=nullptr;
	const DataflowGraphPublisher* owner=nullptr;
	DataflowGraphRunTicket runTicket{};
	DataflowDynamicTokenContext runToken{};
	DataflowGraphAuthorityState originalState{},candidateState{};
	unsigned inputSlot=ActivationBuffers;
	bool BaseUnchanged() const {
		return phase==Prepared && liveSlot && *liveSlot==original && original && candidate
			&& original->GraphStateMatches(originalState) && candidate->GraphStateMatches(candidateState);
	}
public:
	DataflowGraphModelTransaction()=default;
	DataflowGraphModelTransaction(const DataflowGraphModelTransaction&)=delete;
	DataflowGraphModelTransaction& operator=(const DataflowGraphModelTransaction&)=delete;
	// Keep a measurable frame boundary for the non-loadable graph stack audit.
	__attribute__((noinline)) bool Prepare(DataflowProjectionAuthority*& live,DataflowProjectionAuthority& after,const DataflowGraphPublisher& publisher,
		const DataflowGraphRunTicket& ticket,const DataflowDynamicTokenContext& token,
		unsigned slot,DataflowProjectionProgram& scratch,unsigned& stage)
	{
		if (phase!=Empty) return false;
		phase=Rejected;
		if (!live || &after==live || !publisher.Unpublished()
			|| !ticket.epoch || !ticket.firstSequence || ticket.firstSequence>DataflowWorkerSequenceLimit
			|| (ticket.count!=DataflowGraphPromptSteps && ticket.count!=DataflowGraphDecodeSteps)
			|| ticket.count-1>DataflowWorkerSequenceLimit-ticket.firstSequence || slot>=ActivationBuffers)
			return false;
		const auto* manifest=live->Manifest();DataflowDynamicTokenContext canonical{};
		if (!manifest || !DataflowDynamicTokenContext::Build(*manifest,token.token,token.position,canonical)
			|| !DataflowDynamicTokenMatches(canonical,token) || !live->CaptureGraphState(originalState)
			|| !DataflowGraphMetadata::Prepare(*live,after,token.position,token.token,slot,
				ticket.count==DataflowGraphDecodeSteps,scratch,stage)
			|| !after.CaptureGraphState(candidateState)) return false;
		liveSlot=&live;original=live;candidate=&after;owner=&publisher;
		runTicket=ticket;runToken=token;inputSlot=slot;phase=Prepared;return true;
	}
	bool Matches(const DataflowDynamicTokenContext& token,const DataflowGraphRunTicket& ticket,unsigned slot) const {
		return BaseUnchanged() && slot==inputSlot && ticket.epoch==runTicket.epoch
			&& ticket.firstSequence==runTicket.firstSequence && ticket.count==runTicket.count
			&& DataflowDynamicTokenMatches(token,runToken);
	}
	bool Commit(const DataflowGraphPublisher& publisher,const DataflowGraphCompletion& completion) {
		if (!BaseUnchanged() || owner!=&publisher || !publisher.Published()
			|| !publisher.TokenMatches(runToken) || publisher.InputSlot()!=inputSlot
			|| !Matches(runToken,publisher.Ticket(),publisher.InputSlot())
			|| !completion.AuditedFor(publisher)) return false;
		retired=original;*liveSlot=candidate;candidate=nullptr;phase=Committed;return true;
	}
	// Only the session owner may destroy this retired authority, after it has
	// ruled out all outstanding references. The new live authority is not ours.
	DataflowProjectionAuthority* Retired() const { return phase==Committed?retired:nullptr; }
};
}
#endif
#endif
