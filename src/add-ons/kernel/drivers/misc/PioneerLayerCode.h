/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_LAYER_CODE_H
#define PIONEER_LAYER_CODE_H
#include "PioneerCoordinatorProbe.h"
#include "PioneerLayerProof.h"
#include "BlackholeLayerCoordinatorImage.h"
#include "PioneerWideOutput.h"
struct PioneerLayerLease {
	bool saved=false,dirty=false,ready=false,running=false,unsafe=false;
};
struct PioneerLayerJob {
	PioneerLayerLease* code;
	uint64_t upOffset,downOffset;
	unsigned operation;
	PioneerWideOutputLease* wide;
	PioneerLayerJob(PioneerLayerLease* lease,uint64_t up,uint64_t down,unsigned op=4,PioneerWideOutputLease* out=NULL)
		:code(lease),upOffset(up),downOffset(down),operation(op),wide(out) {}
};
template<class IO> auto PioneerLayerMapping(IO& io,int)->decltype(io.LayerMappingStable()) { return io.LayerMappingStable(); }
template<class IO> bool PioneerLayerMapping(IO&,long) { return false; }
template<class IO>
bool PioneerLayerPrepare(IO& io,const tt_dram_samples_result& baseline,const tt_compute_noc_result& noc,
	PioneerResidentLease& store,const PioneerCoordinatorLease& stack,PioneerLayerLease& lease)
{
	if (!stack.ready || !stack.saved || !stack.fast || !stack.fused || !stack.compact || stack.readback || stack.running || stack.unsafe) return false;
	if (lease.ready) return !lease.running && !lease.unsafe && PioneerLayerMapping(io,0);
	if (lease.saved || lease.unsafe || !io.LayerSavedWords() || !PioneerLayerMapping(io,0)
		|| !PioneerCoordinatorLocalHeld(io,baseline,noc,store)) return false;
	for (unsigned i=0;i<TT_LAYER_CODE_WORDS;i++) io.LayerSavedWords()[i]=io.LayerCodeRead(i);
	lease.saved=true;
	if (!PioneerCoordinatorLocalHeld(io,baseline,noc,store) || !PioneerLayerMapping(io,0)) { lease.unsafe=true; return false; }
	lease.dirty=true;
	for (unsigned i=0;i<TT_LAYER_CODE_WORDS;i++) io.LayerCodeWrite(i,BlackholeLayerCodeWord(i));
	bool exact=true;
	for (unsigned i=0;i<TT_LAYER_CODE_WORDS;i++) exact &= io.LayerCodeRead(i)==BlackholeLayerCodeWord(i);
	lease.ready=exact; return exact;
}
template<class IO>
bool PioneerLayerClose(IO& io,const tt_dram_samples_result& baseline,const tt_compute_noc_result& noc,
	PioneerResidentLease& store,PioneerLayerLease& lease,bool safe)
{
	if (!lease.saved) return safe && !lease.unsafe;
	safe=safe && !lease.unsafe && !lease.running && io.LayerSavedWords() && PioneerLayerMapping(io,0)
		&& PioneerCoordinatorLocalHeld(io,baseline,noc,store);
	if (safe && lease.dirty) {
		for (unsigned i=0;i<TT_LAYER_CODE_WORDS;i++) io.LayerCodeWrite(i,io.LayerSavedWords()[i]);
		bool exact=true;
		for (unsigned i=0;i<TT_LAYER_CODE_WORDS;i++) exact &= io.LayerCodeRead(i)==io.LayerSavedWords()[i];
		if (exact) lease.dirty=false;
	}
	lease.ready=false; lease.unsafe |= !safe || lease.dirty;
	if (lease.unsafe) { store.recovery_required=true; io.QuarantineHostPage(); }
	return !lease.unsafe;
}
#endif
