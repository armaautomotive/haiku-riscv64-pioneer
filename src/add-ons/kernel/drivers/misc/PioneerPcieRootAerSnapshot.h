/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_PCIE_ROOT_AER_SNAPSHOT_H
#define PIONEER_PCIE_ROOT_AER_SNAPSHOT_H

#include <stdint.h>
#include "PioneerRecoveryProtocol.h"

template<class IO>
bool PioneerPcieRootFindAer(IO& io,uint32_t& result)
{
	result=0;
	if (io.RootRead(0,4)!=0x20421e30) return false;
	uint32_t visited[32]={};
	uint32_t cap=0x100;
	for (unsigned hop=0;cap && hop<256;hop++) {
		if (cap<0x100 || cap>0xffc || (cap&3)) return false;
		const unsigned bit=cap/4;
		if (visited[bit/32] & (1U<<(bit%32))) return false;
		visited[bit/32] |= 1U<<(bit%32);
		const uint32_t header=io.RootRead(cap,4);
		if (!header || header==UINT32_MAX) return false;
		if ((header&0xffff)==1) {
			if (cap>0xfc8 || !((header>>16)&15)) return false;
			result=cap;return true;
		}
		cap=(header>>20)&0xfff;
	}
	return false;
}

// Read-only root-port AER evidence. Never acknowledges or clears errors.
// IO is bound to the pinned root port by the caller, not userspace.
template<class IO>
bool PioneerPcieRootAerSnapshot(IO& io,uint32_t& uncorrectable,uint32_t& source)
{
	uncorrectable=source=UINT32_MAX;
	uint32_t cap=0;
	if (!PioneerPcieRootFindAer(io,cap)) return false;
	const uint32_t errors=io.RootRead(cap+4,4);
	const uint32_t requester=io.RootRead(cap+0x34,4);
	if (errors==UINT32_MAX || requester==UINT32_MAX) return false;
	uncorrectable=errors;source=requester;return true;
}

// Fixed root-port registers only. A partial read is marked unavailable and
// never treated as proof that the link, DMA, or errors are safe.
template<class IO>
bool PioneerPcieRootTraceSample(IO& io,tt_pioneer_pcie_trace_sample& s)
{
	s={};
	if (io.RootRead(0,4)!=0x20421e30) return false;
	uint32_t cap=io.RootRead(0x34,1);uint64_t visited=0;bool found=false;
	for (unsigned hop=0;cap && hop<48;hop++) {
		if (cap<0x40 || cap>0xec || (cap&3) || (visited&(UINT64_C(1)<<(cap/4)))) return false;
		visited|=UINT64_C(1)<<(cap/4);
		const uint32_t header=io.RootRead(cap,2);
		if ((header&0xff)==0x10) {
			const uint32_t flags=io.RootRead(cap+2,2);
			if (found || ((flags&15)!=1 && (flags&15)!=2) || ((flags>>4)&15)!=4) return false;
			s.device_status=io.RootRead(cap+0xa,2);s.link_status=io.RootRead(cap+0x12,2);
			if (s.device_status>0xffff || s.link_status>0xffff) return false;
			found=true;
		}
		cap=(header>>8)&0xff;
	}
	if (cap || !found) return false;
	uint32_t aer=0;if (!PioneerPcieRootFindAer(io,aer)) return false;
	s.uncorrectable=io.RootRead(aer+4,4);s.uncorrectable_mask=io.RootRead(aer+8,4);
	s.severity=io.RootRead(aer+0xc,4);s.root_status=io.RootRead(aer+0x30,4);
	s.source=io.RootRead(aer+0x34,4);
	for (unsigned i=0;i<4;i++) s.header[i]=io.RootRead(aer+0x1c+4*i,4);
	if (s.uncorrectable==UINT32_MAX || s.uncorrectable_mask==UINT32_MAX
		|| s.severity==UINT32_MAX || s.root_status==UINT32_MAX || s.source==UINT32_MAX) return false;
	for (auto word:s.header) if (word==UINT32_MAX) return false;
	s.valid=1;return true;
}
#endif
