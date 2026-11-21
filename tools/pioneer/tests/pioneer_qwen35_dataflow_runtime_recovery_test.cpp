/* SPDX-License-Identifier: MIT */
#include "../PioneerQwen35DataflowRuntimeRecovery.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>

using namespace PioneerQwen35;

struct MockRecoveryIO {
	uint32_t status[DataflowRuntimeRecoveryNoCStatusCount]{};
	unsigned clearAfter=0xffffffffU;
	unsigned clearIndex=DataflowRuntimeRecoveryNoCStatusCount;
	unsigned pauses=0;
	unsigned selectedCore=DataflowRuntimeRecoveryNoCStatusCount;
	bool mappingStable=true;

	bool SelectRuntimeCore(unsigned core) { selectedCore=core; return core<120; }
	bool MapQ38Panel() { return true; }
	bool DataflowPanelMappingStable() { return mappingStable; }
	uint32_t ResidentNocRead(unsigned address)
	{
		if (address>=0x240 && address<0x2c0 && (address&3)==0)
			return status[DataflowRuntimeRecoveryNoCInitiators+(address-0x240)/4];
		for (unsigned initiator=0;initiator<DataflowRuntimeRecoveryNoCInitiators;
			initiator++) {
			if (address==initiator*0x800+0x40) return status[initiator];
		}
		return UINT32_MAX;
	}
	void Pause()
	{
		pauses++;
		if (pauses>=clearAfter
			&& clearIndex<DataflowRuntimeRecoveryNoCStatusCount)
			status[clearIndex]=0;
	}
};

int main()
{
	unsigned detail=99;
	MockRecoveryIO idle{};
	const bool idleResult=DataflowRuntimeWaitForNoCIdle(idle,0,8,detail);
	assert(idleResult);
	assert(detail==0 && idle.pauses==0 && idle.selectedCore==0);

	MockRecoveryIO drains{};
	drains.status[DataflowRuntimeRecoveryNoCInitiators+13]=0x45;
	drains.clearAfter=3;
	drains.clearIndex=DataflowRuntimeRecoveryNoCInitiators+13;
	assert(DataflowRuntimeWaitForNoCIdle(drains,17,8,detail));
	assert(detail==0 && drains.pauses==3 && drains.status[17]==0);

	MockRecoveryIO stuck{};
	stuck.status[2]=1;
	assert(!DataflowRuntimeWaitForNoCIdle(stuck,0,5,detail));
	assert(detail==DataflowRuntimeRecoveryNoCIdleDetailBase+2
		&& stuck.pauses==5 && stuck.status[2]==1);

	MockRecoveryIO remap{};
	remap.status[DataflowRuntimeRecoveryNoCInitiators+4]=1;
	remap.clearAfter=3;
	remap.clearIndex=DataflowRuntimeRecoveryNoCInitiators+4;
	remap.mappingStable=false;
	assert(!DataflowRuntimeWaitForNoCIdle(remap,0,8,detail));
	assert(detail==DataflowRuntimeRecoveryMappingDetail && remap.pauses==0);

	puts("PASS Qwen35 recovery only drains a read-only NoC wait and leaves stuck commands untouched");
	return 0;
}
