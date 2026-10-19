/* SPDX-License-Identifier: MIT */
#define B_DEVICE_OP_CODES_END 1000
#define TT_PIONEER_DRIVER_RELOAD
#define TT_PIONEER_EXPERIMENTAL_Q38_BANKS
#define PIONEER_DRIVER_BUILD_TAG UINT64_C(0x2026101900010001)
#include "../../../src/add-ons/kernel/drivers/misc/pioneer_p100a_reload_guard.cpp"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerDriverReloadProtocol.h"
#include <cstdio>
#include <cstring>
constexpr unsigned TT_Q38_DISPATCH_DRIVER=55;
struct PioneerProbeCookie { int team=17; void* context=nullptr; };
static PioneerProbeCookie* sRuntimeOwner=nullptr;
static sem_id sLock=7;
static bool sBankStorageOnly=false;
static unsigned copies=0,copyFault=0;
static int activeTeam=17;
struct thread_info { int team; };
static int find_thread(void*) { return 1; }
static int get_thread_info(int,thread_info* info) { info->team=activeTeam; return B_OK; }
static status_t user_memcpy(void* to,const void* from,size_t n) {
	if (++copies==copyFault) return -99;
	memcpy(to,from,n); return B_OK;
}
#include "pioneer_reload_state.inc"
#include "pioneer_lock.inc"
#include "pioneer_reload_control.inc"
static tt_pioneer_driver_reload Request(bool quiesce=false) {
	tt_pioneer_driver_reload r={}; r.version=1;
	if (quiesce) { r.expected_tag=PIONEER_DRIVER_BUILD_TAG; r.expected_generation=sReloadGeneration; }
	return r;
}
int main()
{
	assert(sGuard.info.flags==B_KEEP_LOADED && sGuard.info.std_ops(B_MODULE_INIT)==B_OK);
	sReloadGuard=&sGuard; assert(sReloadGuard->attach(&sReloadGeneration)==B_OK && sReloadGeneration==1);
	uint64 duplicate=99; assert(sGuard.attach(&duplicate)==B_DEV_NOT_READY && !duplicate);
	PioneerProbeCookie cookie,other; sOpenCookies=1;
	{
		auto r=Request(); assert(DriverReloadControl(&cookie,TT_PCI_PROBE_RELOAD_STATUS,&r,sizeof(r))==B_OK
			&& r.guard_version==1 && r.driver==55 && r.build_tag==PIONEER_DRIVER_BUILD_TAG && r.generation==1
			&& !r.quiesced && !r.cold && !r.active_owner && r.open_count==1);
		assert(AcquireProbeLock(&cookie,true)==B_OK); release_sem(sLock);
	}
	for (unsigned fault=0;fault<10;fault++) {
		auto r=Request(true); unsigned length=sizeof(r);
		if (!fault) r.expected_tag^=1; if (fault==1) r.expected_generation++;
		if (fault==2) r.expected_tag=0; if (fault==3) r.expected_generation=0;
		if (fault==4) sOpenCookies=2; if (fault==5) sRuntimeOwner=&other;
		if (fault==6) cookie.context=&other; if (fault==7) r.reserved[1]=1;
		if (fault==8) activeTeam++; if (fault==9) length--;
		assert(DriverReloadControl(&cookie,TT_PCI_PROBE_RELOAD_QUIESCE,&r,length)!=B_OK && !sReloadQuiesced && !Cold());
		sOpenCookies=1; sRuntimeOwner=nullptr; cookie.context=nullptr; activeTeam=17;
	}
	{
		auto r=Request(true); assert(DriverReloadControl(&cookie,TT_PCI_PROBE_RELOAD_QUIESCE,&r,sizeof(r))==B_OK
			&& r.quiesced && !r.cold && !r.active_owner);
		assert(AcquireProbeLock(&cookie,true)==B_BUSY && AcquireProbeLock(&cookie,false)==B_BUSY && !Cold());
	}
	// The previous driver detaches without touching hardware. The SAME
	// resident module gives its replacement a new monotonic generation.
	assert(sGuard.detach(sReloadGeneration,true)==B_OK);
	assert(sGuard.attach(&sReloadGeneration)==B_OK && sReloadGeneration==2);
	sReloadQuiesced=false;
	{
		auto r=Request(true); copyFault=copies+2;
		assert(DriverReloadControl(&cookie,TT_PCI_PROBE_RELOAD_QUIESCE,&r,sizeof(r))==-99 && sReloadQuiesced);
		assert(AcquireProbeLock(&cookie,true)==B_BUSY); copyFault=0;
	}
	// The actual driver's sticky latch publishes to the actual resident
	// module immediately. Reassigning false is NOT a recovery operation.
	sColdRecoveryRequired=true; sColdRecoveryRequired=false;
	assert(Cold() && sColdRecoveryRequired && AcquireProbeLock(&cookie,true)==B_DEV_NOT_READY);
	{
		auto r=Request(); assert(DriverReloadControl(&cookie,TT_PCI_PROBE_RELOAD_STATUS,&r,sizeof(r))==B_OK && r.cold);
		r=Request(true); assert(DriverReloadControl(&cookie,TT_PCI_PROBE_RELOAD_QUIESCE,&r,sizeof(r))==B_BUSY);
	}
	assert(sGuard.detach(sReloadGeneration,false)==B_DEV_NOT_READY);
	uint64 replacement=123; assert(sGuard.attach(&replacement)==B_DEV_NOT_READY && !replacement);
	assert(sGuard.info.std_ops(B_MODULE_UNINIT)==B_BUSY);
	for (unsigned fault=0;fault<4;fault++) {
		PioneerDriverReloadState state; uint64_t first=0,second=0;
		assert(state.Attach(first));
		if (!fault) state.Fault(first);
		if (fault==1) assert(!state.Detach(first,false));
		if (fault==2) assert(!state.Detach(first+1,true));
		if (fault==3) { state.Fault(first+1); assert(!state.Detach(first,true)); }
		assert(state.cold && !state.Attach(second) && !second);
	}
	puts("PASS actual resident guard+extracted driver control:clean generations,active/extra FD/stale identity refusal,quiesce blocks ALL hardware,failed reply remains blocked,cold survives unload/reload and cannot clear;NO PCI/MMIO/reset/native qualification");
}
