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
	unsigned delays=0,readyCalls=0,readyAfter=0,failReadyAt=0,failPollReadyAt=0,pollReadyCalls=0;
	unsigned cardReads=0,cardReadsWhileDown=0,markerReadsAtFirstPoll=0;
	bool rootReady=true;
	std::vector<Write> writes;
	bool RootReady() {
		++readyCalls;
		if (delays) ++pollReadyCalls;
		return rootReady && readyCalls!=failReadyAt && (!readyAfter || !delays || delays>=readyAfter)
			&& (!delays || pollReadyCalls!=failPollReadyAt);
	}
	uint32_t CardRead(uint16_t offset,uint8_t size)
	{
		++cardReads;
		if (!rootReady || (readyAfter && delays && delays<readyAfter)) ++cardReadsWhileDown;
		if (offset==4 && size==2 && delays==1) ++markerReadsAtFirstPoll;
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
	pci.clearMarkerAfter=6;
	assert(BlackholeAsicResetResumeWait(pci,4,result));
	assert(result.stage==BLACKHOLE_ASIC_RESET_COMPLETE && result.polls==6);
	assert(pci.delays==6 && pci.writes.size()==3);
}

static void TestResumeRefusesInvalidOrUnsafeEvidence()
{
	BlackholeAsicResetResult result{};FakePci pci;
	assert(!BlackholeAsicResetResumeWait(pci,4,result) && pci.writes.empty());
	assert(!BlackholeAsicReset(pci,1,result) && result.stage==BLACKHOLE_ASIC_RESET_TIMEOUT);
	auto invalid=result;invalid.trigger_control=0;
	assert(!BlackholeAsicResetResumeWait(pci,4,invalid) && pci.writes.size()==3);
	pci.command&=~0x0040u;pci.bridgeCommand|=0x0004u;
	assert(!BlackholeAsicResetResumeWait(pci,1,result) && result.stage==BLACKHOLE_ASIC_RESET_TIMEOUT);
	pci.bridgeCommand&=~0x0004u;pci.rootVendor=0xffffffff;
	assert(!BlackholeAsicResetResumeWait(pci,1,result) && result.stage==BLACKHOLE_ASIC_RESET_TIMEOUT);
	pci.rootVendor=0x20421e30;
	assert(BlackholeAsicResetResumeWait(pci,1,result) && pci.writes.size()==3);
}

static void TestRootReadinessGatesEndpointReads()
{
	BlackholeAsicResetResult result{};FakePci pci;
	pci.rootReady=false;
	assert(!BlackholeAsicReset(pci,4,result) && pci.cardReads==0 && pci.writes.empty());
	// Losing root readiness before the marker, after it, or after the target
	// must stop immediately; no failed setup is automatically replayed.
	pci=FakePci{};pci.failReadyAt=3;
	assert(!BlackholeAsicReset(pci,2,result) && pci.writes.empty());
	pci=FakePci{};pci.failReadyAt=4;
	assert(!BlackholeAsicReset(pci,2,result) && pci.writes.size()==1
		&& result.stage==BLACKHOLE_ASIC_RESET_MARKER && !BlackholeAsicResetResumable(result));
	pci=FakePci{};pci.failReadyAt=6;
	assert(!BlackholeAsicReset(pci,2,result) && pci.writes.size()==2
		&& result.trigger_target==1 && !result.trigger_control && !BlackholeAsicResetResumable(result));
	pci=FakePci{};pci.readyAfter=3;pci.clearMarkerAfter=3;
	assert(BlackholeAsicReset(pci,5,result));
	assert(result.polls==3 && pci.cardReadsWhileDown==0 && pci.writes.size()==3);
	pci=FakePci{};pci.clearMarkerAfter=1;pci.failPollReadyAt=2;
	assert(BlackholeAsicReset(pci,3,result));
	assert(result.polls==2 && pci.markerReadsAtFirstPoll==0 && pci.writes.size()==3);
	pci=FakePci{};pci.clearMarkerAfter=1;pci.failPollReadyAt=3;
	assert(BlackholeAsicReset(pci,3,result));
	assert(result.polls==2 && pci.writes.size()==3);
	pci=FakePci{};pci.readyAfter=10;
	assert(!BlackholeAsicReset(pci,2,result));
	assert(result.stage==BLACKHOLE_ASIC_RESET_TIMEOUT && result.marker_after==UINT32_MAX
		&& pci.cardReadsWhileDown==0 && pci.writes.size()==3);
	pci.readyAfter=0;pci.clearMarkerAfter=3;
	assert(BlackholeAsicResetResumeWait(pci,2,result) && pci.writes.size()==3);
}

int main()
{
	TestOneResetSequence();
	TestFailClosedPreflight();
	TestTimeoutDoesNotRetryOrClearMarker();
	TestResumeRefusesInvalidOrUnsafeEvidence();
	TestRootReadinessGatesEndpointReads();
	puts("PASS Blackhole ASIC reset: exact three-write order, bus-master and identity gates, bounded marker wait, no retry or marker clear;NO PCI/MMIO/reset/native qualification");
}
