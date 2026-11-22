/* SPDX-License-Identifier: MIT */
#include <cassert>
#include <cstdint>
#include <cstdio>
#include "../PioneerQwen35DataflowNocProtocol.h"
#include "../../../src/add-ons/kernel/drivers/misc/BlackholeResidentReadProbe.h"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerComputeFleet.h"

using namespace PioneerQwen35;

int main()
{
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
