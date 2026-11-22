/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_QWEN35_DATAFLOW_STAGE_LEDGER_H
#define PIONEER_QWEN35_DATAFLOW_STAGE_LEDGER_H

#include "PioneerQwen35DataflowGraphRunProtocol.h"
#include <string.h>

namespace PioneerQwen35 {

// Owned by one locked, sealed graph session. A backend may Record only after
// its staging write has proved the entire descriptor and command marker.
// The ledger is not itself an MMIO or model-authority proof. It prevents
// duplicate, out-of-order, stale and partial-fleet publication and keeps an
// exact copy of each staged descriptor until its command is attempted.
class DataflowGraphStageLedger {
	DataflowGraphRunDescriptor staged[DataflowCoreCount]{};
	unsigned nextStage=0,nextPublish=0,current=0,previous=0;
	bool awaitingVerification=false,publishing=false;
public:
	DataflowGraphStageLedger()=default;
	DataflowGraphStageLedger(const DataflowGraphStageLedger&)=delete;
	DataflowGraphStageLedger& operator=(const DataflowGraphStageLedger&)=delete;
	bool CanStage(unsigned core,const DataflowGraphRunDescriptor& descriptor) const {
		if (publishing || awaitingVerification || core!=nextStage || core>=DataflowCoreCount
			|| !DataflowGraphRunValid(descriptor,core)) return false;
		if (!core) return descriptor.triggerSequence==previous+1;
		const auto& root=staged[0];
		return descriptor.triggerSequence==current && descriptor.pollLimit==root.pollLimit
			&& descriptor.inputSlot==root.inputSlot && descriptor.ticket.epoch==root.ticket.epoch
			&& descriptor.ticket.firstSequence==root.ticket.firstSequence
			&& descriptor.ticket.count==root.ticket.count && descriptor.binding.epoch==root.binding.epoch
			&& descriptor.binding.count==root.binding.count
			&& DataflowDynamicTokenMatches(descriptor.token,root.token)
			&& !memcmp(descriptor.routes,root.routes,sizeof(root.routes));
	}
	bool Record(unsigned core,const DataflowGraphRunDescriptor& descriptor) {
		if (!CanStage(core,descriptor)) return false;
		if (!core) current=descriptor.triggerSequence;
		staged[core]=descriptor;nextStage++;awaitingVerification=true;return true;
	}
	bool Verify(unsigned core,const DataflowGraphRunDescriptor& descriptor) {
		if (!awaitingVerification || !nextStage || core!=nextStage-1
			|| memcmp(&staged[core],&descriptor,sizeof(descriptor))) return false;
		awaitingVerification=false;return true;
	}
	const DataflowGraphRunDescriptor* BeginPublish(unsigned core,unsigned sequence) {
		if (awaitingVerification || nextStage!=DataflowCoreCount || core!=nextPublish
			|| core>=DataflowCoreCount || sequence!=current
			|| staged[core].triggerSequence!=sequence) return nullptr;
		publishing=true;const auto* result=&staged[core];nextPublish++;
		if (nextPublish==DataflowCoreCount) {
			previous=current;current=0;nextStage=nextPublish=0;publishing=false;
		}
		return result;
	}
};
}
#endif
