/* SPDX-License-Identifier: MIT */
#ifndef BLACKHOLE_HOST_WRITE_PROBE_H
#define BLACKHOLE_HOST_WRITE_PROBE_H
#include "BlackholeHostReadProbe.h"
struct tt_host_write_result {
	tt_host_read_result transfer; // version/state/ownership/rollback; before/after are WRITE counters
	uint32_t write_completed,host_visible,host_polls,read_issued,read_completed,read_polls;
	uint32_t read_before[4],read_after[4];
};
static inline uint32_t BlackholeHostWriteCounter(unsigned word)
{
	const uint32_t offsets[]={0x204,0x220,0x210,0x228}; // ACK, non-posted flits, accepted, non-posted requests
	return word<4?offsets[word]:UINT32_MAX;
}
static inline bool BlackholeHostWriteReadRegister(uint32_t offset)
{
	return BlackholeResidentReadRegister(offset) || offset==0x204 || offset==0x220 || offset==0x228;
}
template<class IO>
bool BlackholeHostWriteCommand(IO& io,const uint32_t* command)
{
	if (!BlackholeResidentIdle(io)) return false;
	for (unsigned w=0;w<10;w++) io.ResidentNocWrite(w*4,command[w]);
	bool ready=BlackholeResidentIdle(io);
	for (unsigned w=0;w<10;w++) ready &= io.ResidentNocRead(w*4)==command[w];
	return ready;
}
// Exactly one fixed acknowledged non-inline L1 -> owned-host write, then
// one read back into a DISTINCT poisoned L1 region (64 bytes or 4KiB). No arbitrary
// host addresses, DMA-engine commands, counters clears, core starts or retries.
// A NoC ACK alone is NOT proof that a PCIe posted write has reached host RAM.
template<class IO,unsigned Bytes=64>
bool BlackholeHostWriteProbe(IO& io,tt_dram_samples_result& baseline,
	const tt_compute_noc_result& n,const tt_host_path_result& path,
	const tt_host_dma_audit_result& audit,tt_host_write_result& result)
{
	static_assert(Bytes==64 || Bytes==4096,"Only two fixed private host-write policies exist");
	const unsigned words=Bytes/4,hostWord=Bytes==64?32:0;
	result={}; tt_host_read_result& r=result.transfer;
	r.version=1; r.bme_restored=r.l1_restored=r.niu_restored=r.tlb_restored=1;
	if (audit.version!=1 || audit.state!=TT_HOST_DMA_AUDIT_PASSED || !audit.tlb_restored || audit.recovery_required
		|| path.version!=1 || path.state!=TT_HOST_PATH_PASSED || !path.tlb_restored || path.recovery_required
		|| baseline.version!=1 || baseline.state!=TT_DRAM_SAMPLE_ROUTES_OK || baseline.routes_verified!=7
		|| !baseline.data_restored || !baseline.tlb_restored || baseline.recovery_required
		|| n.version!=1 || n.state!=TT_COMPUTE_NOC_PASSED || !n.tlb_restored || n.recovery_required
		|| !BlackholeHostSourceRoute(n.source[0]) || !BlackholeDramBuildRoutes(baseline)) return false;
	r.state=TT_HOST_READ_BACKUP_FAILED;
	uint32_t* saved=io.HostReadBackup(2*words);
	if (!saved) return false;
	r.state=TT_HOST_READ_PAGE_FAILED;
	bool prepared=PioneerPrepareHostPage(io,r.page);
	bool savedTlb=false,modified=false,configured=false,bmeTouched=false,canRestore=true;
	uint32_t config=n.source[0][0];
	do {
		if (!prepared) break;
		r.state=TT_HOST_READ_CONFIG_FAILED; r.phase=1;
		for (unsigned s=0;s<2;s++) for (unsigned w=0;w<3;w++) r.tlb_before[s][w]=io.DramTlbRead(s,w);
		if ((r.tlb_before[0][2]&0xffffc000) || (r.tlb_before[1][2]&0xffffc000)) break;
		savedTlb=true; r.tlb_restored=0; baseline.cores_checked=0;
		if (!BlackholeDramAllCoresHeld(io,baseline) || !BlackholeDramConfigure(io,1,0xffa00000,1,2,false)) break;
		if (!BlackholeDramSourceStable(io,baseline) || !BlackholeResidentSourceStable(io,n,config)
			|| !BlackholeResidentIdle(io) || (io.ResidentNocRead(0x178)&0xffff)) break;
		bool pristine=true;
		for (unsigned w=0;w<10;w++) pristine &= io.ResidentNocRead(w*4)==0;
		if (!pristine || !BlackholeHostReadPciQuiet(io,0x402)) break;
		r.phase=2;
		for (unsigned w=0;w<3;w++) io.DramTlbWrite(0,w,BlackholeHostDbiTlbWord(w));
		bool stable=true;
		for (unsigned w=0;w<3;w++) stable &= io.DramTlbRead(0,w)==BlackholeHostDbiTlbWord(w);
		if (!stable) break;
		for (unsigned w=0;w<4;w++) stable &= io.HostHdmaRead(w)==audit.hdma[0][w];
		for (unsigned w=0;w<3;w++) io.DramTlbWrite(0,w,BlackholeHostSiiTlbWord(w));
		bool mapped=true;
		for (unsigned w=0;w<3;w++) mapped &= io.DramTlbRead(0,w)==BlackholeHostSiiTlbWord(w);
		if (!mapped) break;
		for (unsigned w=0;w<8;w++) stable &= io.HostSiiRead(w)==path.sii[0][w];
		for (unsigned w=0;w<2;w++) stable &= io.HostInboundRead(w)==path.inbound[0][w];
		if (!stable || !BlackholeHostReadMapStable(io,r.page)) { r.state=TT_HOST_READ_CHANGED; break; }
		if (!BlackholeDramConfigure(io,0,0,1,2,false)) break;
		for (unsigned w=0;w<2*words;w++) saved[w]=io.ResidentL1Read(w);
		modified=true; r.l1_restored=0; r.phase=3;
		for (unsigned w=0;w<words;w++) {
			io.ResidentL1Write(w,PioneerHostPagePattern(2,hostWord+w));
			io.ResidentL1Write(words+w,~PioneerHostPagePattern(2,hostWord+w));
		}
		bool poisoned=true;
		for (unsigned w=0;w<words;w++) poisoned &= io.ResidentL1Read(w)==PioneerHostPagePattern(2,hostWord+w)
			&& io.ResidentL1Read(words+w)==~PioneerHostPagePattern(2,hostWord+w);
		if (!poisoned) break;
		configured=true; r.niu_restored=0;
		io.ResidentNocWrite(0x100,config|0x10000);
		if (!BlackholeResidentSourceStable(io,n,config|0x10000)) break;
		uint64_t target=r.page.physical+hostWord*4;
		// RESP_MARKED requests the NoC ACK. MEM_RD_DROP_ACK suppresses only
		// the unused NoC-overlay notification, NOT that write acknowledgement.
		const uint32_t writeCommand[]={0x180000-2*Bytes,0,1|(2<<6),uint32_t(target),uint32_t(target>>32),19|(24<<6),0,0x212,Bytes,0};
		if (!BlackholeHostWriteCommand(io,writeCommand) || !BlackholeResidentSourceStable(io,n,config|0x10000)
			|| !BlackholeDramSourceStable(io,baseline) || !BlackholeHostReadMapStable(io,r.page)) break;
		for (unsigned w=0;w<TT_HOST_PAGE_WORDS;w++) r.host_mismatches+=io.HostPageRead(w)!=PioneerHostPagePattern(1,w);
		if (r.host_mismatches || !BlackholeHostReadPciQuiet(io,0x402)) break;
		r.phase=4; bmeTouched=true; r.bme_restored=0;
		io.HostPciWrite(1,4,2,0x406); if (io.HostPciRead(1,4,2)!=0x406) break;
		r.bme_enabled|=2;
		io.HostPciWrite(0,4,2,0x406); if (io.HostPciRead(0,4,2)!=0x406) break;
		r.bme_enabled|=1;
		if (!BlackholeHostReadPciQuiet(io,0x406) || !BlackholeResidentIdle(io)
			|| !BlackholeResidentSourceStable(io,n,config|0x10000) || !BlackholeDramSourceStable(io,baseline)) break;
		for (unsigned w=0;w<4;w++) r.before[w]=io.ResidentNocRead(BlackholeHostWriteCounter(w));
		r.phase=5; r.issued=1; r.state=TT_HOST_READ_TIMEOUT;
		io.ResidentNocWrite(0x40,1);
		for (unsigned attempt=0;attempt<256;attempt++) {
			r.polls++;
			uint32_t pending=io.ResidentNocRead(0x40);
			for (unsigned w=0;w<4;w++) r.after[w]=io.ResidentNocRead(BlackholeHostWriteCounter(w));
			if (!pending && uint32_t(r.after[0]-r.before[0])==1 && uint32_t(r.after[1]-r.before[1])==Bytes/64
				&& uint32_t(r.after[2]-r.before[2])==1 && uint32_t(r.after[3]-r.before[3])==1
				&& BlackholeResidentIdle(io)) { result.write_completed=1; break; }
			io.Pause();
		}
		if (!result.write_completed) { canRestore=false; break; }
		r.phase=6;
		for (unsigned attempt=0;attempt<256;attempt++) {
			result.host_polls++; r.mismatches=0;
			for (unsigned w=0;w<words;w++) r.mismatches+=io.HostPageRead(hostWord+w)!=PioneerHostPagePattern(2,hostWord+w);
			if (!r.mismatches) { result.host_visible=1; break; }
			io.Pause();
		}
		if (!result.host_visible) { canRestore=false; break; }
		// Full host visibility, source integrity and guards precede even
		// the NEW readback request. A missing ACK/data never causes a retry.
		for (unsigned w=0;w<TT_HOST_PAGE_WORDS;w++) r.host_mismatches+=io.HostPageRead(w)
			!=PioneerHostPagePattern(w>=hostWord && w<hostWord+words?2:1,w);
		bool sourceOkay=true;
		for (unsigned w=0;w<words;w++) sourceOkay &= io.ResidentL1Read(w)==PioneerHostPagePattern(2,hostWord+w);
		if (r.host_mismatches || !sourceOkay || !BlackholeResidentSourceStable(io,n,config|0x10000)
			|| !BlackholeDramSourceStable(io,baseline) || !BlackholeHostReadMapStable(io,r.page)
			|| !BlackholeHostReadPciQuiet(io,0x406)) { r.state=TT_HOST_READ_CHANGED; canRestore=false; break; }
		r.phase=7;
		const uint32_t readCommand[]={uint32_t(target),uint32_t(target>>32),19|(24<<6),0x180000-Bytes,0,1|(2<<6),0,0,Bytes,0};
		if (!BlackholeHostWriteCommand(io,readCommand)) { canRestore=false; break; }
		for (unsigned w=0;w<4;w++) result.read_before[w]=io.ResidentNocRead(0x208+w*4);
		result.read_issued=1; io.ResidentNocWrite(0x40,1);
		for (unsigned attempt=0;attempt<256;attempt++) {
			result.read_polls++;
			uint32_t pending=io.ResidentNocRead(0x40);
			for (unsigned w=0;w<4;w++) result.read_after[w]=io.ResidentNocRead(0x208+w*4);
			if (!pending && uint32_t(result.read_after[0]-result.read_before[0])==1
				&& uint32_t(result.read_after[1]-result.read_before[1])==Bytes/64
				&& uint32_t(result.read_after[2]-result.read_before[2])==1
				&& uint32_t(result.read_after[3]-result.read_before[3])==1
				&& BlackholeResidentIdle(io)) { result.read_completed=r.completed=1; break; }
			io.Pause();
		}
		if (!result.read_completed) { canRestore=false; break; }
		r.phase=8;
		if (!BlackholeResidentSourceStable(io,n,config|0x10000) || !BlackholeDramSourceStable(io,baseline)
			|| !BlackholeHostReadPciQuiet(io,0x406) || !BlackholeHostReadMapStable(io,r.page)) {
			r.state=TT_HOST_READ_CHANGED; canRestore=false; break;
		}
		for (unsigned w=0;w<2*words;w++) r.mismatches+=io.ResidentL1Read(w)!=PioneerHostPagePattern(2,hostWord+w%words);
		for (unsigned w=0;w<TT_HOST_PAGE_WORDS;w++) r.host_mismatches+=io.HostPageRead(w)
			!=PioneerHostPagePattern(w>=hostWord && w<hostWord+words?2:1,w);
		if (r.host_mismatches) canRestore=false;
		r.state=r.mismatches || r.host_mismatches?TT_HOST_READ_MISMATCH:TT_HOST_READ_PASSED;
	} while (false);
	// Once any host write was attempted, release requires the entire
	// host-visible + distinct-L1 readback proof, not merely a NoC ACK.
	if (r.issued && !result.read_completed) canRestore=false;
	if (bmeTouched) for (unsigned attempt=0;attempt<2 && !r.bme_restored;attempt++) {
		io.HostPciWrite(0,4,2,0x402); io.HostPciWrite(1,4,2,0x402);
		r.bme_restored=BlackholeHostReadPciQuiet(io,0x402);
	}
	canRestore &= r.bme_restored!=0;
	if (modified && canRestore) {
		canRestore=BlackholeResidentSourceStable(io,n,configured?config|0x10000:config)
			&& BlackholeDramSourceStable(io,baseline) && BlackholeResidentIdle(io);
		if (canRestore && configured) {
			for (unsigned attempt=0;attempt<2 && !r.niu_restored;attempt++) {
				for (unsigned w=0;w<10;w++) io.ResidentNocWrite(w*4,0);
				io.ResidentNocWrite(0x100,config);
				r.niu_restored=BlackholeResidentSourceStable(io,n,config);
				for (unsigned w=0;w<10;w++) r.niu_restored &= io.ResidentNocRead(w*4)==0;
			}
			canRestore=r.niu_restored!=0;
		}
		if (canRestore) for (unsigned attempt=0;attempt<2 && !r.l1_restored;attempt++) {
			for (unsigned w=0;w<2*words;w++) io.ResidentL1Write(w,saved[w]);
			r.l1_restored=io.ResetRead()==0x47800;
			for (unsigned w=0;w<2*words;w++) r.l1_restored &= io.ResidentL1Read(w)==saved[w];
		}
	}
	if (savedTlb) for (unsigned attempt=0;attempt<2 && !r.tlb_restored;attempt++) {
		for (unsigned s=0;s<2;s++) for (unsigned w=0;w<3;w++) io.DramTlbWrite(s,w,r.tlb_before[s][w]);
		r.tlb_restored=1;
		for (unsigned s=0;s<2;s++) for (unsigned w=0;w<3;w++) {
			r.tlb_after[s][w]=io.DramTlbRead(s,w); r.tlb_restored &= r.tlb_after[s][w]==r.tlb_before[s][w];
		}
	}
	if (!canRestore || !r.bme_restored || !r.l1_restored || !r.niu_restored || !r.tlb_restored) {
		r.recovery_required=1;
		if (r.state!=TT_HOST_READ_TIMEOUT && r.state!=TT_HOST_READ_CHANGED) r.state=TT_HOST_READ_RESTORE_FAILED;
	}
	if (r.page.allocated) {
		if (r.recovery_required) { io.QuarantineHostPage(); r.quarantined=1; }
		else {
			r.page.release_status=io.ReleaseHostPage(); r.page.released=r.page.release_status==0;
			if (!r.page.released) { r.page.state=TT_HOST_PAGE_RELEASE_FAILED; r.state=TT_HOST_READ_RESTORE_FAILED; }
		}
	}
	return r.state==TT_HOST_READ_PASSED && !r.recovery_required && r.page.released;
}
#endif
