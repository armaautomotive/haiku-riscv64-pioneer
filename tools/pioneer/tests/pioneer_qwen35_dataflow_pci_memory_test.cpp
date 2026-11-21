/* SPDX-License-Identifier: MIT */
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstring>

#include "../../../src/add-ons/kernel/drivers/misc/PioneerQwen35DataflowPciMemory.h"

struct MockPciRaw {
	uint32_t panel[DataflowPciL1ViewWords]{};
	unsigned boot[PioneerQwen35::DataflowCoreCount];
	unsigned reset[PioneerQwen35::DataflowCoreCount];
	PioneerQwen35::DataflowWorkerMailbox mailbox[PioneerQwen35::DataflowCoreCount]{};
	PioneerQwen35::DataflowQueueControl queue[PioneerQwen35::DataflowCoreCount]
		[PioneerQwen35::DataflowCircularBufferCount]{};
	bool serviceImage[PioneerQwen35::DataflowCoreCount]{};
	unsigned selected=UINT32_MAX;
	unsigned writes=0;
	bool mapped=true;
	bool idle=true;
	bool idleForWrite=true;

	MockPciRaw()
	{
		for (unsigned core=0;core<PioneerQwen35::DataflowCoreCount;core++) {
			boot[core]=PioneerQwen35::DataflowServiceBootWord;
			reset[core]=0x46800;
		}
	}
	bool SelectRuntimeCore(unsigned core) { selected=core; return core<120; }
	bool DataflowPanelMappingStable() const { return mapped; }
	bool DataflowCoreIdleForHold(unsigned core) const { return core<120 && idle; }
	bool DataflowCoreIdleForWrite(unsigned core,unsigned,unsigned) const
		{ return core<120 && idleForWrite; }
	bool DataflowCoreServiceImageMatches(unsigned core) const
		{ return core<120 && serviceImage[core]; }
	unsigned ResetRead() const { assert(selected<120); return reset[selected]; }
	unsigned RuntimeCodeRead(unsigned word) const
		{ assert(word==0 && selected<120); return boot[selected]; }
	void RuntimeCodeWrite(unsigned word,unsigned value)
	{
		assert(word==0 && selected<120);
		boot[selected]=value;
		writes++;
	}
	void RuntimeResetWrite(unsigned value) { assert(selected<120); reset[selected]=value; }
	uint32_t Q38PanelRead(unsigned address) const
	{
		assert(selected<120 && address>=DataflowPciL1ViewBase
			&& address<DataflowPciL1ViewBase+DataflowPciL1ViewBytes);
		if (address>=PioneerQwen35::DataflowWorkerMailboxAddress
			&& address<PioneerQwen35::DataflowWorkerMailboxAddress
				+PioneerQwen35::DataflowWorkerMailboxBytes) {
			const uint32_t* words=reinterpret_cast<const uint32_t*>(&mailbox[selected]);
			return words[(address-PioneerQwen35::DataflowWorkerMailboxAddress)/4];
		}
		if (address>=PioneerQwen35::DataflowQueueStateAddress
			&& address<PioneerQwen35::DataflowQueueStateAddress
				+PioneerQwen35::DataflowQueueStateBytes) {
			const uint32_t* words=reinterpret_cast<const uint32_t*>(queue[selected]);
			return words[(address-PioneerQwen35::DataflowQueueStateAddress)/4];
		}
		return panel[(address-DataflowPciL1ViewBase)/sizeof(uint32_t)];
	}
	void Q38PanelWrite(unsigned address,uint32_t value)
	{
		assert(selected<120 && address>=DataflowPciL1ViewBase
			&& address<DataflowPciL1ViewBase+DataflowPciL1ViewBytes);
		if (address>=PioneerQwen35::DataflowWorkerMailboxAddress
			&& address<PioneerQwen35::DataflowWorkerMailboxAddress
				+PioneerQwen35::DataflowWorkerMailboxBytes) {
			uint32_t* words=reinterpret_cast<uint32_t*>(&mailbox[selected]);
			words[(address-PioneerQwen35::DataflowWorkerMailboxAddress)/4]=value;
			if (serviceImage[selected]
				&& address==PioneerQwen35::DataflowWorkerMailboxAddress
					+offsetof(PioneerQwen35::DataflowWorkerMailbox,shutdownSequence)
				&& value==PioneerQwen35::DataflowWorkerShutdownRequest) {
				mailbox[selected].state=PioneerQwen35::DataflowWorkerStopped;
				mailbox[selected].failure=0;
				mailbox[selected].phase=0;
			}
		} else if (address>=PioneerQwen35::DataflowQueueStateAddress
			&& address<PioneerQwen35::DataflowQueueStateAddress
				+PioneerQwen35::DataflowQueueStateBytes) {
			uint32_t* words=reinterpret_cast<uint32_t*>(queue[selected]);
			words[(address-PioneerQwen35::DataflowQueueStateAddress)/4]=value;
		} else panel[(address-DataflowPciL1ViewBase)/sizeof(uint32_t)]=value;
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
	assert(DataflowPciRunningWriteRange(DataflowProgramAddress,
		sizeof(DataflowCoreInvocation)));
	assert(DataflowPciRunningWriteRange(DataflowProgramAddress,
		sizeof(DataflowRmsInvocation)));
	assert(DataflowPciRunningWriteRange(
		DataflowWorkerMailboxAddress+offsetof(DataflowWorkerMailbox,commandSequence),4));
	assert(DataflowPciRunningWriteRange(
		DataflowWorkerMailboxAddress+offsetof(DataflowWorkerMailbox,shutdownSequence),4));
	assert(!DataflowPciRunningWriteRange(
		DataflowWorkerMailboxAddress+offsetof(DataflowWorkerMailbox,state),4));

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
	assert(raw.reset[3]==0x46800);
	const unsigned commandAddress=DataflowWorkerMailboxAddress
		+offsetof(DataflowWorkerMailbox,commandSequence);
	raw.idleForWrite=false;
	assert(!memory.WriteCoreMemory(3,DataflowProgramAddress,&text,sizeof(text)));
	assert(!memory.WriteCoreMemory(3,commandAddress,&text,sizeof(text)));
	assert(!memory.WriteCoreMemory(3,DataflowWorkerMailboxAddress
		+offsetof(DataflowWorkerMailbox,state),&text,sizeof(text)));
	raw.idleForWrite=true;
	raw.reset[3]=0x87ffff;
	assert(!memory.WriteCoreMemory(3,DataflowProgramAddress,&text,sizeof(text)));
	raw.reset[3]=0x46800;
	assert(memory.WriteCoreMemory(3,DataflowProgramAddress,&text,sizeof(text)));
	assert(memory.WriteCoreMemory(3,commandAddress,&text,sizeof(text)));
	assert(memory.HoldCore(3));
	assert(raw.boot[3]==DataflowServiceBootWord);

	assert(memory.RestoreCoreState(3,snapshot));
	assert(memory.VerifyCoreStateRestored(3,snapshot));
	assert(raw.reset[3]==0x47800);
	assert(raw.boot[3]==PioneerQwen35::DataflowServiceBootWord);
	for (unsigned i=0;i<DataflowPciL1ViewWords;i++)
		assert(raw.panel[i]==originalPanel[i]);

	memory.ReleaseCoreSnapshot(snapshot);
	assert(!snapshot.captured && !snapshot.localWords);
	assert(memory.CaptureCoreState(4));
	assert(memory.HoldCore(4));
	const uint32_t changed=0x01020304;
	assert(memory.WriteCoreMemory(4,DataflowServiceEntryAddress,&changed,sizeof(changed)));
	assert(memory.RestoreCoreState(4));
	assert(memory.VerifyCoreStateRestored(4));
	assert(!memory.ResumeRestoredFleet()); // incomplete 120-core snapshot set
	memory.ReleaseCoreSnapshot(4);
	for (unsigned core=0;core<PioneerQwen35::DataflowCoreCount;core++)
		raw.reset[core]=0x46800;
	raw.reset[0]=0x47800;
	for (unsigned core=0;core<PioneerQwen35::DataflowCoreCount;core++)
		assert(memory.CaptureCoreState(core));
	for (unsigned core=0;core<PioneerQwen35::DataflowCoreCount;core++)
		assert(memory.HoldCore(core));
	for (unsigned core=0;core<PioneerQwen35::DataflowCoreCount;core++)
		assert(memory.RestoreCoreState(core));
	for (unsigned core=0;core<PioneerQwen35::DataflowCoreCount;core++)
		assert(memory.VerifyCoreStateRestored(core));
	assert(memory.ResumeRestoredFleet());
	assert(raw.reset[0]==0x47800);
	for (unsigned core=1;core<PioneerQwen35::DataflowCoreCount;core++)
		assert(raw.reset[core]==0x46800);
	for (unsigned core=0;core<PioneerQwen35::DataflowCoreCount;core++)
		memory.ReleaseCoreSnapshot(core);

	for (unsigned core=0;core<PioneerQwen35::DataflowCoreCount;core++) {
		raw.reset[core]=0x46800;
		raw.serviceImage[core]=false;
		raw.mailbox[core]={};
		raw.mailbox[core].magic=DataflowWorkerMailboxMagic;
		raw.mailbox[core].version=DataflowWorkerMailboxVersion;
		raw.mailbox[core].state=DataflowWorkerReady;
	}
	raw.reset[0]=0x47800;
	raw.serviceImage[1]=true;
	raw.serviceImage[17]=true;
	raw.serviceImage[119]=true;
	for (unsigned core=0;core<PioneerQwen35::DataflowCoreCount;core++)
		assert(memory.CaptureCoreState(core));
	assert(memory.StopAndAuditAll());
	for (unsigned core=0;core<PioneerQwen35::DataflowCoreCount;core++)
		assert(raw.reset[core]==0x47800);
	assert(raw.mailbox[1].state==DataflowWorkerStopped);
	assert(raw.mailbox[17].state==DataflowWorkerStopped);
	assert(raw.mailbox[119].state==DataflowWorkerStopped);
	for (unsigned core=0;core<PioneerQwen35::DataflowCoreCount;core++)
		assert(memory.RestoreCoreState(core));
	for (unsigned core=0;core<PioneerQwen35::DataflowCoreCount;core++)
		assert(memory.VerifyCoreStateRestored(core));
	for (unsigned core=0;core<PioneerQwen35::DataflowCoreCount;core++)
		raw.serviceImage[core]=false;
	assert(memory.ResumeRestoredFleet());
	assert(raw.reset[0]==0x47800);
	for (unsigned core=1;core<PioneerQwen35::DataflowCoreCount;core++)
		assert(raw.reset[core]==0x46800);
	for (unsigned core=0;core<PioneerQwen35::DataflowCoreCount;core++)
		memory.ReleaseCoreSnapshot(core);
	puts("PASS PCI L1 bounds, resident stop, and 120-core clean restore");
	return 0;
}
