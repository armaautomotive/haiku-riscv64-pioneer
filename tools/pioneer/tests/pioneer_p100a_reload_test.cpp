/* SPDX-License-Identifier: MIT */
#define B_DEVICE_OP_CODES_END 1000
#define TT_PIONEER_DRIVER_RELOAD 1
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
	assert(sRecoveryGuard.info.flags==B_KEEP_LOADED
		&& sRecoveryGuard.info.std_ops(B_MODULE_INIT)==B_OK);
	assert(modules[0]==&sGuard.info && modules[1]==&sRecoveryGuard.info && modules[2]==NULL);
	// Haiku's VFS resolves the first regular file in the module name. Both
	// exports must resolve to the same installed reload-guard image; otherwise
	// get_module(recovery) cannot find the shared pinned state.
	const char* imagePath="generic/pioneer_p100a_reload_guard";
	assert(strncmp(sGuard.info.name,imagePath,strlen(imagePath))==0
		&& sGuard.info.name[strlen(imagePath)]=='/');
	assert(strncmp(sRecoveryGuard.info.name,imagePath,strlen(imagePath))==0
		&& sRecoveryGuard.info.name[strlen(imagePath)]=='/');
	sReloadGuard=&sGuard; assert(sReloadGuard->attach(&sReloadGeneration)==B_OK && sReloadGeneration==1);
	uint64 duplicate=99; assert(sGuard.attach(&duplicate)==B_DEV_NOT_READY && !duplicate);
	PioneerProbeCookie cookie,other; sOpenCookies=1;
	{
		auto r=Request(true); assert(DriverReloadControl(&cookie,TT_PCI_PROBE_RELOAD_STATUS,&r,sizeof(r))==B_OK
			&& r.guard_version==1 && r.driver==55 && r.build_tag==PIONEER_DRIVER_BUILD_TAG && r.generation==1
			&& !r.quiesced && !r.cold && !r.active_owner && r.open_count==1);
		assert(r.version==1 && !r.expected_tag && !r.expected_generation
			&& !r.reserved[0] && !r.reserved[1] && !r.reserved[2]);
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
	// The recovery interface keeps the legacy reload ABI intact. It refuses
	// incomplete device evidence, stale tickets, and a fault racing the reset.
	{
		uint64 ticket=0,newGeneration=0;
		assert(sRecoveryGuard.begin(sReloadGeneration,&ticket)==B_OK && ticket);
		PioneerDriverRecoveryProof proof={};
		assert(sRecoveryGuard.complete(ticket,&proof,&newGeneration)==B_DEV_NOT_READY
			&& !newGeneration && Cold());
		assert(sRecoveryGuard.abort(ticket)==B_OK && Cold());
		assert(sRecoveryGuard.complete(ticket,&proof,&newGeneration)==B_DEV_NOT_READY);
		assert(sRecoveryGuard.begin(sReloadGeneration,&ticket)==B_OK && ticket);
		proof.flags=PIONEER_RECOVERY_PROOF_REQUIRED;
		proof.core_count=120; proof.reset_generation=1; proof.firmware_generation=1;
		PioneerDriverRecoveryProof incomplete=proof;
		incomplete.core_count=119;
		assert(sRecoveryGuard.complete(ticket,&incomplete,&newGeneration)==B_DEV_NOT_READY
			&& !newGeneration && Cold());
		incomplete=proof; incomplete.noc_outstanding_mask=1;
		assert(sRecoveryGuard.complete(ticket,&incomplete,&newGeneration)==B_DEV_NOT_READY
			&& !newGeneration && Cold());
		incomplete=proof; incomplete.retained_owners=1;
		assert(sRecoveryGuard.complete(ticket,&incomplete,&newGeneration)==B_DEV_NOT_READY
			&& !newGeneration && Cold());
		const uint64 before=sReloadGeneration;
		assert(sRecoveryGuard.complete(ticket,&proof,&newGeneration)==B_OK
			&& newGeneration==before+1 && !Cold());
		assert(sRecoveryGuard.abort(ticket)==B_BAD_VALUE);
		sReloadGeneration=newGeneration;
		sColdRecoveryRequired.ClearAfterVerifiedRecovery();
		assert(!sColdRecoveryRequired && !sGuard.cold());
	}
	// A new fault during a recovery ticket invalidates its proof, even when
	// all device audit fields otherwise pass.
	{
		sColdRecoveryRequired=true;
		uint64 ticket=0,newGeneration=0;
		assert(sRecoveryGuard.begin(sReloadGeneration,&ticket)==B_OK && ticket);
		PioneerDriverRecoveryProof proof={}; proof.flags=PIONEER_RECOVERY_PROOF_REQUIRED;
		proof.core_count=120; proof.reset_generation=2; proof.firmware_generation=2;
		sGuard.fault(sReloadGeneration);
		assert(sRecoveryGuard.complete(ticket,&proof,&newGeneration)==B_DEV_NOT_READY
			&& !newGeneration && Cold());
		assert(sRecoveryGuard.abort(ticket)==B_OK && Cold());
	}
	// Fault publication during the cold-clear CAS must preserve quarantine
	// and keep the generation reachable for another recovery attempt.
	{
		uint64 ticket=0,newGeneration=0;
		assert(sRecoveryGuard.begin(sReloadGeneration,&ticket)==B_OK);
		PioneerDriverRecoveryProof proof={};proof.flags=PIONEER_RECOVERY_PROOF_REQUIRED;
		proof.core_count=120;proof.reset_generation=3;proof.firmware_generation=3;
		injectAtomicFault=true;
		assert(sRecoveryGuard.complete(ticket,&proof,&newGeneration)==B_DEV_NOT_READY
			&& !newGeneration && Cold() && sState.generation==sReloadGeneration);
		assert(sRecoveryGuard.begin(sReloadGeneration,&ticket)==B_OK);
		assert(sRecoveryGuard.abort(ticket)==B_OK);
	}
	assert(sGuard.detach(sReloadGeneration,false)==B_DEV_NOT_READY);
	uint64 replacement=123; assert(sGuard.attach(&replacement)==B_DEV_NOT_READY && !replacement);
	assert(sGuard.info.std_ops(B_MODULE_UNINIT)==B_BUSY);
	// Epoch exhaustion permanently retains quarantine instead of wrapping and
	// making an ancient recovery ticket valid again.
	sFaultState=(int32)0xfffffffdU;
	sGuard.fault(sReloadGeneration);
	assert(uint32(atomic_get(&sFaultState))==0xffffffffU && Cold());
	uint64 exhaustedTicket=0;
	assert(sRecoveryGuard.begin(sReloadGeneration,&exhaustedTicket)==B_DEV_NOT_READY
		&& !exhaustedTicket);
	for (unsigned fault=0;fault<4;fault++) {
		PioneerDriverReloadState state; uint64_t first=0,second=0;
		assert(state.Attach(first));
		if (!fault) state.Fault(first);
		if (fault==1) assert(!state.Detach(first,false));
		if (fault==2) assert(!state.Detach(first+1,true));
		if (fault==3) { state.Fault(first+1); assert(!state.Detach(first,true)); }
		assert(state.cold && !state.Attach(second) && !second);
	}
	puts("PASS resident reload/recovery guard+extracted control:clean generations,exclusive/stale identity refusal,proof-gated recovery,NoC/core/owner and concurrent-fault refusal,cold survives abort/unload;NO PCI/MMIO/reset/native qualification");
}
