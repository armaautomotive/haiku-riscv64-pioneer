/* SPDX-License-Identifier: MIT */
// Actual extracted native scope+physical119 audit. A memory-only callback
// stands in for Retain;its actual full implementation is separately tested
// by the extracted global-lock/lifetime suites. NO native selection here.
#define PIONEER_Q38_NON_ROOT_HELD_MODEL_ONLY
#include "pioneer_q38_non_root_held_test.cpp"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38StateZeroController.h"
#include "../../../src/add-ons/kernel/drivers/misc/BlackholeArcClockGuard.h"
#include <functional>
#include <memory>
constexpr int B_OK=0;
static int currentThread=42;
static int find_thread(void*) { return currentThread; }
struct PioneerStoreContext;
struct PioneerMmioIO { int area=1; };
struct PioneerSessionIO:NonRootIO,PioneerMmioIO {
	PioneerStoreContext* q38MutableContext=nullptr;
	uint32_t dummy=0,*nodeID=&dummy,*tlb=&dummy,*reset=&dummy;
	int tlbArea=1,resetArea=1;
	unsigned runtimeCore=0;
	bool fleetEnabled=false,q38SpanAllowed=false;
	tt_dram_samples_result dram={};
	bool StateZeroBeforeCohort(); bool StateZeroOtherCoresHeld();
	bool NonRootHeldOwner(); bool NonRootHeldMappingStable();
};
struct FakeLifecycle { bool saved=false,dirty=false,ready=false,running=false,unsafe=false,awaiting=false; };
struct FakeClock {
	tt_arc_clock_guard_result snapshot={}; bool requires=true,unsafe=false,quarantined=false;
	FakeClock() { snapshot.state=TT_ARC_CLOCK_BUSY_VERIFIED; snapshot.busy_requested=snapshot.busy_acknowledged=1; }
	const tt_arc_clock_guard_result& Snapshot() const { return snapshot; }
	bool RequiresExplicitEnd() const { return requires; }
};
struct PioneerStoreContext {
	PioneerSessionIO io;
	PioneerResidentBanks& banks;
	uint8_t* initialized=nullptr;
	struct DramReference { tt_dram_samples_result& dram; };
	struct Baseline1 { DramReference baseline; };
	struct Baseline2 { Baseline1 baseline; };
	struct Baseline3 { Baseline2 baseline; };
	Baseline3 baseline{{{{io.dram}}}};
	PioneerQ38GdnBanks* q38GdnBanks=nullptr;
	PioneerQ38StateZeroLease* q38StateZero=nullptr;
	PioneerQ38GdnIdentity q38GdnIdentity={1,2,3,7};
	PioneerQ38SpanLease* q38Span=nullptr;
	void* q38Dispatch=nullptr; void* q38Cohort=nullptr; void* fleet=nullptr;
	FakeLifecycle q38Delta,panelProbe,streamProbe;
	PioneerResidentLease lease;
	PioneerBulkUploadLease bulk;
	PioneerOutputBufferLease bulkHost;
	PioneerWideOutputLease wideHost;
	FakeClock q38Clock;
	int q38GdnCallThread=42,runtimeCoreMode=1;
	bool bankStorageOnly=true,otherUnsafe=false,retained=false;
	uint64_t bytes=PioneerResidentBanks::Total;
	unsigned retainCalls=0;
	explicit PioneerStoreContext(PioneerResidentBanks& owner):banks(owner) {}
	bool Unsafe(bool controlledZero=false) const {
		return otherUnsafe || q38Clock.unsafe || q38Clock.quarantined || (q38GdnBanks && q38GdnBanks->Unsafe())
			|| (q38StateZero && (q38StateZero->unsafe || (!controlledZero && q38StateZero->running)));
	}
};
struct PioneerProbeCookie { PioneerStoreContext* context=nullptr; uint32_t magic=0x50313030; int team=17; };
static PioneerProbeCookie* sRuntimeOwner=nullptr;
static bool sColdRecoveryRequired=false,sReloadQuiesced=false,sBankStorageOnly=true;
static uint64_t sReloadGeneration=7;
static const void* sQ38LocalDeltaOwner=nullptr;
static void RetainQ38DispatchOwner(PioneerStoreContext& c,tt_resident_session_result&) {
	c.retained=true; c.retainCalls++; sColdRecoveryRequired=true; c.q38Clock.quarantined=true;
	if (c.q38StateZero) c.q38StateZero->unsafe=true;
	if (c.q38GdnBanks) c.q38GdnBanks->Fault(); // MEMORY ONLY;no root view restoration
	c.banks.Quarantine();
}
#include "native_scope.inc"
struct Fixture {
	StoreMock proofModel;
	tt_dram_samples_result proof=BankProof(proofModel);
	PioneerResidentBanks banks;
	std::unique_ptr<PioneerQ38GdnBanks> state=std::make_unique<PioneerQ38GdnBanks>();
	std::vector<uint8_t> initialized=std::vector<uint8_t>(PioneerResidentBanks::Pages,0);
	PioneerQ38StateZeroLease zero;
	PioneerStoreContext c{banks};
	PioneerProbeCookie cookie{&c};
	bool ownsAttached=false;
	Fixture(bool bind=true) {
		ownsAttached=!bind;
		sRuntimeOwner=&cookie; sColdRecoveryRequired=sReloadQuiesced=false; sBankStorageOnly=true;
		sReloadGeneration=7; sQ38LocalDeltaOwner=nullptr; currentThread=42;
		c.io.dram=proof; assert(banks.Bind(c.io.dram));
		std::vector<PioneerQ38GdnRegion> regions;
		for (unsigned layer=0;layer<64;layer++) if (layer%4!=3) for (unsigned slot=0;slot<2;slot++) {
			unsigned index=unsigned(regions.size()); regions.push_back({layer,slot,0,48,index%7,0x04000000+(index/7)*48*65536});
		}
		c.initialized=initialized.data(); c.io.q38MutableContext=&c;
		if (bind) {
			assert(state->Bind(banks,c.io.dram,c.q38GdnIdentity,initialized.data(),unsigned(initialized.size()),regions.data(),unsigned(regions.size())));
			c.q38GdnBanks=state.get(); c.q38StateZero=&zero;
		} else { c.q38GdnIdentity={}; c.io.q38SpanAllowed=true; c.q38GdnCallThread=-1; }
		for (unsigned w=0;w<16;w++) c.io.expected[w]=proof.source[w];
	}
	~Fixture() { if (ownsAttached) { delete c.q38GdnBanks; delete c.q38StateZero; } }
	void Inert() { assert(!c.io.StateZeroBeforeCohort() && !c.io.StateZeroOtherCoresHeld() && !c.io.touches); }
};
#ifndef PIONEER_Q38_STATE_ZERO_NATIVE_SCOPE_MODEL_ONLY
int main()
{
	(void)&Input; (void)&Prepare; (void)&Upload; (void)&Close;
	{
		Fixture f; assert(f.c.io.StateZeroBeforeCohort() && !f.c.io.touches);
		assert(f.state->MatchesInitializationOwner(f.banks,f.c.io.dram,f.c.q38GdnIdentity)
			&& !f.state->MatchesInitializationOwner(f.banks,f.proof,f.c.q38GdnIdentity)); // copied bytes are NOT same proof
		assert(f.c.io.StateZeroOtherCoresHeld() && f.c.io.observed.size()==119 && f.c.io.resets==238 && !f.c.retained);
		f.zero.identity=f.c.q38GdnIdentity; f.zero.saved=f.zero.ready=f.zero.running=true;
		assert(f.c.Unsafe() && !f.c.Unsafe(true) && f.c.io.StateZeroBeforeCohort());
		f.zero.running=false; f.zero.ready=false; f.zero.closed=true;
		assert(f.c.io.StateZeroBeforeCohort()); // controller CLOSE can be idempotent;ordinary commands remain interlocked
	}
	{
		Fixture f,foreign; sRuntimeOwner=&f.cookie;
		f.c.q38GdnBanks=foreign.state.get(); f.Inert(); // SAME ID values never transfer bank/context ownership
	}
	{
		Fixture f; PioneerSessionIO foreign; foreign.q38MutableContext=&f.c;
		assert(!foreign.StateZeroBeforeCohort() && !foreign.StateZeroOtherCoresHeld() && !foreign.touches);
		f.cookie.context=nullptr; f.Inert();
	}
	std::vector<std::function<void(Fixture&)>> changes={
		[](auto& f){ f.c.io.q38MutableContext=nullptr; },[](auto&){ sRuntimeOwner=nullptr; },
		[](auto& f){ f.cookie.magic=0; },
		[](auto&){ sColdRecoveryRequired=true; },[](auto&){ sReloadQuiesced=true; },
		[](auto&){ sBankStorageOnly=false; },[](auto& f){ f.c.bankStorageOnly=false; },
		[](auto& f){ f.c.bytes-=4096; },[](auto& f){ f.c.runtimeCoreMode=0; },
		[](auto& f){ f.c.io.runtimeCore=1; },[](auto& f){ f.c.io.fleetEnabled=true; },
		[](auto& f){ f.c.io.q38SpanAllowed=true; },[](auto& f){ f.c.q38GdnCallThread=-1; },
		[](auto&){ currentThread=43; },[](auto& f){ f.c.q38GdnBanks=nullptr; },
		[](auto& f){ f.c.q38StateZero=nullptr; },[](auto& f){ f.c.q38GdnIdentity.session=0; },
		[](auto&){ sReloadGeneration++; },[](auto& f){ f.c.q38Dispatch=&f; },
		[](auto& f){ f.c.q38Cohort=&f; },[](auto& f){ f.c.fleet=&f; },
		[](auto& f){ sQ38LocalDeltaOwner=&f; },[](auto& f){ f.c.q38Delta.saved=true; },
		[](auto& f){ f.c.q38Delta.dirty=true; },[](auto& f){ f.c.q38Delta.ready=true; },
		[](auto& f){ f.c.q38Delta.running=true; },[](auto& f){ f.c.q38Delta.awaiting=true; },
		[](auto& f){ f.c.panelProbe.saved=true; },[](auto& f){ f.c.panelProbe.dirty=true; },
		[](auto& f){ f.c.panelProbe.ready=true; },[](auto& f){ f.c.streamProbe.saved=true; },
		[](auto& f){ f.c.streamProbe.dirty=true; },[](auto& f){ f.c.streamProbe.ready=true; },
		[](auto& f){ f.zero.identity=f.c.q38GdnIdentity; f.zero.identity.sequence++; },
		[](auto& f){ f.zero.identity.session=1; },[](auto& f){ f.zero.identity.sequence=1; },
		[](auto& f){ f.zero.identity.modelEpoch=1; },[](auto& f){ f.zero.identity.driverGeneration=1; },
		[](auto& f){ f.zero.saved=true; },[](auto& f){ f.zero.dirty=true; },
		[](auto& f){ f.zero.ready=true; },[](auto& f){ f.zero.running=true; },[](auto& f){ f.zero.nonce=1; },
		[](auto& f){ f.c.q38Clock.requires=false; },[](auto& f){ f.c.q38Clock.snapshot.state=0; },
		[](auto& f){ f.c.q38Clock.snapshot.busy_requested=0; },[](auto& f){ f.c.q38Clock.snapshot.busy_acknowledged=0; },
		[](auto& f){ f.c.q38Clock.snapshot.restored=1; },[](auto& f){ f.c.otherUnsafe=true; },
		[](auto& f){ f.c.q38Clock.unsafe=true; },[](auto& f){ f.zero.unsafe=true; },
		[](auto& f){ f.state->Fault(); },[](auto& f){ f.c.q38GdnIdentity.modelEpoch++; },
	};
	for (const auto& change:changes) { Fixture f; change(f); f.Inert(); }
	for (unsigned field=0;field<5;field++) {
		Fixture f; PioneerQ38SpanLease span; span.closed=true; f.c.q38Span=&span;
		assert(f.c.io.StateZeroBeforeCohort());
		if (!field) span.closed=false; else if (field==1) span.dirty=true; else if (field==2) span.ready=true;
		else if (field==3) span.running=true; else span.unsafe=true;
		f.Inert();
	}
	for (unsigned field=0;field<6;field++) {
		Fixture f;
		if (!field) f.c.io.PioneerMmioIO::area=-1; else if (field==1) f.c.io.nodeID=nullptr;
		else if (field==2) f.c.io.tlbArea=-1; else if (field==3) f.c.io.tlb=nullptr;
		else if (field==4) f.c.io.resetArea=-1; else f.c.io.reset=nullptr;
		assert(!f.c.io.NonRootHeldMappingStable() && !f.c.io.StateZeroOtherCoresHeld() && !f.c.io.touches);
	}
	for (unsigned kind=0;kind<3;kind++) for (unsigned position:{1U,2U,3U}) {
		Fixture f; unsigned total=kind==0?360:kind==1?238:32,fail=position==1?1:position==2?total/2:total;
		if (!kind) f.c.io.failTlb=fail; else if (kind==1) f.c.io.failReset=fail; else f.c.io.failSource=fail;
		assert(!f.c.io.StateZeroOtherCoresHeld() && f.c.io.unknown && f.c.retained && f.c.retainCalls==1
			&& f.zero.unsafe && f.state->Unsafe() && f.banks.Quarantined() && f.c.q38Clock.quarantined && sColdRecoveryRequired);
		unsigned touches=f.c.io.touches; assert(!f.c.io.StateZeroOtherCoresHeld() && f.c.io.touches==touches && f.c.retainCalls==1);
	}
	printf("PASS OFFLINE ACTUAL native zero scope:real bank/identity binding,%zu CPU-only refusal gates,closed-span handoff,idempotent-close scope,ordinary Unsafe still rejects controlled-running root,ALL119 physical audit,first/middle/last source/TLB/reset faults hand off to MEMORY-ONLY retention BEFORE any later IO;NO native allocation/call-seal/driver/compute readiness\n",changes.size()+5+6);
}
#endif
