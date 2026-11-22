/* SPDX-License-Identifier: MIT */
#define PIONEER_MMIO_PROBE_MODEL_ONLY
#include "pioneer_mmio_probe_test.cpp"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerPostResetPci.h"
#include <cstdio>

struct DisableMasterIO:MockIO {
	void CardWrite(unsigned offset,unsigned width,uint32_t value) {
		if (offset==4 && value==0x402 && command[0]==0x406) {
			assert(width==2 && (command[0]==0x402 || command[0]==0x406));
			if (++writes!=droppedWrite) command[0]=value;
			return;
		}
		MockIO::CardWrite(offset,width,value);
	}
	void RootWrite(unsigned offset,unsigned width,uint32_t value) {
		if (offset==4 && value==0x402 && command[1]==0x406) {
			assert(width==2 && command[0]==0x402);
			if (++writes!=droppedWrite) command[1]=value;
			return;
		}
		MockIO::RootWrite(offset,width,value);
	}
};

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
	MockIO active;tt_pioneer_mmio_result saved{};tt_pioneer_mmio_transaction transaction{};
	assert(BlackholePioneerMmioBegin(active,saved,transaction));
	unsigned stage=0;assert(PioneerPostResetPciFinish(active,saved,stage));
	assert(stage==PioneerPostResetPciFinishComplete && active.command[0]==0
		&& active.command[1]==0 && active.mapped);
	for (unsigned i=0;i<6;i++) assert(active.bars[i]==saved.bars_before[i]);
	for (unsigned i=0;i<3;i++) assert(active.bridge[i]==saved.bridge_before[i]);
	MockIO badSaved;assert(BlackholePioneerMmioBegin(badSaved,saved,transaction));
	saved.bars_before[0]=0xdeadbeef;const unsigned writes=badSaved.writes;
	assert(!PioneerPostResetPciFinish(badSaved,saved,stage)
		&& stage==PioneerPostResetPciFinishRefused && badSaved.writes==writes);
	for (unsigned drop=1;drop<=11;drop++) {
		MockIO trial;assert(BlackholePioneerMmioBegin(trial,saved,transaction));
		trial.droppedWrite=trial.writes+drop;
		const bool okay=PioneerPostResetPciFinish(trial,saved,stage);
		assert(!(trial.command[0]&4) && !(trial.command[1]&4));
		if (okay) assert(stage==PioneerPostResetPciFinishComplete
			&& trial.command[0]==0 && trial.command[1]==0
			&& memcmp(trial.bars,saved.bars_before,sizeof(trial.bars))==0
			&& memcmp(trial.bridge,saved.bridge_before,sizeof(trial.bridge))==0);
	}
	DisableMasterIO before;assert(BlackholePioneerMmioBegin(before,saved,transaction));
	before.command[0]=before.command[1]=0x406;
	assert(PioneerPreResetDisableMaster(before) && before.command[0]==0x402
		&& before.command[1]==0x402);
	DisableMasterIO lost;assert(BlackholePioneerMmioBegin(lost,saved,transaction));
	lost.command[0]=lost.command[1]=0x406;lost.droppedWrite=lost.writes+1;
	assert(!PioneerPreResetDisableMaster(lost) && lost.command[0]==0x406
		&& lost.command[1]==0x406);
	std::puts("PASS post-reset PCI active-layout restore: real plan, bus-master/marker/interrupt refusal, dropped writes, decoder ordering; SIMULATED PCI ONLY");
}
