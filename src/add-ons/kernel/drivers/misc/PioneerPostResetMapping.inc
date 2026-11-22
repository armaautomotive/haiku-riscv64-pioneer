/* SPDX-License-Identifier: MIT */
#include "PioneerPostResetAudit.h"

// Dedicated post-reset read-only windows. The caller owns the PCI reservation,
// holds sLock, has completed the ASIC reset and restored PCI decoding, and
// retains this object on any failure. No pre-reset TLB or L1 snapshot is used.
struct PioneerPostResetMapping : ConfigIO {
	const tt_pioneer_profile& profile;
	area_id areas[7]={-1,-1,-1,-1,-1,-1,-1};
	volatile uint32* tlb=NULL;
	const volatile uint32* node=NULL;
	const volatile uint32* reset=NULL;
	const volatile uint32* noc=NULL;
	const volatile uint32* hdma=NULL;
	const volatile uint32* scratch=NULL;
	const volatile uint32* csm=NULL;
	tt_dram_samples_result topology={};
	tt_arc_telemetry_result firmwareTelemetry={};
	uint32 saved[3]={},dmaSaved[3]={};
	bool attempted=false,ready=false,savedValid=false,modified=false,dmaModified=false;
	bool firmwareAttempted=false,firmwareReady=false,topologyReady=false;
	bool fleetAudited=false,dmaAudited=false;
	unsigned selected=PIONEER_POST_RESET_CORES;

	explicit PioneerPostResetMapping(const tt_pioneer_profile& p):profile(p) {}
	PioneerPostResetMapping(const PioneerPostResetMapping&)=delete;
	PioneerPostResetMapping& operator=(const PioneerPostResetMapping&)=delete;
	// Close must succeed before the object or any retained owner is released.
	~PioneerPostResetMapping() = default;

	uint32 RootRead(uint16 offset,uint8 size) {
		return sPCI->read_pci_config(profile.root_bus,0,0,offset,size);
	}
	bool PciReady() {
		PioneerBlackholeBarPlan plan;
		if (!PioneerMinimumBarLayoutMatches(profile)
			|| !PioneerPlanBlackholeBars(profile.ranges,TT_PIONEER_RANGE_COUNT,plan)
			|| plan.hostStart[0]!=profile.bar0_host_start
			|| plan.hostEnd[0]!=profile.bar0_host_end
			|| plan.hostStart[1]!=profile.bar2_host_start
			|| plan.hostEnd[1]!=profile.bar2_host_end) return false;
		if (!(Read(0,4)==0xb1401e52 && Read(0x2c,4)==0x00431e52
			&& RootRead(0,4)==0x20421e30 && (RootRead(8,4)>>8)==0x060400
			&& Read(4,2)==0x402 && RootRead(4,2)==0x402
			&& RootRead(0x18,4)==0x003f0100 && RootRead(0x20,4)==0x0000fff0)) return false;
		for (unsigned i=0;i<6;i++) if (Read(0x10+4*i,4)!=plan.bars[i]) return false;
		for (unsigned i=0;i<3;i++) if (RootRead(0x24+4*i,4)!=plan.bridge[i]) return false;
		return true;
	}
	bool Open() {
		if (attempted) return false;
		attempted=true;
		if (!PioneerMinimumBarLayoutMatches(profile) || !PciReady()
			|| profile.bar0_host_start>UINT64_MAX-0x1fd05000) return false;
		const uint32 offsets[]={0x1fc00000,0x1fd04000,0x19312000,0x19320000,0x19180000,0x19230000,0x19200000};
		const size_t lengths[]={B_PAGE_SIZE,B_PAGE_SIZE,B_PAGE_SIZE,0x12000,B_PAGE_SIZE,B_PAGE_SIZE,0x20000};
		void* views[7]={};
		for (unsigned i=0;i<7;i++) {
			uint32 flags=B_KERNEL_READ_AREA|B_UNCACHED_MEMORY;
			if (!i) flags|=B_KERNEL_WRITE_AREA;
			areas[i]=map_physical_memory("P100A fresh post-reset audit window",
				profile.bar0_host_start+offsets[i],lengths[i],B_ANY_KERNEL_ADDRESS,
				flags,&views[i]);
			if (areas[i]<B_OK || !views[i]) return false;
		}
		tlb=(volatile uint32*)((uint8*)views[0]+0x96c);
		node=(const volatile uint32*)views[1];
		reset=(const volatile uint32*)((const uint8*)views[2]+0x1b0);
		noc=(const volatile uint32*)views[3];
		hdma=(const volatile uint32*)views[4];
		scratch=(const volatile uint32*)views[5];
		csm=(const volatile uint32*)views[6];
		const uint32 id=PioneerRead32(node+0x44/4);
		if ((id&0xfff)!=2 && (id&0xfff)!=11) return false;
		for (unsigned i=0;i<3;i++) {
			saved[i]=PioneerRead32(tlb+i);
			dmaSaved[i]=PioneerRead32(tlb-3+i);
		}
		if ((saved[2]|dmaSaved[2])&0xffffc000) return false;
		savedValid=true;ready=true;return true;
	}
	uint32 TlbRead(unsigned i) { return i<3?PioneerRead32(tlb+i):UINT32_MAX; }
	void TlbWrite(unsigned i,uint32 value) {
		if (i<3) { modified=true;PioneerWrite32(tlb+i,value); }
	}
	uint32 ScratchRead(unsigned n) {
		if (n!=2 && n!=4 && n!=11 && n!=12 && n!=13) return UINT32_MAX;
		return PioneerRead32(scratch+(0x400+n*4)/4);
	}
	uint32 CsmRead(uint32 address) {
		return BlackholeTelemetryRange(address,4)
			?PioneerRead32(csm+(address-0x10000000)/4):UINT32_MAX;
	}
	// One bounded attempt. A failed TLB restore ends the attempt immediately;
	// the caller retains this mapping and the old owner for diagnosis.
	bool ProbeFirmware(unsigned pollLimit,tt_arc_scratch_result& result) {
		result={};
		if (!ready || firmwareAttempted || !pollLimit || pollLimit>100 || !PciReady()) return false;
		firmwareAttempted=true;
		bool bootReady=false;
		for (unsigned poll=0;poll<pollLimit;poll++) {
			if (!PciReady()) return false;
			if (BlackholeArcScratchProbe(*this,result)) {
				bootReady=true;break;
			}
			if (!result.tlb_restored || result.state!=TT_ARC_SCRATCH_NOT_READY) return false;
			if (poll+1<pollLimit && snooze(100000)!=B_OK) return false;
		}
		if (!bootReady || !PciReady()
			|| !BlackholeArcTelemetryProbe(*this,result,firmwareTelemetry)
			|| (firmwareTelemetry.present_mask&0x3c1)!=0x3c1
			|| firmwareTelemetry.values[0]!=0x12050001
			|| firmwareTelemetry.values[6]!=0xf7 || firmwareTelemetry.values[7]!=0x5515
			|| firmwareTelemetry.values[8]!=0 || firmwareTelemetry.values[9]!=0xfff
			|| !PciReady()) return false;
		firmwareReady=true;
		for (unsigned i=0;i<TT_DRAM_SOURCE_WORDS;i++)
			topology.source[i]=PioneerRead32(node+BlackholeDramSourceOffset(i)/4);
		topologyReady=BlackholeDramBuildRoutes(topology);
		return topologyReady;
	}
	bool SourceStable() {
		if (!ready || !firmwareReady || !topologyReady || !PciReady()) return false;
		for (unsigned i=0;i<TT_DRAM_SOURCE_WORDS;i++)
			if (PioneerRead32(node+BlackholeDramSourceOffset(i)/4)!=topology.source[i]) return false;
		return true;
	}
	bool SelectPostResetCore(unsigned core) {
		selected=PIONEER_POST_RESET_CORES;
		if (!ready || !firmwareReady || !topologyReady || core>=PIONEER_POST_RESET_CORES || !PciReady()) return false;
		const unsigned x=PioneerWorkerX(core),y=PioneerWorkerY(core);
		modified=true;
		for (unsigned i=0;i<3;i++) PioneerWrite32(tlb+i,BlackholeDramFixedTlbWord(0xffa00000,x,y,true,i));
		for (unsigned i=0;i<3;i++)
			if (PioneerRead32(tlb+i)!=BlackholeDramFixedTlbWord(0xffa00000,x,y,true,i)) return false;
		const uint32 id=PioneerRead32(noc+0x44/4);
		const uint32 endpoint=PioneerRead32(noc+0x48/4);
		const uint32 expected=BlackholeDramSourceX(topology.source,x)|(y<<6);
		if ((id&0xfff)!=expected || ((id>>12)&127)!=17 || ((id>>19)&127)!=12
			|| (endpoint&0xffffff00)!=0x10000) return false;
		selected=core;return true;
	}
	bool ReadPostResetState(uint32_t& value) {
		if (!ready || selected>=PIONEER_POST_RESET_CORES) return false;
		value=PioneerRead32(reset);return value!=UINT32_MAX;
	}
	bool ReadPostResetNoC(unsigned network,unsigned offset,uint32_t& value) {
		if (!ready || selected>=PIONEER_POST_RESET_CORES || network>=2 || (offset&3)
			|| !(offset==0x40 || offset==0x840 || offset==0x1040 || offset==0x1840
				|| (offset>=0x240 && offset<=0x2bc))) return false;
		value=PioneerRead32(noc+(network*0x10000+offset)/4);
		return value!=UINT32_MAX;
	}
	void PostResetAuditPause() { spin(100); }
	bool Audit(PioneerPostResetAuditResult& result) {
		fleetAudited=false;dmaAudited=false;
		fleetAudited=ready && SourceStable() && PioneerPostResetAudit(*this,result) && SourceStable();
		return fleetAudited;
	}
	uint32 HostPciRead(unsigned peer,uint16 offset,uint8 size) {
		return peer==0?Read(offset,size):peer==1?RootRead(offset,size):UINT32_MAX;
	}
	// The existing pinned firmware uses HDMA channel 0. Check read/write
	// enables and states as well as PCIe pending/error status on both ends.
	// The caller still owns and retains every host buffer until all checks and
	// subsequent resource retirement succeed.
	bool AuditHostDma() {
		dmaAudited=false;
		if (!fleetAudited || !SourceStable() || !savedValid
			|| (PioneerRead32(node+0x44/4)&0xfff)!=2
			|| !BlackholeHostSourceRoute(topology.source)) return false;
		dmaModified=true;
		for (unsigned i=0;i<3;i++) PioneerWrite32(tlb-3+i,BlackholeHostDbiTlbWord(i));
		for (unsigned i=0;i<3;i++)
			if (PioneerRead32(tlb-3+i)!=BlackholeHostDbiTlbWord(i)) return false;
		uint32 firstStatus[2]={},firstDma[4]={};
		for (unsigned sample=0;sample<3;sample++) {
			if (!PciReady()) return false;
			for (unsigned peer=0;peer<2;peer++) {
				uint32 status=0;
				if (!BlackholeHostPcieStatus(*this,peer,status)
					|| (sample && status!=firstStatus[peer])) return false;
				if (!sample) firstStatus[peer]=status;
			}
			for (unsigned i=0;i<4;i++) {
				const uint32 value=PioneerRead32(hdma+BlackholeHostHdmaOffset(i)/4);
				if ((i%2==0 && value!=0) || (i%2==1 && value!=0 && value!=3)
					|| (sample && value!=firstDma[i])) return false;
				if (!sample) firstDma[i]=value;
			}
			if (sample<2) PostResetAuditPause();
		}
		uint32 resetState=0;
		dmaAudited=ReadPostResetState(resetState) && resetState==PIONEER_POST_RESET_HELD
			&& SourceStable();
		return dmaAudited;
	}
	bool Close() {
		if (dmaModified) {
			if (!savedValid || !PciReady()) return false;
			for (unsigned i=0;i<3;i++) PioneerWrite32(tlb-3+i,dmaSaved[i]);
			for (unsigned i=0;i<3;i++) if (PioneerRead32(tlb-3+i)!=dmaSaved[i]) return false;
			dmaModified=false;
		}
		if (modified) {
			if (!savedValid || !PciReady()) return false;
			for (unsigned i=0;i<3;i++) PioneerWrite32(tlb+i,saved[i]);
			for (unsigned i=0;i<3;i++) if (PioneerRead32(tlb+i)!=saved[i]) return false;
			modified=false;
		}
		ready=false;selected=PIONEER_POST_RESET_CORES;
		for (unsigned i=0;i<7;i++) if (areas[i]>=B_OK) {
			if (delete_area(areas[i])!=B_OK) return false;
			areas[i]=-1;
		}
		tlb=NULL;node=reset=noc=hdma=scratch=csm=NULL;return true;
	}
};
