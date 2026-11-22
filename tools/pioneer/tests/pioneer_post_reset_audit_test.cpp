/* SPDX-License-Identifier: MIT */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include "../../../src/add-ons/kernel/drivers/misc/PioneerPostResetAudit.h"

struct FakeAudit {
	unsigned pass=0,core=0,selects=0,reads=0,pauses=0;
	unsigned failPass=2,failCore=0,failNoC=0,failOffset=0;
	unsigned mode=0;
	bool SelectPostResetCore(unsigned next) {
		core=next;selects++;
		return !(mode==1 && pass==failPass && core==failCore);
	}
	bool ReadPostResetState(uint32_t& value) {
		if (mode==2 && pass==failPass && core==failCore) return false;
		value=(mode==3 && pass==failPass && core==failCore)?0x46800:0x47800;
		return true;
	}
	bool ReadPostResetNoC(unsigned noc,unsigned offset,uint32_t& value) {
		reads++;
		const bool hit=pass==failPass && core==failCore
			&& noc==failNoC && offset==failOffset;
		if (mode==4 && hit) return false;
		value=(mode==5 && hit)?1:0;
		return true;
	}
	void PostResetAuditPause() { pauses++;pass++; }
};

int main()
{
	PioneerPostResetAuditResult r{};
	FakeAudit clean;
	assert(PioneerPostResetAudit(clean,r));
	assert(r.failure==PioneerPostResetAuditNone && r.audited==240
		&& clean.selects==240 && clean.reads==240*2*36 && clean.pauses==1);
	FakeAudit held;held.mode=3;held.failPass=1;held.failCore=119;
	assert(!PioneerPostResetAudit(held,r));
	assert(r.failure==PioneerPostResetAuditCoreNotHeld && r.pass==1
		&& r.core==119 && r.audited==239 && r.value==0x46800);
	FakeAudit traffic;traffic.mode=5;traffic.failPass=0;traffic.failCore=27;
	traffic.failNoC=1;traffic.failOffset=0x240+31*4;
	assert(!PioneerPostResetAudit(traffic,r));
	assert(r.failure==PioneerPostResetAuditOutstanding && r.core==27
		&& r.noc==1 && r.index==35 && r.value==1);
	FakeAudit busy;busy.mode=5;busy.failPass=1;busy.failCore=0;
	busy.failNoC=0;busy.failOffset=3*0x800+0x40;
	assert(!PioneerPostResetAudit(busy,r));
	assert(r.failure==PioneerPostResetAuditNoCBusy && r.pass==1
		&& r.core==0 && r.noc==0 && r.index==3);
	FakeAudit missing;missing.mode=4;missing.failPass=0;missing.failCore=4;
	missing.failNoC=1;missing.failOffset=0x40;
	assert(!PioneerPostResetAudit(missing,r));
	assert(r.failure==PioneerPostResetAuditNoCRead && r.core==4 && r.noc==1);
	FakeAudit select;select.mode=1;select.failPass=0;select.failCore=0;
	assert(!PioneerPostResetAudit(select,r));
	assert(r.failure==PioneerPostResetAuditSelect && !select.reads);
	puts("PASS post-reset fleet audit: both NoCs and all 120 cores, repeated pass, read failures, busy and outstanding traffic; FAKE IO ONLY");
}
