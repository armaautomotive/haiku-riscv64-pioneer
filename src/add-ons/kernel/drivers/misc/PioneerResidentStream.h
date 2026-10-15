/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_RESIDENT_STREAM_H
#define PIONEER_RESIDENT_STREAM_H
#include "PioneerResidentStore.h"
// Operational 4KiB upload with a complete card-to-owned-host verification
// loop instead of two per-word PCIe payload scans. Separately native-verified
// with independent legacy GDDR readback. The original store remains.
struct tt_resident_stream_result {
	uint32_t version,state,phase,requests,completed,mismatches,wrote_device;
	uint32_t host_write_issued,host_visible,readback_complete,recovery_required;
	uint32_t before[6][4],after[6][4],polls[6];
};
template<class IO>
bool PioneerResidentStreamGuard(IO& io,const tt_dram_samples_result& baseline,const tt_compute_noc_result& noc,
	const tt_host_page_result& host,const PioneerResidentLease& lease,uint32_t pci)
{
	bool okay=io.ResetRead()==0x47800 && BlackholeResidentSourceStable(io,noc,lease.config|0x10000)
		&& BlackholeDramSourceStable(io,baseline) && BlackholeResidentIdle(io)
		&& BlackholeHostReadPciQuiet(io,pci) && BlackholeHostReadMapStable(io,host);
	for (unsigned s=0;s<2;s++) for (unsigned w=0;w<3;w++) okay &= io.DramTlbRead(s,w)
		==BlackholeDramFixedTlbWord(s?0xffa00000:0,1,2,false,w);
	return okay;
}
template<class IO>
bool PioneerResidentStreamCommand(IO& io,PioneerResidentLease& lease,const uint32_t* command,
	bool write,unsigned step,tt_resident_stream_result& r)
{
	if (step>=6 || !BlackholeResidentIdle(io)) return false;
	for (unsigned w=0;w<10;w++) io.ResidentNocWrite(w*4,command[w]);
	for (unsigned w=0;w<10;w++) if (io.ResidentNocRead(w*4)!=command[w]) return false;
	for (unsigned w=0;w<4;w++) r.before[step][w]=io.ResidentNocRead(write?BlackholeHostWriteCounter(w):0x208+w*4);
	lease.pending=true; r.requests++; r.phase=step+1;
	if (step==4) r.host_write_issued=1;
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
bool PioneerResidentStreamPage(IO& io,const tt_dram_samples_result& baseline,const tt_compute_noc_result& noc,
	const tt_host_page_result& host,PioneerResidentLease& lease,uint64_t capacity,uint64_t offset,
	const uint32_t* data,tt_resident_stream_result& r)
{
	r={}; r.version=1;
	if (!data || !lease.ready || lease.pending || lease.recovery_required || !PioneerResidentOffset(capacity,offset)
		|| host.state!=TT_HOST_PAGE_READY || !host.memory_type_set || host.released) return false;
	r.state=TT_STORE_CONFIG_FAILED;
	const uint32_t address=PioneerResidentAddress(capacity,offset);
	const uint32_t hostRead[]={uint32_t(host.physical),uint32_t(host.physical>>32),19|(24<<6),0x17e000,0,1|(2<<6),0,0,4096,0};
	const uint32_t toGddr[]={0x17e000,0,1|(2<<6),address,0,baseline.translated[0][0]|(baseline.translated[0][1]<<6),0,0x212,4096,0};
	const uint32_t fromGddr[]={address,0,baseline.translated[0][0]|(baseline.translated[0][1]<<6),0x17f000,0,1|(2<<6),0,0,4096,0};
	const uint32_t toHost[]={0x17f000,0,1|(2<<6),uint32_t(host.physical),uint32_t(host.physical>>32),19|(24<<6),0,0x212,4096,0};
	bool bmeTouched=false;
	do {
		if (!PioneerResidentStreamGuard(io,baseline,noc,host,lease,0x402)) { lease.recovery_required=true; r.state=TT_STORE_CHANGED; break; }
		lease.l1_dirty=true; lease.l1_restored=false;
		for (unsigned w=0;w<1024;w++) io.HostPageWrite(w,data[w]);
		for (unsigned w=0;w<1024;w++) r.mismatches+=io.HostPageRead(w)!=data[w];
		if (r.mismatches) { r.state=TT_STORE_MISMATCH; break; }
		bmeTouched=true;
		io.HostPciWrite(1,4,2,0x406); if (io.HostPciRead(1,4,2)!=0x406) break;
		io.HostPciWrite(0,4,2,0x406);
		if (!PioneerResidentStreamGuard(io,baseline,noc,host,lease,0x406)) { lease.recovery_required=true; break; }
		if (!PioneerResidentStreamCommand(io,lease,hostRead,false,0,r)) break;
		io.HostPciWrite(0,4,2,0x402); io.HostPciWrite(1,4,2,0x402);
		if (!PioneerResidentStreamGuard(io,baseline,noc,host,lease,0x402)) { lease.recovery_required=true; break; }
		r.wrote_device=1;
		if (!PioneerResidentStreamCommand(io,lease,toGddr,true,1,r)) break;
		if (!PioneerResidentStreamGuard(io,baseline,noc,host,lease,0x402)) { lease.recovery_required=true; break; }
		if (!PioneerResidentStreamCommand(io,lease,fromGddr,false,2,r)) break;
		if (!PioneerResidentStreamGuard(io,baseline,noc,host,lease,0x402)) { lease.recovery_required=true; break; }
		// All previous owned-host reads have completed. Poison every host
		// word, then use an ordered read to establish a distinct poisoned
		// L1 return region without per-word PCIe traffic.
		for (unsigned w=0;w<1024;w++) io.HostPageWrite(w,~data[w]);
		for (unsigned w=0;w<1024;w++) r.mismatches+=io.HostPageRead(w)!=~data[w];
		if (r.mismatches) { r.state=TT_STORE_MISMATCH; break; }
		io.HostPciWrite(1,4,2,0x406); if (io.HostPciRead(1,4,2)!=0x406) break;
		io.HostPciWrite(0,4,2,0x406);
		if (!PioneerResidentStreamGuard(io,baseline,noc,host,lease,0x406)) { lease.recovery_required=true; break; }
		if (!PioneerResidentStreamCommand(io,lease,hostRead,false,3,r)) break;
		if (!PioneerResidentStreamGuard(io,baseline,noc,host,lease,0x406)) { lease.recovery_required=true; break; }
		if (!PioneerResidentStreamCommand(io,lease,toHost,true,4,r)) break;
		// A NoC ACK is insufficient for a PCIe posted write. Require the
		// entire CPU-visible payload to replace its complement poison.
		for (unsigned attempt=0;attempt<256;attempt++) {
			r.mismatches=0;
			for (unsigned w=0;w<1024;w++) r.mismatches+=io.HostPageRead(w)!=data[w];
			if (!r.mismatches) { r.host_visible=1; break; }
			io.Pause();
		}
		if (!r.host_visible) { r.state=TT_STORE_MISMATCH; lease.recovery_required=true; break; }
		if (!PioneerResidentStreamGuard(io,baseline,noc,host,lease,0x406)) { lease.recovery_required=true; break; }
		// Fresh non-relaxed PCIe read after full visibility, back into L1
		// distinct from the write source. Completion/IDs and another full
		// CPU comparison prove ordering before reusing/freeing the page.
		if (!PioneerResidentStreamCommand(io,lease,hostRead,false,5,r)) break;
		r.readback_complete=1;
		if (!PioneerResidentStreamGuard(io,baseline,noc,host,lease,0x406)) { lease.recovery_required=true; break; }
		for (unsigned w=0;w<1024;w++) r.mismatches+=io.HostPageRead(w)!=data[w];
		if (r.mismatches) { r.state=TT_STORE_CHANGED; lease.recovery_required=true; break; }
		r.state=TT_STORE_PASSED;
	} while (false);
	if (r.host_write_issued && (!r.host_visible || !r.readback_complete)) lease.recovery_required=true;
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
