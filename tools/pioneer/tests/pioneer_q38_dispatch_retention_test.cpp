/* SPDX-License-Identifier: MIT */
#define PIONEER_Q38_DISPATCH_CONTROL_MOCK_ONLY
#include "pioneer_q38_dispatch_control_test.cpp"
static unsigned genericCleanup=0;
// Native generic cleanup is NOT emulated. The real extracted gate MUST
// return before entering it after uncertainty;the real cookie release below
// must then retain its owner/context/reservation rather than delete them.
static void EndResidentSession(PioneerProbeCookie* cookie,tt_resident_session_result& result,bool)
{
	auto& c=*cookie->context; bool clean=!sColdRecoveryRequired && !c.Unsafe();
	if (!EndQ38DispatchOwnership(c,result,clean)) return;
#ifdef PIONEER_Q38_NORMAL_CLOCK
	if (!c.q38Clock.End(c.clockIO.Arc(),c.clockIO.Health(),clean)) { RetainQ38DispatchOwner(c,result); return; }
#endif
	genericCleanup++; delete cookie->context; cookie->context=nullptr;
	sRuntimeOwner=nullptr; sBankStorageOnly=false;
}
#include "pioneer_q38_dispatch_cookie_release.inc"
int main(int argc,char** argv)
{
	assert(argc==2);
	(void)&Input; (void)&Prepare; (void)&Upload; (void)&Close; (void)&PreparePanel; (void)&Run; (void)&Job; (void)&ClosePanel; (void)&Finish;
	(void)&End;
	for (unsigned fault=0;fault<3;fault++) {
		auto* cookie=new PioneerProbeCookie; Begin(*cookie,argv[1]); auto& c=*cookie->context;
		if (fault) {
			auto job=Request(); if (fault==1) c.io.rootFault=4;
			assert(Q38DispatchControl(cookie,TT_PCI_PROBE_Q38_DISPATCH_RUN,job.get(),sizeof(*job))==B_OK);
			if (fault==2) { c.io.closeFault=3; c.io.target=119; c.io.closing=true; }
		} else { c.lease.recovery_required=true; sColdRecoveryRequired=true; }
		unsigned writes=c.io.Writes(),selects=c.io.selects;
		assert(ReleaseResidentCookie(cookie)==B_OK && cookie->magic==0x50313030 && cookie->context==&c && sRuntimeOwner==cookie
			&& c.dispatchOwnerRetained && sColdRecoveryRequired && sBankStorageOnly && !genericCleanup);
		assert(c.bulkHost.quarantined && (!c.wideHost.allocated || c.wideHost.quarantined));
		if (fault!=2) assert(c.io.Writes()==writes && c.io.selects==selects);
		writes=c.io.Writes(); selects=c.io.selects; c.io.quarantined=true;
		assert(ReleaseResidentCookie(cookie)==B_OK && c.io.Writes()==writes && c.io.selects==selects && !genericCleanup);
		// Test-fabric disposal ONLY;production retains these until physical
		// cold recovery. There are no real device mappings/host pages here.
		delete cookie->context; delete cookie; sRuntimeOwner=nullptr; sBankStorageOnly=sColdRecoveryRequired=false;
	}
	auto* cookie=new PioneerProbeCookie; Begin(*cookie,argv[1]); auto job=Request();
	assert(Q38DispatchControl(cookie,TT_PCI_PROBE_Q38_DISPATCH_RUN,job.get(),sizeof(*job))==B_OK);
#ifdef TT_PIONEER_Q38_WORK_MODE
	// A CPU numeric mismatch is invisible to kernel metadata. Closing
	// after an otherwise passed RUN MUST NOT silently execute STOP/END.
	auto& retained=*cookie->context; unsigned writes=retained.io.Writes(),selects=retained.io.selects;
	assert(ReleaseResidentCookie(cookie)==B_OK && cookie->context==&retained && retained.dispatchOwnerRetained
		&& sColdRecoveryRequired && !genericCleanup && writes==retained.io.Writes() && selects==retained.io.selects);
	assert(ReleaseResidentCookie(cookie)==B_OK && writes==retained.io.Writes() && selects==retained.io.selects);
	delete cookie->context; delete cookie; sRuntimeOwner=nullptr; sBankStorageOnly=sColdRecoveryRequired=false;
	cookie=new PioneerProbeCookie; Begin(*cookie,argv[1]); job=Request();
	assert(Q38DispatchControl(cookie,TT_PCI_PROBE_Q38_WORK_RUN,job.get(),sizeof(*job))==B_OK);
	cookie->context->io.closing=true; tt_resident_session_result result={};
	EndResidentSession(cookie,result,true); // explicit END before close
#else
	cookie->context->io.closing=true;
#endif
	assert(ReleaseResidentCookie(cookie)==B_OK && genericCleanup==1 && !sRuntimeOwner && !sBankStorageOnly && !sColdRecoveryRequired);
#ifdef PIONEER_Q38_SPAN_PRELOAD
	for (unsigned state:{0U,1U,2U}) {
		cookie=new PioneerProbeCookie; Begin(*cookie,argv[1]); auto& c=*cookie->context;
		c.q38Span=new PioneerQ38SpanLease; c.q38Span->saved=c.q38Span->ready=true;
		c.q38Span->closed=state==2;
		unsigned writes=c.io.Writes(),selects=c.io.selects;
		if (state) assert(ReleaseResidentCookie(cookie)==B_OK);
		else { tt_resident_session_result r={}; bool clean=true;
			assert(!EndQ38DispatchOwnership(c,r,clean) && !clean); }
		assert(c.dispatchOwnerRetained && c.q38Span && c.q38Span->closed==(state==2) && sColdRecoveryRequired && genericCleanup==1
			&& writes==c.io.Writes() && selects==c.io.selects && c.bulkHost.quarantined && (!c.wideHost.allocated || c.wideHost.quarantined));
		writes=c.io.Writes(); assert(ReleaseResidentCookie(cookie)==B_OK && c.io.Writes()==writes);
		delete cookie->context; delete cookie; sRuntimeOwner=nullptr; sBankStorageOnly=sColdRecoveryRequired=false;
	}
	cookie=new PioneerProbeCookie; Begin(*cookie,argv[1]); cookie->context->q38Span=new PioneerQ38SpanLease;
	cookie->context->q38Span->closed=true;
	tt_resident_session_result spanEnd={}; EndResidentSession(cookie,spanEnd,true); // explicit BANK_END
	assert(ReleaseResidentCookie(cookie)==B_OK && genericCleanup==2 && !sRuntimeOwner && !sColdRecoveryRequired);
	puts("PASS REAL span lifetime handoff: premature BANK_END or implicit close retains cookie/context/PCI/BOTH host owners with ZERO device IO; completed SPAN_END plus explicit BANK_END permits normal cleanup");
#endif
	puts("PASS REAL extracted54 END gate+cookie release:preflight/run/terminal-STOP uncertainty retains cookie/context/PCI owner/BOTH host leases,no generic cleanup/unreserve/delete/reset/mapping restore or repeated IO;known clean STOP proceeds once;NO native hardware qualification");
#ifdef PIONEER_Q38_NORMAL_CLOCK
	for (unsigned fault=0;fault<3;fault++) {
		cookie=new PioneerProbeCookie; Begin(*cookie,argv[1]); auto& c=*cookie->context;
		assert(c.q38Clock.Begin(c.clockIO.Arc(),c.clockIO.Health()) && c.q38Clock.RequiresExplicitEnd());
		if (fault==1) c.q38Clock.Quarantine(); // external compute UNKNOWN
		else if (fault==2) { c.clockIO.timeout=true; assert(!c.q38Clock.End(c.clockIO.Arc(),c.clockIO.Health(),true)); }
		const unsigned writes=c.io.Writes(),selects=c.io.selects,clockIO=c.clockIO.io,cleanups=genericCleanup;
		assert(!c.q38Dispatch && !c.q38Span); // CLOCK ONLY must require explicit END too
		assert(ReleaseResidentCookie(cookie)==B_OK && cookie->context==&c && cookie->magic==0x50313030 && sRuntimeOwner==cookie);
		assert(c.dispatchOwnerRetained && c.q38Clock.Unsafe() && sColdRecoveryRequired && sBankStorageOnly
			&& c.bulkHost.quarantined && (!c.wideHost.allocated || c.wideHost.quarantined));
		assert(genericCleanup==cleanups && writes==c.io.Writes() && selects==c.io.selects && clockIO==c.clockIO.io && !c.clockIO.unmaps);
		assert(ReleaseResidentCookie(cookie)==B_OK && clockIO==c.clockIO.io && writes==c.io.Writes() && genericCleanup==cleanups);
		delete cookie->context; delete cookie; sRuntimeOwner=nullptr; sBankStorageOnly=sColdRecoveryRequired=false; // TEST disposal ONLY
	}
	puts("PASS REAL clock-only cookie release: healthy lost caller/compute UNKNOWN/clock timeout retain ENTIRE context+cookie+PCI+ALL hosts+clock views,NO idle/unmap/STOP/reset/retry even before first span or matrix");
#endif
#ifdef PIONEER_Q38_GDN_MUTABLE_STATE
	for (unsigned owner=0;owner<3;owner++) for (unsigned phase=0;phase<4;phase++) {
		cookie=new PioneerProbeCookie; Begin(*cookie,argv[1]); auto& c=*cookie->context;
		if (owner!=1) c.q38GdnBanks=new PioneerQ38GdnBanks;
		if (owner!=0) {
			c.q38StateZero=new PioneerQ38StateZeroLease;
			c.q38StateZero->ready=phase==1; c.q38StateZero->running=phase==2; c.q38StateZero->closed=phase==3;
		}
		const unsigned writes=c.io.Writes(),selects=c.io.selects; const int copyBefore=copyCalls;
#ifdef PIONEER_Q38_REUSE_ACTIVATIONS
		const uint64_t epoch=sQ38InputEpoch;
#endif
		assert(Q38MutableScopeOwned(cookie) && AcquireProbeLock(cookie,true)==B_NOT_ALLOWED);
		assert(AcquireProbeLock(cookie,false)==B_NOT_ALLOWED);
		PioneerProbeCookie foreign;
		assert(AcquireQ38GdnSessionLock(&foreign)==B_BUSY);
#ifdef PIONEER_Q38_LOCAL_DELTA_PROBE
		assert(AcquireQ38DeltaLock(cookie)==B_NOT_ALLOWED); // same owner is NOT an escape for another compute family
#endif
		auto attempted=Request();
		assert(Q38DispatchControl(cookie,TT_PCI_PROBE_Q38_WORK_RUN,attempted.get(),sizeof(*attempted))==B_NOT_ALLOWED);
		assert(!c.q38Dispatch && !c.dispatchOwnerRetained && !sColdRecoveryRequired && copyCalls==copyBefore+1
			&& writes==c.io.Writes() && selects==c.io.selects);
#ifdef PIONEER_Q38_REUSE_ACTIVATIONS
		assert(epoch==sQ38InputEpoch); // rejected locks cannot mutate reuse authority
#endif
		assert(AcquireQ38GdnSessionLock(cookie)==B_OK); release_sem(sLock); // CPU STATUS/explicit END namespace only
		sColdRecoveryRequired=true;
		assert(AcquireQ38GdnSessionLock(cookie)==B_DEV_NOT_READY && writes==c.io.Writes() && selects==c.io.selects);
		delete cookie->context; delete cookie; sRuntimeOwner=nullptr; sBankStorageOnly=sColdRecoveryRequired=false; // TEST disposal ONLY
	}
	puts("PASS ACTUAL gated GDN global lock:bank-owner/zero-owner/both and before/ready/running/closed phases block ordinary bank/legacy/Work/delta commands BEFORE IO or input-epoch mutation;only SAME-owner CPU STATUS/explicit END namespace allowed,foreign/cold owner refused;NO native GDN capability");
	for (unsigned variant=0;variant<10;variant++) for (unsigned explicitEnd=0;explicitEnd<2;explicitEnd++) {
		if (!variant && explicitEnd) continue; // owner-only explicit END has no saved zero image
		cookie=new PioneerProbeCookie; Begin(*cookie,argv[1]); auto& c=*cookie->context;
		c.q38GdnBanks=new PioneerQ38GdnBanks;
		if (variant) {
			c.q38StateZero=new PioneerQ38StateZeroLease;
			auto& zero=*c.q38StateZero;
			zero.saved=variant>=2; zero.ready=variant>=2;
			zero.running=variant==3 || variant==7;
			zero.unsafe=variant==4 || variant==6;
			zero.closed=variant>=5;
		}
		if (variant==8) c.q38GdnBanks->Fault();
		if (variant==9) c.bankStorageOnly=false; // misbound private owner MUST NOT enter generic legacy rollback
#ifdef PIONEER_Q38_NORMAL_CLOCK
		assert(c.q38Clock.Begin(c.clockIO.Arc(),c.clockIO.Health()));
		const unsigned clockIO=c.clockIO.io;
#endif
		const unsigned writes=c.io.Writes(),selects=c.io.selects,cleanups=genericCleanup;
		if (explicitEnd && variant==5) {
			// Only known-clean zero CLOSE plus an explicit BANK_END may
			// reach ordinary cleanup. No hardware adapter is installed.
			tt_resident_session_result result={}; EndResidentSession(cookie,result,true);
			assert(!cookie->context && genericCleanup==cleanups+1 && !sColdRecoveryRequired);
			assert(ReleaseResidentCookie(cookie)==B_OK);
			continue;
		}
#ifdef PIONEER_Q38_LOCAL_DELTA_PROBE
		// The early GDN gate must precede even a saved legacy-delta
		// close. Such a close would otherwise perform device IO.
		if (explicitEnd) c.q38Delta.saved=true;
#endif
		if (explicitEnd) {
			tt_resident_session_result result={}; bool clean=true;
			assert(!EndQ38DispatchOwnership(c,result,clean) && !clean && result.cold && result.active && !result.host_released);
		} else assert(ReleaseResidentCookie(cookie)==B_OK);
		assert(cookie->magic==0x50313030 && cookie->context==&c && sRuntimeOwner==cookie && sBankStorageOnly
			&& c.dispatchOwnerRetained && sColdRecoveryRequired && c.q38GdnBanks->Unsafe()
			&& (!c.q38StateZero || c.q38StateZero->unsafe) && c.bulkHost.quarantined
			&& (!c.wideHost.allocated || c.wideHost.quarantined) && genericCleanup==cleanups
			&& writes==c.io.Writes() && selects==c.io.selects);
#ifdef PIONEER_Q38_NORMAL_CLOCK
		assert(c.q38Clock.Unsafe() && clockIO==c.clockIO.io && !c.clockIO.unmaps);
#endif
		assert(ReleaseResidentCookie(cookie)==B_OK && writes==c.io.Writes() && selects==c.io.selects && genericCleanup==cleanups);
		delete cookie->context; delete cookie; sRuntimeOwner=nullptr; sBankStorageOnly=sColdRecoveryRequired=false; // TEST disposal ONLY
	}
	puts("PASS REAL gated GDN context retention: owner-only/lost caller/unprepared/ready/running/unsafe/closed-without-END retain cookie/context/PCI/ALL hosts/clock with ZERO IO; premature BANK_END refuses BEFORE delta close; only known zero CLOSE plus explicit END cleans once;NO native GDN adapter or qualification");
#endif
}
