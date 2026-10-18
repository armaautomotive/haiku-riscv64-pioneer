/* SPDX-License-Identifier: MIT */
#define PIONEER_Q38_COHORT_POLICY_MOCK_ONLY
#include "pioneer_q38_cohort_probe_test.cpp"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38DispatchBootstrap.h"
struct BootstrapIO:CohortIO {
	unsigned fault=0,target=0,armed=0,released=0;
	BootstrapIO():CohortIO(120) {}
	uint32_t Q38PanelRead(uint32_t a) { return C().Q38PanelRead(a); }
	void Q38PanelWrite(uint32_t a,uint32_t v) { assert(!quarantined); C().Q38PanelWrite(a,v); }
	uint32_t RuntimePcRead() { return fault==4 && current==target?0x72004:TT_Q38_PEER_WAIT_LOAD; }
	void RuntimeResetWrite(uint32_t value) {
		assert(!quarantined && current && value!=0x47800); auto& c=C(); c.resets.push_back(value);
		if (value==0x87ffff) { armed++; if (!(fault==1 && current==target)) c.coreReset=value; return; }
		assert(value==0x46800 && armed==119); released++;
		if (fault==2 && current==target) return;
		c.coreReset=value;
		const unsigned at=(0x74140-TT_Q38_PANEL_VIEW_BASE)/4;
		if (fault==3 && current==target) return;
		for (unsigned i=0;i<16;i++) c.view[at+i]=0;
		c.view[at+3]=TT_Q38_PEER_MARKER;
		if (current==target) switch(fault) {
			case 5:c.view[at+2]=0x46503851;break;
			case 6:c.counters[0]++;break;
			case 7:c.config^=1;break;
			case 8:c.view[at+7]=1;break;
		}
	}
};
struct BootstrapFixture {
	BootstrapIO io;
	std::unique_ptr<PioneerQ38DispatchLease> lease{new PioneerQ38DispatchLease};
	std::vector<std::unique_ptr<PanelOwners>> owners;
	tt_dram_samples_result dram; PioneerResidentBanks banks;
	BootstrapFixture() {
		dram=BankProof(*io.core[0]); assert(banks.Bind(dram));
		for (unsigned w=0;w<120;w++) {
			owners.emplace_back(new PanelOwners(*io.core[w])); auto& n=lease->worker[w];
			n.master=&owners[w]->store; n.noc=owners[w]->noc; if (w) n.noc.version=PioneerWorkerVersion(w);
			assert(PioneerQ38DispatchPrepareRole(io,dram,banks,*lease,w));
		}
	}
	bool Boot() { return PioneerQ38DispatchBootstrap(io,dram,banks,*lease); }
};
#ifndef PIONEER_Q38_DISPATCH_BOOTSTRAP_MOCK_ONLY
int main()
{
	(void)&Input; (void)&Prepare; (void)&Upload; (void)&Close; (void)&PreparePanel; (void)&Run; (void)&Job; (void)&ClosePanel; (void)&Finish;
	{
		BootstrapFixture f; assert(f.Boot() && f.lease->booted && !f.lease->unsafe && f.io.armed==119 && f.io.released==119);
		unsigned writes=f.io.Writes(); assert(f.Boot() && f.io.Writes()==writes);
		assert(f.io.core[0]->coreReset==0x47800 && !f.lease->worker[0].image.running);
		for (unsigned w=1;w<120;w++) assert(f.io.core[w]->coreReset==0x46800 && f.lease->worker[w].image.running);
		assert(!PioneerQ38DispatchPrepareRole(f.io,f.dram,f.banks,*f.lease,0) && f.io.Writes()==writes);
	}
	for (unsigned fault=1;fault<=8;fault++) for (unsigned worker:{1U,60U,119U}) {
		BootstrapFixture f; f.io.fault=fault; f.io.target=worker;
		assert(!f.Boot() && f.lease->unsafe && f.banks.Quarantined());
		unsigned writes=f.io.Writes(),selects=f.io.selects; f.io.quarantined=true;
		assert(!f.Boot() && !PioneerQ38DispatchPrepareRole(f.io,f.dram,f.banks,*f.lease,0));
		assert(f.io.Writes()==writes && f.io.selects==selects);
		for (unsigned w=0;w<120;w++) assert(f.lease->worker[w].image.unsafe && f.lease->worker[w].Store().recovery_required);
		for (const auto& core:f.io.core) for (unsigned value:core->resets) assert(value!=0x47800);
	}
	puts("PASS OFFLINE persistent bootstrap controller:exact root/peer role images+264KiB save,ALL119 staged/armed before release,ALL119 physical idle-PC/source/zero-NoC proofs before ready,24 first/middle/last reset/status/PC/counter/source faults quarantineALL without retry/reset/restore/free;NO native driver/launch qualification");
}
#endif
