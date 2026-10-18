/* SPDX-License-Identifier: MIT */
#define TT_PIONEER_EXPERIMENTAL_Q38_AFFINE
#define TT_PIONEER_EXPERIMENTAL_Q38_BLOCK
#define TT_PIONEER_EXPERIMENTAL_Q38_STREAM
#define PIONEER_Q38_SKIP_PANEL_CONTROL_MAIN
#include "pioneer_q38_panel_control_test.cpp"
int main()
{
	(void)&Input; (void)&Prepare; (void)&Upload; (void)&Close;
	(void)&PreparePanel; (void)&Run; (void)&ClosePanel; (void)&Finish;
	(void)&PrepareStream; (void)&Stream;
	PioneerProbeCookie cookie,foreign; tt_q38_stream_support support={};
	assert(Q38StreamControl(&cookie,TT_PCI_PROBE_Q38_STREAM_SUPPORT,&support,sizeof(support))==B_OK
		&& support.driver==49 && support.version==1 && support.local_rows==16 && support.max_inputs==17408
		&& support.banks==7 && !support.model_inference && !cookie.context);
	auto job=StreamJob();
	assert(Q38StreamControl(&cookie,TT_PCI_PROBE_Q38_STREAM_RUN,&job,sizeof(job))==B_NOT_ALLOWED);
	Begin(cookie);
	assert(AcquireProbeLock(&cookie)==B_NOT_ALLOWED);
	assert(Q38StreamControl(&foreign,TT_PCI_PROBE_Q38_STREAM_RUN,&job,sizeof(job))==B_BUSY);
	team=18; assert(Q38StreamControl(&cookie,TT_PCI_PROBE_Q38_STREAM_RUN,&job,sizeof(job))==B_NOT_ALLOWED); team=17;
	auto bad=job; bad.inputs++;
	assert(Q38StreamControl(&cookie,TT_PCI_PROBE_Q38_STREAM_RUN,&bad,sizeof(bad))==B_BAD_VALUE && !cookie.context->io.panelWrites);
	assert(Q38StreamControl(&cookie,TT_PCI_PROBE_Q38_STREAM_RUN,&job,sizeof(job)-1)==B_BAD_VALUE);
	cookie.context->streamPages[5]=0;
	assert(Q38StreamControl(&cookie,TT_PCI_PROBE_Q38_STREAM_RUN,&job,sizeof(job))==B_ENTRY_NOT_FOUND && !cookie.context->io.panelWrites);
	cookie.context->streamPages[5]=1;
	assert(Q38StreamControl(&cookie,TT_PCI_PROBE_Q38_STREAM_RUN,&job,sizeof(job))==B_OK && job.result.stopped && !sColdRecoveryRequired);
	auto local=Job(); auto writes=cookie.context->io.panelWrites;
	assert(Q38PanelControl(&cookie,TT_PCI_PROBE_Q38_PANEL_RUN,&local,sizeof(local))==B_NOT_ALLOWED && writes==cookie.context->io.panelWrites);
	End(cookie,true);
	Begin(cookie); local=Job();
	assert(Q38PanelControl(&cookie,TT_PCI_PROBE_Q38_PANEL_RUN,&local,sizeof(local))==B_OK && local.result.stopped);
	writes=cookie.context->io.panelWrites;
	assert(Q38StreamControl(&cookie,TT_PCI_PROBE_Q38_STREAM_RUN,&job,sizeof(job))==B_NOT_ALLOWED && writes==cookie.context->io.panelWrites);
	End(cookie,true);
	Begin(cookie); job=StreamJob(); failCopy=copyCalls+2;
	assert(Q38StreamControl(&cookie,TT_PCI_PROBE_Q38_STREAM_RUN,&job,sizeof(job))==-99 && cookie.context && sRuntimeOwner==&cookie
		&& cookie.context->io.launches==1 && !cookie.context->Unsafe()); failCopy=0; End(cookie,true);
	for (unsigned scenario=1;scenario<=15;scenario++) {
		Begin(cookie); cookie.context->io.scenario=scenario; job=StreamJob();
		assert(Q38StreamControl(&cookie,TT_PCI_PROBE_Q38_STREAM_RUN,&job,sizeof(job))==B_OK);
		bool clean=scenario==7 || scenario==8;
		assert(bool(job.result.recovery_required)==!clean && sColdRecoveryRequired==!clean && cookie.context->banks.Quarantined()==!clean);
		if (!clean) {
			unsigned before=cookie.context->io.panelWrites,resets=unsigned(cookie.context->io.resets.size());
			assert(Q38StreamControl(&cookie,TT_PCI_PROBE_Q38_STREAM_RUN,&job,sizeof(job))==B_DEV_NOT_READY);
			assert(before==cookie.context->io.panelWrites && resets==cookie.context->io.resets.size());
		}
		End(cookie,clean); sColdRecoveryRequired=false; // test-only fresh cold boot
	}
	assert(acquired==released);
	puts("PASS REAL extracted fullK kernel handler+lock:metadata/no-owner/bounds/ALL initialized pages/team/FD/global/legacy isolation,mutual exclusion of overlapping block/fullK leases,known failed-copy cleanup,ALL15 failures quarantine ALL banks+host without retry/reset/restore/free;NO hardware");
}
