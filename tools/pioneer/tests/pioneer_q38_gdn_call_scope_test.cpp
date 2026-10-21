/* SPDX-License-Identifier: MIT */
#define PIONEER_Q38_STATE_ZERO_NATIVE_SCOPE_MODEL_ONLY
#include "pioneer_q38_state_zero_native_scope_test.cpp"
using status_t=int; using thread_id=int;
constexpr int B_BAD_VALUE=-4,B_NOT_ALLOWED=-3,B_BUSY=-2,B_DEV_NOT_READY=-1;
struct thread_info { int team; };
static int currentTeam=17,threadStatus=0,sLock=1,acquired=0,released=0,acquireStatus=B_OK;
static status_t get_thread_info(int,thread_info* info) { info->team=currentTeam; return threadStatus; }
static status_t acquire_sem(int lock) { assert(lock==sLock); acquired++; return acquireStatus; }
static void release_sem(int lock) { assert(lock==sLock); released++; }
#include "native_lock.inc"
#include "call_scope.inc"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38GdnFreshPlan.h"
#include <new>
#include "bind_owner.inc"
static void Reset() { currentTeam=17; threadStatus=0; acquired=released=0; acquireStatus=B_OK; }
#ifndef PIONEER_Q38_GDN_CALL_SCOPE_MODEL_ONLY
int main()
{
	(void)&Input; (void)&Prepare; (void)&Upload; (void)&Close;
	{
		Fixture f; Reset(); f.c.q38GdnCallThread=-1; assert(!f.c.io.StateZeroBeforeCohort());
		{ PioneerQ38GdnCallScope scope(&f.cookie); assert(scope.Status()==B_OK && scope.Context()==&f.c
			&& acquired==1 && !released && f.c.q38GdnCallThread==42 && f.c.io.StateZeroBeforeCohort()); }
		assert(released==1 && f.c.q38GdnCallThread==-1 && !f.c.io.StateZeroBeforeCohort() && !f.c.retained && !f.c.io.touches);
		assert(AcquireProbeLock(&f.cookie,true)==B_NOT_ALLOWED); // scope end is NOT mutable lifetime END
	}
	for (unsigned fault=0;fault<15;fault++) {
		Fixture f; Reset(); f.c.q38GdnCallThread=-1; PioneerProbeCookie foreign=f.cookie;
		PioneerProbeCookie* owner=&f.cookie;
		if (!fault) owner=nullptr; else if (fault==1) f.cookie.magic=0;
		else if (fault==2) currentThread=-1; else if (fault==3) threadStatus=-1;
		else if (fault==4) currentTeam=18; else if (fault==5) owner=&foreign;
		else if (fault==6) sColdRecoveryRequired=true; else if (fault==7) sReloadQuiesced=true;
		else if (fault==8) f.cookie.context=nullptr; else if (fault==9) sBankStorageOnly=false;
		else if (fault==10) f.c.bankStorageOnly=false; else if (fault==11) f.c.bytes-=4096;
		else if (fault==12) f.c.io.q38MutableContext=nullptr; else if (fault==13) f.c.q38GdnCallThread=43;
		else acquireStatus=B_BUSY;
		{ PioneerQ38GdnCallScope scope(owner); assert(scope.Status()!=B_OK && !scope.Context()); }
		assert(!f.c.io.touches && !f.c.retained && f.c.q38GdnCallThread==(fault==13?43:-1));
		assert(released==acquired-int(fault==14)); // failed acquisition owns no semaphore
	}
	for (unsigned fault=0;fault<5;fault++) {
		Fixture f; Reset(); f.c.q38GdnCallThread=-1;
		{
			PioneerQ38GdnCallScope scope(&f.cookie); assert(scope.Context()==&f.c);
			if (!fault) f.c.q38GdnCallThread=43; else if (fault==1) currentThread=43;
			else if (fault==2) f.cookie.context=nullptr; else if (fault==3) f.cookie.magic=0;
			else sRuntimeOwner=nullptr;
		}
		assert(f.c.retained && f.c.retainCalls==1 && f.zero.unsafe && f.state->Unsafe()
			&& sColdRecoveryRequired && acquired==1 && released==1 && !f.c.io.touches);
	}
	{
		Fixture f; Reset(); f.c.q38GdnCallThread=-1; f.c.otherUnsafe=true;
		{ PioneerQ38GdnCallScope scope(&f.cookie); assert(scope.Status()==B_DEV_NOT_READY && !scope.Context()); }
		assert(f.c.retained && f.c.retainCalls==1 && sColdRecoveryRequired && acquired==1 && released==1
			&& f.c.q38GdnCallThread==-1 && !f.c.io.touches); // NEVER grant a seal to pre-existing uncertainty
	}
	{
		Fixture f(false); Reset(); const PioneerQ38GdnIdentity identity={55,1,77,7};
		const auto before=f.initialized;
		assert(!BindQ38MutableOwner(f.c,identity) && !f.c.q38GdnBanks && !f.c.q38StateZero && !f.c.io.touches);
		{
			PioneerQ38GdnCallScope scope(&f.cookie); assert(scope.Context()==&f.c && BindQ38MutableOwner(f.c,identity));
			assert(f.c.q38GdnBanks && f.c.q38StateZero && f.c.q38GdnIdentity==identity && !f.c.io.q38SpanAllowed
				&& f.c.io.StateZeroBeforeCohort() && f.initialized==before && !f.c.io.touches);
			unsigned pages=UINT32_MAX; assert(f.c.q38GdnBanks->InitializationProgress(identity,pages) && !pages);
			auto* owner=f.c.q38GdnBanks; assert(!BindQ38MutableOwner(f.c,identity) && owner==f.c.q38GdnBanks);
			for (unsigned layer=0;layer<64;layer++) if (layer%4!=3) for (unsigned slot=0;slot<2;slot++) for (unsigned worker=0;worker<96;worker++) {
				PioneerQ38GdnBankExtent extent; assert(owner->InitializeHalf(identity,layer,slot,worker,extent)
					&& !f.banks.Range(f.c.io.dram,extent.bank,extent.address-PioneerResidentBanks::Base));
			}
		}
		assert(f.c.q38GdnCallThread==-1 && !f.c.retained && !f.c.io.touches && acquired==released);
	}
	for (unsigned fault=0;fault<14;fault++) {
		Fixture f(false); Reset(); PioneerQ38GdnIdentity identity={55,1,77,7};
		PioneerQ38GdnCallScope scope(&f.cookie); assert(scope.Context()==&f.c);
		if (!fault) identity.session=0; else if (fault==1) identity.driverGeneration=8;
		else if (fault==2) f.c.initialized=nullptr; else if (fault==3) f.c.otherUnsafe=true;
		else if (fault==4) f.c.runtimeCoreMode=2; else if (fault==5) f.c.io.runtimeCore=1;
		else if (fault==6) f.c.io.fleetEnabled=true; else if (fault==7) f.c.q38Dispatch=&f;
		else if (fault==8) f.c.q38GdnIdentity.session=1; else if (fault==9) f.c.q38Delta.saved=true;
		else if (fault==10) f.c.panelProbe.ready=true; else if (fault==11) f.c.q38Clock.snapshot.restored=1;
		else if (fault==12) std::fill(f.initialized.begin(),f.initialized.end(),1);
		else f.initialized.back()=2;
		assert(!BindQ38MutableOwner(f.c,identity) && !f.c.q38GdnBanks && !f.c.q38StateZero && f.c.io.q38SpanAllowed
			&& !f.c.retained && !f.c.io.touches);
	}
	{
		Fixture f(false); Reset(); const PioneerQ38GdnIdentity identity={55,1,77,7};
		PioneerQ38GdnCallScope scope(&f.cookie); assert(scope.Context()==&f.c);
		f.c.io.dram.source[1]^=1; // claim proof refuses AFTER owners are attached
		assert(!BindQ38MutableOwner(f.c,identity) && f.c.q38GdnBanks && f.c.q38StateZero && f.c.retained
			&& f.c.q38StateZero->unsafe && f.c.q38GdnBanks->Unsafe() && f.banks.Quarantined() && sColdRecoveryRequired
			&& !f.c.io.touches && !BindQ38MutableOwner(f.c,identity));
	}
	puts("PASS OFFLINE ACTUAL private GDN call scope+global lock:exact FD/team/thread/context/bank ownership,15 early/locked refusals,seal valid ONLY while lock held,5 destructor identity/owner/thread faults request MEMORY-ONLY full retention,balanced semaphore release,ZERO IO/END/STOP/ARC/reset/unmap/free;NO public/native capability");
	puts("PASS OFFLINE ACTUAL private owner binding:heap672-stripe plan/real bank claims/ALL9216 half extents,14 prepublication refusals leave ordinary scope intact,post-attachment claim-proof failure retains ALL private owners before return,initialized bitmap untouched,ZERO IO and NO initialization/model/compute-ready grant;NO public/native selection");
}
#endif
