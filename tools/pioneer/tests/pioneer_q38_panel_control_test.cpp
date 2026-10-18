/* SPDX-License-Identifier: MIT */
// REAL extracted kernel control/lock,real prepare/run/cleanup templates;
// only Haiku syscalls/mappings/PCI reservation are synthetic.
#define PIONEER_Q38_PANEL_POLICY_MOCK_ONLY
#define B_DEVICE_OP_CODES_END 1000
#include "pioneer_q38_panel_probe_test.cpp"
#define TT_PIONEER_EXPERIMENTAL_Q38_BANKS
using status_t=int;
using uint32=uint32_t;
constexpr status_t B_OK=0,B_DEV_NOT_READY=-1,B_BUSY=-2,B_NOT_ALLOWED=-3,
	B_BAD_VALUE=-4,B_NO_MEMORY=-5;
struct thread_info { int team; };
static int team=17,threadStatus=0;
static int find_thread(void*) { return 1; }
static int get_thread_info(int,thread_info* info) { info->team=team; return threadStatus; }
struct Base1 { tt_dram_samples_result dram={}; };
struct Base2 { Base1 baseline; tt_compute_noc_result noc={}; };
struct Base3 { Base2 baseline; };
struct Base4 { Base3 baseline; };
struct PioneerStoreContext {
	Base4 baseline;
	PanelIO io;
	PioneerResidentBanks banks;
	PioneerResidentLease lease;
	PioneerQ38PanelLease panelProbe;
	tt_host_page_result host={};
	bool bankStorageOnly=true;
	PioneerStoreContext() {
		baseline.baseline.baseline.baseline.dram=BankProof(io); baseline.baseline.baseline.noc=io.Compute();
		assert(banks.Bind(baseline.baseline.baseline.baseline.dram) && PioneerPrepareHostPage(io,host));
	}
	bool Unsafe() { return panelProbe.running || panelProbe.unsafe || banks.Quarantined() || lease.pending || lease.recovery_required; }
};
struct PioneerProbeCookie { int team=17; PioneerStoreContext* context=nullptr; };
static PioneerProbeCookie* sRuntimeOwner=nullptr;
static bool sBankStorageOnly=false,sColdRecoveryRequired=false;
static int sLock=1,acquired=0,released=0,copyCalls=0,failCopy=0;
static status_t acquire_sem(int lock) { assert(lock==1); acquired++; return B_OK; }
static void release_sem(int lock) { assert(lock==1); released++; }
#include "pioneer_lock.inc"
static status_t user_memcpy(void* to,const void* from,size_t bytes) {
	if (++copyCalls==failCopy) return -99;
	memcpy(to,from,bytes); return B_OK;
}
#include "pioneer_q38_panel_control.inc"
static void Begin(PioneerProbeCookie& cookie)
{
	assert(!cookie.context && !sRuntimeOwner); cookie.context=new PioneerStoreContext;
	sRuntimeOwner=&cookie; sBankStorageOnly=true;
}
static void End(PioneerProbeCookie& cookie,bool clean)
{
	auto* c=cookie.context; assert(c);
	auto& dram=c->baseline.baseline.baseline.baseline.dram; auto& noc=c->baseline.baseline.baseline.noc;
	assert(PioneerQ38PanelClose(c->io,dram,noc,c->lease,c->banks,c->panelProbe,clean)==clean);
	if (c->lease.saved) assert(PioneerResidentLeaseEnd(c->io,noc,c->lease)==clean);
	if (clean) { assert(!c->Unsafe() && c->io.ReleaseHostPage()==0); if (c->lease.saved) c->io.Safe(); }
	else assert(c->io.HostReadMock::quarantined && !c->io.PageMock::releases);
	delete c; cookie.context=nullptr; sRuntimeOwner=nullptr; sBankStorageOnly=false;
}
int main()
{
	(void)&Input; (void)&Prepare; (void)&Upload; (void)&Close;
	(void)&PreparePanel; (void)&Run; (void)&ClosePanel; (void)&Finish;
	PioneerProbeCookie cookie,foreign; tt_q38_panel_support support={};
	assert(Q38PanelControl(&cookie,TT_PCI_PROBE_Q38_PANEL_SUPPORT,&support,sizeof(support))==B_OK
		&& support.driver==TT_Q38_PANEL_CURRENT_DRIVER && support.version==TT_Q38_PANEL_ABI_VERSION && support.local_only && support.type_mask==0x5100 && !cookie.context);
	team=18; assert(Q38PanelControl(&cookie,TT_PCI_PROBE_Q38_PANEL_SUPPORT,&support,sizeof(support))==B_NOT_ALLOWED); team=17;
	auto job=Job();
	assert(Q38PanelControl(&cookie,TT_PCI_PROBE_Q38_PANEL_RUN,&job,sizeof(job))==B_NOT_ALLOWED);
	Begin(cookie);
	assert(AcquireProbeLock(&cookie)==B_NOT_ALLOWED); // legacy still blocked
	assert(Q38PanelControl(&foreign,TT_PCI_PROBE_Q38_PANEL_RUN,&job,sizeof(job))==B_BUSY);
	team=18; assert(Q38PanelControl(&cookie,TT_PCI_PROBE_Q38_PANEL_RUN,&job,sizeof(job))==B_NOT_ALLOWED); team=17;
	auto bad=job; bad.bytes--;
	assert(Q38PanelControl(&cookie,TT_PCI_PROBE_Q38_PANEL_RUN,&bad,sizeof(bad))==B_BAD_VALUE && !cookie.context->io.panelWrites);
	assert(Q38PanelControl(&cookie,TT_PCI_PROBE_Q38_PANEL_RUN,&job,sizeof(job)-1)==B_BAD_VALUE);
	assert(Q38PanelControl(&cookie,TT_PCI_PROBE_Q38_PANEL_RUN,&job,sizeof(job))==B_OK && job.result.stopped && !sColdRecoveryRequired);
	End(cookie,true);
	// Failed result copyout does not orphan or detach the same FD owner;
	// normal final close may cleanly release a KNOWN completed/stopped job.
	Begin(cookie); job=Job(); failCopy=copyCalls+2;
	assert(Q38PanelControl(&cookie,TT_PCI_PROBE_Q38_PANEL_RUN,&job,sizeof(job))==-99 && cookie.context && sRuntimeOwner==&cookie
		&& cookie.context->io.launches==1 && !cookie.context->Unsafe()); failCopy=0; End(cookie,true);
	for (unsigned scenario=1;scenario<=9;scenario++) {
		Begin(cookie); cookie.context->io.scenario=scenario; job=Job();
		assert(Q38PanelControl(&cookie,TT_PCI_PROBE_Q38_PANEL_RUN,&job,sizeof(job))==B_OK);
		bool clean=scenario==7 || scenario==8;
		assert(bool(job.result.recovery_required)==!clean && sColdRecoveryRequired==!clean && cookie.context->banks.Quarantined()==!clean);
		if (!clean) {
			unsigned writes=cookie.context->io.panelWrites,resets=unsigned(cookie.context->io.resets.size());
			assert(Q38PanelControl(&cookie,TT_PCI_PROBE_Q38_PANEL_RUN,&job,sizeof(job))==B_DEV_NOT_READY);
			assert(writes==cookie.context->io.panelWrites && resets==cookie.context->io.resets.size());
		}
		End(cookie,clean);
		sColdRecoveryRequired=false; // test-only new cold boot,NOT a runtime retry
	}
	assert(acquired==released);
	puts("PASS: REAL local-panel kernel handler+lock,team/FD/global-owner/legacy isolation,precopy bounds,known copyout cleanup,ALL uncertain launches quarantine ALL banks+host with no later commands/free;NO hardware");
}
