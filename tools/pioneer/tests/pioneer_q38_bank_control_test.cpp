/* SPDX-License-Identifier: MIT */
// Runs the ACTUAL extracted kernel dispatch/lock functions,not a second
// implementation of their policy. Only Haiku syscalls/context plumbing are
// mocked; transfers use the real lease/store/bank authority above.
#define PIONEER_BANKS_MOCK_ONLY
#include "pioneer_resident_banks_test.cpp"
#define TT_PIONEER_EXPERIMENTAL_Q38_BANKS
#define B_DEVICE_OP_CODES_END 1000
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38BankProtocol.h"
using status_t=int;
using uint32=uint32_t;
using uint64=uint64_t;
constexpr status_t B_OK=0,B_DEV_NOT_READY=-1,B_BUSY=-2,B_NOT_ALLOWED=-3,
	B_BAD_VALUE=-4,B_NO_MEMORY=-5,B_ENTRY_NOT_FOUND=-6;
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
	BanksMock io;
	PioneerResidentBanks banks;
	PioneerResidentLease lease;
	tt_host_page_result host={};
	bool bankStorageOnly=true;
	uint8_t* initialized;
	uint32_t* hashes;
	uint32_t uploadedPages=0;
	PioneerStoreContext() {
		initialized=(uint8_t*)calloc(PioneerResidentBanks::Pages,1);
		hashes=(uint32_t*)calloc(PioneerResidentBanks::Pages,4);
		assert(initialized && hashes);
		baseline.baseline.baseline.baseline.dram=BankProof(io);
		baseline.baseline.baseline.noc=io.Compute();
		assert(banks.Bind(baseline.baseline.baseline.baseline.dram));
		assert(PioneerPrepareHostPage(io,host));
	}
	~PioneerStoreContext() { free(initialized); free(hashes); }
	bool Unsafe() { return banks.Quarantined() || lease.pending || lease.recovery_required; }
	void Snapshot(tt_resident_session_result& r) {
		r={}; r.version=1; r.state=TT_RESIDENT_SESSION_READY;
		r.bytes=PioneerResidentBanks::Total; r.active=1; r.uploaded_pages=uploadedPages;
	}
};
struct PioneerProbeCookie { int team=17; PioneerStoreContext* context=nullptr; };
static PioneerProbeCookie* sRuntimeOwner=nullptr;
static bool sBankStorageOnly=false,sColdRecoveryRequired=false;
static int sLock=1,acquired=0,released=0,copyCalls=0,failCopy=0,beginCalls=0,endCalls=0;
static status_t acquire_sem(int lock) { assert(lock==1); acquired++; return B_OK; }
static void release_sem(int lock) { assert(lock==1); released++; }
#include "pioneer_lock.inc"
static status_t user_memcpy(void* to,const void* from,size_t bytes) {
	if (++copyCalls==failCopy) return -99;
	memcpy(to,from,bytes); return B_OK;
}
static status_t BeginResidentSession(PioneerProbeCookie* cookie,uint64 bytes,tt_resident_session_result& r,bool bankOnly) {
	beginCalls++; assert(bytes==PioneerResidentBanks::Total && bankOnly);
	if (cookie->context) return B_BUSY;
	cookie->context=new PioneerStoreContext; sRuntimeOwner=cookie; sBankStorageOnly=true;
	cookie->context->Snapshot(r); return B_OK;
}
static void EndResidentSession(PioneerProbeCookie* cookie,tt_resident_session_result& r,bool recheck) {
	assert(recheck && cookie->context); endCalls++;
	auto* c=cookie->context; bool clean=!c->Unsafe();
	if (c->lease.saved) clean=PioneerResidentLeaseEnd(c->io,c->baseline.baseline.baseline.noc,c->lease) && clean;
	if (clean) { assert(c->io.ReleaseHostPage()==0); if (c->lease.saved) c->io.Safe(); }
	else { assert(c->io.quarantined && !c->io.releases); sColdRecoveryRequired=true; }
	r={}; r.state=clean?TT_RESIDENT_SESSION_CLOSED:TT_RESIDENT_SESSION_CLEANUP_FAILED;
	r.host_released=clean; r.pci_restored=clean; r.cold=!clean;
	delete c; cookie->context=nullptr; sRuntimeOwner=nullptr; sBankStorageOnly=false;
}
#include "pioneer_q38_bank_control.inc"
static void Begin(PioneerProbeCookie& cookie) {
	tt_q38_bank_session session={}; session.version=1;
	assert(Q38BankControl(&cookie,TT_PCI_PROBE_Q38_BANK_BEGIN,&session,sizeof(session))==B_OK);
	assert(session.result.state==TT_RESIDENT_SESSION_READY && session.result.bytes==PioneerResidentBanks::Total);
}
static void End(PioneerProbeCookie& cookie) {
	tt_q38_bank_session session={}; session.version=1;
	assert(Q38BankControl(&cookie,TT_PCI_PROBE_Q38_BANK_END,&session,sizeof(session))==B_OK);
	assert(session.result.state==TT_RESIDENT_SESSION_CLOSED && session.result.host_released);
}
int main()
{
	PioneerProbeCookie cookie,foreign;
	tt_q38_bank_support support={};
	assert(Q38BankControl(&cookie,TT_PCI_PROBE_Q38_BANK_SUPPORT,&support,sizeof(support))==B_OK);
	assert(support.driver==44 && support.version==1 && support.banks==7 && support.storage_only
		&& support.bytes_per_bank==PioneerResidentBanks::Capacity && support.max_pages==1 && !beginCalls);
	team=18; assert(Q38BankControl(&cookie,TT_PCI_PROBE_Q38_BANK_SUPPORT,&support,sizeof(support))==B_NOT_ALLOWED); team=17;
	threadStatus=-97; assert(Q38BankControl(&cookie,TT_PCI_PROBE_Q38_BANK_SUPPORT,&support,sizeof(support))==-97); threadStatus=0;
	for (unsigned version:{0U,2U,44U,UINT32_MAX}) {
		tt_q38_bank_session bad={}; bad.version=version;
		assert(Q38BankControl(&cookie,TT_PCI_PROBE_Q38_BANK_BEGIN,&bad,sizeof(bad))==B_BAD_VALUE && !beginCalls);
	}
	Begin(cookie); assert(sBankStorageOnly && sRuntimeOwner==&cookie);
	// ALL legacy calls use the default lock gate and must refuse,even on
	// the owning FD. A second FD is refused before it can reserve a bank.
	assert(AcquireProbeLock(&cookie)==B_NOT_ALLOWED && AcquireProbeLock(&foreign,true)==B_BUSY);
	tt_q38_bank_session session={}; session.version=1;
	assert(Q38BankControl(&cookie,TT_PCI_PROBE_Q38_BANK_BEGIN,&session,sizeof(session))==B_BUSY);
	assert(cookie.context && !endCalls); // repeated BEGIN must NOT close a live owner
	tt_q38_bank_page page={}; page.version=1;
	assert(Q38BankControl(&cookie,TT_PCI_PROBE_Q38_BANK_READ,&page,sizeof(page))==B_ENTRY_NOT_FOUND);
	for (unsigned b=0;b<7;b++) {
		page.bank=b; page.offset=PioneerResidentBanks::Capacity-4096;
		for (unsigned w=0;w<1024;w++) page.data[w]=0x12340000^(b<<16)^w;
		assert(Q38BankControl(&cookie,TT_PCI_PROBE_Q38_BANK_UPLOAD,&page,sizeof(page))==B_OK);
		assert(page.result.state==TT_STORE_PASSED && !page.result.recovery_required);
	}
	for (unsigned b=0;b<7;b++) {
		page.bank=b; memset(page.data,0,sizeof(page.data));
		assert(Q38BankControl(&cookie,TT_PCI_PROBE_Q38_BANK_READ,&page,sizeof(page))==B_OK);
		for (unsigned w=0;w<1024;w++) assert(page.data[w]==(0x12340000U^(b<<16)^w));
	}
	assert(cookie.context->uploadedPages==7 && cookie.context->io.seen==127);
	unsigned requests=cookie.context->io.requests;
	page.bank=7;
	assert(Q38BankControl(&cookie,TT_PCI_PROBE_Q38_BANK_UPLOAD,&page,sizeof(page))==B_BAD_VALUE);
	page.bank=0; page.offset=UINT64_MAX;
	assert(Q38BankControl(&cookie,TT_PCI_PROBE_Q38_BANK_UPLOAD,&page,sizeof(page))==B_BAD_VALUE);
	assert(cookie.context->io.requests==requests); End(cookie);
	// A successful begin whose user result cannot be copied must unwind;
	// no orphan global owner/pinned host page is left behind.
	session={}; session.version=1; failCopy=copyCalls+2;
	assert(Q38BankControl(&cookie,TT_PCI_PROBE_Q38_BANK_BEGIN,&session,sizeof(session))==-99);
	assert(!cookie.context && !sRuntimeOwner && !sBankStorageOnly); failCopy=0;
	// A payload mismatch is not an UNKNOWN completion. Invalidate exactly
	// that page,not a different bank,without claiming a successful read.
	Begin(cookie); page={}; page.version=1;
	assert(Q38BankControl(&cookie,TT_PCI_PROBE_Q38_BANK_UPLOAD,&page,sizeof(page))==B_OK);
	cookie.context->hashes[0]^=1;
	assert(Q38BankControl(&cookie,TT_PCI_PROBE_Q38_BANK_READ,&page,sizeof(page))==B_OK);
	assert(page.result.state==TT_STORE_MISMATCH && !cookie.context->uploadedPages && !cookie.context->Unsafe());
	assert(Q38BankControl(&cookie,TT_PCI_PROBE_Q38_BANK_READ,&page,sizeof(page))==B_ENTRY_NOT_FOUND); End(cookie);
	// Actual control handler,not the caller,quarantines ALL banks on an
	// uncertain request. No later bank operation or legacy call can issue.
	Begin(cookie); cookie.context->io.failRequest=2; page={}; page.version=1; page.bank=6;
	assert(Q38BankControl(&cookie,TT_PCI_PROBE_Q38_BANK_UPLOAD,&page,sizeof(page))==B_OK);
	assert(page.result.recovery_required && sColdRecoveryRequired && cookie.context->banks.Quarantined());
	requests=cookie.context->io.requests;
	for (unsigned b=0;b<7;b++) {
		page.bank=b;
		assert(Q38BankControl(&cookie,TT_PCI_PROBE_Q38_BANK_UPLOAD,&page,sizeof(page))==B_DEV_NOT_READY);
	}
	assert(cookie.context->io.requests==requests && AcquireProbeLock(&cookie)==B_DEV_NOT_READY);
	// Final-FD-close deliberately bypasses ordinary cold gating,but MUST
	// quarantine the host page and report cold rather than free/restore.
	EndResidentSession(&cookie,session.result,true); assert(session.result.cold && !session.result.host_released);
	assert(acquired==released+0 /* successful held test gate below: none */);
}
