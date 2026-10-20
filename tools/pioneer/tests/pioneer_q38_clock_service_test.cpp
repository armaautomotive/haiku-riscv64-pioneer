/* SPDX-License-Identifier: MIT */
#define PIONEER_Q38_CLOCK_LEASE_MODEL_ONLY
#include "pioneer_q38_clock_lease_test.cpp"
#define PIONEER_BANKS_MOCK_ONLY
#include "pioneer_resident_banks_test.cpp"
// Self-referential mirrors let the ACTUAL native source use its long
// baseline/control member paths, without substituting those functions.
struct ClockProof {
	ClockProof& baseline=*this;
	ClockProof& control=*this;
	tt_dram_samples_result dram={};
	tt_host_dma_audit_result audit={};
	tt_arc_scratch_result arc=StrictQueue::Arc();
	tt_arc_telemetry_result telemetry=StrictQueue::Health();
};
struct ClockClosed { bool closed=false,saved=false,unsafe=false,running=false; };
struct PioneerStoreContext {
	ClockProof baseline;
	PioneerResidentBanks banks;
	bool bankStorageOnly=true,otherUnsafe=false;
	uint64_t bytes=PioneerResidentBanks::Total;
	ClockClosed* q38Dispatch=nullptr;
	ClockClosed* q38Cohort=nullptr;
	ClockClosed* q38Span=nullptr;
#ifdef PIONEER_Q38_GDN_MUTABLE_STATE
	void* q38GdnBanks=nullptr;
	ClockClosed* q38StateZero=nullptr;
#endif
	ClockClosed panelProbe,streamProbe;
	void* fleet=nullptr;
	int runtimeCoreMode=-1;
	ClockLeaseIO io;
	PioneerQ38ClockLease<ClockLeaseIO> q38Clock{io};
	explicit PioneerStoreContext(bool foreign=false) {
		StoreMock proof; baseline.dram=BankProof(proof); baseline.dram.cores_checked=120;
		baseline.audit.version=1; baseline.audit.state=TT_HOST_DMA_AUDIT_PASSED; baseline.audit.tlb_restored=1;
		if (foreign) { static tt_dram_samples_result other; other=baseline.dram; assert(banks.Bind(other)); }
		else assert(banks.Bind(baseline.dram));
	}
	bool Unsafe() const { return otherUnsafe || q38Clock.Unsafe()
#ifdef PIONEER_Q38_GDN_MUTABLE_STATE
		|| (q38StateZero && (q38StateZero->unsafe || q38StateZero->running))
#endif
		; }
};
static void dprintf(const char*,...) {} // native diagnostics are CPU metadata ONLY
#include "pioneer_q38_clock_service.inc"

int main()
{
	(void)&Input; (void)&Prepare; (void)&Upload; (void)&Close;
	{
		PioneerStoreContext c;
		assert(BeginQ38NormalClock(c) && c.io.clock==1350);
		const unsigned before=c.io.io;
		assert(!BeginQ38NormalClock(c) && c.io.io==before && c.io.maps==1);
		ClockClosed dispatch,span; dispatch.closed=span.closed=true;
		c.q38Dispatch=&dispatch; c.q38Span=&span;
		assert(FinishQ38NormalClock(c,true) && c.io.clock==800 && c.io.unmaps==1);
		const unsigned done=c.io.io; assert(FinishQ38NormalClock(c,true) && c.io.io==done && c.io.unmaps==1);
	}
	for (unsigned fault=0;fault<17;fault++) {
		PioneerStoreContext c(fault==16); ClockClosed participant;
		if (!fault) c.bankStorageOnly=false; else if (fault==1) c.bytes-=4096;
		else if (fault==2) c.otherUnsafe=true; else if (fault==3) c.q38Dispatch=&participant;
		else if (fault==4) c.q38Cohort=&participant; else if (fault==5) c.q38Span=&participant;
		else if (fault==6) c.panelProbe.saved=true; else if (fault==7) c.streamProbe.saved=true;
		else if (fault==8) c.fleet=&participant; else if (fault==9) c.runtimeCoreMode=0;
		else if (fault==10) c.baseline.dram.cores_checked=119; else if (fault==11) c.baseline.audit.version=0;
		else if (fault==12) c.baseline.audit.state=TT_HOST_DMA_AUDIT_BUSY;
		else if (fault==13) c.baseline.audit.tlb_restored=0; else if (fault==14) c.baseline.audit.recovery_required=1;
		else if (fault==15) c.banks.Quarantine();
		assert(!BeginQ38NormalClock(c) && !c.io.io && !c.io.maps); // BEFORE any view or message
	}
	for (unsigned fault=0;fault<5;fault++) {
		PioneerStoreContext c; ClockClosed active; assert(BeginQ38NormalClock(c));
		if (!fault) c.q38Dispatch=&active; else if (fault==1) c.q38Span=&active;
		else if (fault==2) c.otherUnsafe=true; else if (fault==3) c.q38Clock.Quarantine();
		const unsigned before=c.io.io;
		assert(!FinishQ38NormalClock(c,fault!=4) && c.q38Clock.Unsafe() && c.io.io==before && !c.io.unmaps);
		assert(!FinishQ38NormalClock(c,true) && !BeginQ38NormalClock(c) && c.io.io==before);
	}
	{
		PioneerStoreContext c; c.io.timeout=true;
		assert(!BeginQ38NormalClock(c) && c.q38Clock.Unsafe());
		const unsigned before=c.io.io;
		assert(!FinishQ38NormalClock(c,false) && !BeginQ38NormalClock(c) && c.io.io==before && !c.io.unmaps);
	}
#ifdef PIONEER_Q38_GDN_MUTABLE_STATE
	for (unsigned participant=0;participant<2;participant++) {
		PioneerStoreContext c; ClockClosed active;
		if (participant) c.q38StateZero=&active; else c.q38GdnBanks=&active;
		assert(!BeginQ38NormalClock(c) && !c.io.io && !c.io.maps);
	}
	for (unsigned state=0;state<4;state++) {
		PioneerStoreContext c; ClockClosed zero; assert(BeginQ38NormalClock(c));
		c.q38StateZero=&zero; zero.closed=state!=0; zero.unsafe=state==2; zero.running=state==3;
		const unsigned before=c.io.io;
		if (state==1) assert(FinishQ38NormalClock(c,true) && c.io.clock==800 && c.io.unmaps==1);
		else assert(!FinishQ38NormalClock(c,true) && c.q38Clock.Unsafe() && c.io.io==before && !c.io.unmaps);
	}
	puts("PASS ACTUAL gated GDN clock interlock: existing private owner/zero lease refuses BEGIN before IO; unclosed/unsafe/running zero refuses IDLE/unmap; known clean zero CLOSE allows explicit normal-clock END;OFFLINE ONLY");
#endif
	puts("PASS ACTUAL native clock begin/finish helpers: same bank identity/full120-held/quiet-DMA/pristine scope BEFORE any IO;open span/dispatch/unknown forbid IDLE/disposal;explicit proven-clean end only;OFFLINE NOT native authority");
}
