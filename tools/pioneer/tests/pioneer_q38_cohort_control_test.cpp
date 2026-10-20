/* SPDX-License-Identifier: MIT */
#define PIONEER_Q38_COHORT_POLICY_MOCK_ONLY
#define TT_PIONEER_EXPERIMENTAL_Q38_BANKS
#define TT_PIONEER_EXPERIMENTAL_Q38_COHORT
#define B_DEVICE_OP_CODES_END 1000
#include "pioneer_q38_cohort_probe_test.cpp"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38StreamProbe.h"
#include <new>
using status_t=int; using uint32=uint32_t;
constexpr status_t B_OK=0,B_DEV_NOT_READY=-1,B_BUSY=-2,B_NOT_ALLOWED=-3,B_BAD_VALUE=-4,B_NO_MEMORY=-5,B_ENTRY_NOT_FOUND=-6;
struct thread_info { int team; };
static int team=17,threadStatus=0;
static int find_thread(void*) { return 1; }
static int get_thread_info(int,thread_info* info) { info->team=team; return threadStatus; }
// Kernel banks bind by identity. Use a baseline hierarchy whose deepest
// dram is a REFERENCE,not a byte-identical replacement in the host fixture.
struct Ref1 { tt_dram_samples_result& dram; };
struct Ref2 { Ref1 baseline; tt_compute_noc_result noc; };
struct Ref3 { Ref2 baseline; };
struct Ref4 { Ref3 baseline; };
struct BoundContext: CohortFixture {
#ifdef PIONEER_Q38_GDN_MUTABLE_STATE
	void* q38GdnBanks=nullptr;
	void* q38StateZero=nullptr; // no GDN IO/controller in this legacy fixture
#endif
	Ref4 baseline;
	PioneerQ38CohortLease* q38Cohort;
	PioneerResidentLease& lease;
	bool q38CohortRoutes=false,bankStorageOnly=true;
	PioneerQ38PanelLease panelProbe,streamProbe;
	uint8_t* initialized; uint64_t bytes=PioneerResidentBanks::Total;
	BoundContext():CohortFixture(3),baseline{{{{dram},owners[0]->noc}}},q38Cohort(CohortFixture::lease.get()),
		lease(owners[0]->store),initialized(CohortFixture::initialized.data()) {}
	bool Unsafe() { return q38Cohort->unsafe || banks.Quarantined() || lease.recovery_required || panelProbe.unsafe || streamProbe.unsafe; }
};
using PioneerStoreContext=BoundContext;
struct PioneerProbeCookie { int team=17; BoundContext* context=nullptr; };
static PioneerProbeCookie* sRuntimeOwner=nullptr;
static bool sBankStorageOnly=false,sColdRecoveryRequired=false;
static int sLock=1,acquired=0,released=0,copyCalls=0,failCopy=0;
static status_t acquire_sem(int lock) { assert(lock==1); acquired++; return B_OK; }
static void release_sem(int lock) { assert(lock==1); released++; }
#include "pioneer_lock.inc"
static status_t user_memcpy(void* to,const void* from,size_t bytes) {
	if (++copyCalls==failCopy) return -99; memcpy(to,from,bytes); return B_OK;
}
#include "pioneer_q38_cohort_prepare.inc"
#include "pioneer_q38_cohort_control.inc"
#include "pioneer_q38_panel_control.inc"
#include "pioneer_q38_stream_control.inc"
static void Begin(PioneerProbeCookie& cookie) {
	assert(!cookie.context && !sRuntimeOwner); cookie.context=new BoundContext;
	sRuntimeOwner=&cookie; sBankStorageOnly=true;
}
static void End(PioneerProbeCookie& cookie,bool clean) {
	cookie.context->Close(clean); delete cookie.context; cookie.context=nullptr;
	sRuntimeOwner=nullptr; sBankStorageOnly=false;
}
int main()
{
	(void)&Input; (void)&Prepare; (void)&Upload; (void)&Close; (void)&PreparePanel; (void)&Run; (void)&Job; (void)&ClosePanel; (void)&Finish;
	PioneerProbeCookie cookie,foreign; tt_q38_cohort_support support={};
	assert(Q38CohortControl(&cookie,TT_PCI_PROBE_Q38_COHORT_SUPPORT,&support,sizeof(support))==B_OK
		&& support.driver==TT_Q38_COHORT_DRIVER && support.version==TT_Q38_COHORT_ABI && support.workers==120 && support.rows==256 && support.max_inputs==17408
		&& support.banks==7 && support.offset_alignment==64 && !support.model_inference && !cookie.context);
	Begin(cookie); auto job=std::make_unique<tt_q38_cohort_probe>(*cookie.context->job);
	assert(AcquireProbeLock(&cookie)==B_NOT_ALLOWED);
	assert(Q38CohortControl(&foreign,TT_PCI_PROBE_Q38_COHORT_RUN,job.get(),sizeof(*job))==B_BUSY);
	team=18; assert(Q38CohortControl(&cookie,TT_PCI_PROBE_Q38_COHORT_RUN,job.get(),sizeof(*job))==B_NOT_ALLOWED); team=17;
	job->part[2].offset++;
	assert(Q38CohortControl(&cookie,TT_PCI_PROBE_Q38_COHORT_RUN,job.get(),sizeof(*job))==B_BAD_VALUE && !cookie.context->io.starts);
	job->part[2].offset--;
	assert(Q38CohortControl(&cookie,TT_PCI_PROBE_Q38_COHORT_RUN,job.get(),sizeof(*job)-1)==B_BAD_VALUE);
	cookie.context->initialized[11]=0;
	assert(Q38CohortControl(&cookie,TT_PCI_PROBE_Q38_COHORT_RUN,job.get(),sizeof(*job))==B_ENTRY_NOT_FOUND && !cookie.context->io.starts);
	cookie.context->initialized[11]=1;
	assert(Q38CohortControl(&cookie,TT_PCI_PROBE_Q38_COHORT_RUN,job.get(),sizeof(*job))==B_OK && job->result.stopped==3 && !sColdRecoveryRequired);
	auto local=Job(); auto stream=std::make_unique<tt_q38_stream_probe>(); stream->version=1; stream->type=12; stream->inputs=5120;
	for (unsigned w=0;w<2560;w++) stream->activation[w]=PioneerInt8Operand(1)|(uint32_t(PioneerInt8Operand(1))<<16);
	for (unsigned b=0;b<20;b++) stream->scales[b]=0x3c800000;
	assert(Q38PanelControl(&cookie,TT_PCI_PROBE_Q38_PANEL_RUN,&local,sizeof(local))==B_NOT_ALLOWED);
	assert(Q38StreamControl(&cookie,TT_PCI_PROBE_Q38_STREAM_RUN,stream.get(),sizeof(*stream))==B_NOT_ALLOWED);
	End(cookie,true);
	Begin(cookie); job=std::make_unique<tt_q38_cohort_probe>(*cookie.context->job); failCopy=copyCalls+2;
	assert(Q38CohortControl(&cookie,TT_PCI_PROBE_Q38_COHORT_RUN,job.get(),sizeof(*job))==-99 && cookie.context && sRuntimeOwner==&cookie && !cookie.context->Unsafe());
	failCopy=0; End(cookie,true);
	for (unsigned fault:{2U,4U,8U,14U,15U,16U}) {
		Begin(cookie); job=std::make_unique<tt_q38_cohort_probe>(*cookie.context->job); cookie.context->io.scenario=fault; cookie.context->io.badWorker=2;
		assert(Q38CohortControl(&cookie,TT_PCI_PROBE_Q38_COHORT_RUN,job.get(),sizeof(*job))==B_OK);
		bool clean=fault==16; assert(bool(job->result.recovery_required)==!clean && sColdRecoveryRequired==!clean);
		if (!clean) assert(Q38CohortControl(&cookie,TT_PCI_PROBE_Q38_COHORT_RUN,job.get(),sizeof(*job))==B_DEV_NOT_READY);
		End(cookie,clean); sColdRecoveryRequired=false;
	}
	assert(acquired==released);
	puts("PASS REAL extracted cohort prepare/handler/lock:private routes/team/FD/all-bank legacy isolation,ALL extents before launch,known copyout cleanup,UNKNOWN quarantine and no retries;NO native hardware");
}
