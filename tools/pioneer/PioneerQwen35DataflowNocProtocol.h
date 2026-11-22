/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_QWEN35_DATAFLOW_NOC_PROTOCOL_H
#define PIONEER_QWEN35_DATAFLOW_NOC_PROTOCOL_H
#include <stdint.h>

namespace PioneerQwen35 {

// Blackhole NOC_CFG(NOC_ID_LOGICAL), not NOC_RET_ADDR_HI (0x14), which is
// mutable command-buffer state. The logical coordinate is {y[5:0],x[5:0]}.
constexpr uint32_t DataflowNocLogicalIdOffset=0x148;
constexpr uint32_t DataflowNocReadControl=0x2090;  // marked response, static VC1
constexpr uint32_t DataflowNocWriteControl=0x2092; // nonposted write, static VC1

inline bool DataflowNocLocalRouteValid(uint32_t route)
{
	return route!=0 && route<=0xfff;
}

inline void DataflowNocBuildCommand(bool write,uint32_t remote,uint32_t route,
	uint32_t local,uint32_t self,unsigned bytes,uint32_t (&command)[10])
{
	command[0]=write?local:remote;command[1]=0;command[2]=write?self:route;
	command[3]=write?remote:local;command[4]=0;command[5]=write?route:self;
	command[6]=0;command[7]=write?DataflowNocWriteControl:DataflowNocReadControl;
	command[8]=bytes;command[9]=0;
}
}
#endif
