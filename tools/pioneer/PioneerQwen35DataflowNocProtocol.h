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

enum DataflowNocFifoStatus : unsigned {
	DataflowNocFifoReady=0, DataflowNocFifoBadConfig=1,
	DataflowNocFifoNotEmpty=2, DataflowNocFifoOverflow=3
};
// For FIFO mode, test availability before reading any request counters: the
// NIU counts a request only after it leaves its FIFO. Callers must ensure the
// NIU configuration cannot be changed concurrently by another agent.
// https://github.com/tenstorrent/tt-isa-documentation/blob/main/BlackholeA0/NoC/MemoryMap.md#niu-request-fifos
template<class Read>
inline DataflowNocFifoStatus DataflowNocCheckFifo(Read read,uint32_t& observed)
{
	constexpr uint32_t clockDisabled=1U<<12,fifoEnabled=1U<<16;
	const uint32_t before=read(0x100);
	if (before==UINT32_MAX || (before&clockDisabled)) {
		observed=before; return DataflowNocFifoBadConfig;
	}
	if (before&fifoEnabled) {
		observed=read(0x64);
		if (observed!=0x10101010U) return DataflowNocFifoNotEmpty;
		observed=read(0x68);
		if (observed) return DataflowNocFifoOverflow;
	}
	const uint32_t after=read(0x100);
	if (after==UINT32_MAX || ((before^after)&(clockDisabled|fifoEnabled))
		|| (after&clockDisabled)) {
		observed=after; return DataflowNocFifoBadConfig;
	}
	observed=after; return DataflowNocFifoReady;
}

// Blackhole documents NIU_MST_RD_DATA_WORD_RECEIVED (0x20c) as
// UnsupportedFunctionality when read. Keep its historical diagnostic slot,
// but never access it or use it as a completion condition.
// https://github.com/tenstorrent/tt-isa-documentation/blob/main/BlackholeA0/NoC/Counters.md
constexpr uint32_t DataflowNocSupportedCounterMask=0xfdU;
inline bool DataflowNocCounterSupported(unsigned index) {
	return index<8 && (DataflowNocSupportedCounterMask & (1U<<index));
}
inline unsigned DataflowNocCounterDelta(bool write,unsigned bytes,unsigned index) {
	const unsigned flits=bytes/64;
	const unsigned delta[8]={write?0U:1U,0,1,write?0U:1U,
		write?1U:0U,write?flits:0U,1,write?1U:0U};
	return index<8?delta[index]:0;
}
template<class Read> inline void DataflowNocCaptureExpected(Read& read,bool write,
	unsigned bytes,uint32_t (&expected)[8]) {
	for (unsigned i=0;i<8;i++)
		expected[i]=DataflowNocCounterSupported(i)?read(i)+DataflowNocCounterDelta(write,bytes,i):0;
}
template<class Read> inline uint32_t DataflowNocMatchingCounters(Read& read,
	const uint32_t (&expected)[8],unsigned* firstMismatch=nullptr,uint32_t* mismatchValue=nullptr) {
	uint32_t mask=0;unsigned first=8;uint32_t value=0;
	for (unsigned i=0;i<8;i++) if (DataflowNocCounterSupported(i)) {
		const uint32_t observed=read(i);
		if (observed==expected[i]) mask|=1U<<i;
		else if (first==8) { first=i;value=observed; }
	}
	if (firstMismatch) *firstMismatch=first;
	if (mismatchValue) *mismatchValue=value;
	return mask;
}

// A queued FIFO entry is not reflected in the counters. Do not read a
// completion counter until the availability value has returned and passed.
template<class ReadFifo,class ReadCounter>
inline bool DataflowNocFifoCountersComplete(ReadFifo readFifo,ReadCounter& readCounter,
	const uint32_t (&expected)[8]) {
	uint32_t observed=0;
	return DataflowNocCheckFifo(readFifo,observed)==DataflowNocFifoReady
		&& DataflowNocMatchingCounters(readCounter,expected)==DataflowNocSupportedCounterMask;
}

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
