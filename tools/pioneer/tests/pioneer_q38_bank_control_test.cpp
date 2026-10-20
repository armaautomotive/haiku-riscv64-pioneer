/* SPDX-License-Identifier: MIT */
// Runs the ACTUAL extracted kernel dispatch/lock functions,not a second
// implementation of their policy. Only Haiku syscalls/context plumbing are
// mocked; transfers use the real lease/store/bank authority above.
#define PIONEER_BANKS_MOCK_ONLY
#include "pioneer_resident_banks_test.cpp"
#ifdef PIONEER_Q38_NORMAL_CLOCK
#define PIONEER_Q38_CLOCK_LEASE_MODEL_ONLY
#include "pioneer_q38_clock_lease_test.cpp"
#endif
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
#ifdef PIONEER_Q38_GDN_MUTABLE_STATE
	void* q38GdnBanks=nullptr;
	void* q38StateZero=nullptr; // presence ONLY in this unrelated bank-IO fixture
#endif
#ifdef PIONEER_Q38_NORMAL_CLOCK
	ClockLeaseIO clockIO;
	PioneerQ38ClockLease<ClockLeaseIO> q38Clock{clockIO};
#endif
	Base4 baseline;
	BanksMock io;
	PioneerResidentBanks banks;
	PioneerResidentLease lease;
	PioneerBulkUploadLease bulk;
	PioneerOutputBufferLease bulkHost;
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
	bool Unsafe() { return
#ifdef PIONEER_Q38_NORMAL_CLOCK
		q38Clock.Unsafe() ||
#endif
		banks.Quarantined() || lease.pending || lease.recovery_required
		|| bulk.unsafe || bulkHost.pending || bulkHost.quarantined; }
	void Snapshot(tt_resident_session_result& r) {
		r={}; r.version=1; r.state=TT_RESIDENT_SESSION_READY;
		r.bytes=PioneerResidentBanks::Total; r.active=1; r.uploaded_pages=uploadedPages;
	}
};
struct PioneerProbeCookie { int team=17; PioneerStoreContext* context=nullptr; };
static PioneerProbeCookie* sRuntimeOwner=nullptr;
static bool sBankStorageOnly=false,sColdRecoveryRequired=false;
static int sLock=1,acquired=0,released=0,copyCalls=0,failCopy=0,beginCalls=0,endCalls=0;
#ifdef PIONEER_Q38_NORMAL_CLOCK
static bool selectClockOnBegin=false;
static unsigned clockIOAtBegin=0;
// The real retained-context implementation is tested by the independent
// actual-cookie suite. This plumbing stub MUST NOT perform any cleanup IO.
static void RetainQ38DispatchOwner(PioneerStoreContext& c,tt_resident_session_result& r) {
	c.q38Clock.Quarantine(); c.banks.Quarantine(); c.lease.recovery_required=true;
	c.io.QuarantineHostPage(); sColdRecoveryRequired=true;
	r={}; r.active=1; r.cold=1; r.state=TT_RESIDENT_SESSION_CLEANUP_FAILED;
}
#endif
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
#ifdef PIONEER_Q38_NORMAL_CLOCK
	if (selectClockOnBegin) {
		auto& c=*cookie->context;
		assert(c.q38Clock.Begin(c.clockIO.Arc(),c.clockIO.Health())); clockIOAtBegin=c.clockIO.io;
	}
#endif
	cookie->context->Snapshot(r); return B_OK;
}
static void EndResidentSession(PioneerProbeCookie* cookie,tt_resident_session_result& r,bool recheck) {
	assert(recheck && cookie->context); endCalls++;
	auto* c=cookie->context; bool clean=!c->Unsafe();
	if (c->bulk.saved) clean=PioneerBulkUploadClose(c->io,c->baseline.baseline.baseline.baseline.dram,
		c->baseline.baseline.baseline.noc,c->host,c->lease,c->bulkHost,c->bulk) && clean;
	if (c->lease.saved) clean=PioneerResidentLeaseEnd(c->io,c->baseline.baseline.baseline.noc,c->lease) && clean;
	if (clean) {
		if (c->bulkHost.allocated) assert(PioneerReleaseOutputBuffer(c->io,c->bulkHost,true));
		assert(c->io.ReleaseHostPage()==0); if (c->lease.saved) c->io.Safe();
	} else {
		assert(c->io.HostReadMock::quarantined && !c->io.PageMock::releases);
		if (c->bulkHost.allocated) assert(c->io.OutputBufferMock::quarantined && !c->io.OutputBufferMock::releases);
		sColdRecoveryRequired=true;
	}
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
	(void)&Input; (void)&Prepare; (void)&Upload; (void)&Close;
	PioneerProbeCookie cookie,foreign;
	tt_q38_bank_support support={};
	assert(Q38BankControl(&cookie,TT_PCI_PROBE_Q38_BANK_SUPPORT,&support,sizeof(support))==B_OK);
	assert(support.driver==45 && support.version==1 && support.banks==7 && support.storage_only
		&& support.bytes_per_bank==PioneerResidentBanks::Capacity && support.max_pages==4 && !beginCalls);
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
	// Actual bulk dispatch + transport. Vary sizes and placement; write all
	// banks before independent PIO reads, including untouched neighbours.
	Begin(cookie);
	tt_q38_bank_bulk bulk={}; bulk.version=1;
	uint32_t expected[7][8][1024]={};
	for (unsigned bank=0;bank<7;bank++) for (unsigned pages=1;pages<=4;pages++) {
		bulk.bank=bank; bulk.pages=pages;
		bulk.offset=PioneerResidentBanks::Capacity-32768+(pages%2)*4096;
		for (unsigned w=0;w<4096;w++) bulk.data[w]=0x37200000U^(bank<<20)^(pages<<16)^w;
		assert(Q38BankControl(&cookie,TT_PCI_PROBE_Q38_BANK_BULK_UPLOAD,&bulk,sizeof(bulk))==B_OK);
		assert(bulk.result.transfer.state==TT_STORE_PASSED && bulk.result.transfer.completed==6
			&& bulk.result.transfer.readback_complete && !bulk.result.transfer.recovery_required);
		for (unsigned p=0;p<pages;p++) memcpy(expected[bank][pages%2+p],bulk.data+p*1024,4096);
	}
	assert(cookie.context->uploadedPages==7*4);
	for (unsigned bank=0;bank<7;bank++) for (unsigned p=0;p<4;p++) {
		page={}; page.version=1; page.bank=bank; page.offset=PioneerResidentBanks::Capacity-32768+p*4096;
		assert(Q38BankControl(&cookie,TT_PCI_PROBE_Q38_BANK_READ,&page,sizeof(page))==B_OK);
		assert(page.result.state==TT_STORE_PASSED && !memcmp(page.data,expected[bank][p],4096));
	}
	requests=cookie.context->io.requests;
	for (unsigned fault=0;fault<9;fault++) {
		bulk={}; bulk.version=1; bulk.pages=4;
		if (fault==0) bulk.version=2;
		if (fault==1) bulk.bank=7;
		if (fault==2) bulk.reserved=1;
		if (fault==3) bulk.pages=0;
		if (fault==4) bulk.pages=5;
		if (fault==5) bulk.offset=1;
		if (fault==6) bulk.offset=PioneerResidentBanks::Capacity-4096;
		if (fault==7) bulk.offset=UINT64_MAX-4095;
		assert(Q38BankControl(&cookie,TT_PCI_PROBE_Q38_BANK_BULK_UPLOAD,&bulk,sizeof(bulk)-(fault==8))==B_BAD_VALUE);
	}
	assert(requests==cookie.context->io.requests);
	bulk={}; bulk.version=1; bulk.pages=4;
	assert(Q38BankControl(&foreign,TT_PCI_PROBE_Q38_BANK_BULK_UPLOAD,&bulk,sizeof(bulk))==B_BUSY);
	team=18; assert(Q38BankControl(&cookie,TT_PCI_PROBE_Q38_BANK_BULK_UPLOAD,&bulk,sizeof(bulk))==B_NOT_ALLOWED); team=17;
	End(cookie);
	// Every bank and every stage can become UNKNOWN. No subsequent upload,
	// END via normal gate, host release, or bank-local recovery is allowed.
	for (unsigned bank=0;bank<7;bank++) for (unsigned step=1;step<=6;step++) {
		Begin(cookie); bulk={}; bulk.version=1; bulk.bank=bank; bulk.pages=4;
		cookie.context->io.failRequest=step;
		assert(Q38BankControl(&cookie,TT_PCI_PROBE_Q38_BANK_BULK_UPLOAD,&bulk,sizeof(bulk))==B_OK);
		assert(bulk.result.transfer.recovery_required && sColdRecoveryRequired && cookie.context->banks.Quarantined());
		requests=cookie.context->io.requests;
		for (unsigned other=0;other<7;other++) {
			bulk.bank=other;
			assert(Q38BankControl(&cookie,TT_PCI_PROBE_Q38_BANK_BULK_UPLOAD,&bulk,sizeof(bulk))==B_DEV_NOT_READY);
		}
		assert(requests==cookie.context->io.requests);
		EndResidentSession(&cookie,session.result,true); assert(session.result.cold && !session.result.host_released);
		// Test-only simulated NEW cold boot, not a runtime retry/unquarantine.
		sColdRecoveryRequired=false;
	}
	// A successful begin whose user result cannot be copied must unwind;
	// no orphan global owner/pinned host page is left behind.
	session={}; session.version=1; failCopy=copyCalls+2;
	assert(Q38BankControl(&cookie,TT_PCI_PROBE_Q38_BANK_BEGIN,&session,sizeof(session))==-99);
	assert(!cookie.context && !sRuntimeOwner && !sBankStorageOnly); failCopy=0;
#ifdef PIONEER_Q38_NORMAL_CLOCK
	selectClockOnBegin=true; session={}; session.version=1; failCopy=copyCalls+2;
	const int endsBefore=endCalls;
	assert(Q38BankControl(&cookie,TT_PCI_PROBE_Q38_BANK_BEGIN,&session,sizeof(session))==-99); failCopy=0;
	assert(cookie.context && sRuntimeOwner==&cookie && sBankStorageOnly && sColdRecoveryRequired && endCalls==endsBefore);
	auto& clockOwner=*cookie.context;
	assert(clockOwner.q38Clock.Unsafe() && clockOwner.q38Clock.RequiresExplicitEnd() && clockOwner.io.HostReadMock::quarantined
		&& clockOwner.banks.Quarantined() && clockOwner.clockIO.clock==1350 && !clockOwner.clockIO.unmaps && clockOwner.clockIO.io==clockIOAtBegin);
	assert(Q38BankControl(&cookie,TT_PCI_PROBE_Q38_BANK_END,&session,sizeof(session))==B_DEV_NOT_READY && clockOwner.clockIO.io==clockIOAtBegin);
	delete cookie.context; cookie.context=nullptr; sRuntimeOwner=nullptr; sBankStorageOnly=sColdRecoveryRequired=selectClockOnBegin=false; // TEST-only cold disposal
	puts("PASS ACTUAL BANK_BEGIN lost copyout with normal clock:retained owner/cookie/PCI/host/clock views,NO implicit END/IDLE/unmap or later hardware operation");
#endif
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
