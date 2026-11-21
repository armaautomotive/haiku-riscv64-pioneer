/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_QWEN35_DATAFLOW_RUNTIME_RECOVERY_H
#define PIONEER_QWEN35_DATAFLOW_RUNTIME_RECOVERY_H

#include <stdint.h>

namespace PioneerQwen35 {

enum : unsigned {
	DataflowRuntimeRecoveryNoCInitiators=4,
	DataflowRuntimeRecoveryNoCQueues=32,
	DataflowRuntimeRecoveryNoCStatusCount=36,
	DataflowRuntimeRecoveryNoCIdleDetailBase=0x100,
	DataflowRuntimeRecoveryMappingDetail=2,
	DataflowRuntimeRecoveryPollLimit=65536
};

template<class IO>
bool DataflowRuntimeRecoveryNoCIdleScan(IO& io,unsigned& busyIndex,
	uint32_t& busyValue)
{
	busyIndex=DataflowRuntimeRecoveryNoCStatusCount;
	busyValue=0;
	for (unsigned initiator=0;initiator<DataflowRuntimeRecoveryNoCInitiators;
		initiator++) {
		const uint32_t value=io.ResidentNocRead(initiator*0x800+0x40);
		if (value) {
			busyIndex=initiator;
			busyValue=value;
			return false;
		}
	}
	for (unsigned queue=0;queue<DataflowRuntimeRecoveryNoCQueues;queue++) {
		const uint32_t value=io.ResidentNocRead(0x240+queue*4);
		if (value&0xffU) {
			busyIndex=DataflowRuntimeRecoveryNoCInitiators+queue;
			busyValue=value;
			return false;
		}
	}
	return true;
}

// Waits only for a previously failed, read-only weight transfer to leave the
// device's NoC command registers quiet. It never clears a command or resets a
// core. A timeout leaves the caller's recovery latch untouched; detail values
// 0x100..0x123 identify the first register that stayed busy.
template<class IO>
bool DataflowRuntimeWaitForNoCIdle(IO& io,unsigned core,unsigned pollLimit,
	unsigned& detail)
{
	detail=0;
	if (core>=120 || !pollLimit || !io.SelectRuntimeCore(core)
		|| !io.MapQ38Panel() || !io.DataflowPanelMappingStable()) {
		detail=DataflowRuntimeRecoveryMappingDetail;
		return false;
	}
	unsigned polls=0;
	for (;;) {
		if (!io.DataflowPanelMappingStable()) {
			detail=DataflowRuntimeRecoveryMappingDetail;
			return false;
		}
		unsigned busyIndex=DataflowRuntimeRecoveryNoCStatusCount;
		uint32_t busyValue=0;
		if (DataflowRuntimeRecoveryNoCIdleScan(io,busyIndex,busyValue))
			return true;
		if (busyIndex>=DataflowRuntimeRecoveryNoCStatusCount) {
			detail=DataflowRuntimeRecoveryMappingDetail;
			return false;
		}
		const unsigned address=busyIndex<DataflowRuntimeRecoveryNoCInitiators
			?busyIndex*0x800+0x40
			:0x240+(busyIndex-DataflowRuntimeRecoveryNoCInitiators)*4;
		while (polls<pollLimit) {
			io.Pause();
			polls++;
			if (!io.DataflowPanelMappingStable()) {
				detail=DataflowRuntimeRecoveryMappingDetail;
				return false;
			}
			busyValue=io.ResidentNocRead(address);
			if (busyIndex>=DataflowRuntimeRecoveryNoCInitiators)
				busyValue&=0xffU;
			if (!busyValue) break;
		}
		if (polls>=pollLimit && busyValue) {
			detail=DataflowRuntimeRecoveryNoCIdleDetailBase+busyIndex;
			return false;
		}
	}
}

} // namespace PioneerQwen35

#endif
