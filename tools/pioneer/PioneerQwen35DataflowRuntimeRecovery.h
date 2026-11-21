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
	DataflowRuntimeRecoveryEmbeddingFailureTag=0x400,
	DataflowRuntimeRecoveryPollLimit=65536
};

// The ioctl has eleven recovery-detail bits. Preserve a marker, the four-bit
// NoC transfer stage, and a six-bit index. Indices 0..35 are NoC status
// registers, 36..43 are completion counters, 44 is the initiator doorbell,
// 45..54 are command words, 55 is route identity, and 56 is descriptor
// validation. The full status value travels in the adjacent 32-bit result.
inline unsigned DataflowRuntimeRecoveryPackEmbeddingFailure(unsigned detail)
{
	const unsigned stage=(detail>>8)&0xffU;
	const unsigned index=detail&0xffU;
	unsigned encodedIndex=63;
	if ((stage==4 || stage==8) && index<10) encodedIndex=45U+index;
	else if (stage==7 && index>=0x80U && index<0x88U)
		encodedIndex=36U+index-0x80U;
	else if ((stage==6) && index==0xffU) encodedIndex=44;
	else if (stage==3) encodedIndex=55;
	else if (stage==1 || stage==10) encodedIndex=56;
	else if (index<DataflowRuntimeRecoveryNoCStatusCount) encodedIndex=index;
	return DataflowRuntimeRecoveryEmbeddingFailureTag
		|((stage&0xfU)<<6)|(encodedIndex&0x3fU);
}

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
	unsigned& detail,uint32_t* busyStatus=nullptr)
{
	detail=0;
	if (busyStatus) *busyStatus=0;
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
			if (busyStatus) *busyStatus=busyValue;
			return false;
		}
	}
}

} // namespace PioneerQwen35

#endif
