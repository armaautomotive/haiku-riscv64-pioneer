/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_QWEN35_DEDICATED_PEER_LEASE_H
#define PIONEER_QWEN35_DEDICATED_PEER_LEASE_H
#include <stdint.h>

// Held-fleet route preparation for the dedicated service. No worker L1 or
// firmware image is read or saved. A failed attempt is quarantined by the
// enclosing fleet owner, including when an MMIO write may have landed.
template<class IO,class Dram,class Node>
bool PioneerQwen35DedicatedPeerLeaseBegin(IO& io,const Dram& dram,unsigned worker,Node& node)
{
	auto& store=node.Store(); const auto& noc=node.noc;
	if (!worker || worker>=120 || store.controls_saved || store.saved || store.configured
		|| store.pending || store.ready || store.recovery_required || store.l1_dirty
		|| noc.version!=PioneerWorkerVersion(worker) || noc.state!=TT_COMPUTE_NOC_PASSED
		|| !noc.tlb_restored || noc.recovery_required
		|| !PioneerWorkerRoute(dram,noc.source[0],worker) || !BlackholeHostSourceRoute(noc.source[0])
		|| !io.SelectRuntimeCore(worker)) return false;
	for (unsigned s=0;s<2;s++) for (unsigned w=0;w<3;w++) store.tlb[s][w]=io.DramTlbRead(s,w);
	if ((store.tlb[0][2]&0xffffc000) || (store.tlb[1][2]&0xffffc000)) return false;
	store.controls_saved=true;store.tlb_restored=false;store.config=noc.source[0][0];
	if (io.ResetRead()!=0x47800 || io.ResetRead()!=0x47800 || !BlackholeHostReadPciQuiet(io,0x402)
		|| !BlackholeResidentSourceStable(io,noc,store.config) || !BlackholeResidentIdle(io)
		|| (io.ResidentNocRead(0x178)&0xffff)) return false;
	for (unsigned w=0;w<10;w++) if (io.ResidentNocRead(w*4)) return false;
	store.configured=true;store.niu_restored=false;
	io.ResidentNocWrite(0x100,store.config|0x10000);
	if (!BlackholeResidentSourceStable(io,noc,store.config|0x10000)) {
		store.recovery_required=true;return false;
	}
	store.ready=true;return true;
}

// Only before service installation: no worker L1 has been used by this
// lease. The caller audits the entire held fleet before starting restoration.
template<class IO,class Noc,class Store>
bool PioneerQwen35DedicatedPeerLeaseEnd(IO& io,const Noc& noc,Store& store)
{
	if (!store.ready || !store.configured || store.saved || !store.controls_saved
		|| store.pending || store.recovery_required || store.l1_dirty
		|| io.ResetRead()!=0x47800 || io.ResetRead()!=0x47800
		|| !BlackholeResidentSourceStable(io,noc,store.config|0x10000)
		|| !BlackholeResidentIdle(io) || !BlackholeHostReadPciQuiet(io,0x402)) return false;
	for (unsigned w=0;w<10;w++) if (io.ResidentNocRead(w*4)) return false;
	io.ResidentNocWrite(0x100,store.config);
	if (!BlackholeResidentSourceStable(io,noc,store.config)) return false;
	for (unsigned w=0;w<10;w++) if (io.ResidentNocRead(w*4)) return false;
	store.niu_restored=true;
	for (unsigned s=0;s<2;s++) for (unsigned w=0;w<3;w++) io.DramTlbWrite(s,w,store.tlb[s][w]);
	for (unsigned s=0;s<2;s++) for (unsigned w=0;w<3;w++)
		if (io.DramTlbRead(s,w)!=store.tlb[s][w]) return false;
	store.tlb_restored=true;store.ready=false;return true;
}
#endif
