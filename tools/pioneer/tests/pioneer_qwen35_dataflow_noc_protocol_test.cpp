/* SPDX-License-Identifier: MIT */
#include <cassert>
#include <cstdint>
#include <cstdio>
#include "../PioneerQwen35DataflowNocProtocol.h"
#include "../../../src/add-ons/kernel/drivers/misc/BlackholeResidentReadProbe.h"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerComputeFleet.h"

using namespace PioneerQwen35;

struct FifoRegisters {
	uint32_t config=0,available=0x10101010U,overflow=0;
	unsigned configReads=0,availableReads=0,overflowReads=0;
	bool flipConfig=false;
	uint32_t Read(unsigned offset) {
		if (offset==0x100) {
			++configReads;
			return flipConfig && configReads==2 ? config^(1U<<16) : config;
		}
		if (offset==0x64) { ++availableReads; return available; }
		if (offset==0x68) { ++overflowReads; return overflow; }
		assert(false); return UINT32_MAX;
	}
};

int main()
{
	{
		FifoRegisters regs; uint32_t observed=0;
		auto sample=[&]() { return DataflowNocCheckFifo([&](unsigned offset) {
			return regs.Read(offset);
		},observed); };
		assert(sample()==DataflowNocFifoReady && regs.configReads==2
			&& !regs.availableReads && !regs.overflowReads);
		regs.config=1U<<16;
		assert(sample()==DataflowNocFifoReady && regs.availableReads==1 && regs.overflowReads==1);
		for (unsigned initiator=0;initiator<4;initiator++) {
			regs.available=0x10101010U-(1U<<(initiator*8));
			const unsigned priorOverflowReads=regs.overflowReads;
			assert(sample()==DataflowNocFifoNotEmpty && observed==regs.available
				&& regs.overflowReads==priorOverflowReads);
		}
		regs.available=0x10101010U; regs.overflow=4;
		assert(sample()==DataflowNocFifoOverflow && observed==4);
		regs.config=1U<<12;
		assert(sample()==DataflowNocFifoBadConfig && observed==regs.config);
		regs.config=UINT32_MAX;
		assert(sample()==DataflowNocFifoBadConfig);
		regs.config=0; regs.configReads=0; regs.flipConfig=true;
		assert(sample()==DataflowNocFifoBadConfig && observed==(1U<<16));
	}
	{
		FifoRegisters regs; regs.config=1U<<16;
		uint32_t expected[8]={}; unsigned counterReads=0;
		auto fifo=[&](unsigned offset) { return regs.Read(offset); };
		auto counter=[&](unsigned i) { assert(i<8 && i!=1); ++counterReads; return 0U; };
		regs.available=0x1010100fU;
		assert(!DataflowNocFifoCountersComplete(fifo,counter,expected) && !counterReads);
		regs.available=0x10101010U; regs.overflow=1;
		assert(!DataflowNocFifoCountersComplete(fifo,counter,expected) && !counterReads);
		regs.overflow=0;
		assert(DataflowNocFifoCountersComplete(fifo,counter,expected) && counterReads==7);
		expected[2]=1;
		assert(!DataflowNocFifoCountersComplete(fifo,counter,expected) && counterReads==14);
	}
	// A stale return-coordinate register may contain a plausible but unrelated
	// core. The worker and driver must both read the logical-ID register.
	uint32_t regs[0x150/4]={};
	regs[0x14/4]=0x312;
	regs[0x148/4]=0x081;
	uint32_t self=0;
	assert(DataflowNocDecodeLocalRoute(regs[DataflowNocLogicalIdOffset/4],self));
	assert(self==0x081 && self!=regs[0x14/4]);
	assert(BlackholeResidentReadRegister(DataflowNocLogicalIdOffset));
	assert(DataflowNocRouteMatchesWorker(self,0));
	assert(!DataflowNocRouteMatchesWorker(self,1) && !DataflowNocRouteMatchesWorker(self,120));
	for (unsigned core=0;core<120;core++) {
		const uint32_t expected=PioneerWorkerX(core)|(PioneerWorkerY(core)<<6);
		assert(DataflowNocRouteMatchesWorker(expected,core));
		assert(!DataflowNocRouteMatchesWorker(expected^(1U<<6),core));
	}
	assert(DataflowNocDecodeLocalRoute(0xabcd0081,self) && self==0x81);
	assert(!DataflowNocDecodeLocalRoute(0,self) && !DataflowNocDecodeLocalRoute(0xffffffff,self)
		&& !DataflowNocDecodeLocalRoute(0x1000,self));
	self=0x81;
	uint32_t command[10]={};
	DataflowNocBuildCommand(false,0x04185380,0x312,0x90000,self,3392,command);
	const uint32_t readExpected[]={0x04185380,0,0x312,0x90000,0,0x081,0,0x2090,3392,0};
	for (unsigned i=0;i<10;i++) assert(command[i]==readExpected[i]);
	DataflowNocBuildCommand(true,0x04185380,0x312,0x90000,self,64,command);
	const uint32_t writeExpected[]={0x90000,0,0x081,0x04185380,0,0x312,0,0x2092,64,0};
	for (unsigned i=0;i<10;i++) assert(command[i]==writeExpected[i]);
	// The NIU may change only reserved control bits after submission. Such a
	// change is not a lost command; a changed VC or request type still is.
	assert(DataflowNocCommandWordMatches(7,0x2090,0x2090|0x07fc1c00));
	assert(!DataflowNocCommandWordMatches(7,0x2090,0x2090^0x2000));
	assert(!DataflowNocCommandWordMatches(7,0x2090,0x2090^2));
	assert(!DataflowNocCommandWordMatches(5,0x81,0x481));
	// 0x20c is unsupported on Blackhole. Neither the baseline nor any poll
	// may read it, while all supported counters still gate completion.
	for (unsigned mode=0;mode<2;mode++) {
		const bool write=mode!=0;
		uint32_t observed[8]={0xffffffffU,0xdeadbeefU,3,4,5,6,3,8};
		unsigned reads=0;
		auto read=[&](unsigned i) { assert(i<8 && i!=1);reads++;return observed[i]; };
		uint32_t expected[8]{};
		DataflowNocCaptureExpected(read,write,2048,expected);
		assert(reads==7 && expected[1]==0);
		for (unsigned i=0;i<8;i++) if (DataflowNocCounterSupported(i))
			observed[i]+=DataflowNocCounterDelta(write,2048,i);
		unsigned mismatch=0;uint32_t value=0;
		assert(DataflowNocMatchingCounters(read,expected,&mismatch,&value)==DataflowNocSupportedCounterMask
			&& reads==14 && mismatch==8);
		observed[3]++;
		assert(DataflowNocMatchingCounters(read,expected,&mismatch,&value)
			==(DataflowNocSupportedCounterMask & ~(1U<<3))
			&& mismatch==3 && value==observed[3]);
	}
	std::puts("PASS dataflow NoC logical return route and marked static-VC1 read/write commands; NO HARDWARE");
}
