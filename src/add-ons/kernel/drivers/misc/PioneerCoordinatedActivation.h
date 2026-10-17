/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_COORDINATED_ACTIVATION_H
#define PIONEER_COORDINATED_ACTIVATION_H
#include "PioneerBulkUpload.h"
#include "PioneerRuntimeMatvec.h"
// Fixed owned activation window, optionally compact for the separate image:
// signed integer overlays at80000, exact FP32 scales at83000, zero gaps.
// Uses the EXISTING distinct/pinned/noncacheable16KiB host owner. One
// acknowledged host READ replaces repeated4KiB BME enable/disable cycles.
// No new address/route/DMA permission, learned sum, RISC launch or reset.
static inline uint32_t PioneerCoordinatedActivationWord(unsigned word,unsigned inputs,
	const uint32_t* vector,const uint32_t* scales,bool compact=false)
{
	if (word<inputs/2) return vector[word];
	unsigned first=compact?inputs/2:3072;
	if (word>=first && word-first<inputs/32) return scales[word-first];
	return 0;
}
template<class IO>
unsigned PioneerCoordinatedActivationCompare(IO& io,unsigned inputs,const uint32_t* vector,const uint32_t* scales,bool compact=false)
{
	unsigned mismatch=0; io.BulkHostFence();
	unsigned words=compact?inputs/2+inputs/32:4096;
	for (unsigned w=0;w<words;w++) mismatch+=io.BulkHostLoad(w)!=PioneerCoordinatedActivationWord(w,inputs,vector,scales,compact);
	io.BulkHostFence(); return mismatch;
}
template<class IO>
bool PioneerCoordinatedActivation(IO& io,const tt_dram_samples_result& baseline,const tt_compute_noc_result& noc,
	const tt_host_page_result& input,PioneerResidentLease& store,PioneerOutputBufferLease& host,
	PioneerBulkUploadLease& bulk,PioneerRuntimeMatvecLease& compute,unsigned inputs,const uint32_t* vector,const uint32_t* scales,tt_bulk_upload_result& result,bool compact=false)
{
	result={}; auto& r=result.transfer; r.version=1; r.state=TT_STORE_CONFIG_FAILED;
	if (!vector || !scales || !PioneerQ8CompactShape(inputs,32) || io.RuntimeBufferWords()!=TT_Q8_BULK_SAVED_BUFFER_WORDS
		|| !compute.ready || !compute.saved || !compute.bulk || compute.buffer_words!=TT_Q8_BULK_SAVED_BUFFER_WORDS || compute.running || compute.unsafe
		|| !bulk.ready || bulk.unsafe || !host.ready || host.pending || host.quarantined || store.pending || store.recovery_required) return false;
	for (unsigned i=0;i<inputs/2;i++) if (!PioneerInt8OperandValid(uint16_t(vector[i])) || !PioneerInt8OperandValid(uint16_t(vector[i]>>16))) return false;
	for (unsigned i=0;i<inputs/32;i++) if ((scales[i]&0x7f800000)==0x7f800000) return false;
	unsigned words=compact?inputs/2+inputs/32:4096,bytes=words*4,pages=(bytes+4095)/4096;
	bool touched=false,passed=false;
	do {
		if (!PioneerBulkUploadGuard(io,baseline,noc,input,store,host,0x402)) { bulk.unsafe=true; break; }
		io.BulkHostFence();
		for (unsigned w=0;w<words;w++) io.BulkHostStore(w,PioneerCoordinatedActivationWord(w,inputs,vector,scales,compact));
		io.BulkHostFence();
		r.mismatches=PioneerCoordinatedActivationCompare(io,inputs,vector,scales,compact);
		if (r.mismatches) { r.state=TT_STORE_MISMATCH; break; }
		result.pages=pages; result.host_words=words; result.host_ready=result.l1_prepared=1;
		touched=true;
		io.HostPciWrite(1,4,2,0x406); if (io.HostPciRead(1,4,2)!=0x406) break;
		io.HostPciWrite(0,4,2,0x406);
		if (!PioneerBulkUploadGuard(io,baseline,noc,input,store,host,0x406)) { bulk.unsafe=true; break; }
		const uint32_t command[]={uint32_t(host.physical),uint32_t(host.physical>>32),19|(24<<6),0x80000,0,129,0,0,bytes,0};
		// The runtime buffer lease, not the bulk scratch, owns80000..84000.
		compute.buffers_dirty=true;
		r.wrote_device=1;
		if (!PioneerBulkUploadCommand(io,store,host,command,false,0,pages,r,compact?bytes:0)) break;
		if (!PioneerBulkUploadGuard(io,baseline,noc,input,store,host,0x406)) { bulk.unsafe=true; break; }
		r.mismatches=PioneerCoordinatedActivationCompare(io,inputs,vector,scales,compact);
		if (r.mismatches) { r.state=TT_STORE_CHANGED; bulk.unsafe=true; break; }
		passed=true;
	} while (false);
	if (touched) {
		bool off=false;
		for (unsigned attempt=0;attempt<2 && !off;attempt++) {
			io.HostPciWrite(0,4,2,0x402); io.HostPciWrite(1,4,2,0x402); off=BlackholeHostReadPciQuiet(io,0x402);
		}
		if (!off) bulk.unsafe=true;
	}
	if (passed && !PioneerBulkUploadGuard(io,baseline,noc,input,store,host,0x402)) bulk.unsafe=true;
	r.recovery_required=bulk.unsafe || store.recovery_required || store.pending || host.pending;
	if (r.recovery_required) {
		bulk.unsafe=true; compute.unsafe=true; store.recovery_required=true; io.QuarantineHostPage(); PioneerQuarantineOutputBuffer(io,host);
	}
	if (passed && !r.recovery_required) r.state=TT_STORE_PASSED;
	return passed && r.state==TT_STORE_PASSED && r.requests==1 && r.completed==1 && !r.recovery_required;
}
#endif
