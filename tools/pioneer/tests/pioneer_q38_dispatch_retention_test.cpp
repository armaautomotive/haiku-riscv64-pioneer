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
}
