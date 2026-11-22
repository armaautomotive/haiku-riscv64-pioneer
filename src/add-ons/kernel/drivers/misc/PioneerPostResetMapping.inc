/* SPDX-License-Identifier: MIT */
#include "PioneerPostResetAudit.h"

// Dedicated post-reset read-only windows. The caller owns the PCI reservation,
// holds sLock, has completed the ASIC reset and restored PCI decoding, and
// retains this object on any failure. No pre-reset TLB or L1 snapshot is used.
struct PioneerPostResetMapping : ConfigIO {
	const tt_pioneer_profile& profile;
	area_id areas[4]={-1,-1,-1,-1};
	volatile uint32* tlb=NULL;
	const volatile uint32* node=NULL;
	const volatile uint32* reset=NULL;
	const volatile uint32* noc=NULL;
	tt_dram_samples_result topology={};
	uint32 saved[3]={};
	bool attempted=false,ready=false,savedValid=false,modified=false;
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
		const uint32 offsets[]={0x1fc00000,0x1fd04000,0x19312000,0x19320000};
		const size_t lengths[]={B_PAGE_SIZE,B_PAGE_SIZE,B_PAGE_SIZE,0x12000};
		void* views[4]={};
		for (unsigned i=0;i<4;i++) {
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
		const uint32 id=PioneerRead32(node+0x44/4);
		if ((id&0xfff)!=2 && (id&0xfff)!=11) return false;
		for (unsigned i=0;i<TT_DRAM_SOURCE_WORDS;i++)
			topology.source[i]=PioneerRead32(node+BlackholeDramSourceOffset(i)/4);
		if (!BlackholeDramBuildRoutes(topology)) return false;
		for (unsigned i=0;i<3;i++) saved[i]=PioneerRead32(tlb+i);
		if (saved[2]&0xffffc000) return false;
		savedValid=true;ready=true;return true;
	}
	bool SourceStable() {
		if (!ready || !PciReady()) return false;
		for (unsigned i=0;i<TT_DRAM_SOURCE_WORDS;i++)
			if (PioneerRead32(node+BlackholeDramSourceOffset(i)/4)!=topology.source[i]) return false;
		return true;
	}
	bool SelectPostResetCore(unsigned core) {
		selected=PIONEER_POST_RESET_CORES;
		if (!ready || core>=PIONEER_POST_RESET_CORES || !PciReady()) return false;
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
		return ready && SourceStable() && PioneerPostResetAudit(*this,result) && SourceStable();
	}
	bool Close() {
		if (modified) {
			if (!savedValid || !PciReady()) return false;
			for (unsigned i=0;i<3;i++) PioneerWrite32(tlb+i,saved[i]);
			for (unsigned i=0;i<3;i++) if (PioneerRead32(tlb+i)!=saved[i]) return false;
			modified=false;
		}
		ready=false;selected=PIONEER_POST_RESET_CORES;
		for (unsigned i=0;i<4;i++) if (areas[i]>=B_OK) {
			if (delete_area(areas[i])!=B_OK) return false;
			areas[i]=-1;
		}
		tlb=NULL;node=reset=noc=NULL;return true;
	}
};
