/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_QWEN38_COHORT_HOST_H
#define PIONEER_QWEN38_COHORT_HOST_H
#include "PioneerQwen38FleetPlan.h"
#include "PioneerQwen38Activation.h"
#include "../../src/add-ons/kernel/drivers/misc/PioneerQ38CohortProtocol.h"
#include <memory>
// Host DATA adapter for an authenticated,immutable GGUF source and its exact
// tiled plan. No FD/open/ioctl/DMA/coordinates/CPU product or fallback here.
// This is NOT an installed inference backend. Caller must authenticate the
// FULL same source before future upload/execution,not just validate metadata.
struct PioneerQ38CohortInput {
	unsigned type=0,inputs=0;
	uint32_t activation[TT_Q38_COHORT_INPUT_WORDS]={},scales[TT_Q38_COHORT_SCALE_WORDS]={};
};
static inline bool PioneerQ38CohortQuantize(unsigned type,unsigned inputs,const float* source,PioneerQ38CohortInput& output)
{
	if (!source || !PioneerQ38PanelProbeBytes(type) || (inputs!=5120 && inputs!=6144 && inputs!=17408)) return false;
	PioneerQ38CohortInput stage; stage.type=type; stage.inputs=inputs;
	unsigned count=PioneerQ38CohortBlockInputs(type);
	for (unsigned block=0;block<inputs/count;block++) {
		PioneerQ38Activation0 zero; PioneerQ38ActivationK k;
		if (type==8) {
			if (!PioneerQ38Quantize0(source+block*count,count,&zero)) return false;
			stage.scales[block]=PioneerAttentionBits(PioneerHalfFloat(uint16_t(zero.words[0])));
		} else {
			if (!PioneerQ38QuantizeK(source+block*count,count,&k)) return false;
			stage.scales[block]=k.words[0];
		}
		uint32_t magnitude=stage.scales[block]&0x7fffffff;
		if (magnitude && (magnitude<0x2f800000 || magnitude>0x47800000)) return false; // kernel's explicit probe range
		for (unsigned lane=0;lane<count;lane++) {
			unsigned rawByte=type==8?lane+2:lane;
			uint32_t word=type==8?zero.words[rawByte/4]:k.words[1+rawByte/4];
			int value=int((word>>((rawByte%4)*8))&255); if (value>=128) value-=256;
			unsigned at=block*count+lane;
			stage.activation[at/2]|=uint32_t(PioneerInt8Operand(value))<<((at%2)*16);
		}
	}
	output=stage; return true;
}
class PioneerQ38CohortWave {
	const PioneerQwen38Gguf* source=nullptr;
	const PioneerQwen38PackedPlan* packed=nullptr;
	const PioneerQwen38FleetPlan* schedule=nullptr; // all three owners outlive this wave
	uint64_t epoch=0;
	unsigned boundTensor=851,boundWave=0;
public:
	unsigned type=0,inputs=0,outputs=0,workers=0,tensor=851,wave=0;
	tt_q38_cohort_part part[120]={}; unsigned firstRow[120]={};
	bool Ready() const {
		if (!source || !packed || !schedule || source->Epoch()!=epoch || !source->Stable()
			|| !schedule->MetadataMatches(*source,*packed) || schedule->selected!=boundTensor
			|| tensor!=boundTensor || wave!=boundWave || boundTensor>=source->Tensors().size()) return false;
		const auto& t=source->Tensors()[boundTensor];
		if (type!=t.type || inputs!=t.inputs || outputs!=t.outputs) return false;
		unsigned active=0;
		for (const auto& j:schedule->jobs) if (j.wave==boundWave) {
			if (j.worker!=active || active>=120 || part[active].bank!=j.bank || part[active].groups!=j.groups
				|| part[active].offset!=j.proposedOffset || firstRow[active]!=j.firstRow) return false;
			active++;
		}
		if (!active || workers!=active) return false;
		for (unsigned w=active;w<120;w++) if (part[w].bank || part[w].groups || part[w].offset || firstRow[w]) return false;
		return source->Stable(); // fresh AFTER no-IO traversal too;no lifetime file cache
	}
	bool Build(const PioneerQwen38Gguf& model,const PioneerQwen38PackedPlan& layout,const PioneerQwen38FleetPlan& schedule,unsigned number)
	{
		PioneerQ38CohortWave stage;
		if (!schedule.MatchesSource(model,layout) || schedule.selected>=model.Tensors().size()) return false;
		const auto& t=model.Tensors()[schedule.selected];
		stage.type=t.type; stage.inputs=unsigned(t.inputs); stage.outputs=unsigned(t.outputs); stage.tensor=schedule.selected; stage.wave=number;
		// Revalidate fragment identity against THIS layout,not merely an
		// equally shaped second model or a user-supplied physical pointer.
		// This is private immutable metadata: authenticate its owner before
		// and after the loop, not twice (and multiple fstats) per worker.
		const auto& parts=layout.parts;
		for (const auto& j:schedule.jobs) if (j.wave==number) {
			if (j.worker!=stage.workers || j.worker>=120 || j.fragment>=parts.size()) return false;
			const auto& p=parts[j.fragment];
			unsigned groupBytes=16*PioneerQ38TiledRowBytes(t.type,t.inputs);
			if (p.kind!=PIONEER_Q38_WEIGHT || p.tensor!=stage.tensor || p.bank!=j.bank || !groupBytes || p.firstRow>j.firstRow
				|| j.firstRow>stage.outputs || j.groups*16>stage.outputs-j.firstRow || j.firstRow-p.firstRow>p.rows
				|| j.groups*16>p.rows-(j.firstRow-p.firstRow) || j.bytes!=j.groups*groupBytes
				|| j.proposedOffset!=uint64_t(p.proposedAddress)-layout.Base+uint64_t((j.firstRow-p.firstRow)/16)*groupBytes) return false;
			stage.part[j.worker]={j.bank,j.groups,j.proposedOffset}; stage.firstRow[j.worker]=j.firstRow; stage.workers++;
		}
		if (!stage.workers || !schedule.MatchesSource(model,layout)) return false;
		stage.source=&model; stage.packed=&layout; stage.schedule=&schedule; stage.epoch=model.Epoch();
		stage.boundTensor=schedule.selected; stage.boundWave=number;
		*this=stage; return true;
	}
};
static inline std::unique_ptr<tt_q38_cohort_probe> PioneerQ38CohortRequest(const PioneerQ38CohortWave& wave,const PioneerQ38CohortInput& input)
{
	if (!wave.Ready() || wave.type!=input.type || wave.inputs!=input.inputs) return nullptr;
	auto request=std::unique_ptr<tt_q38_cohort_probe>(new(std::nothrow) tt_q38_cohort_probe{}); if (!request) return nullptr;
	request->version=TT_Q38_COHORT_ABI; request->type=wave.type; request->inputs=wave.inputs; request->workers=wave.workers;
	memcpy(request->part,wave.part,sizeof(wave.part));
	memcpy(request->activation,input.activation,sizeof(input.activation)); memcpy(request->scales,input.scales,sizeof(input.scales));
	if (!PioneerQ38CohortValid(*request) || !wave.Ready()) return nullptr;
	return request;
}
static inline bool PioneerQ38CohortResultProof(const tt_q38_cohort_probe& job)
{
	if (!PioneerQ38CohortValid(job)) return false;
	const auto& result=job.result;
	if (result.version!=1 || result.state!=TT_RUNTIME_PASSED || result.workers!=job.workers || result.started!=job.workers
		|| result.completed!=job.workers || result.stopped!=job.workers || result.recovery_required) return false;
	for (unsigned w=0;w<job.workers;w++) {
		const auto& core=result.core[w]; unsigned blocks=job.inputs/PioneerQ38CohortBlockInputs(job.type),reads=job.part[w].groups*blocks;
		unsigned flits=PioneerQ38CohortBytes(job.type,job.inputs,job.part[w].groups)/64;
		if (w) { reads+=(job.inputs*2+16383)/16384+1; flits+=(job.inputs*2+((blocks*4+63)&~63U))/64; }
		if (core.version!=1 || core.state!=TT_RUNTIME_PASSED || core.prepared!=1 || core.context_reset!=1 || core.started!=1 || core.completed!=1
			|| core.stopped!=1 || core.recovery_required || core.phase!=0x7200c || core.panels!=job.part[w].groups*(job.inputs/16)
			|| core.marker!=0x31483851 || core.pc[0]!=0x72038 || core.pc[1]!=core.pc[0]) return false;
		for (unsigned c=0;c<8;c++) if (uint32_t(core.after[c]-core.before[c])!=(c==1?flits:(c==0 || c==2 || c==3 || c==6)?reads:0)) return false;
		for (unsigned row=0;row<256;row++) {
			uint32_t bits=job.output[w][row];
			if (row<job.part[w].groups*16) { if ((bits&0x7f800000)==0x7f800000) return false; }
			else if (bits) return false;
		}
	}
	const tt_q38_panel_result zero={};
	for (unsigned w=job.workers;w<120;w++) {
		if (memcmp(&job.result.core[w],&zero,sizeof(zero))) return false;
		for (unsigned row=0;row<256;row++) if (job.output[w][row]) return false;
	}
	return true;
}
// Writes ONLY the caller's private full-matrix scratch after ALL proof/input/
// result checks. The future session publishes that scratch only when EVERY
// wave succeeds. No reference dot product or host numerical substitution.
static inline bool PioneerQ38CohortCollect(const PioneerQ38CohortWave& wave,const PioneerQ38CohortInput& input,
	const tt_q38_cohort_probe& request,float* scratch,unsigned outputs)
{
	if (!wave.Ready() || !scratch || outputs!=wave.outputs || input.type!=wave.type || input.inputs!=wave.inputs
		|| request.type!=wave.type || request.inputs!=wave.inputs || request.workers!=wave.workers
		|| memcmp(request.part,wave.part,sizeof(wave.part)) || memcmp(request.activation,input.activation,sizeof(input.activation))
		|| memcmp(request.scales,input.scales,sizeof(input.scales)) || !PioneerQ38CohortResultProof(request)
		|| !PioneerQ38Disjoint(reinterpret_cast<const float*>(&request),sizeof(request)/4,scratch,outputs)) return false;
	for (unsigned w=0;w<wave.workers;w++) for (unsigned row=0;row<wave.part[w].groups*16;row++) {
		if (wave.firstRow[w]>outputs || row>=outputs-wave.firstRow[w]) return false;
	}
	if (!wave.Ready()) return false;
	for (unsigned w=0;w<wave.workers;w++) for (unsigned row=0;row<wave.part[w].groups*16;row++)
		scratch[wave.firstRow[w]+row]=PioneerAttentionFloat(request.output[w][row]);
	return true;
}
#endif
