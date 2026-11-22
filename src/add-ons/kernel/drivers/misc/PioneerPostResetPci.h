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
	if (root==0x406) io.RootWrite(4,2,0x402);
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
	io.CardWrite(4,2,0);io.RootWrite(4,2,0);
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
	io.RootWrite(4,2,0x402);
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
	io.CardWrite(4,2,0);io.RootWrite(4,2,0);
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
