/* SPDX-License-Identifier: MIT */
#define B_DEVICE_OP_CODES_END 1000
#define TT_PIONEER_EXPERIMENTAL_Q38_BANKS
#define PIONEER_Q38_DISPATCH_CLOSE_MOCK_ONLY
#include "pioneer_q38_dispatch_close_test.cpp"
#ifdef PIONEER_Q38_NORMAL_CLOCK
#define PIONEER_Q38_CLOCK_LEASE_MODEL_ONLY
#include "pioneer_q38_clock_lease_test.cpp"
#endif
#ifdef PIONEER_Q38_SPAN_PRELOAD
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38SpanController.h"
#endif
using status_t=int; using uint32=uint32_t;
#ifdef PIONEER_Q38_LOCAL_DELTA_PROBE
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38DeltaProbe.h"
#endif
constexpr status_t B_OK=0,B_DEV_NOT_READY=-1,B_BUSY=-2,B_NOT_ALLOWED=-3,B_BAD_VALUE=-4,B_NO_MEMORY=-5,B_ENTRY_NOT_FOUND=-6;
struct thread_info { int team; };
static int team=17,threadStatus=0;
static int find_thread(void*) { return 1; }
static int get_thread_info(int,thread_info* info) { info->team=team; return threadStatus; }
struct Ref1 { tt_dram_samples_result& dram; };
struct Ref2 { Ref1 baseline; tt_compute_noc_result noc; };
struct Ref3 { Ref2 baseline; };
struct Ref4 { Ref3 baseline; };
struct BoundContext {
#ifdef PIONEER_Q38_NORMAL_CLOCK
	ClockLeaseIO clockIO;
	PioneerQ38ClockLease<ClockLeaseIO> q38Clock{clockIO};
#endif
#ifdef PIONEER_Q38_LOCAL_DELTA_PROBE
	PioneerQ38DeltaProbeLease q38Delta;
#endif
#ifdef PIONEER_Q38_SPAN_PRELOAD
	PioneerQ38SpanLease* q38Span=nullptr;
#endif
	CloseIO io; PanelOwners root;
	tt_dram_samples_result dram; PioneerResidentBanks banks; Ref4 baseline;
	PioneerResidentLease& lease;
	PioneerQ38DispatchLease* q38Dispatch=nullptr; PioneerQ38CohortLease* q38Cohort=nullptr;
	bool q38DispatchRoutes=false,bankStorageOnly=true,dispatchOwnerRetained=false;
#ifdef PIONEER_Q38_WORK_OUTPUT_DMA
	tt_host_page_result host; PioneerOutputBufferLease bulkHost; PioneerWideOutputLease wideHost;
#else
	struct HostLease { bool allocated=true,released=false,ready=true,quarantined=false; } bulkHost,wideHost;
#endif
	PioneerQ38PanelLease panelProbe,streamProbe;
	std::vector<uint8_t> initializedVector; uint8_t* initialized; uint64_t bytes=PioneerResidentBanks::Total;
	explicit BoundContext(const char* image):io(image),root(*io.core[0]),dram(root.dram),baseline{{{{dram},root.noc}}},
		lease(root.store),initializedVector(PioneerResidentBanks::Pages,1),initialized(initializedVector.data()) {
		assert(banks.Bind(dram));
#ifdef PIONEER_Q38_WORK_OUTPUT_DMA
		host=io.dmaInput; bulkHost=io.dmaBulk; io.dmaPendingOwner=&wideHost;
#endif
	}
	bool Unsafe() { return
#ifdef PIONEER_Q38_NORMAL_CLOCK
		q38Clock.Unsafe() ||
#endif
#ifdef PIONEER_Q38_SPAN_PRELOAD
		(q38Span && (q38Span->unsafe || q38Span->running)) ||
#endif
		(q38Dispatch && PioneerQ38DispatchUnsafe(*q38Dispatch)) || banks.Quarantined() || lease.recovery_required || panelProbe.unsafe || streamProbe.unsafe; }
	~BoundContext() {
#ifdef PIONEER_Q38_SPAN_PRELOAD
		delete q38Span;
#endif
		delete q38Dispatch;
	}
};
using PioneerStoreContext=BoundContext;
struct PioneerProbeCookie { uint32_t magic=0x50313030; int team=17; BoundContext* context=nullptr; };
static PioneerProbeCookie* sRuntimeOwner=nullptr;
static bool sBankStorageOnly=false,sColdRecoveryRequired=false;
static int sLock=1,acquired=0,released=0,copyCalls=0,failCopy=0;
static status_t acquire_sem(int lock) { assert(lock==1); acquired++; return B_OK; }
static void release_sem(int lock) { assert(lock==1); released++; }
#include "pioneer_lock.inc"
#ifdef PIONEER_Q38_LOCAL_DELTA_PROBE
[[maybe_unused]] static const auto& DeltaLockQualification=AcquireQ38DeltaLock;
#endif
static status_t user_memcpy(void* to,const void* from,size_t bytes) {
	if (++copyCalls==failCopy) return -99; memcpy(to,from,bytes); return B_OK;
}
#include "pioneer_q38_dispatch_prepare.inc"
#include "pioneer_q38_dispatch_control.inc"
#include "pioneer_q38_dispatch_retain.inc"
static void Begin(PioneerProbeCookie& cookie,const char* image) {
	assert(!cookie.context && !sRuntimeOwner); cookie.context=new BoundContext(image);
	sRuntimeOwner=&cookie; sBankStorageOnly=true;
}
static void End(PioneerProbeCookie& cookie,bool clean) {
	auto& c=*cookie.context; c.io.closing=true;
	if (clean) {
		if (c.q38Dispatch) assert(PioneerQ38DispatchClose(c.io,c.dram,c.banks,*c.q38Dispatch,true));
		assert(!c.Unsafe());
	} else {
		assert(c.Unsafe()); unsigned writes=c.io.Writes(),selects=c.io.selects;
		tt_resident_session_result result={}; bool known=false;
		assert(!EndQ38DispatchOwnership(c,result,known) && c.dispatchOwnerRetained && result.active==1 && result.cold==1 && !result.host_released
			&& c.bulkHost.quarantined && (!c.wideHost.allocated || c.wideHost.quarantined) && sRuntimeOwner==&cookie && cookie.context==&c);
		assert(writes==c.io.Writes() && selects==c.io.selects);
	}
	delete cookie.context; cookie.context=nullptr; sRuntimeOwner=nullptr; sBankStorageOnly=false;
}
static std::unique_ptr<tt_q38_dispatch_probe> Request() {
	auto r=std::make_unique<tt_q38_dispatch_probe>(); r->version=1; r->type=12; r->inputs=5120; r->peers=7;
	for (unsigned w=0;w<7;w++) r->part[w]={w,1,64};
	for (unsigned w=0;w<2560;w++) r->activation[w]=0x40014001;
	for (unsigned b=0;b<20;b++) r->scales[b]=0x3c800000;
	return r;
}
#ifndef PIONEER_Q38_DISPATCH_CONTROL_MOCK_ONLY
int main(int argc,char** argv)
{
	assert(argc==2);
	(void)&Input; (void)&Prepare; (void)&Upload; (void)&Close; (void)&PreparePanel; (void)&Run; (void)&Job; (void)&ClosePanel; (void)&Finish;
	PioneerProbeCookie cookie,foreign; tt_q38_dispatch_support support={};
	assert(Q38DispatchControl(&cookie,TT_PCI_PROBE_Q38_DISPATCH_SUPPORT,&support,sizeof(support))==B_OK
		&& support.driver==TT_Q38_DISPATCH_DRIVER && support.version==1 && support.peers==119 && support.rows==256 && support.max_inputs==17408
		&& support.banks==7 && support.offset_alignment==64 && support.persistent==1 && !support.model_inference && !cookie.context);
	//54/55/56 reject EACH OTHER'S support/RUN before allocation/IO.
	for (unsigned other:{0x54f2U,0x55f2U,0x56f2U,0x57f2U,0x58f2U,0x59f2U}) if (B_DEVICE_OP_CODES_END+other!=TT_PCI_PROBE_Q38_DISPATCH_SUPPORT)
		assert(Q38DispatchControl(&cookie,B_DEVICE_OP_CODES_END+other,&support,sizeof(support))==B_BAD_VALUE && !cookie.context);
	Begin(cookie,argv[1]); auto job=Request();
	unsigned namespaceWrites=cookie.context->io.Writes(),namespaceSelects=cookie.context->io.selects;
	for (unsigned other:{0x54f3U,0x55f3U,0x56f3U,0x57f3U,0x58f3U,0x59f3U}) if (B_DEVICE_OP_CODES_END+other!=TT_PCI_PROBE_Q38_DISPATCH_RUN)
		assert(Q38DispatchControl(&cookie,B_DEVICE_OP_CODES_END+other,job.get(),sizeof(*job))==B_BAD_VALUE
			&& !cookie.context->q38Dispatch && cookie.context->io.Writes()==namespaceWrites && cookie.context->io.selects==namespaceSelects);
	assert(AcquireProbeLock(&cookie)==B_NOT_ALLOWED);
	assert(Q38DispatchControl(&foreign,TT_PCI_PROBE_Q38_DISPATCH_RUN,job.get(),sizeof(*job))==B_BUSY);
	team=18; assert(Q38DispatchControl(&cookie,TT_PCI_PROBE_Q38_DISPATCH_RUN,job.get(),sizeof(*job))==B_NOT_ALLOWED); team=17;
	job->part[2].offset++;
	assert(Q38DispatchControl(&cookie,TT_PCI_PROBE_Q38_DISPATCH_RUN,job.get(),sizeof(*job))==B_BAD_VALUE && !cookie.context->q38Dispatch);
	job->part[2].offset--;
	assert(Q38DispatchControl(&cookie,TT_PCI_PROBE_Q38_DISPATCH_RUN,job.get(),sizeof(*job)-1)==B_BAD_VALUE);
	cookie.context->initialized[0]=0; unsigned writes=cookie.context->io.Writes(),selects=cookie.context->io.selects;
	assert(Q38DispatchControl(&cookie,TT_PCI_PROBE_Q38_DISPATCH_RUN,job.get(),sizeof(*job))==B_ENTRY_NOT_FOUND
		&& !cookie.context->q38Dispatch && cookie.context->io.Writes()==writes && cookie.context->io.selects==selects);
	cookie.context->initialized[0]=1;
	cookie.context->panelProbe.saved=true;
	assert(Q38DispatchControl(&cookie,TT_PCI_PROBE_Q38_DISPATCH_RUN,job.get(),sizeof(*job))==B_NOT_ALLOWED);
	cookie.context->panelProbe.saved=false;
	assert(Q38DispatchControl(&cookie,TT_PCI_PROBE_Q38_DISPATCH_RUN,job.get(),sizeof(*job))==B_OK
		&& job->result.waiting==7 && !sColdRecoveryRequired && !cookie.context->Unsafe());
	PioneerQ38DispatchHistory history; assert(history.Accept(*job) && !history.Accept(*job));
	assert(Q38DispatchControl(&cookie,TT_PCI_PROBE_Q38_DISPATCH_RUN,job.get(),sizeof(*job))==B_OK && job->result.nonce==2 && history.Accept(*job));
#ifdef PIONEER_Q38_REUSE_ACTIVATIONS
	assert(cookie.context->io.inputMode==2);
	assert(Q38DispatchControl(&cookie,TT_PCI_PROBE_Q38_DISPATCH_SUPPORT,&support,sizeof(support))==B_OK);
	assert(Q38DispatchControl(&cookie,TT_PCI_PROBE_Q38_DISPATCH_RUN,job.get(),sizeof(*job))==B_OK && cookie.context->io.inputMode==1);
	assert(Q38DispatchControl(&cookie,TT_PCI_PROBE_Q38_DISPATCH_RUN,job.get(),sizeof(*job))==B_OK && cookie.context->io.inputMode==2);
	assert(AcquireProbeLock(&cookie,true)==B_OK); release_sem(sLock);
	assert(Q38DispatchControl(&cookie,TT_PCI_PROBE_Q38_DISPATCH_RUN,job.get(),sizeof(*job))==B_OK && cookie.context->io.inputMode==1);
	sQ38InputEpoch=UINT64_MAX;
	for (unsigned repeat=0;repeat<2;repeat++) assert(Q38DispatchControl(&cookie,TT_PCI_PROBE_Q38_DISPATCH_RUN,job.get(),sizeof(*job))==B_OK
		&& cookie.context->io.inputMode==1 && !cookie.context->q38Dispatch->inputReuse.valid);
	sQ38InputEpoch=1;
#endif
	End(cookie,true);
#ifdef TT_PIONEER_Q38_WORK_MODE
	assert(Q38DispatchControl(&cookie,TT_PCI_PROBE_Q38_WORK_SUPPORT,&support,sizeof(support))==B_OK && support.driver==55);
	Begin(cookie,argv[1]); job=Request();
	assert(Q38DispatchControl(&foreign,TT_PCI_PROBE_Q38_WORK_RUN,job.get(),sizeof(*job))==B_BUSY);
	team=18; assert(Q38DispatchControl(&cookie,TT_PCI_PROBE_Q38_WORK_RUN,job.get(),sizeof(*job))==B_NOT_ALLOWED); team=17;
	for (unsigned op:{unsigned(TT_PCI_PROBE_Q38_WORK_RUN),unsigned(TT_PCI_PROBE_Q38_WORK_RUN),unsigned(TT_PCI_PROBE_Q38_DISPATCH_RUN),unsigned(TT_PCI_PROBE_Q38_DISPATCH_RUN)}) {
		assert(Q38DispatchControl(&cookie,op,job.get(),sizeof(*job))==B_OK && !cookie.context->Unsafe());
		if (op==TT_PCI_PROBE_Q38_WORK_RUN) for (const auto& peer:job->result.peer) for (auto word:peer) assert(!word);
	}
	End(cookie,true);
	Begin(cookie,argv[1]); job=Request(); cookie.context->io.rootFault=4;
	assert(Q38DispatchControl(&cookie,TT_PCI_PROBE_Q38_WORK_RUN,job.get(),sizeof(*job))==B_OK && sColdRecoveryRequired && cookie.context->Unsafe());
	assert(Q38DispatchControl(&cookie,TT_PCI_PROBE_Q38_WORK_RUN,job.get(),sizeof(*job))==B_DEV_NOT_READY);
	End(cookie,false); sColdRecoveryRequired=false;
#else
	assert(Q38DispatchControl(&cookie,B_DEVICE_OP_CODES_END+0x60f2,&support,sizeof(support))==B_BAD_VALUE);
#endif
	Begin(cookie,argv[1]); job=Request(); failCopy=copyCalls+2;
	assert(Q38DispatchControl(&cookie,TT_PCI_PROBE_Q38_DISPATCH_RUN,job.get(),sizeof(*job))==-99
		&& cookie.context && sRuntimeOwner==&cookie && !cookie.context->Unsafe());
	failCopy=0; End(cookie,true);
	for (unsigned fault:{2U,3U,4U,5U,6U,7U,10U}) {
		Begin(cookie,argv[1]); job=Request(); cookie.context->io.rootFault=fault;
		assert(Q38DispatchControl(&cookie,TT_PCI_PROBE_Q38_DISPATCH_RUN,job.get(),sizeof(*job))==B_OK);
		assert(job->result.recovery_required && sColdRecoveryRequired && cookie.context->Unsafe());
		for (const auto& rows:job->output) for (unsigned value:rows) assert(!value);
		assert(Q38DispatchControl(&cookie,TT_PCI_PROBE_Q38_DISPATCH_RUN,job.get(),sizeof(*job))==B_DEV_NOT_READY);
		End(cookie,false); sColdRecoveryRequired=false;
	}
	assert(acquired==released);
	puts("PASS REAL extracted54 prepare/handler/lock+actual dispatcher:ALL120 private routes/roles and119 bootstrap,team/FD/legacy isolation,ALL initialized extents before prepare IO,two jobs with persistent WAIT,copyout failure clean END,7 late faults whole-owner cold quarantine/no retry;NO native card qualification");
}
#endif
