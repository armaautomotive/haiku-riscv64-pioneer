/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_DISPATCH_HOST_H
#define PIONEER_Q38_DISPATCH_HOST_H
#include "PioneerQwen38CohortHost.h"
#include "../../src/add-ons/kernel/drivers/misc/PioneerQ38DispatchProof.h"
// Read-only userspace reply COHERENCE,NOT kernel authorization. Physical
// snapshots/source seals in the reply are trusted ONLY after the guarded
// ioctl has independently validated its PRIVATE boot/extent authority.
// This helper has no FD/IO/start/reset/route-writing capability.
class PioneerQ38DispatchHistory {
	uint32_t root=0,peer[119]={},source[120][16]={};
	bool seen[120]={};
public:
	bool Accept(const tt_q38_dispatch_probe& job) {
		if (!PioneerQ38DispatchValid(job) || root>=0x7ffffffe) return false;
		auto a=std::unique_ptr<PioneerQ38DispatchAuthority>(new(std::nothrow) PioneerQ38DispatchAuthority{}); if (!a) return false;
		a->nextRoot=root+1;
		for (unsigned c=0;c<8;c++) { a->before[c]=job.result.before[c]; a->after[c]=job.result.after[c]; }
		for (unsigned c=0;c<16;c++) a->source[0][c]=seen[0]?source[0][c]:job.result.stats[32+c];
		const unsigned routes[]={18|(12<<6),18|(15<<6),18|(18<<6),17|(12<<6),17|(15<<6),17|(18<<6),17|(21<<6)};
		for (unsigned w=0;w<job.peers;w++) {
			if (peer[w]>=0x7ffffffe) return false;
			a->nextPeer[w]=peer[w]+1;
			for (unsigned c=0;c<16;c++) a->source[w+1][c]=seen[w+1]?source[w+1][c]:job.result.peer[w][32+c];
			auto& e=a->extent[w]; const auto& p=job.part[w];
			e.address=uint32_t(0x04000000+p.offset); e.route=routes[p.bank];
			e.bytes=PioneerQ38CohortBytes(job.type,job.inputs,p.groups);
			e.blocks=p.groups*(job.inputs/PioneerQ38CohortBlockInputs(job.type));
			e.reads=e.blocks+(job.inputs*2+16383)/16384+1;
			e.flits=e.bytes/64+(job.inputs*2+(((job.inputs/PioneerQ38CohortBlockInputs(job.type))*4+63)&~63U))/64;
		}
		if (!PioneerQ38DispatchProof(job,*a)) return false;
		root=a->nextRoot; seen[0]=true; memcpy(source[0],a->source[0],sizeof(source[0]));
		for (unsigned w=0;w<job.peers;w++) { peer[w]=a->nextPeer[w]; seen[w+1]=true; memcpy(source[w+1],a->source[w+1],sizeof(source[0])); }
		return true;
	}
};
static inline std::unique_ptr<tt_q38_dispatch_probe> PioneerQ38DispatchRequest(const PioneerQ38CohortWave& wave,const PioneerQ38CohortInput& input)
{
	if (!wave.Ready() || wave.workers>119 || wave.type!=input.type || wave.inputs!=input.inputs) return nullptr;
	auto request=std::unique_ptr<tt_q38_dispatch_probe>(new(std::nothrow) tt_q38_dispatch_probe{}); if (!request) return nullptr;
	request->version=1; request->type=wave.type; request->inputs=wave.inputs; request->peers=wave.workers;
	memcpy(request->part,wave.part,sizeof(request->part));
	memcpy(request->activation,input.activation,sizeof(input.activation)); memcpy(request->scales,input.scales,sizeof(input.scales));
	if (!PioneerQ38DispatchValid(*request) || !wave.Ready()) return nullptr;
	return request;
}
static inline bool PioneerQ38DispatchCollect(const PioneerQ38CohortWave& wave,const PioneerQ38CohortInput& input,
	const tt_q38_dispatch_probe& request,PioneerQ38DispatchHistory& history,float* scratch,unsigned outputs)
{
	if (!wave.Ready() || wave.workers>119 || !scratch || outputs!=wave.outputs || input.type!=wave.type || input.inputs!=wave.inputs
		|| request.type!=wave.type || request.inputs!=wave.inputs || request.peers!=wave.workers
		|| memcmp(request.part,wave.part,sizeof(request.part)) || memcmp(request.activation,input.activation,sizeof(input.activation))
		|| memcmp(request.scales,input.scales,sizeof(input.scales))
		|| !PioneerQ38Disjoint(reinterpret_cast<const float*>(&request),sizeof(request)/4,scratch,outputs)) return false;
	for (unsigned w=0;w<wave.workers;w++) if (wave.firstRow[w]>outputs || wave.part[w].groups*16>outputs-wave.firstRow[w]) return false;
	if (!wave.Ready() || !history.Accept(request)) return false;
	for (unsigned w=0;w<wave.workers;w++) for (unsigned row=0;row<wave.part[w].groups*16;row++)
		scratch[wave.firstRow[w]+row]=PioneerAttentionFloat(request.output[w][row]);
	return true;
}
#endif
