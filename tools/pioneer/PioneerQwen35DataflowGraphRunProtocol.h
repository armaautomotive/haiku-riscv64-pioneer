/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_QWEN35_DATAFLOW_GRAPH_RUN_PROTOCOL_H
#define PIONEER_QWEN35_DATAFLOW_GRAPH_RUN_PROTOCOL_H

#include "PioneerQwen35DataflowDeviceGraph.h"
#include "PioneerQwen35DataflowDynamicToken.h"
#include "PioneerQwen35DataflowConvProgress.h"

namespace PioneerQwen35 {
enum : unsigned {
	DataflowGraphRunAddress=0xab100,
	DataflowGraphRunBytes=512,
	DataflowGraphProgressAddress=DataflowGraphRunAddress+DataflowGraphRunBytes,
	DataflowGraphOperationProgressAddress=DataflowGraphProgressAddress+sizeof(uint32_t),
	DataflowGraphRunMagic=0x31524735,
	DataflowGraphRunVersion=1,
	DataflowGraphRunCommand=0x31434735,
	DataflowGraphFaultMagic=0x31464735,
	DataflowGraphProgressMagic=0x51350000,
	DataflowGraphRunMaxPolls=1048576,
#ifdef PIONEER_QWEN35_PAIRED_RECURRENCE
	DataflowGraphDecodeSteps=524,
	DataflowGraphPromptSteps=521
#else
	DataflowGraphDecodeSteps=548,
	DataflowGraphPromptSteps=545
#endif
};

// This single-word observation is diagnostic only: it does not prove that a
// transaction has settled or permit the host to release graph resources.
enum DataflowGraphProgressPhase : unsigned {
	DataflowGraphProgressFetch=1, DataflowGraphProgressAuthorize=2,
	DataflowGraphProgressExecute=3, DataflowGraphProgressBarrier=4,
	DataflowGraphProgressResetQueues=5
};
inline uint32_t DataflowGraphProgressWord(unsigned step,unsigned phase) {
	return step<DataflowGraphDecodeSteps && phase>=DataflowGraphProgressFetch
		&& phase<=DataflowGraphProgressResetQueues
		?DataflowGraphProgressMagic | (step<<3) | phase:0;
}
inline bool DataflowGraphProgressDetail(uint32_t word,unsigned& detail) {
	const unsigned step=(word&0xffffU)>>3,phase=word&7U;
	if ((word&0xffff0000U)!=DataflowGraphProgressMagic
		|| !DataflowGraphProgressWord(step,phase)) return false;
	detail=0x40000000U | (step<<4) | phase;return true;
}

inline uint32_t DataflowConvProgressWord(uint32_t graph,unsigned group,unsigned stage) {
	unsigned detail=0;
	return DataflowGraphProgressDetail(graph,detail)
		&& (detail&0xfU)==DataflowGraphProgressExecute && group<8
		&& stage>=DataflowConvProgressQueues && stage<=DataflowConvProgressFinished
		?0xc5000000U | ((graph&0x1fffU)<<10) | (group<<5) | stage:0;
}
inline bool DataflowConvProgressDetail(uint32_t graph,uint32_t word,unsigned& detail) {
	const unsigned group=(word>>5)&7U,stage=word&31U;
	if (!word || word!=DataflowConvProgressWord(graph,group,stage)) return false;
	detail=0x40000000U | (group<<5) | stage;return true;
}

// On a retained device-graph failure, the worker records its graph fault,
// completed step and propagated peer status before publishing failure. The
// host reads these only from a stable terminal mailbox.
inline bool DataflowGraphFaultDetail(const DataflowWorkerMailbox& mailbox,
	unsigned& detail,unsigned& status) {
	if (mailbox.reserved[0]!=DataflowGraphFaultMagic || !mailbox.reserved[1]
		|| mailbox.reserved[1]>DataflowGraphQueueFault
		|| mailbox.reserved[2]>DataflowGraphDecodeSteps
		|| mailbox.reserved[3]>DataflowCoreCount) return false;
	detail=0x80000000U | mailbox.reserved[1] | (mailbox.reserved[2]<<4)
		| (mailbox.reserved[3]<<14);
	status=mailbox.reserved[4];return true;
}
static_assert(Layers==32,"graph completion counts require the fixed Qwen35 decoder");
static_assert(DataflowBarrierReleaseCommit+DataflowBarrierPacketBytes==DataflowGraphRunAddress
	&& DataflowGraphRunAddress+DataflowGraphRunBytes==DataflowGraphProgressAddress
	&& DataflowGraphOperationProgressAddress+sizeof(uint32_t)<=DataflowBarrierArrivalBase,
	"run descriptor and progress word stay inside saved L1 and outside barrier slots");

enum : unsigned {
	DataflowGraphRunFixedBytes=6*sizeof(uint32_t)+sizeof(DataflowGraphRunTicket)
		+sizeof(DataflowStreamBinding)+sizeof(Range)+2*Banks*sizeof(uint32_t)
		+sizeof(DataflowDynamicTokenContext)+sizeof(uint32_t)
};
static_assert(unsigned(DataflowGraphRunFixedBytes)<=unsigned(DataflowGraphRunBytes)
	&& (DataflowGraphRunFixedBytes%4)==0,"graph descriptor fields fit one fixed record");
struct alignas(64) DataflowGraphRunDescriptor {
	uint32_t magic,version,core,triggerSequence,pollLimit,inputSlot;
	DataflowGraphRunTicket ticket;
	DataflowStreamBinding binding;
	Range lease;
	unsigned routes[2][Banks];
	DataflowDynamicTokenContext token;
	uint32_t checksum;
	uint32_t reserved[(DataflowGraphRunBytes-DataflowGraphRunFixedBytes)/4];
};
static_assert(sizeof(DataflowGraphRunDescriptor)==DataflowGraphRunBytes,
	"the run descriptor occupies exactly one 512-byte L1 slot");

inline uint32_t DataflowGraphRunChecksum(const DataflowGraphRunDescriptor& d) {
	const auto* bytes=reinterpret_cast<const uint8_t*>(&d);
	uint32_t hash=2166136261U;
	for (unsigned i=0;i<offsetof(DataflowGraphRunDescriptor,checksum);i+=4) {
		const uint32_t word=uint32_t(bytes[i]) | (uint32_t(bytes[i+1])<<8)
			| (uint32_t(bytes[i+2])<<16) | (uint32_t(bytes[i+3])<<24);
		hash=(hash^word)*16777619U;
	}
	return hash;
}
// Validate the inputs before staging any worker descriptor. This avoids
// constructing and hashing a throwaway 512-byte record in the preflight
// pass. The caller supplies the actual target core, not a core number taken
// from the binding itself.
inline bool DataflowGraphRunInputsValid(const DataflowStreamBinding& binding,const Range& lease,
	const unsigned (&routes)[2][Banks],const DataflowDynamicTokenContext& token,
	const DataflowGraphRunTicket& ticket,unsigned trigger,unsigned pollLimit,unsigned inputSlot,unsigned core) {
	return core<DataflowCoreCount && binding.core==core && trigger && trigger<=DataflowWorkerSequenceLimit
		&& pollLimit && pollLimit<=DataflowGraphRunMaxPolls && inputSlot<ActivationBuffers
		&& ticket.epoch && ticket.firstSequence && ticket.firstSequence<=DataflowWorkerSequenceLimit
		&& (binding.count==DataflowGraphPromptSteps || binding.count==DataflowGraphDecodeSteps)
		&& (ticket.count==DataflowGraphPromptSteps || ticket.count==DataflowGraphDecodeSteps)
		&& ticket.count<=binding.count
		&& ticket.count-1<=DataflowWorkerSequenceLimit-ticket.firstSequence
		&& token.Valid() && DataflowStreamBindingMatches(binding,lease,routes,core);
}
inline bool DataflowGraphRunValid(const DataflowGraphRunDescriptor& d,unsigned core) {
	if (d.magic!=DataflowGraphRunMagic || d.version!=DataflowGraphRunVersion
		|| d.core!=core || !DataflowGraphRunInputsValid(d.binding,d.lease,d.routes,d.token,
			d.ticket,d.triggerSequence,d.pollLimit,d.inputSlot,core)
		|| d.checksum!=DataflowGraphRunChecksum(d)) return false;
	for (unsigned word:d.reserved) if (word) return false;
	return true;
}

// Model ownership and ticket freshness are proved by the session owner, not
// by this byte checksum or by a user-supplied descriptor.
inline bool DataflowGraphRunBuild(const DataflowStreamBinding& binding,const Range& lease,
	const unsigned (&routes)[2][Banks],const DataflowDynamicTokenContext& token,
	const DataflowGraphRunTicket& ticket,unsigned trigger,unsigned pollLimit,unsigned inputSlot,
	DataflowGraphRunDescriptor& output)
{
	DataflowGraphRunDescriptor d{};
	d.magic=DataflowGraphRunMagic;d.version=DataflowGraphRunVersion;d.core=binding.core;
	d.triggerSequence=trigger;d.pollLimit=pollLimit;d.inputSlot=inputSlot;d.ticket=ticket;d.binding=binding;
	d.lease=lease;d.token=token;
	for (unsigned n=0;n<2;n++) for (unsigned b=0;b<Banks;b++) d.routes[n][b]=routes[n][b];
	d.checksum=DataflowGraphRunChecksum(d);
	if (!DataflowGraphRunValid(d,binding.core)) return false;
	output=d;return true;
}
}
#endif
