/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_INITIALIZE_OWNED_STATE_H
#define PIONEER_Q38_INITIALIZE_OWNED_STATE_H
#include "PioneerQ38StateZeroController.h"
// OFFLINE private transaction,NO public/native operation selects it. Caller
// owns the exact bank/context/identity/call seal/global lock/clock. Bounded
// one-shot initialization of ALL48x2x48 heads;never a partial-ready model.
struct PioneerQ38InitializeOwnedResult {
	uint32_t version=1,regions=0,jobs=0,pages=0,closed=0,published=0,recovery_required=0;
};
template<class IO> bool PioneerQ38InitializeOwnedState(IO& io,const tt_dram_samples_result& dram,const tt_compute_noc_result& noc,
	PioneerQ38GdnBanks& state,PioneerResidentBanks& banks,const PioneerQ38GdnIdentity& identity,const tt_host_page_result& input,
	PioneerResidentLease& store,PioneerBulkUploadLease& bulk,PioneerOutputBufferLease& transfer,PioneerWideOutputLease& wide,
	PioneerQ38StateZeroLease& lease,PioneerQ38InitializeOwnedResult& result)
{
	result={}; result.version=1; unsigned regions=0,planned=0,progress=0;
	if (lease.saved || lease.dirty || lease.ready || lease.running || lease.unsafe || lease.closed || lease.nonce
		|| !io.StateZeroBeforeCohort() || !state.InitializationRegions(identity,regions)
		|| !state.InitializationProgress(identity,progress) || progress || state.InitializationPublished(identity)) return false;
	// Complete geometry validation BEFORE first IO. Private Bind already
	// proves coverage/disjoint ownership;this is not a caller address list.
	for (unsigned region=0;region<regions;region++) {
		unsigned pages=0;
		if (!state.InitializationPages(identity,region,pages) || !pages || pages>768 || (pages&15)
			|| planned>PioneerQ38GdnZeroProgress::Pages-pages) return false;
		planned+=pages;
	}
	if (planned!=PioneerQ38GdnZeroProgress::Pages) return false;
	auto fail=[&]() {
		result.recovery_required=1; result.published=0;
		return PioneerQ38StateZeroFail(io,state,banks,store,bulk,transfer,wide,lease);
	};
	if (!PioneerQ38StateZeroPrepare(io,dram,noc,state,banks,identity,input,store,bulk,transfer,wide,lease)) {
		result.recovery_required=lease.unsafe; return false;
	}
	for (unsigned region=0;region<regions;region++) {
		unsigned pages=0; if (!state.InitializationPages(identity,region,pages)) return fail();
		for (unsigned first=0;first<pages;) {
			unsigned chunk=pages-first; if (chunk>256) chunk=256;
			PioneerQ38StateZeroResult job;
			if (!PioneerQ38StateZeroRun(io,dram,noc,state,banks,input,store,bulk,transfer,wide,lease,identity,region,first,chunk,job)) return fail();
			result.jobs++; result.pages+=chunk; first+=chunk;
		}
		result.regions++;
	}
	if (result.pages!=PioneerQ38GdnZeroProgress::Pages
		|| !PioneerQ38StateZeroClose(io,dram,noc,state,banks,input,store,bulk,transfer,wide,lease,true)
		|| !lease.closed || !state.InitializationPublished(identity)) return fail();
	result.closed=result.published=1; return true; // zero-init ONLY,NOT compute/read/model authority
}
#endif
