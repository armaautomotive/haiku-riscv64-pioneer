/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_QWEN35_DATAFLOW_BOUND_WORKSPACE_H
#define PIONEER_QWEN35_DATAFLOW_BOUND_WORKSPACE_H

#include "PioneerQwen35DataflowAuthorization.h"

#ifdef PIONEER_QWEN35_DEVICE_DISPATCH
namespace PioneerQwen35 {
// Reconstruct workspace boundaries from a manifest already validated against
// the live initialized-page ledger. This is not a substitute for that audit.
class DataflowBoundWorkspace {
	uint32_t start[Banks]{};
	bool ready=false;
	bool Observe(const RowSpace& space) {
		if (!space.Valid()) return false;
		for (unsigned i=0;i<space.count;i++) {
			const auto& p=space.parts[i];
			const uint64_t bytes=uint64_t(p.rows)*space.rowBytes;
			const uint64_t end=uint64_t(p.address)+((bytes+4095)&~UINT64_C(4095));
			if (p.bank>=Banks || end>BankLimit || end<=p.address) return false;
			if (end>start[p.bank]) start[p.bank]=uint32_t(end);
		}
		return true;
	}
public:
	bool Initialize(const DataflowProjectionAuthority& authority) {
		if (ready || !authority.Bound() || authority.Pending()) return false;
		const auto* m=authority.Manifest();if (!m) return false;
		for (unsigned b=0;b<Banks;b++) start[b]=BankBase;
		for (unsigned layer=0;layer<=Layers;layer++) {
			const unsigned roles=layer==Layers?2U:Dense(layer)?11U:unsigned(LayerRoles);
			for (unsigned role=0;role<roles;role++) {
				const auto* w=m->Weight(layer,role);if (!w || !Observe(w->place)) return false;
			}
		}
		for (unsigned layer=0;layer<Layers;layer++) {
			if (Dense(layer)) { if (!Observe(m->layers[layer].kv)) return false; }
			else for (unsigned slot=0;slot<2;slot++)
				if (!Observe(m->layers[layer].delta[slot]) || !Observe(m->layers[layer].conv[slot])) return false;
		}
		const Range rope=authority.PlanningRope();
		if (rope.bank!=0 || rope.address!=start[0] || rope.bytes!=m->context*Rotary*sizeof(float)) return false;
		for (unsigned b=0;b<Banks;b++) if ((start[b]&4095)
			|| uint64_t(start[b])+ActivationWorkspaceBytes>BankLimit) return false;
		ready=true;return true;
	}
	bool Workspace(unsigned bank,uint32_t& address,uint32_t& bytes) const {
		if (!ready || bank>=Banks) return false;
		address=start[bank];bytes=ActivationWorkspaceBytes;return true;
	}
};
}
#endif
#endif
