/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_RUNTIME_OUTPUT_H
#define PIONEER_RUNTIME_OUTPUT_H
#include "PioneerResidentStream.h"

// Fixed output L1 -> the session's sole owned host page. Call only after
// verified runtime completion AND verified reset hold. Never expose a caller
// address or treat a PCIe posted-write ACK alone as host visibility/order.
// This candidate is deliberately separate from the native-qualified upload.
template<class IO>
bool PioneerRuntimeOutputPage(IO& io,const tt_dram_samples_result& baseline,const tt_compute_noc_result& noc,
	const tt_host_page_result& host,PioneerResidentLease& lease,unsigned page,unsigned words,
	uint32_t* snapshot,uint32_t* output,tt_resident_stream_result& r)
{
	r={}; r.version=1;
	if (page>=4 || !words || words>1024 || !snapshot || !output || !lease.ready || lease.pending
		|| lease.recovery_required || host.state!=TT_HOST_PAGE_READY || !host.memory_type_set || host.released) return false;
	r.state=TT_STORE_CONFIG_FAILED;
	const uint32_t poisonRead[]={uint32_t(host.physical),uint32_t(host.physical>>32),19|(24<<6),0x17e000,0,1|(2<<6),0,0,4096,0};
	const uint32_t toHost[]={0x84000+page*4096,0,1|(2<<6),uint32_t(host.physical),uint32_t(host.physical>>32),19|(24<<6),0,0x212,4096,0};
	const uint32_t orderedRead[]={uint32_t(host.physical),uint32_t(host.physical>>32),19|(24<<6),0x17f000,0,1|(2<<6),0,0,4096,0};
	bool bmeTouched=false;
	do {
		if (!PioneerResidentStreamGuard(io,baseline,noc,host,lease,0x402)) { lease.recovery_required=true; break; }
		lease.l1_dirty=true; lease.l1_restored=false;
		// Every finite FP32 result differs from this poison. Padding was
		// explicitly staged as 0x7fc00000 before computation. A remaining
		// all-ones word is ambiguous, even if the NoC reports completion.
		for (unsigned w=0;w<1024;w++) io.HostPageWrite(w,0xffffffff);
		for (unsigned w=0;w<1024;w++) r.mismatches+=io.HostPageRead(w)!=0xffffffff;
		if (r.mismatches) { r.state=TT_STORE_MISMATCH; break; }
		bmeTouched=true;
		io.HostPciWrite(1,4,2,0x406); if (io.HostPciRead(1,4,2)!=0x406) break;
		io.HostPciWrite(0,4,2,0x406);
		if (!PioneerResidentStreamGuard(io,baseline,noc,host,lease,0x406)) { lease.recovery_required=true; break; }
		if (!PioneerResidentStreamCommand(io,lease,poisonRead,false,0,r)) break;
		if (!PioneerResidentStreamGuard(io,baseline,noc,host,lease,0x406)) { lease.recovery_required=true; break; }
		if (!PioneerResidentStreamCommand(io,lease,toHost,true,4,r)) break;
		for (unsigned attempt=0;attempt<256;attempt++) {
			r.mismatches=0;
			for (unsigned w=0;w<1024;w++) {
				uint32_t value=io.HostPageRead(w);
				r.mismatches+=w<words?value==0xffffffff:value!=0x7fc00000;
			}
			if (!r.mismatches) { r.host_visible=1; break; }
			io.Pause();
		}
		if (!r.host_visible) { r.state=TT_STORE_MISMATCH; lease.recovery_required=true; break; }
		for (unsigned w=0;w<1024;w++) snapshot[w]=io.HostPageRead(w);
		if (!PioneerResidentStreamGuard(io,baseline,noc,host,lease,0x406)) { lease.recovery_required=true; break; }
		// The fresh read is ordered after the posted write, with a distinct
		// destination, while all compute RISCs remain held. Do not rewrite
		// payloads, reset a core, or free this page after uncertain completion.
		if (!PioneerResidentStreamCommand(io,lease,orderedRead,false,5,r)) break;
		r.readback_complete=1;
		if (!PioneerResidentStreamGuard(io,baseline,noc,host,lease,0x406)) { lease.recovery_required=true; break; }
		for (unsigned w=0;w<1024;w++) {
			uint32_t value=io.HostPageRead(w);
			// Counter completion and rereading the HOST page do not prove
			// what arrived in the fresh L1 destination, or that the original
			// result survived the write unchanged. This conservative baseline
			// checks all three byte streams, padding included. A future device-
			// side exact comparison can remove these slow payload PIO reads;
			// it must not replace them with a CRC or weaken this equality gate.
			uint32_t returned=io.ResidentL1Read(1024+w);
			uint32_t original=io.RuntimeBufferRead(4096+page*1024+w);
			r.mismatches+=value!=snapshot[w] || returned!=snapshot[w] || original!=snapshot[w]
				|| (w<words?value==0xffffffff:value!=0x7fc00000);
		}
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
	bool passed=r.state==TT_STORE_PASSED && !r.recovery_required;
	if (passed) for (unsigned w=0;w<words;w++) output[w]=snapshot[w];
	return passed;
}
#endif
