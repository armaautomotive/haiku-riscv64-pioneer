/* SPDX-License-Identifier: MIT */
#ifndef BLACKHOLE_ASIC_RESET_H
#define BLACKHOLE_ASIC_RESET_H

#include <stdint.h>

enum : uint32_t {
	BLACKHOLE_ASIC_RESET_IDLE=0,
	BLACKHOLE_ASIC_RESET_PREFLIGHT=1,
	BLACKHOLE_ASIC_RESET_MARKER=2,
	BLACKHOLE_ASIC_RESET_TRIGGER=3,
	BLACKHOLE_ASIC_RESET_WAIT=4,
	BLACKHOLE_ASIC_RESET_COMPLETE=5,
	BLACKHOLE_ASIC_RESET_TIMEOUT=6
};

struct BlackholeAsicResetResult {
	uint32_t stage=BLACKHOLE_ASIC_RESET_IDLE;
	uint32_t polls=0;
	uint32_t command_before=0;
	uint32_t marker_after=0;
	uint32_t trigger_target=0;
	uint32_t trigger_control=0;
};

inline bool BlackholeAsicResetResumable(const BlackholeAsicResetResult& result)
{
	return (result.stage==BLACKHOLE_ASIC_RESET_WAIT || result.stage==BLACKHOLE_ASIC_RESET_TIMEOUT
			|| result.stage==BLACKHOLE_ASIC_RESET_COMPLETE)
		&& !(result.command_before&0x0044) && result.command_before<=0xffff
		&& result.trigger_target==1 && result.trigger_control==0x11;
}

// Poll only a previously issued reset. The caller must retain the exact
// kernel-owned result and an independently validated recovery ticket; this
// routine neither proves that ownership nor writes or clears the marker.
// A timeout can be polled again without issuing another reset.
template<class IO>
bool
BlackholeAsicResetResumeWait(IO& io,unsigned pollLimit,BlackholeAsicResetResult& result)
{
	if (!pollLimit || !BlackholeAsicResetResumable(result)) return false;
	if (result.stage==BLACKHOLE_ASIC_RESET_COMPLETE) return true;
	result.stage=BLACKHOLE_ASIC_RESET_WAIT;
	for (unsigned poll=0;poll<pollLimit;poll++) {
		io.RecoveryDelay();
		const uint32_t vendor=io.CardRead(0,4);
		const uint32_t marker=io.CardRead(4,2);
		const uint32_t rootVendor=io.RootRead(0,4);
		const uint32_t rootClass=io.RootRead(8,4);
		const uint32_t bridgeCommand=io.RootRead(4,2);
		if (result.polls!=UINT32_MAX) ++result.polls;
		result.marker_after=marker;
		if (vendor==0xb1401e52 && rootVendor==0x20421e30
			&& (rootClass>>8)==0x060400 && marker!=UINT32_MAX
			&& bridgeCommand!=UINT32_MAX && !(marker&0x0044) && !(bridgeCommand&0x0004)) {
			result.stage=BLACKHOLE_ASIC_RESET_COMPLETE;
			return true;
		}
	}
	result.stage=BLACKHOLE_ASIC_RESET_TIMEOUT;
	return false;
}

// Blackhole ASIC reset follows the three PCI config writes used by the
// Tenstorrent kernel driver: set PCI_COMMAND_PARITY as a reset marker, then
// program the interface timer target and force its interrupt. Callers must
// serialize the operation and keep bus mastering disabled. Never reissue a
// reset after an uncertain write; a completed trigger may only be polled.
template<class IO>
bool
BlackholeAsicReset(IO& io,unsigned pollLimit,BlackholeAsicResetResult& result)
{
	result={};
	result.stage=BLACKHOLE_ASIC_RESET_PREFLIGHT;
	if (!pollLimit || io.CardRead(0,4)!=0xb1401e52
		|| io.RootRead(0,4)!=0x20421e30
		|| (io.RootRead(8,4)>>8)!=0x060400)
		return false;
	const uint32_t command=io.CardRead(4,2);
	const uint32_t bridgeCommand=io.RootRead(4,2);
	result.command_before=command;
	// Bus mastering must already be disabled, and the parity bit must be clear
	// so this invocation cannot be mistaken for a completion of an old reset.
	if (command==UINT32_MAX || bridgeCommand==UINT32_MAX
		|| (command&0x0004) || (bridgeCommand&0x0004)
		|| (command&0x0040))
		return false;
	io.CardWrite(4,2,command|0x0040);
	result.stage=BLACKHOLE_ASIC_RESET_MARKER;
	if (io.CardRead(4,2)!=(command|0x0040)) return false;
	io.CardWrite(0x934,4,1);
	result.trigger_target=1;
	io.CardWrite(0x930,4,0x11);
	result.trigger_control=0x11;
	result.stage=BLACKHOLE_ASIC_RESET_WAIT;
	return BlackholeAsicResetResumeWait(io,pollLimit,result);
}

#endif
