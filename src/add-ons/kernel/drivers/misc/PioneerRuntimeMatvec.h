/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_RUNTIME_MATVEC_H
#define PIONEER_RUNTIME_MATVEC_H
#include "BlackholeRuntimeMatvecImage.h"
#include "PioneerResidentStore.h"
#include "PioneerRuntimeOutput.h"
enum { TT_RUNTIME_REFUSED=0,TT_RUNTIME_PASSED=1,TT_RUNTIME_CONFIG_FAILED=2,TT_RUNTIME_STAGE_FAILED=3,
	TT_RUNTIME_START_FAILED=4,TT_RUNTIME_COMPLETION_FAILED=5,TT_RUNTIME_NONFINITE=6,TT_RUNTIME_COLD=7,TT_RUNTIME_READ_FAILED=8 };
struct tt_runtime_matvec_result {
	uint32_t version,state,prepared,started,completed,stopped,recovery_required,context_reset;
	uint32_t staged_pages,polls,phase,tiles,marker,pc[2],before[4],after[4],nonfinite;
};
struct PioneerRuntimeMatvecLease {
	bool saved=false,code_dirty=false,buffers_dirty=false,ready=false,running=false,unsafe=false;
};
template<class IO>
bool PioneerRuntimeSources(IO& io,const tt_dram_samples_result& baseline,const tt_compute_noc_result& noc,const PioneerResidentLease& store)
{
	bool stable=true;
	for (unsigned w=0;w<16;w++) stable &= io.ResidentNocRead(BlackholeDramSourceOffset(w))==(w?noc.source[0][w]:store.config|0x10000);
	return stable
		&& BlackholeDramSourceStable(io,baseline) && BlackholeHostReadPciQuiet(io,0x402);
}
template<class IO>
bool PioneerRuntimeMatvecPrepare(IO& io,const tt_dram_samples_result& baseline,const tt_compute_noc_result& noc,
	const PioneerResidentLease& store,PioneerRuntimeMatvecLease& compute)
{
	if (compute.ready) return true;
	uint32_t* saved=io.RuntimeSavedWords();
	if (!saved || compute.saved || !store.ready || store.pending || store.recovery_required || io.ResetRead()!=0x47800
		|| !PioneerRuntimeSources(io,baseline,noc,store) || !BlackholeResidentIdle(io)) return false;
	for (unsigned i=0;i<3;i++) {
		uint32_t value=io.RuntimeControlRead(i);
		if ((i==0 && (value&1)) || (i!=0 && value)) return false;
	}
	for (unsigned w=0;w<=1024;w++) saved[w]=io.RuntimeCodeRead(w);
	for (unsigned w=0;w<8192;w++) saved[1025+w]=io.RuntimeBufferRead(w);
	compute.saved=true;
	if (io.ResetRead()!=0x47800 || !PioneerRuntimeSources(io,baseline,noc,store) || !BlackholeResidentIdle(io)) {
		compute.unsafe=true; return false;
	}
	compute.code_dirty=true;
	for (unsigned w=0;w<1024;w++) io.RuntimeCodeWrite(1+w,BlackholeRuntimeMatvecCodeWord(w));
	io.RuntimeCodeWrite(0,0x0006a06f);
	bool valid=io.RuntimeCodeRead(0)==0x0006a06f;
	for (unsigned w=0;w<1024;w++) valid &= io.RuntimeCodeRead(1+w)==BlackholeRuntimeMatvecCodeWord(w);
	compute.ready=valid; return valid;
}
template<class IO>
bool PioneerRuntimeMatvecClose(IO& io,const tt_dram_samples_result& baseline,const tt_compute_noc_result& noc,
	PioneerResidentLease& store,PioneerRuntimeMatvecLease& compute)
{
	if (!compute.saved) return !compute.unsafe;
	bool safe=!compute.unsafe && !compute.running && !store.pending && !store.recovery_required
		&& io.ResetRead()==0x47800 && PioneerRuntimeSources(io,baseline,noc,store) && BlackholeResidentIdle(io);
	uint32_t* saved=io.RuntimeSavedWords();
	if (safe && compute.code_dirty) for (unsigned attempt=0;attempt<2 && compute.code_dirty;attempt++) {
		for (unsigned w=1;w<=1024;w++) io.RuntimeCodeWrite(w,saved[w]);
		io.RuntimeCodeWrite(0,saved[0]);
		bool restored=true;
		for (unsigned w=0;w<=1024;w++) restored &= io.RuntimeCodeRead(w)==saved[w];
		compute.code_dirty=!restored;
	}
	if (safe && !compute.code_dirty && compute.buffers_dirty) for (unsigned attempt=0;attempt<2 && compute.buffers_dirty;attempt++) {
		for (unsigned w=0;w<8192;w++) io.RuntimeBufferWrite(w,saved[1025+w]);
		bool restored=true;
		for (unsigned w=0;w<8192;w++) restored &= io.RuntimeBufferRead(w)==saved[1025+w];
		compute.buffers_dirty=!restored;
	}
	compute.ready=false; compute.unsafe |= !safe || compute.code_dirty || compute.buffers_dirty;
	if (compute.unsafe) store.recovery_required=true;
	return !compute.unsafe;
}
template<class IO>
bool PioneerRuntimeStage(IO& io,const tt_dram_samples_result& baseline,const tt_compute_noc_result& noc,
	const tt_host_page_result& host,PioneerResidentLease& store,bool output,unsigned page,
	const uint32_t* data,tt_runtime_matvec_result& r)
{
	if ((!output && page>=3) || (output && page>=4)) return false;
	if (!PioneerRuntimeSources(io,baseline,noc,store) || !BlackholeResidentIdle(io) || !BlackholeHostReadMapStable(io,host)) {
		store.recovery_required=true; return false;
	}
	for (unsigned w=0;w<1024;w++) io.HostPageWrite(w,data[w]);
	for (unsigned w=0;w<1024;w++) if (io.HostPageRead(w)!=data[w]) return false;
	bool touched=true,passed=false;
	do {
		io.HostPciWrite(1,4,2,0x406); if (io.HostPciRead(1,4,2)!=0x406) break;
		io.HostPciWrite(0,4,2,0x406); if (!BlackholeHostReadPciQuiet(io,0x406)) break;
		uint32_t destination=(output?0x84000:0x80000)+page*4096;
		const uint32_t command[]={uint32_t(host.physical),uint32_t(host.physical>>32),19|(24<<6),destination,0,1|(2<<6),0,0,4096,0};
		tt_resident_store_result transfer={};
		passed=PioneerResidentStoreCommand(io,store,command,false,0,transfer);
	} while (false);
	if (touched) {
		bool off=false;
		for (unsigned attempt=0;attempt<2 && !off;attempt++) {
			io.HostPciWrite(0,4,2,0x402); io.HostPciWrite(1,4,2,0x402);
			off=BlackholeHostReadPciQuiet(io,0x402);
		}
		if (!off) store.recovery_required=true;
	}
	if (passed && (!PioneerRuntimeSources(io,baseline,noc,store) || !BlackholeHostReadMapStable(io,host))) {
		store.recovery_required=true; passed=false;
	}
	if (passed) r.staged_pages++;
	return passed && !store.pending && !store.recovery_required;
}
template<class IO>
bool PioneerRuntimeMatvec(IO& io,const tt_dram_samples_result& baseline,const tt_compute_noc_result& noc,
	const tt_host_page_result& host,PioneerResidentLease& store,PioneerRuntimeMatvecLease& compute,
	uint64_t capacity,uint64_t offset,unsigned inputs,unsigned outputs,const uint32_t* input,uint32_t* output,
	tt_runtime_matvec_result& r,bool dmaOutput=false)
{
	r={}; r.version=1;
	if (!PioneerMatvecWeightRange(capacity,offset,inputs,outputs) || !input || !output || !compute.ready
		|| !store.ready || store.pending || store.recovery_required || compute.unsafe) return false;
	for (unsigned w=0;w<inputs/2;w++) if ((input[w]&0x7f80)==0x7f80 || (input[w]&0x7f800000)==0x7f800000) return false;
	uint32_t* stage=io.RuntimeStageWords(); if (!stage) return false;
	r.prepared=1; r.state=TT_RUNTIME_STAGE_FAILED;
	compute.buffers_dirty=true;
	bool okay=true;
	for (unsigned page=0;page<(inputs*2+4095)/4096 && okay;page++) {
		for (unsigned w=0;w<1024;w++) stage[w]=page*1024+w<inputs/2?input[page*1024+w]:0;
		okay=PioneerRuntimeStage(io,baseline,noc,host,store,false,page,stage,r);
	}
	for (unsigned page=0;page<(outputs*4+4095)/4096 && okay;page++) {
		for (unsigned w=0;w<1024;w++) stage[w]=0x7fc00000;
		okay=PioneerRuntimeStage(io,baseline,noc,host,store,true,page,stage,r);
	}
	if (!okay) goto finished;
	r.state=TT_RUNTIME_CONFIG_FAILED;
	io.RuntimeCodeWrite(1+0x500/4,PioneerResidentAddress(capacity,offset));
	io.RuntimeCodeWrite(1+0x504/4,inputs/16); io.RuntimeCodeWrite(1+0x508/4,outputs/16);
	if (io.RuntimeCodeRead(1+0x500/4)!=PioneerResidentAddress(capacity,offset)
		|| io.RuntimeCodeRead(1+0x504/4)!=inputs/16 || io.RuntimeCodeRead(1+0x508/4)!=outputs/16) goto finished;
	for (unsigned w=0;w<3;w++) io.RuntimeStatusWrite(w,0);
	{
		const uint32_t command[]={PioneerResidentAddress(capacity,offset),0,18|(12<<6),0x70700,0,1|(2<<6),0,0,576,0};
		if (!BlackholeHostWriteCommand(io,command) || io.ResetRead()!=0x47800 || !PioneerRuntimeSources(io,baseline,noc,store)) goto finished;
	}
	r.state=TT_RUNTIME_START_FAILED; r.context_reset=1;
	io.RuntimeResetWrite(0x87ffff); io.Pause();
	if (io.ResetRead()!=0x87ffff || io.ResetRead()!=0x87ffff || !PioneerRuntimeSources(io,baseline,noc,store)
		|| !BlackholeResidentIdle(io)) goto finished;
	for (unsigned w=0;w<4;w++) r.before[w]=io.ResidentNocRead(0x208+w*4);
	r.started=1; compute.running=true; r.state=TT_RUNTIME_COMPLETION_FAILED;
	io.RuntimeResetWrite(0x46800);
	for (unsigned attempt=0;attempt<50000;attempt++) {
		r.polls++;
		if (io.ResetRead()!=0x46800) break;
		r.phase=io.RuntimeStatusRead(0); r.tiles=io.RuntimeStatusRead(1); r.marker=io.RuntimeStatusRead(2);
		if (r.marker==0x4e4f4346) break;
		if (r.phase==TT_RUNTIME_MATVEC_FINAL_PHASE && r.tiles==(inputs/16)*(outputs/16) && r.marker==0x4d415431) {
			io.Pause(); r.pc[0]=io.RuntimePcRead(); io.Pause(); r.pc[1]=io.RuntimePcRead();
			bool idle=BlackholeResidentIdle(io);
			for (unsigned w=0;w<4;w++) r.after[w]=io.ResidentNocRead(0x208+w*4);
			if (idle && r.pc[0]==TT_RUNTIME_MATVEC_PARK_PC && r.pc[1]==TT_RUNTIME_MATVEC_PARK_PC
				&& uint32_t(r.after[0]-r.before[0])==r.tiles && uint32_t(r.after[1]-r.before[1])==r.tiles*9
				&& uint32_t(r.after[2]-r.before[2])==r.tiles && uint32_t(r.after[3]-r.before[3])==r.tiles) {
				r.completed=1; break;
			}
		}
		io.Pause();
	}
	if (!r.completed || !PioneerRuntimeSources(io,baseline,noc,store) || io.ResetRead()!=0x46800 || !BlackholeResidentIdle(io)) goto finished;
	if (!dmaOutput) for (unsigned w=0;w<outputs;w++) {
		output[w]=io.RuntimeBufferRead(4096+w);
		r.nonfinite+=(output[w]&0x7f800000)==0x7f800000;
	}
	io.RuntimeResetWrite(0x87ffff); io.Pause();
	if (io.ResetRead()!=0x87ffff || io.ResetRead()!=0x87ffff) goto finished;
	io.RuntimeResetWrite(0x47800); io.Pause();
	r.stopped=io.ResetRead()==0x47800 && io.ResetRead()==0x47800;
	if (r.stopped) compute.running=false;
	if (r.stopped && dmaOutput) {
		// No host DMA while compute is running. Preserve the existing PIO
		// operation as an independent native comparison path.
		r.state=TT_RUNTIME_READ_FAILED;
		for (unsigned page=0;page<(outputs+1023)/1024;page++) {
			unsigned words=outputs-page*1024<1024?outputs-page*1024:1024;
			tt_resident_stream_result transfer={};
			if (!PioneerRuntimeOutputPage(io,baseline,noc,host,store,page,words,stage,output+page*1024,transfer)) goto finished;
		}
		for (unsigned w=0;w<outputs;w++) r.nonfinite+=(output[w]&0x7f800000)==0x7f800000;
	}
	r.state=!r.stopped?TT_RUNTIME_COLD:r.nonfinite?TT_RUNTIME_NONFINITE:TT_RUNTIME_PASSED;
finished:
	if (!r.started && r.context_reset && io.ResetRead()==0x87ffff && io.ResetRead()==0x87ffff) {
		io.RuntimeResetWrite(0x47800); io.Pause();
	}
	if (store.pending || store.recovery_required || (r.started && (!r.completed || !r.stopped))
		|| (!r.started && io.ResetRead()!=0x47800)) {
		compute.unsafe=true; store.recovery_required=true; r.recovery_required=1;
		io.QuarantineHostPage();
	}
	return r.state==TT_RUNTIME_PASSED && !r.recovery_required;
}
#endif
