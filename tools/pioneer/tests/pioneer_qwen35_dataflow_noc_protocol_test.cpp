/* SPDX-License-Identifier: MIT */
#include <cassert>
#include <cstdint>
#include <cstdio>
#include "../PioneerQwen35DataflowNocProtocol.h"
#include "../../../src/add-ons/kernel/drivers/misc/BlackholeResidentReadProbe.h"

using namespace PioneerQwen35;

int main()
{
	// A stale return-coordinate register may contain a plausible but unrelated
	// core. The worker and driver must both read the logical-ID register.
	uint32_t regs[0x150/4]={};
	regs[0x14/4]=0x312;
	regs[0x148/4]=0x081;
	const uint32_t self=regs[DataflowNocLogicalIdOffset/4];
	assert(self==0x081 && self!=regs[0x14/4]);
	assert(BlackholeResidentReadRegister(DataflowNocLogicalIdOffset));
	assert(DataflowNocLocalRouteValid(self));
	assert(!DataflowNocLocalRouteValid(0) && !DataflowNocLocalRouteValid(0xffffffff)
		&& !DataflowNocLocalRouteValid(0x1000));
	uint32_t command[10]={};
	DataflowNocBuildCommand(false,0x04185380,0x312,0x90000,self,3392,command);
	const uint32_t readExpected[]={0x04185380,0,0x312,0x90000,0,0x081,0,0x2090,3392,0};
	for (unsigned i=0;i<10;i++) assert(command[i]==readExpected[i]);
	DataflowNocBuildCommand(true,0x04185380,0x312,0x90000,self,64,command);
	const uint32_t writeExpected[]={0x90000,0,0x081,0x04185380,0,0x312,0,0x2092,64,0};
	for (unsigned i=0;i<10;i++) assert(command[i]==writeExpected[i]);
	std::puts("PASS dataflow NoC logical return route and marked static-VC1 read/write commands; NO HARDWARE");
}
