/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_RESIDENT_STORE_H
#define PIONEER_RESIDENT_STORE_H
#include "PioneerResidentSession.h"
#include "BlackholeHostWriteProbe.h"
#include "PioneerResidentBanks.h"
enum { TT_STORE_REFUSED=0,TT_STORE_PASSED=1,TT_STORE_CONFIG_FAILED=2,TT_STORE_TIMEOUT=3,TT_STORE_MISMATCH=4,TT_STORE_CHANGED=5 };
struct tt_resident_store_result {
	uint32_t version,state,phase,requests,completed,recovery_required,mismatches,wrote_device;
	uint32_t before[3][4],after[3][4],polls[3];
};
struct PioneerResidentLease {
	uint32_t* saved=nullptr;
	uint32_t tlb[2][3]={},config=0;
	bool controls_saved=false,configured=false,l1_dirty=false,pending=false,ready=false,recovery_required=false;
	bool l1_restored=true,niu_restored=true,tlb_restored=true;
};
static inline uint32_t PioneerResidentPageHash(const uint32_t* data)
{
	uint32_t hash=2166136261U;
	for (unsigned w=0;w<1024;w++) for (unsigned byte=0;byte<4;byte++) hash=(hash^((data[w]>>(8*byte))&255))*16777619U;
	return hash; // integrity bookkeeping, not a cryptographic authenticity claim
}
template<class IO>
bool PioneerResidentLeaseBegin(IO& io,const tt_compute_noc_result& noc,PioneerResidentLease& lease,unsigned y=2)
{
	if ((y!=2 && y!=3) || lease.controls_saved || lease.saved || noc.version!=(y==3?2U:1U) || noc.state!=TT_COMPUTE_NOC_PASSED
		|| !noc.tlb_restored || noc.recovery_required || !BlackholeComputeNocRouteValid(noc.source[0],y)
		|| (y==3 && !BlackholeHostSourceRoute(noc.source[0]))) return false;
	lease.saved=io.HostReadBackup(2048); if (!lease.saved) return false;
	for (unsigned s=0;s<2;s++) for (unsigned w=0;w<3;w++) lease.tlb[s][w]=io.DramTlbRead(s,w);
	if ((lease.tlb[0][2]&0xffffc000) || (lease.tlb[1][2]&0xffffc000)) return false;
	lease.controls_saved=true; lease.tlb_restored=false; lease.config=noc.source[0][0];
	if (!BlackholeDramConfigure(io,0,0,1,y,false) || !BlackholeDramConfigure(io,1,0xffa00000,1,y,false)) return false;
	if (!BlackholeHostReadPciQuiet(io,0x402) || !BlackholeResidentSourceStable(io,noc,lease.config)
		|| !BlackholeResidentIdle(io) || (io.ResidentNocRead(0x178)&0xffff)) return false;
	for (unsigned w=0;w<10;w++) if (io.ResidentNocRead(w*4)) return false;
	for (unsigned w=0;w<2048;w++) lease.saved[w]=io.ResidentL1Read(w);
	lease.configured=true; lease.niu_restored=false;
	io.ResidentNocWrite(0x100,lease.config|0x10000);
	if (!BlackholeResidentSourceStable(io,noc,lease.config|0x10000)) return false;
	lease.ready=true; return true;
}
template<class IO>
bool PioneerResidentLeaseEnd(IO& io,const tt_compute_noc_result& noc,PioneerResidentLease& lease)
{
	bool safe=!lease.pending && !lease.recovery_required;
	if (safe && lease.configured) {
		uint32_t config=io.ResidentNocRead(0x100);
		safe=(config==lease.config || config==(lease.config|0x10000))
			&& BlackholeResidentSourceStable(io,noc,config) && BlackholeResidentIdle(io) && BlackholeHostReadPciQuiet(io,0x402);
		if (safe) {
			for (unsigned attempt=0;attempt<2 && !lease.niu_restored;attempt++) {
				for (unsigned w=0;w<10;w++) io.ResidentNocWrite(w*4,0);
				io.ResidentNocWrite(0x100,lease.config);
				lease.niu_restored=BlackholeResidentSourceStable(io,noc,lease.config);
				for (unsigned w=0;w<10;w++) lease.niu_restored &= io.ResidentNocRead(w*4)==0;
			}
			safe=lease.niu_restored;
		}
	}
	if (safe && lease.l1_dirty) for (unsigned attempt=0;attempt<2 && !lease.l1_restored;attempt++) {
		for (unsigned w=0;w<2048;w++) io.ResidentL1Write(w,lease.saved[w]);
		lease.l1_restored=io.ResetRead()==0x47800;
		for (unsigned w=0;w<2048;w++) lease.l1_restored &= io.ResidentL1Read(w)==lease.saved[w];
	}
	if (lease.controls_saved) for (unsigned attempt=0;attempt<2 && !lease.tlb_restored;attempt++) {
		for (unsigned s=0;s<2;s++) for (unsigned w=0;w<3;w++) io.DramTlbWrite(s,w,lease.tlb[s][w]);
		lease.tlb_restored=true;
		for (unsigned s=0;s<2;s++) for (unsigned w=0;w<3;w++) lease.tlb_restored &= io.DramTlbRead(s,w)==lease.tlb[s][w];
	}
	lease.ready=false;
	lease.recovery_required |= !safe || !lease.l1_restored || !lease.niu_restored || !lease.tlb_restored;
	return !lease.recovery_required;
}
template<class IO>
bool PioneerResidentStoreCommand(IO& io,PioneerResidentLease& lease,const uint32_t* command,
	bool write,unsigned step,tt_resident_store_result& r)
{
	if (step>=3 || !BlackholeResidentIdle(io)) return false;
	for (unsigned w=0;w<10;w++) io.ResidentNocWrite(w*4,command[w]);
	for (unsigned w=0;w<10;w++) if (io.ResidentNocRead(w*4)!=command[w]) return false;
	for (unsigned w=0;w<4;w++) r.before[step][w]=io.ResidentNocRead(write?BlackholeHostWriteCounter(w):0x208+w*4);
	lease.pending=true; r.requests++; r.phase=step+1;
	io.ResidentNocWrite(0x40,1);
	for (unsigned attempt=0;attempt<256;attempt++) {
		r.polls[step]++;
		uint32_t pending=io.ResidentNocRead(0x40);
		for (unsigned w=0;w<4;w++) r.after[step][w]=io.ResidentNocRead(write?BlackholeHostWriteCounter(w):0x208+w*4);
		if (!pending && uint32_t(r.after[step][0]-r.before[step][0])==1
			&& uint32_t(r.after[step][1]-r.before[step][1])==64
			&& uint32_t(r.after[step][2]-r.before[step][2])==1
			&& uint32_t(r.after[step][3]-r.before[step][3])==1 && BlackholeResidentIdle(io)) {
			lease.pending=false; r.completed++; return true;
		}
		io.Pause();
	}
	lease.recovery_required=true; r.state=TT_STORE_TIMEOUT; return false;
}
template<class IO>
bool PioneerResidentStorePage(IO& io,const tt_dram_samples_result& baseline,const tt_compute_noc_result& noc,
	const tt_host_page_result& host,PioneerResidentLease& lease,uint64_t capacity,uint64_t offset,
	bool upload,uint32_t* data,tt_resident_store_result& r,
	const PioneerResidentBanks* banks=nullptr,unsigned bank=0)
{
	r={}; r.version=1;
	uint32_t address=0,route=0,index=0;
	if (banks) {
		if (capacity!=PioneerResidentBanks::Capacity || !banks->Page(baseline,bank,offset,address,route,index)) return false;
	} else {
		if (bank || !PioneerResidentOffset(capacity,offset)) return false;
		address=PioneerResidentAddress(capacity,offset);
		route=baseline.translated[0][0]|(baseline.translated[0][1]<<6);
	}
	if (!data || !lease.ready || lease.pending || lease.recovery_required
		|| host.state!=TT_HOST_PAGE_READY || !host.memory_type_set || host.released) return false;
	r.state=TT_STORE_CONFIG_FAILED;
	bool good=BlackholeResidentSourceStable(io,noc,lease.config|0x10000)
		&& BlackholeDramSourceStable(io,baseline) && BlackholeResidentIdle(io)
		&& BlackholeHostReadPciQuiet(io,0x402) && BlackholeHostReadMapStable(io,host);
	for (unsigned s=0;s<2;s++) for (unsigned w=0;w<3;w++) good &= io.DramTlbRead(s,w)
		==BlackholeDramFixedTlbWord(s?0xffa00000:0,1,2,false,w);
	bool bmeTouched=false;
	do {
		if (!good) { lease.recovery_required=true; r.state=TT_STORE_CHANGED; break; }
		lease.l1_dirty=true; lease.l1_restored=false;
		if (upload) {
			for (unsigned w=0;w<1024;w++) io.HostPageWrite(w,data[w]);
			for (unsigned w=0;w<1024;w++) r.mismatches+=io.HostPageRead(w)!=data[w];
			if (r.mismatches) { r.state=TT_STORE_MISMATCH; break; }
			bmeTouched=true;
			io.HostPciWrite(1,4,2,0x406); if (io.HostPciRead(1,4,2)!=0x406) break;
			io.HostPciWrite(0,4,2,0x406); if (!BlackholeHostReadPciQuiet(io,0x406)) break;
			const uint32_t fromHost[]={uint32_t(host.physical),uint32_t(host.physical>>32),19|(24<<6),0x17e000,0,1|(2<<6),0,0,4096,0};
			if (!PioneerResidentStoreCommand(io,lease,fromHost,false,0,r)) break;
			// Host DMA is finished before any GDDR transaction. BME must be
			// proved off, even though later commands remain device-local.
			io.HostPciWrite(0,4,2,0x402); io.HostPciWrite(1,4,2,0x402);
			if (!BlackholeHostReadPciQuiet(io,0x402)) { lease.recovery_required=true; break; }
			for (unsigned w=0;w<1024;w++) r.mismatches+=io.ResidentL1Read(w)!=data[w];
			if (r.mismatches) { r.state=TT_STORE_MISMATCH; break; }
			if (!BlackholeResidentSourceStable(io,noc,lease.config|0x10000)
				|| !BlackholeDramSourceStable(io,baseline) || !BlackholeHostReadMapStable(io,host)) {
				r.state=TT_STORE_CHANGED; lease.recovery_required=true; break;
			}
			const uint32_t toGddr[]={0x17e000,0,1|(2<<6),address,0,route,0,0x212,4096,0};
			r.wrote_device=1;
			if (!PioneerResidentStoreCommand(io,lease,toGddr,true,1,r)) break;
			if (!BlackholeResidentSourceStable(io,noc,lease.config|0x10000)
				|| !BlackholeDramSourceStable(io,baseline) || !BlackholeHostReadMapStable(io,host)) {
				r.state=TT_STORE_CHANGED; lease.recovery_required=true; break;
			}
		}
		// Return to the initiating NIU, never to a different DRAM NIU.
		const uint32_t fromGddr[]={address,0,route,0x17f000,0,1|(2<<6),0,0,4096,0};
		if (!PioneerResidentStoreCommand(io,lease,fromGddr,false,upload?2:0,r)) break;
		if (!BlackholeResidentSourceStable(io,noc,lease.config|0x10000)
			|| !BlackholeDramSourceStable(io,baseline) || !BlackholeHostReadMapStable(io,host)
			|| !BlackholeResidentIdle(io) || !BlackholeHostReadPciQuiet(io,0x402)) {
			r.state=TT_STORE_CHANGED; lease.recovery_required=true; break;
		}
		for (unsigned w=0;w<1024;w++) {
			uint32_t value=io.ResidentL1Read(1024+w);
			if (upload) r.mismatches+=value!=data[w]; else data[w]=value;
		}
		r.state=r.mismatches?TT_STORE_MISMATCH:TT_STORE_PASSED;
	} while (false);
	if (bmeTouched) {
		bool off=false;
		for (unsigned attempt=0;attempt<2 && !off;attempt++) {
			io.HostPciWrite(0,4,2,0x402); io.HostPciWrite(1,4,2,0x402);
			off=BlackholeHostReadPciQuiet(io,0x402);
		}
		if (!off) lease.recovery_required=true;
	}
	r.recovery_required=lease.recovery_required || lease.pending;
	if (r.recovery_required) io.QuarantineHostPage();
	return r.state==TT_STORE_PASSED && !r.recovery_required;
}
#endif
