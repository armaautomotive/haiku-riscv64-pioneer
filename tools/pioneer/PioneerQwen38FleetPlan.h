/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_QWEN38_FLEET_PLAN_H
#define PIONEER_QWEN38_FLEET_PLAN_H
#include "PioneerQwen38PackedPlan.h"
// OFFLINE scheduling blueprint ONLY. No coordinates,MMIO,allocation,upload,
// start/reset or model capability. In particular proposedOffset can be64B
// aligned,NOT page aligned:installed49/50's page-offset ABI CANNOT consume
// this plan. A future PRIVATE tensor registry/complete initialized-extent/
// per-worker route proof is required before any native implementation.
struct PioneerQ38FleetPart {
	unsigned worker,wave,fragment,bank,firstRow,groups,bytes;
	uint64_t proposedOffset;
};
class PioneerQwen38FleetPlan {
	const PioneerQwen38Gguf* source=nullptr;
	const PioneerQwen38PackedPlan* packed=nullptr;
	uint64_t epoch=0;
	unsigned selected=0,workerCount=0;
	std::vector<PioneerQ38FleetPart> jobs,empty;
public:
	bool Ready() const { return source && packed && source->Epoch()==epoch && source->Stable() && packed->Tiled() && packed->MatchesSource(*source); }
	bool MatchesSource(const PioneerQwen38Gguf& model,const PioneerQwen38PackedPlan& layout) const { return Ready() && source==&model && packed==&layout; }
	const std::vector<PioneerQ38FleetPart>& Jobs() const { return Ready()?jobs:empty; }
	unsigned Tensor() const { return Ready()?selected:851; }
	unsigned Workers() const { return Ready()?workerCount:0; }
	bool Build(const PioneerQwen38Gguf& model,const PioneerQwen38PackedPlan& plan,unsigned tensor,unsigned workers=120,unsigned maxGroups=16) {
		source=nullptr; packed=nullptr; epoch=0; workerCount=0; jobs.clear();
		if (!model.Stable() || !plan.Tiled() || !plan.MatchesSource(model) || tensor>=model.Tensors().size() || !workers || workers>120 || !maxGroups || maxGroups>16) return false;
		const auto& t=model.Tensors()[tensor];
		unsigned rowBytes=PioneerQ38TiledRowBytes(t.type,t.inputs),groupBytes=rowBytes*16;
		if (!t.type || !rowBytes || !t.outputs || (t.outputs&15) || (t.inputs!=5120 && t.inputs!=6144 && t.inputs!=17408) || (groupBytes&63)) return false;
		struct Piece { unsigned fragment,groups,quota,done=0,emitted=0; const PioneerQ38PackedPart* part; };
		std::vector<Piece> pieces; bool banks[7]={}; unsigned minJobs=0,total=0;
		for (unsigned i=0;i<plan.Parts().size();i++) {
			const auto& p=plan.Parts()[i]; if (p.kind!=PIONEER_Q38_WEIGHT || p.tensor!=tensor) continue;
			if (p.bank>=7 || banks[p.bank] || !p.rows || (p.firstRow&15) || (p.rows&15) || p.firstRow>t.outputs || p.rows>t.outputs-p.firstRow
				|| p.rowBytes!=rowBytes || uint64_t(p.rows)*rowBytes!=p.bytes || (p.proposedAddress&4095) || p.proposedAddress<plan.Base
				|| !p.allocated || (p.allocated&4095) || p.bytes>p.allocated || p.allocated-p.bytes>=4096
				|| uint64_t(p.proposedAddress)-plan.Base>=plan.Capacity || p.allocated>plan.Capacity-(uint64_t(p.proposedAddress)-plan.Base)) return false;
			banks[p.bank]=true; unsigned groups=p.rows/16,quota=(groups+maxGroups-1)/maxGroups;
			pieces.push_back({i,groups,quota,0,0,&p}); total+=groups; minJobs+=quota;
		}
		std::sort(pieces.begin(),pieces.end(),[](const Piece& a,const Piece& b){return a.part->firstRow<b.part->firstRow;});
		unsigned row=0;
		for (const auto& piece:pieces) { if (piece.part->firstRow!=row) return false; row+=piece.part->rows; }
		if (row!=t.outputs || !total || total!=t.outputs/16) return false;
		unsigned active=total<workers?total:workers,target=minJobs<active?active:minJobs;
		for (unsigned extra=minJobs;extra<target;extra++) {
			unsigned best=unsigned(pieces.size());
			for (unsigned i=0;i<pieces.size();i++) {
				if (pieces[i].quota==pieces[i].groups) continue;
				if (best==pieces.size() || uint64_t(pieces[i].groups)*pieces[best].quota>uint64_t(pieces[best].groups)*pieces[i].quota) best=i;
			}
			if (best==pieces.size()) return false;
			pieces[best].quota++;
		}
		std::sort(pieces.begin(),pieces.end(),[](const Piece& a,const Piece& b){return a.part->bank<b.part->bank;});
		std::vector<PioneerQ38FleetPart> stage; stage.reserve(target);
		while (stage.size()<target) {
			bool progress=false;
			for (auto& piece:pieces) {
				if (piece.emitted==piece.quota) continue;
				unsigned groups=piece.groups/piece.quota+unsigned(piece.emitted<piece.groups%piece.quota);
				uint64_t offset=uint64_t(piece.part->proposedAddress)-plan.Base+uint64_t(piece.done)*groupBytes;
				unsigned bytes=groups*groupBytes,ordinal=unsigned(stage.size());
				if (!groups || groups>maxGroups || (offset&63) || offset>=plan.Capacity || bytes>plan.Capacity-offset
					|| uint64_t(piece.done+groups)*groupBytes>piece.part->bytes) return false;
				stage.push_back({ordinal%workers,ordinal/workers,piece.fragment,piece.part->bank,piece.part->firstRow+piece.done*16,groups,bytes,offset});
				piece.done+=groups; piece.emitted++; progress=true;
			}
			if (!progress) return false;
		}
		for (const auto& piece:pieces) if (piece.done!=piece.groups || piece.emitted!=piece.quota) return false;
		if (!model.Stable() || !plan.Tiled() || !plan.MatchesSource(model)) return false;
		jobs.swap(stage); source=&model; packed=&plan; epoch=model.Epoch(); selected=tensor; workerCount=workers; return true;
	}
};
#endif
