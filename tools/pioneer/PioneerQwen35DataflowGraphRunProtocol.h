/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_QWEN35_DATAFLOW_GRAPH_RUN_PROTOCOL_H
#define PIONEER_QWEN35_DATAFLOW_GRAPH_RUN_PROTOCOL_H

#include "PioneerQwen35DataflowDeviceGraph.h"
#include "PioneerQwen35DataflowDynamicToken.h"

namespace PioneerQwen35 {
enum : unsigned {
	DataflowGraphRunAddress=0xab100,
	DataflowGraphRunBytes=512,
	DataflowGraphRunMagic=0x31524735,
	DataflowGraphRunVersion=1,
	DataflowGraphRunCommand=0x31434735,
	DataflowGraphRunMaxPolls=1048576
};
static_assert(DataflowBarrierReleaseCommit+DataflowBarrierPacketBytes==DataflowGraphRunAddress
	&& DataflowGraphRunAddress+DataflowGraphRunBytes<=DataflowBarrierArrivalBase,
	"run descriptor stays inside saved L1 and outside barrier slots");

enum : unsigned {
	DataflowGraphRunFixedBytes=6*sizeof(uint32_t)+sizeof(DataflowGraphRunTicket)
		+sizeof(DataflowStreamBinding)+sizeof(Range)+2*Banks*sizeof(uint32_t)
		+sizeof(DataflowDynamicTokenContext)+sizeof(uint32_t)
};
static_assert(unsigned(DataflowGraphRunFixedBytes)<=unsigned(DataflowGraphRunBytes)
	&& (DataflowGraphRunFixedBytes%4)==0,"graph descriptor fields fit one fixed record");
struct alignas(64) DataflowGraphRunDescriptor {
	uint32_t magic,version,core,triggerSequence,pollLimit,reserved0;
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
inline bool DataflowGraphRunValid(const DataflowGraphRunDescriptor& d,unsigned core) {
	if (d.magic!=DataflowGraphRunMagic || d.version!=DataflowGraphRunVersion
		|| core>=DataflowCoreCount || d.core!=core || !d.triggerSequence
		|| d.triggerSequence>DataflowWorkerSequenceLimit || !d.pollLimit
		|| d.pollLimit>DataflowGraphRunMaxPolls || d.reserved0
		|| !d.ticket.epoch || !d.ticket.firstSequence
		|| d.ticket.firstSequence>DataflowWorkerSequenceLimit
		|| !d.ticket.count || d.ticket.count!=d.binding.count
		|| d.ticket.count-1>DataflowWorkerSequenceLimit-d.ticket.firstSequence
		|| !d.token.Valid() || !DataflowStreamBindingMatches(d.binding,d.lease,d.routes,core)
		|| d.checksum!=DataflowGraphRunChecksum(d)) return false;
	for (unsigned word:d.reserved) if (word) return false;
	return true;
}

// Model ownership and ticket freshness are proved by the session owner, not
// by this byte checksum or by a user-supplied descriptor.
inline bool DataflowGraphRunBuild(const DataflowStreamBinding& binding,const Range& lease,
	const unsigned (&routes)[2][Banks],const DataflowDynamicTokenContext& token,
	const DataflowGraphRunTicket& ticket,unsigned trigger,unsigned pollLimit,
	DataflowGraphRunDescriptor& output)
{
	DataflowGraphRunDescriptor d{};
	d.magic=DataflowGraphRunMagic;d.version=DataflowGraphRunVersion;d.core=binding.core;
	d.triggerSequence=trigger;d.pollLimit=pollLimit;d.ticket=ticket;d.binding=binding;
	d.lease=lease;d.token=token;
	for (unsigned n=0;n<2;n++) for (unsigned b=0;b<Banks;b++) d.routes[n][b]=routes[n][b];
	d.checksum=DataflowGraphRunChecksum(d);
	if (!DataflowGraphRunValid(d,binding.core)) return false;
	output=d;return true;
}
}
#endif
