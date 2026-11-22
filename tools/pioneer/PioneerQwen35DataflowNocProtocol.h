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
// NOC_CTRL bits 10..12 and 18..26 are reserved and may be changed by the NIU
// after request submission. Preserve checks of all software-defined fields.
constexpr uint32_t DataflowNocControlReadbackMask=0xf803e3ff;

inline bool DataflowNocDecodeLocalRoute(uint32_t raw,uint32_t& route)
{
	if (raw==UINT32_MAX) return false;
	route=raw&0xfff;
	return route!=0;
}

inline bool DataflowNocRouteMatchesWorker(uint32_t route,unsigned worker)
{
	const unsigned columns[]={1,2,3,4,5,6,7,10,11,12,13,14};
	return worker<120 && route==(columns[worker/10]|((2+worker%10)<<6));
}

inline void DataflowNocBuildCommand(bool write,uint32_t remote,uint32_t route,
	uint32_t local,uint32_t self,unsigned bytes,uint32_t (&command)[10])
{
	command[0]=write?local:remote;command[1]=0;command[2]=write?self:route;
	command[3]=write?remote:local;command[4]=0;command[5]=write?route:self;
	command[6]=0;command[7]=write?DataflowNocWriteControl:DataflowNocReadControl;
	command[8]=bytes;command[9]=0;
}

inline bool DataflowNocCommandWordMatches(unsigned index,uint32_t expected,uint32_t observed)
{
	return index==7 ? ((expected^observed)&DataflowNocControlReadbackMask)==0
		: expected==observed;
}
}
#endif
