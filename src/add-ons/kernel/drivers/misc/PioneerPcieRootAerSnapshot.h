/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_PCIE_ROOT_AER_SNAPSHOT_H
#define PIONEER_PCIE_ROOT_AER_SNAPSHOT_H

#include <stdint.h>

// Read-only root-port AER evidence. Never acknowledges or clears errors.
// IO is bound to the pinned root port by the caller, not userspace.
template<class IO>
bool PioneerPcieRootAerSnapshot(IO& io,uint32_t& uncorrectable,uint32_t& source)
{
	uncorrectable=source=UINT32_MAX;
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
			const uint32_t errors=io.RootRead(cap+4,4);
			const uint32_t requester=io.RootRead(cap+0x34,4);
			if (errors==UINT32_MAX || requester==UINT32_MAX) return false;
			uncorrectable=errors;source=requester;return true;
		}
		cap=(header>>20)&0xfff;
	}
	return false;
}
#endif
