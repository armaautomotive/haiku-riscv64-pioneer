/* SPDX-License-Identifier: MIT */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <vector>
#include "../../../src/add-ons/kernel/drivers/misc/BlackholeAsicReset.h"

struct Write {
	uint16_t offset;
	uint8_t size;
	uint32_t value;
};

struct FakePci {
	uint32_t vendor=0xb1401e52;
	uint32_t rootVendor=0x20421e30;
	uint32_t rootClass=0x06040000;
	uint32_t command=0x0402;
	uint32_t bridgeCommand=0x0402;
	unsigned clearMarkerAfter=0;
	unsigned delays=0;
	std::vector<Write> writes;
	uint32_t CardRead(uint16_t offset,uint8_t size)
	{
		if (offset==0 && size==4) return vendor;
		if (offset==4 && size==2) return command;
		return 0;
	}
	uint32_t RootRead(uint16_t offset,uint8_t size)
	{
		if (offset==0 && size==4) return rootVendor;
		if (offset==8 && size==4) return rootClass;
		if (offset==4 && size==2) return bridgeCommand;
		return 0;
	}
	void CardWrite(uint16_t offset,uint8_t size,uint32_t value)
	{
		writes.push_back({offset,size,value});
		if (offset==4 && size==2) command=value;
	}
	void RecoveryDelay()
	{
		delays++;
		if (clearMarkerAfter && delays>=clearMarkerAfter) command&=~0x0040u;
	}
};

static void TestOneResetSequence()
{
	FakePci pci;
	pci.clearMarkerAfter=3;
	BlackholeAsicResetResult result{};
	assert(BlackholeAsicReset(pci,8,result));
	assert(result.stage==BLACKHOLE_ASIC_RESET_COMPLETE && result.polls==3);
	assert(result.command_before==0x0402 && result.marker_after==0x0402);
	assert(result.trigger_target==1 && result.trigger_control==0x11);
	assert(pci.writes.size()==3);
	assert(pci.writes[0].offset==4 && pci.writes[0].size==2 && pci.writes[0].value==0x0442);
	assert(pci.writes[1].offset==0x934 && pci.writes[1].size==4 && pci.writes[1].value==1);
	assert(pci.writes[2].offset==0x930 && pci.writes[2].size==4 && pci.writes[2].value==0x11);
}

static void TestFailClosedPreflight()
{
	BlackholeAsicResetResult result{};
	FakePci pci;pci.command|=0x0004;
	assert(!BlackholeAsicReset(pci,8,result) && pci.writes.empty());
	pci=FakePci{};pci.bridgeCommand|=0x0004;
	assert(!BlackholeAsicReset(pci,8,result) && pci.writes.empty());
	pci=FakePci{};pci.vendor=0xffffffff;
	assert(!BlackholeAsicReset(pci,8,result) && pci.writes.empty());
	pci=FakePci{};pci.command|=0x0040;
	assert(!BlackholeAsicReset(pci,8,result) && pci.writes.empty());
	pci=FakePci{};
	assert(!BlackholeAsicReset(pci,0,result) && pci.writes.empty());
}

static void TestTimeoutDoesNotRetryOrClearMarker()
{
	FakePci pci;
	BlackholeAsicResetResult result{};
	assert(!BlackholeAsicReset(pci,4,result));
	assert(result.stage==BLACKHOLE_ASIC_RESET_TIMEOUT && result.polls==4);
	assert(result.marker_after==0x0442 && pci.command==0x0442);
	assert(pci.delays==4 && pci.writes.size()==3);
}

int main()
{
	TestOneResetSequence();
	TestFailClosedPreflight();
	TestTimeoutDoesNotRetryOrClearMarker();
	puts("PASS Blackhole ASIC reset: exact three-write order, bus-master and identity gates, bounded marker wait, no retry or marker clear;NO PCI/MMIO/reset/native qualification");
}
