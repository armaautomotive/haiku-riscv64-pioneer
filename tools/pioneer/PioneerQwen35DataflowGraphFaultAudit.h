/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_QWEN35_DATAFLOW_GRAPH_FAULT_AUDIT_H
#define PIONEER_QWEN35_DATAFLOW_GRAPH_FAULT_AUDIT_H

#include "PioneerQwen35DataflowGraphPublisher.h"
#include <string.h>

namespace PioneerQwen35 {

enum DataflowGraphFaultAuditResult {
	DataflowGraphFaultAuditInvalid=0, DataflowGraphFaultAuditFenceFailed,
	DataflowGraphFaultAuditUnsettled, DataflowGraphFaultAuditCoreUnsafe,
	DataflowGraphFaultAuditChanged, DataflowGraphFaultAuditAckFailed,
	DataflowGraphFaultAuditReadyFailed, DataflowGraphFaultAuditSettled
};

// Scratch belongs to the retained graph owner, not the kernel stack. Success
// proves only that the workers are settled for a close attempt: it does not
// commit model state, release allocations, clear a cold latch, or prove that
// service snapshots have been restored. The backend must fence all earlier
// host writes and independently audit BOTH NoCs, queues, routes, worker image,
// and graph peer/fetch traffic for each core. If any proof is unavailable,
// the owner remains retained. The caller serializes this with all card access.
class DataflowGraphFaultAudit {
	DataflowWorkerSnapshot first[DataflowCoreCount]{};
	bool terminal[DataflowCoreCount]{};
	unsigned failedCore=DataflowCoreCount;
	bool attempted=false;
	const DataflowGraphPublisher* settledPublisher=nullptr;
	const DataflowGraphPublisher* acknowledgingPublisher=nullptr;

	static bool Classify(const DataflowWorkerSnapshot& s,unsigned previous,
		unsigned trigger,bool& isTerminal) {
		isTerminal=false;
		if (DataflowWorkerSnapshotReady(s))
			return s.completedSequence==previous || s.completedSequence==trigger;
		if (s.commandSequence!=trigger || s.completedSequence!=trigger
			|| s.acknowledgedSequence!=previous) return false;
		if (s.state==DataflowWorkerComplete && !s.failure && s.phase==2)
			{ isTerminal=true;return true; }
		if (s.state==DataflowWorkerFailed && s.failure && s.phase==5)
			{ isTerminal=true;return true; }
		return false;
	}
public:
	DataflowGraphFaultAudit()=default;
	DataflowGraphFaultAudit(const DataflowGraphFaultAudit&)=delete;
	DataflowGraphFaultAudit& operator=(const DataflowGraphFaultAudit&)=delete;
	unsigned FailedCore() const { return failedCore; }
	bool SettledFor(const DataflowGraphPublisher& publisher) const {
		return settledPublisher==&publisher;
	}
	// Valid only inside Settle's acknowledgement loop, after both full
	// fleet audits. The exact terminal snapshot and sequence are retained.
	bool AcknowledgementPermittedFor(const DataflowGraphPublisher& publisher,
		unsigned core,unsigned sequence) const {
		return acknowledgingPublisher==&publisher && core<DataflowCoreCount && terminal[core]
			&& sequence==publisher.TriggerSequence() && first[core].commandSequence==sequence
			&& first[core].completedSequence==sequence && first[core].acknowledgedSequence<sequence;
	}
	// A failed attempt may have acknowledged only part of the fleet. A new
	// attempt starts with another host-write fence and re-audits all cores;
	// the mixed ready/terminal classification handles those acknowledgements.
	// A completed proof is one-shot and cannot be reset or reused.
	bool ResetUnsettled() {
		if (!attempted || settledPublisher) return false;
		attempted=false;failedCore=DataflowCoreCount;return true;
	}

	template<class IO> DataflowGraphFaultAuditResult Settle(
		const DataflowGraphPublisher& publisher,unsigned pollLimit,IO& io) {
		const unsigned previous=publisher.PreviousSequence(),trigger=publisher.TriggerSequence();
		if (attempted || (!publisher.Faulted() && !publisher.Published())
			|| !trigger || trigger!=previous+1 || trigger>DataflowWorkerSequenceLimit
			|| !pollLimit || pollLimit>DataflowGraphRunMaxPolls)
			return DataflowGraphFaultAuditInvalid;
		attempted=true;
		if (!io.FenceGraphHostWrites()) return DataflowGraphFaultAuditFenceFailed;
		for (unsigned core=0;core<DataflowCoreCount;core++) {
			bool settled=false;
			for (unsigned poll=0;poll<pollLimit;poll++) {
				DataflowWorkerSnapshot snapshot{};bool isTerminal=false;
				if (io.ReadWorkerSnapshot(core,snapshot)
					&& Classify(snapshot,previous,trigger,isTerminal)) { settled=true;break; }
				io.Pause();
			}
			if (!settled) { failedCore=core;return DataflowGraphFaultAuditUnsettled; }
		}
		for (unsigned pass=0;pass<2;pass++) {
			for (unsigned core=0;core<DataflowCoreCount;core++) {
				DataflowWorkerSnapshot s{};bool isTerminal=false;
				if (!io.ReadWorkerSnapshot(core,s) || !Classify(s,previous,trigger,isTerminal)
					|| !io.AuditGraphRecoveryCore(core,s)) {
					failedCore=core;return DataflowGraphFaultAuditCoreUnsafe;
				}
				if (!pass) { first[core]=s;terminal[core]=isTerminal; }
				else if (terminal[core]!=isTerminal || memcmp(&first[core],&s,sizeof(s))) {
					failedCore=core;return DataflowGraphFaultAuditChanged;
				}
			}
			if (!pass) io.Pause();
		}
		{
			acknowledgingPublisher=&publisher;
			struct ClearAcknowledgementPhase {
				const DataflowGraphPublisher*& publisher;
				~ClearAcknowledgementPhase() { publisher=nullptr; }
			} clear{acknowledgingPublisher};
			for (unsigned core=0;core<DataflowCoreCount;core++) {
				if (terminal[core] && !io.Acknowledge(core,trigger)) {
					failedCore=core;return DataflowGraphFaultAuditAckFailed;
				}
			}
		}
		if (!io.FenceGraphHostWrites()) return DataflowGraphFaultAuditFenceFailed;
		for (unsigned core=0;core<DataflowCoreCount;core++) {
			bool ready=false;
			for (unsigned poll=0;poll<pollLimit;poll++) {
				DataflowWorkerSnapshot s{};
				if (io.ReadWorkerSnapshot(core,s) && DataflowWorkerSnapshotReady(s)
					&& (s.completedSequence==previous || s.completedSequence==trigger)
					&& io.AuditGraphRecoveryCore(core,s)) { ready=true;break; }
				io.Pause();
			}
			if (!ready) { failedCore=core;return DataflowGraphFaultAuditReadyFailed; }
		}
		settledPublisher=&publisher;
		return DataflowGraphFaultAuditSettled;
	}
};
}
#endif
