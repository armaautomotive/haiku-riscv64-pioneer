/* SPDX-License-Identifier: MIT */
#define PIONEER_MMIO_PROBE_MODEL_ONLY
#include "pioneer_mmio_probe_test.cpp"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerPostResetPci.h"
#include <cstdio>

int main()
{
	PioneerPostResetPciResult result{};
	MockIO io;io.mapped=true;
	assert(PioneerPostResetPciRestore(io,result));
	assert(result.stage==PioneerPostResetPciReady && io.command[0]==0x402
		&& io.command[1]==0x402 && io.writes==13);
	PioneerBlackholeBarPlan plan{};
	assert(PioneerPlanBlackholeBars(io.profile.ranges,TT_PIONEER_RANGE_COUNT,plan));
	assert(memcmp(io.bars,plan.bars,sizeof(io.bars))==0
		&& memcmp(io.bridge,plan.bridge,sizeof(io.bridge))==0);
	for (unsigned drop=1;drop<=13;drop++) {
		MockIO trial;trial.mapped=true;trial.command[0]=trial.command[1]=0x402;
		trial.droppedWrite=drop;
		bool okay=PioneerPostResetPciRestore(trial,result);
		assert(!(trial.command[0]&4) && !(trial.command[1]&4));
		if (okay) assert(result.stage==PioneerPostResetPciReady
			&& trial.command[0]==0x402 && trial.command[1]==0x402
			&& memcmp(trial.bars,plan.bars,sizeof(trial.bars))==0
			&& memcmp(trial.bridge,plan.bridge,sizeof(trial.bridge))==0);
	}
	MockIO bme;bme.command[0]=0x406;
	assert(!PioneerPostResetPciRestore(bme,result) && !bme.writes);
	MockIO marker;marker.command[0]=0x40;
	assert(!PioneerPostResetPciRestore(marker,result) && !marker.writes);
	MockIO identity;identity.cardID=0xffffffff;
	assert(!PioneerPostResetPciRestore(identity,result) && !identity.writes);
	MockIO interrupt;interrupt.msiControl=1;
	assert(!PioneerPostResetPciRestore(interrupt,result) && !interrupt.writes);
	MockIO disable;disable.mapped=true;disable.command[0]=0x402;
	disable.disableCardFails=true;
	assert(!PioneerPostResetPciRestore(disable,result)
		&& result.stage==PioneerPostResetPciDisable && disable.writes==2);
	std::puts("PASS post-reset PCI active-layout restore: real plan, bus-master/marker/interrupt refusal, dropped writes, decoder ordering; SIMULATED PCI ONLY");
}
