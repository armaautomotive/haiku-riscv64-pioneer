/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_RUNTIME_ATTENTION_H
#define PIONEER_RUNTIME_ATTENTION_H
#include "PioneerAttentionTile.h"
#include "PioneerRuntimeDual.h"

// Qualified immutable code regions are disjoint; only the one-word TRISC0
// entry is shared. Switch it only while BOTH existing workers are proved
// held and NoC/PCI quiet. No core reset, DMA or payload upload here.
template<class IO>
bool PioneerRuntimeAttentionPeersHeld(IO& io,const tt_dram_samples_result& baseline,
	const tt_compute_noc_result& firstNoc,const tt_compute_noc_result& secondNoc,
	PioneerResidentLease& firstStore,PioneerResidentLease& secondStore,
	PioneerRuntimeMatvecLease& firstCompute,PioneerRuntimeMatvecLease& secondCompute,
	PioneerAttentionTileLease& attention)
{
	if (!firstCompute.ready || !secondCompute.ready || !firstCompute.bulk || !secondCompute.bulk
		|| !firstCompute.f32_scales || !secondCompute.f32_scales || !firstCompute.q8 || !secondCompute.q8
		|| firstCompute.running || secondCompute.running || firstCompute.unsafe || secondCompute.unsafe
		|| attention.running || attention.unsafe || firstNoc.version!=1 || secondNoc.version!=2) return false;
	bool okay=true;
	for (unsigned core=0;core<2;core++) {
		if (!io.SelectRuntimeCore(core)) { okay=false; break; }
		const auto& noc=core?secondNoc:firstNoc;
		const auto& store=core?secondStore:firstStore;
		if (!PioneerAttentionTileHeld(io,baseline,noc,store)) { okay=false; break; }
		uint32_t entry=io.RuntimeCodeRead(0);
		if (entry!=0x0006a06f && (core || !attention.ready || entry!=TT_ATTENTION_TILE_BOOT)) { okay=false; break; }
	}
	if (!io.SelectRuntimeCore(0)) okay=false;
	if (!okay) {
		attention.unsafe=true; firstCompute.unsafe=true; secondCompute.unsafe=true;
		firstStore.recovery_required=true; secondStore.recovery_required=true; io.QuarantineHostPage();
	}
	return okay;
}
template<class IO>
bool PioneerRuntimeAttentionSelect(IO& io,const tt_dram_samples_result& baseline,
	const tt_compute_noc_result& firstNoc,const tt_compute_noc_result& secondNoc,
	PioneerResidentLease& firstStore,PioneerResidentLease& secondStore,
	PioneerRuntimeMatvecLease& firstCompute,PioneerRuntimeMatvecLease& secondCompute,
	PioneerAttentionTileLease& attention,bool useAttention)
{
	if (!attention.ready || !attention.saved || !PioneerRuntimeAttentionPeersHeld(io,baseline,firstNoc,secondNoc,
		firstStore,secondStore,firstCompute,secondCompute,attention)) return false;
	uint32_t selected=useAttention?uint32_t(TT_ATTENTION_TILE_BOOT):0x0006a06f;
	if (io.RuntimeCodeRead(0)==selected) return true;
	io.RuntimeCodeWrite(0,selected);
	if (io.RuntimeCodeRead(0)==selected) return true;
	attention.unsafe=true; firstCompute.unsafe=true; secondCompute.unsafe=true;
	firstStore.recovery_required=true; secondStore.recovery_required=true; io.QuarantineHostPage();
	return false;
}
#endif
