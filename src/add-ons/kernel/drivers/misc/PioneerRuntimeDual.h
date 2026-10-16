/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_RUNTIME_DUAL_H
#define PIONEER_RUNTIME_DUAL_H
#include "PioneerRuntimeMatvec.h"
#include "PioneerRuntimeActivationCache.h"

// Two fixed workers, disjoint contiguous row ranges. Never divide the
// reduction dimension: each output keeps the qualified FP32 summation order.
static inline bool PioneerRuntimeDualShape(unsigned inputs,unsigned outputs)
{
	return outputs>=64 && !(outputs&63) && PioneerQ8CompactShape(inputs,outputs);
}
static inline bool PioneerRuntimeDualProof(const tt_runtime_matvec_result (&parts)[2],unsigned inputs,unsigned outputs)
{
	if (!PioneerRuntimeDualShape(inputs,outputs)) return false;
	unsigned tiles=(inputs/16)*(outputs/32),reads=tiles/32;
	for (const auto& r:parts) if (r.version!=1 || r.state!=TT_RUNTIME_PASSED || !r.prepared || !r.context_reset
		|| !r.started || !r.completed || !r.stopped || r.recovery_required || r.nonfinite
		|| r.phase!=TT_RUNTIME_Q8_FINAL_PHASE || r.tiles!=tiles || r.marker!=0x4d415431
		|| r.pc[0]!=TT_RUNTIME_Q8_PARK_PC || r.pc[1]!=r.pc[0]
		|| uint32_t(r.after[0]-r.before[0])!=reads || uint32_t(r.after[1]-r.before[1])!=reads*TT_Q8_BULK_FLITS
		|| uint32_t(r.after[2]-r.before[2])!=reads || uint32_t(r.after[3]-r.before[3])!=reads) return false;
	return true;
}
template<class IO>
bool PioneerRuntimeDualMatvec(IO& io,const tt_dram_samples_result& baseline,
	const tt_compute_noc_result& firstNoc,const tt_compute_noc_result& secondNoc,
	const tt_host_page_result& host,PioneerResidentLease& firstStore,PioneerResidentLease& secondStore,
	PioneerRuntimeMatvecLease& firstCompute,PioneerRuntimeMatvecLease& secondCompute,
	uint64_t capacity,uint64_t offset,unsigned inputs,unsigned outputs,const uint32_t* input,
	const uint32_t* scales,uint32_t* output,tt_runtime_matvec_result (&r)[2],PioneerRuntimeActivationCache* cache=NULL)
{
	for (unsigned i=0;i<2;i++) { r[i]={}; r[i].version=1; }
	if (!PioneerRuntimeDualShape(inputs,outputs) || !input || !scales || !output
		|| !PioneerQ8CompactWeightRange(capacity,offset,inputs,outputs)
		|| firstNoc.version!=1 || secondNoc.version!=2) return false;
	unsigned rows=outputs/2;
	uint64_t offsets[2]={offset,offset+PioneerQ8CompactWeightBytes(inputs,rows)};
	const tt_compute_noc_result* noc[2]={&firstNoc,&secondNoc};
	PioneerResidentLease* store[2]={&firstStore,&secondStore};
	PioneerRuntimeMatvecLease* compute[2]={&firstCompute,&secondCompute};
	for (unsigned i=0;i<2;i++) {
		if (!PioneerQ8CompactWeightRange(capacity,offsets[i],inputs,rows)
			|| !compute[i]->ready || !compute[i]->bulk || compute[i]->unsafe || compute[i]->running
			|| !store[i]->ready || store[i]->pending || store[i]->recovery_required) return false;
	}
	for (unsigned w=0;w<inputs/2;w++)
		if (!PioneerInt8OperandValid(uint16_t(input[w])) || !PioneerInt8OperandValid(uint16_t(input[w]>>16))) return false;
	for (unsigned w=0;w<inputs/32;w++) if ((scales[w]&0x7f800000)==0x7f800000) return false;
	uint32_t* stage=io.RuntimeStageWords(); if (!stage) return false;
	bool uncertain=false,passed=false,armed[2]={false,false},hits[2]={false,false};
	do {
		if (cache) {
			// A matching CPU shadow is insufficient. Check BOTH workers and
			// the shared host/PCI route before any transfer or cache hit.
			for (unsigned i=0;i<2;i++) {
				if (!io.SelectRuntimeCore(i) || io.ResetRead()!=0x47800 || io.ResetRead()!=0x47800
					|| !PioneerRuntimeSources(io,baseline,*noc[i],*store[i]) || !BlackholeResidentIdle(io)
					|| !BlackholeHostReadMapStable(io,host)) { uncertain=true; break; }
				hits[i]=cache[i].Matches(inputs,input,scales);
				if (!hits[i] && !cache[i].Remember(inputs,input,scales)) { uncertain=true; break; }
			}
			if (uncertain) break;
		}
		// Stage/configure BOTH held cores before releasing either. All host
		// DMA is complete, BME off, before any worker can execute.
		for (unsigned i=0;i<2;i++) {
			if (!io.SelectRuntimeCore(i)) { uncertain=true; break; }
			auto& result=r[i]; result.prepared=1; result.state=TT_RUNTIME_STAGE_FAILED;
			compute[i]->buffers_dirty=true;
			bool okay=io.ResetRead()==0x47800;
			for (unsigned page=0;page<(inputs*2+4095)/4096 && okay && !hits[i];page++) {
				for (unsigned w=0;w<1024;w++) stage[w]=page*1024+w<inputs/2?input[page*1024+w]:0;
				okay=PioneerRuntimeStage(io,baseline,*noc[i],host,*store[i],false,page,stage,result,false,2+i);
			}
			if (okay && !hits[i]) {
				for (unsigned w=0;w<1024;w++) stage[w]=w<inputs/32?scales[w]:0;
				okay=PioneerRuntimeStage(io,baseline,*noc[i],host,*store[i],false,3,stage,result,true,2+i);
			}
			for (unsigned page=0;page<(rows*4+4095)/4096 && okay;page++) {
				for (unsigned w=0;w<1024;w++) stage[w]=0x7fc00000;
				okay=PioneerRuntimeStage(io,baseline,*noc[i],host,*store[i],true,page,stage,result,false,2+i);
			}
			if (!okay) break;
			result.state=TT_RUNTIME_CONFIG_FAILED;
			uint32_t address=PioneerResidentAddress(capacity,offsets[i]);
			io.RuntimeCodeWrite(1+0x500/4,address);
			io.RuntimeCodeWrite(1+0x504/4,inputs/16); io.RuntimeCodeWrite(1+0x508/4,rows/16);
			if (io.RuntimeCodeRead(1+0x500/4)!=address || io.RuntimeCodeRead(1+0x504/4)!=inputs/16
				|| io.RuntimeCodeRead(1+0x508/4)!=rows/16) break;
			store[i]->l1_dirty=true; store[i]->l1_restored=false;
			for (unsigned w=0;w<3;w++) io.RuntimeStatusWrite(w,0);
			const uint32_t command[]={address,0,18|(12<<6),TT_Q8_BULK_L1,0,1|((2+i)<<6),0,0,TT_Q8_BULK_BYTES,0};
			if ((command[0]&31)!=(command[3]&31) || !BlackholeHostWriteCommand(io,command)
				|| io.ResetRead()!=0x47800 || !PioneerRuntimeSources(io,baseline,*noc[i],*store[i])) break;
			result.state=TT_RUNTIME_START_FAILED; result.context_reset=1;
			io.RuntimeResetWrite(0x87ffff); io.Pause();
			if (io.ResetRead()!=0x87ffff || io.ResetRead()!=0x87ffff
				|| !PioneerRuntimeSources(io,baseline,*noc[i],*store[i]) || !BlackholeResidentIdle(io)) break;
			for (unsigned w=0;w<4;w++) result.before[w]=io.ResidentNocRead(0x208+w*4);
			armed[i]=true;
		}
		if (uncertain || !armed[0] || !armed[1]) break;
		io.RuntimeProfileMark(TT_PROFILE_STAGED);
		io.RuntimeProfileMark(TT_PROFILE_LAUNCHED);
		for (unsigned i=0;i<2;i++) {
			if (!io.SelectRuntimeCore(i)) { uncertain=true; break; }
			if (io.ResetRead()!=0x87ffff || !BlackholeResidentIdle(io)
				|| !PioneerRuntimeSources(io,baseline,*noc[i],*store[i])) { uncertain=true; break; }
			r[i].started=1; compute[i]->running=true; r[i].state=TT_RUNTIME_COMPLETION_FAILED;
			io.RuntimeResetWrite(0x46800);
			if (io.ResetRead()!=0x46800 || !PioneerRuntimeSources(io,baseline,*noc[i],*store[i])) { uncertain=true; break; }
		}
		if (uncertain) break;
		for (unsigned attempt=0;attempt<50000 && !(r[0].completed && r[1].completed);attempt++) {
			for (unsigned i=0;i<2;i++) {
				if (r[i].completed) continue;
				if (!io.SelectRuntimeCore(i)) { uncertain=true; break; }
				auto& result=r[i]; result.polls++;
				if (io.ResetRead()!=0x46800) { uncertain=true; break; }
				result.phase=io.RuntimeStatusRead(0); result.tiles=io.RuntimeStatusRead(1); result.marker=io.RuntimeStatusRead(2);
				if (result.marker==0x4e4f4346) { uncertain=true; break; }
				if (result.phase==TT_RUNTIME_Q8_FINAL_PHASE && result.tiles==(inputs/16)*(rows/16) && result.marker==0x4d415431) {
					io.Pause(); result.pc[0]=io.RuntimePcRead(); io.Pause(); result.pc[1]=io.RuntimePcRead();
					unsigned reads=result.tiles/32;
					for (unsigned w=0;w<4;w++) result.after[w]=io.ResidentNocRead(0x208+w*4);
					if (result.pc[0]==TT_RUNTIME_Q8_PARK_PC && result.pc[1]==result.pc[0] && BlackholeResidentIdle(io)
						&& uint32_t(result.after[0]-result.before[0])==reads
						&& uint32_t(result.after[1]-result.before[1])==reads*TT_Q8_BULK_FLITS
						&& uint32_t(result.after[2]-result.before[2])==reads
						&& uint32_t(result.after[3]-result.before[3])==reads) result.completed=1;
				}
			}
			if (uncertain) break;
			if (!(r[0].completed && r[1].completed)) io.Pause();
		}
		if (uncertain || !r[0].completed || !r[1].completed) break;
		// Both completion proofs must be known before collecting ANY output
		// or stopping either peer. A partial result is never accepted.
		for (unsigned i=0;i<2;i++) {
			if (!io.SelectRuntimeCore(i) || io.ResetRead()!=0x46800 || !BlackholeResidentIdle(io)
				|| !PioneerRuntimeSources(io,baseline,*noc[i],*store[i])) { uncertain=true; break; }
		}
		if (uncertain) break;
		io.RuntimeProfileMark(TT_PROFILE_COMPLETED);
		for (unsigned i=0;i<2;i++) {
			if (!io.SelectRuntimeCore(i)) { uncertain=true; break; }
			for (unsigned w=0;w<rows;w++) {
				uint32_t value=io.RuntimeBufferRead(4096+w); output[i*rows+w]=value;
				r[i].nonfinite+=(value&0x7f800000)==0x7f800000;
			}
		}
		if (uncertain) break;
		io.RuntimeProfileMark(TT_PROFILE_PIO);
		for (unsigned i=0;i<2;i++) {
			if (!io.SelectRuntimeCore(i) || io.ResetRead()!=0x46800 || !BlackholeResidentIdle(io)
				|| !PioneerRuntimeSources(io,baseline,*noc[i],*store[i])) { uncertain=true; break; }
			io.RuntimeResetWrite(0x87ffff); io.Pause();
			if (io.ResetRead()!=0x87ffff || io.ResetRead()!=0x87ffff) { uncertain=true; break; }
			io.RuntimeResetWrite(0x47800); io.Pause();
			if (io.ResetRead()!=0x47800 || io.ResetRead()!=0x47800) { uncertain=true; break; }
			r[i].stopped=1; compute[i]->running=false;
		}
		if (uncertain) break;
		io.RuntimeProfileMark(TT_PROFILE_STOPPED);
		for (unsigned i=0;i<2;i++) r[i].state=r[i].nonfinite?TT_RUNTIME_NONFINITE:TT_RUNTIME_PASSED;
		passed=!r[0].nonfinite && !r[1].nonfinite;
	} while (false);
	for (unsigned i=0;i<2;i++) uncertain |= store[i]->pending || store[i]->recovery_required
		|| compute[i]->unsafe || (r[i].started && (!r[i].completed || !r[i].stopped));
	if (!uncertain && !r[0].started && !r[1].started) {
		// A known pre-launch setup refusal may leave an armed, but never
		// released, context. No such rollback is allowed after uncertainty.
		uint32_t reset[2]={};
		for (unsigned i=0;i<2;i++) {
			if (!io.SelectRuntimeCore(i)) { uncertain=true; break; }
			reset[i]=io.ResetRead();
			if (!PioneerRuntimeSources(io,baseline,*noc[i],*store[i]) || !BlackholeResidentIdle(io)
				|| (reset[i]!=0x47800 && reset[i]!=0x87ffff)) { uncertain=true; break; }
		}
		for (unsigned i=0;i<2 && !uncertain;i++) {
			if (!io.SelectRuntimeCore(i)) { uncertain=true; break; }
			if (reset[i]==0x87ffff) {
				io.RuntimeResetWrite(0x47800); io.Pause();
				if (io.ResetRead()!=0x47800 || io.ResetRead()!=0x47800) { uncertain=true; break; }
			}
		}
	}
	if (uncertain) {
		for (unsigned i=0;i<2;i++) { compute[i]->unsafe=true; store[i]->recovery_required=true; r[i].recovery_required=1; }
		io.QuarantineHostPage();
	}
	if (cache) {
		if (passed && !uncertain) for (unsigned i=0;i<2;i++) if (!hits[i] && !cache[i].Seal(r[i],rows)) { passed=false; r[i].state=TT_RUNTIME_CONFIG_FAILED; }
		if (!passed || uncertain) for (unsigned i=0;i<2;i++) cache[i].Invalidate();
	}
	return passed && !uncertain;
}
#endif
