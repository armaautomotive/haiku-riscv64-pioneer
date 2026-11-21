/* SPDX-License-Identifier: MIT */
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstring>

#include "../../../src/add-ons/kernel/drivers/misc/PioneerQwen35DataflowPciMemory.h"

struct MockPciRaw {
	uint32_t panel[DataflowPciL1ViewWords]{};
	unsigned boot=PioneerQwen35::DataflowServiceBootWord;
	unsigned reset=0x46800;
	unsigned selected=UINT32_MAX;
	unsigned writes=0;
	bool mapped=true;
	bool idle=true;

	bool SelectRuntimeCore(unsigned core) { selected=core; return core<120; }
	bool DataflowPanelMappingStable() const { return mapped; }
	bool DataflowCoreIdleForHold(unsigned core) const { return core<120 && idle; }
	unsigned ResetRead() const { return reset; }
	unsigned RuntimeCodeRead(unsigned word) const { assert(word==0); return boot; }
	void RuntimeCodeWrite(unsigned word,unsigned value)
	{
		assert(word==0);
		boot=value;
		writes++;
	}
	void RuntimeResetWrite(unsigned value) { reset=value; }
	uint32_t Q38PanelRead(unsigned address) const
	{
		assert(selected<120 && address>=DataflowPciL1ViewBase
			&& address<DataflowPciL1ViewBase+DataflowPciL1ViewBytes);
		return panel[(address-DataflowPciL1ViewBase)/sizeof(uint32_t)];
	}
	void Q38PanelWrite(unsigned address,uint32_t value)
	{
		assert(selected<120 && address>=DataflowPciL1ViewBase
			&& address<DataflowPciL1ViewBase+DataflowPciL1ViewBytes);
		panel[(address-DataflowPciL1ViewBase)/sizeof(uint32_t)]=value;
		writes++;
	}
	void Pause() {}
};

using namespace PioneerQwen35;

int main()
{
	assert(PioneerQwen35DataflowPciMemory<MockPciRaw>::RangeAllowed(
		DataflowServiceBootAddress,sizeof(uint32_t)));
	assert(PioneerQwen35DataflowPciMemory<MockPciRaw>::RangeAllowed(
		DataflowServiceEntryAddress,16));
	assert(PioneerQwen35DataflowPciMemory<MockPciRaw>::RangeAllowed(
		DataflowQueueStateAddress,DataflowQueueStateBytes));
	assert(!PioneerQwen35DataflowPciMemory<MockPciRaw>::RangeAllowed(0x7000,4));
	assert(!PioneerQwen35DataflowPciMemory<MockPciRaw>::RangeAllowed(
		DataflowServiceTextLimit-4,8));
	assert(!PioneerQwen35DataflowPciMemory<MockPciRaw>::RangeAllowed(
		DataflowServiceEntryAddress+1,4));

	MockPciRaw raw;
	for (unsigned i=0;i<DataflowPciL1ViewWords;i++)
		raw.panel[i]=0x13570000u+i;
	uint32_t originalPanel[DataflowPciL1ViewWords];
	memcpy(originalPanel,raw.panel,sizeof(raw.panel));
	PioneerQwen35DataflowPciMemory<MockPciRaw> memory(raw);
	DataflowPciCoreSnapshot snapshot{};

	raw.idle=false;
	assert(!memory.HoldCore(3));
	assert(!memory.CaptureCoreState(3,snapshot));
	raw.idle=true;
	assert(memory.CaptureCoreState(3,snapshot));
	assert(snapshot.captured && snapshot.core==3 && snapshot.reset==0x46800);
	assert(memory.HoldCore(3) && memory.CoreHeld(3));

	const uint32_t text=0x76543210;
	assert(memory.WriteCoreMemory(3,DataflowServiceEntryAddress,&text,sizeof(text)));
	assert(!memory.WriteCoreMemory(3,0x7000,&text,sizeof(text)));
	assert(!memory.WriteCoreMemory(3,DataflowServiceTextLimit-4,&text,8));
	assert(memory.StartCore(3)); // valid boot word starts the core
	assert(raw.reset==0x46800);
	assert(memory.HoldCore(3));
	assert(raw.boot==DataflowServiceBootWord);

	assert(memory.RestoreCoreState(3,snapshot));
	assert(memory.VerifyCoreStateRestored(3,snapshot));
	assert(raw.reset==0x47800);
	assert(raw.boot==PioneerQwen35::DataflowServiceBootWord);
	for (unsigned i=0;i<DataflowPciL1ViewWords;i++)
		assert(raw.panel[i]==originalPanel[i]);

	memory.ReleaseCoreSnapshot(snapshot);
	assert(!snapshot.captured && !snapshot.localWords);
	puts("PASS PCI L1 bounds, hold gates, snapshot, and restore");
	return 0;
}
