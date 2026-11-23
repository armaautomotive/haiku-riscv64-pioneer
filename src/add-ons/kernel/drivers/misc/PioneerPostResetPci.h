/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_POST_RESET_PCI_H
#define PIONEER_POST_RESET_PCI_H

#include "BlackholePioneerMmioProbe.h"

enum PioneerPostResetPciStage {
	PioneerPostResetPciRefused=0,
	PioneerPostResetPciDisable=1,
	PioneerPostResetPciBars=2,
	PioneerPostResetPciBridgeDecode=3,
	PioneerPostResetPciCardDecode=4,
	PioneerPostResetPciReady=5
};

struct PioneerPostResetPciResult {
	unsigned stage=PioneerPostResetPciRefused;
	uint32_t cardCommand=0,rootCommand=0;
};

struct PioneerPciePayloadSnapshot {
	uint32_t capability=0, version=0, supported=0, control=0;
	bool valid=false;
};

// Capture only the endpoint PCIe payload/read-request settings. The device's
// internal DBI shadow and other capability state still require separate
// recovery; this helper alone does not make an ASIC reset safe.
template<class IO>
bool PioneerPciePayloadRead(IO& io,PioneerPciePayloadSnapshot& out)
{
	out={};
	if (io.CardRead(0,4)!=0xb1401e52 || io.CardRead(0x2c,4)!=0x00431e52
		|| io.CardRead(4,2)!=0x402 || io.RootRead(4,2)!=0x402) return false;
	uint32_t cap=io.CardRead(0x34,1);uint64_t seen=0;bool found=false;
	for (unsigned hops=0;cap && hops<48;hops++) {
		if (cap<0x40 || cap>0xfc || (cap&3) || (seen&(UINT64_C(1)<<(cap/4)))) return false;
		seen|=UINT64_C(1)<<(cap/4);
		const uint32_t header=io.CardRead(cap,2);
		if (header>0xffff) return false;
		if ((header&0xff)==0x10) {
			if (found || cap>0xf4) return false;
			out.capability=cap;out.version=io.CardRead(cap+2,2);
			out.supported=io.CardRead(cap+4,4);out.control=io.CardRead(cap+8,2);
			if (out.version>0xffff || ((out.version&15)!=1 && (out.version&15)!=2)
				|| out.supported==UINT32_MAX || out.control>0xffff || (out.control&0x8000)
				|| (out.supported&7)>5 || ((out.control>>5)&7)>(out.supported&7)
				|| ((out.control>>12)&7)>5) return false;
			found=true;
		}
		cap=(header>>8)&0xff;
	}
	if (cap || !found) return false;
	out.valid=true;return true;
}

template<class IO>
bool PioneerPciePayloadRestore(IO& io,const PioneerPciePayloadSnapshot& saved)
{
	PioneerPciePayloadSnapshot current{};
	if (!saved.valid || !PioneerPciePayloadRead(io,current)
		|| current.capability!=saved.capability || current.version!=saved.version
		|| current.supported!=saved.supported) return false;
	const uint32_t mask=0x70e0; // max payload and max read-request size only
	const uint32_t wanted=(current.control&~mask)|(saved.control&mask);
	if (wanted!=current.control) io.CardWrite(current.capability+8,2,wanted);
	PioneerPciePayloadSnapshot after{};
	return PioneerPciePayloadRead(io,after) && after.capability==saved.capability
		&& after.version==saved.version && after.supported==saved.supported
		&& after.control==wanted;
}

// SG2042 root config supports only aligned dword writes. A subword write is
// expanded by the controller into a read-modify-write, which would write back
// and acknowledge any PCI Status W1C bits in the upper half. Write the full
// command/status dword with zero status bits instead. This does not clear or
// otherwise handle PCIe AER or Device Status.
template<class IO>
void PioneerRecoveryRootCommandWrite(IO& io,uint16_t command)
{
	io.RootWrite(4,4,uint32_t(command));
}

// Disable bus mastering on the retained session before requesting reset.
// Do not disable memory decoding here: the reset marker and timer use PCI
// config space, and the post-reset audit will establish fresh MMIO mappings.
template<class IO>
bool PioneerPreResetDisableMaster(IO& io)
{
	const auto& profile=io.Profile();PioneerBlackholeBarPlan plan;
	if (!PioneerMinimumBarLayoutMatches(profile)
		|| !PioneerPlanBlackholeBars(profile.ranges,TT_PIONEER_RANGE_COUNT,plan)
		|| plan.hostStart[0]!=profile.bar0_host_start || plan.hostEnd[0]!=profile.bar0_host_end
		|| plan.hostStart[1]!=profile.bar2_host_start || plan.hostEnd[1]!=profile.bar2_host_end
		|| !io.Quiet() || !BlackholeCardControlsQuiet(io)
		|| io.CardRead(0,4)!=0xb1401e52 || io.CardRead(0x2c,4)!=0x00431e52
		|| io.RootRead(0,4)!=0x20421e30 || (io.RootRead(8,4)>>8)!=0x060400
		|| io.RootRead(0x18,4)!=0x003f0100 || io.RootRead(0x20,4)!=0x0000fff0) return false;
	const uint32_t card=io.CardRead(4,2),root=io.RootRead(4,2);
	if ((card!=0x402 && card!=0x406) || (root!=0x402 && root!=0x406)) return false;
	for (unsigned i=0;i<6;i++) if (io.CardRead(0x10+4*i,4)!=plan.bars[i]) return false;
	for (unsigned i=0;i<3;i++) if (io.RootRead(0x24+4*i,4)!=plan.bridge[i]) return false;
	if (card==0x406) io.CardWrite(4,2,0x402);
	if (io.CardRead(4,2)!=0x402) return false;
	if (root==0x406) PioneerRecoveryRootCommandWrite(io,0x402);
	return io.CardRead(4,2)==0x402 && io.RootRead(4,2)==0x402;
}

// Re-establish only the P100A's fixed active BAR/bridge layout after the ASIC
// reset marker has cleared. This does not restore all PCI capabilities or
// establish firmware readiness. The caller retains the exclusive reservation,
// old host pages and the cold latch on every error, and separately audits DMA.
// No pre-reset device TLB contents are read or written by this operation.
template<class IO>
bool PioneerPostResetPciRestore(IO& io,PioneerPostResetPciResult& result)
{
	result={};
	const auto& profile=io.Profile();
	PioneerBlackholeBarPlan plan;
	if (!PioneerMinimumBarLayoutMatches(profile)
		|| !PioneerPlanBlackholeBars(profile.ranges,TT_PIONEER_RANGE_COUNT,plan)
		|| plan.hostStart[0]!=profile.bar0_host_start
		|| plan.hostEnd[0]!=profile.bar0_host_end
		|| plan.hostStart[1]!=profile.bar2_host_start
		|| plan.hostEnd[1]!=profile.bar2_host_end
		|| !io.Quiet() || !BlackholeCardControlsQuiet(io)
		|| io.CardRead(0,4)!=0xb1401e52 || io.CardRead(0x2c,4)!=0x00431e52
		|| io.RootRead(0,4)!=0x20421e30 || (io.RootRead(8,4)>>8)!=0x060400
		|| io.RootRead(0x18,4)!=0x003f0100 || io.RootRead(0x20,4)!=0x0000fff0
		|| io.Size(0)!=UINT64_C(0x20000000)
		|| io.Size(2)!=UINT64_C(0x100000) || io.Size(4)!=UINT64_C(0x800000000))
		return false;
	result.cardCommand=io.CardRead(4,2);result.rootCommand=io.RootRead(4,2);
	// Marker, bus master, and unexpected command bits must be absent.
	if ((result.cardCommand!=0 && result.cardCommand!=0x402)
		|| (result.rootCommand!=0 && result.rootCommand!=0x402)) return false;
	result.stage=PioneerPostResetPciDisable;
	io.CardWrite(4,2,0);PioneerRecoveryRootCommandWrite(io,0);
	if (io.CardRead(4,2)!=0 || io.RootRead(4,2)!=0) return false;
	result.stage=PioneerPostResetPciBars;
	const unsigned order[]={5,4,1,0,3,2};
	for (unsigned i:order) io.CardWrite(0x10+4*i,4,plan.bars[i]);
	io.RootWrite(0x28,4,plan.bridge[1]);
	io.RootWrite(0x2c,4,plan.bridge[2]);
	io.RootWrite(0x24,4,plan.bridge[0]);
	for (unsigned i=0;i<6;i++) if (io.CardRead(0x10+4*i,4)!=plan.bars[i]) return false;
	for (unsigned i=0;i<3;i++) if (io.RootRead(0x24+4*i,4)!=plan.bridge[i]) return false;
	result.stage=PioneerPostResetPciBridgeDecode;
	PioneerRecoveryRootCommandWrite(io,0x402);
	if (io.RootRead(4,2)!=0x402 || io.CardRead(4,2)!=0) return false;
	result.stage=PioneerPostResetPciCardDecode;
	io.CardWrite(4,2,0x402);
	if (io.RootRead(4,2)!=0x402 || io.CardRead(4,2)!=0x402) return false;
	result.stage=PioneerPostResetPciReady;
	return true;
}

enum PioneerPostResetPciFinishStage {
	PioneerPostResetPciFinishRefused=0,
	PioneerPostResetPciFinishDisable=1,
	PioneerPostResetPciFinishRestore=2,
	PioneerPostResetPciFinishComplete=3
};

// After the fresh post-reset mappings are closed and the fleet/DMA audit
// passes, restore the fixed pre-session PCI layout. The saved values are from
// the original exclusive session, and must be exactly the known Pioneer idle
// state. This touches PCI config only; it never invokes old IO::Unmap(), which
// could write stale pre-reset TLB or core snapshots.
template<class IO>
bool PioneerPostResetPciFinish(IO& io,const tt_pioneer_mmio_result& saved,
	unsigned& stage)
{
	stage=PioneerPostResetPciFinishRefused;
	const auto& profile=io.Profile();
	PioneerBlackholeBarPlan plan;
	if (!PioneerMinimumBarLayoutMatches(profile)
		|| !PioneerPlanBlackholeBars(profile.ranges,TT_PIONEER_RANGE_COUNT,plan)
		|| plan.hostStart[0]!=profile.bar0_host_start || plan.hostEnd[0]!=profile.bar0_host_end
		|| plan.hostStart[1]!=profile.bar2_host_start || plan.hostEnd[1]!=profile.bar2_host_end
		|| saved.version!=1 || saved.commands_before[0] || saved.commands_before[1]
		|| saved.bridge_before[0]!=0x0001fff1 || saved.bridge_before[1] || saved.bridge_before[2]
		|| io.CardRead(0,4)!=0xb1401e52 || io.CardRead(0x2c,4)!=0x00431e52
		|| io.RootRead(0,4)!=0x20421e30 || (io.RootRead(8,4)>>8)!=0x060400
		|| io.RootRead(0x18,4)!=0x003f0100 || io.RootRead(0x20,4)!=0x0000fff0
		|| io.CardRead(4,2)!=0x402 || io.RootRead(4,2)!=0x402
		|| !io.Quiet() || !BlackholeCardControlsQuiet(io)) return false;
	for (unsigned i=0;i<6;i++) {
		if (saved.bars_before[i]!=(i%2==0?0x0cU:0U)
			|| io.CardRead(0x10+4*i,4)!=plan.bars[i]) return false;
	}
	for (unsigned i=0;i<3;i++) if (io.RootRead(0x24+4*i,4)!=plan.bridge[i]) return false;
	stage=PioneerPostResetPciFinishDisable;
	io.CardWrite(4,2,0);PioneerRecoveryRootCommandWrite(io,0);
	if (io.CardRead(4,2)!=0 || io.RootRead(4,2)!=0) return false;
	stage=PioneerPostResetPciFinishRestore;
	const unsigned order[]={5,4,1,0,3,2};
	for (unsigned i:order) io.CardWrite(0x10+4*i,4,saved.bars_before[i]);
	io.RootWrite(0x28,4,saved.bridge_before[1]);
	io.RootWrite(0x2c,4,saved.bridge_before[2]);
	io.RootWrite(0x24,4,saved.bridge_before[0]);
	for (unsigned i=0;i<6;i++) if (io.CardRead(0x10+4*i,4)!=saved.bars_before[i]) return false;
	for (unsigned i=0;i<3;i++) if (io.RootRead(0x24+4*i,4)!=saved.bridge_before[i]) return false;
	if (io.CardRead(4,2)!=0 || io.RootRead(4,2)!=0) return false;
	stage=PioneerPostResetPciFinishComplete;
	return true;
}

#endif
