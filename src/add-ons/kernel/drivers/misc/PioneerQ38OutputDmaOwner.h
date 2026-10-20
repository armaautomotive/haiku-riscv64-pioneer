/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_OUTPUT_DMA_OWNER_H
#define PIONEER_Q38_OUTPUT_DMA_OWNER_H
#include "PioneerWideOutput.h"
#include "BlackholeHostReadProbe.h"
// KERNEL-PRIVATE prerequisite. No public physical address or launch API.
// The caller separately proves all cohort roles/sources/completion and
// retains the ENTIRE context/cookie/ALL host owners on ANY uncertainty.
// This helper NEVER resets/restores/releases/cleans up or retries a write.
template<class IO> bool PioneerQ38OutputDmaOwner(IO& io,const tt_host_page_result& input,
	const PioneerOutputBufferLease& bulk,const PioneerWideOutputLease& wide,bool pending)
{
	return input.state==TT_HOST_PAGE_READY && input.memory_type_set && !input.released
		&& bulk.allocated && bulk.ready && bulk.memory_type_set && !bulk.pending && !bulk.released && !bulk.quarantined
		&& wide.allocated && wide.ready && wide.memory_type_set && !wide.released && !wide.quarantined && wide.pending==pending
		&& BlackholeHostReadMapStable(io,input) && PioneerOutputBufferMapStable(io,bulk,input)
		&& PioneerWideOutputMapStable(io,wide,input,bulk);
}
template<class IO> bool PioneerQ38OutputDmaEnable(IO& io,PioneerWideOutputLease& wide)
{
	if (!wide.ready || wide.pending || wide.quarantined || wide.released) return false;
	wide.pending=true; // BEFORE the first uncertain BME write; never clear on failure
	io.HostPciWrite(1,4,2,0x406); if (io.HostPciRead(1,4,2)!=0x406) return false;
	io.HostPciWrite(0,4,2,0x406); return io.HostPciRead(0,4,2)==0x406;
}
template<class IO> bool PioneerQ38OutputDmaDisableKnownComplete(IO& io)
{
	// ONLY after independent CURRENT physical/source/counter/command/nonce
	// and output-transfer completion proofs. NOT an error cleanup path.
	io.HostPciWrite(0,4,2,0x402); if (io.HostPciRead(0,4,2)!=0x402) return false;
	io.HostPciWrite(1,4,2,0x402); return io.HostPciRead(1,4,2)==0x402;
}
#endif
