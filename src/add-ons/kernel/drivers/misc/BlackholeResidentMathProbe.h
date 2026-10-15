/* SPDX-License-Identifier: MIT */
#ifndef BLACKHOLE_RESIDENT_MATH_PROBE_H
#define BLACKHOLE_RESIDENT_MATH_PROBE_H
#include "BlackholeResidentReadProbe.h"
#include "BlackholeResidentPrograms.h"
#include "BlackholeTensixMatmulProbe.h"
struct tt_resident_math_result {
	uint32_t version,gddr_restored,niu_restored,recovery_required,stage;
	uint32_t before[4],after[4];
	tt_tensix_matmul_result math;
};
template<class IO>
bool BlackholeResidentMathNocStable(IO& io,const tt_compute_noc_result& n,uint32_t config)
{
	bool okay=true;
	for (unsigned w=0;w<16;w++) okay &= io.ResidentNocRead(BlackholeDramSourceOffset(w))==(w==0?config:n.source[0][w]);
	return okay;
}
// Fixed, shape-bounded numerical proofs; no arbitrary executable/descriptor API.
// GDDR stays resident for the complete operation; TRISC0 issues its reads.
// Saved original GDDR and L1 contents remain kernel-local. Compute context
// is explicitly reset as in the earlier tile proof, not saved/restored.
template<class IO,class Program=BlackholeResidentMatrixProgram>
bool BlackholeResidentMathProbe(IO& io,tt_dram_samples_result& baseline,
	const tt_compute_noc_result& n,tt_resident_math_result& result,Program program=Program())
{
	result={}; result.version=1; result.gddr_restored=result.niu_restored=1;
	tt_tensix_matmul_result& r=result.math;
	r.version=program.Version(); r.l1_restored=r.tlb_restored=1;
	uint32_t* saved=io.ResidentMathSavedWords();
	if (!program.Valid() || saved==0 || baseline.version!=1 || baseline.state!=TT_DRAM_SAMPLE_ROUTES_OK
		|| baseline.routes_verified!=7 || !baseline.data_restored || !baseline.tlb_restored || baseline.recovery_required
		|| n.version!=1 || n.state!=TT_COMPUTE_NOC_PASSED || !n.tlb_restored || n.recovery_required
		|| !BlackholeComputeNocRouteValid(n.source[0]) || !BlackholeDramBuildRoutes(baseline)) return false;
	uint32_t* gddrSaved=saved+program.ImageWords();
	for (unsigned s=0;s<2;s++) for (unsigned w=0;w<3;w++) {
		r.tlb_before[s][w]=io.DramTlbRead(s,w);
		if (w==2 && (r.tlb_before[s][w]&0xffffc000)!=0) return false;
	}
	r.tlb_restored=0;
	uint32_t config=n.source[0][0];
	bool gddrModified=false,l1Modified=false,niuModified=false;
	do {
		result.stage=1; r.state=TT_MATMUL_CORE_CHANGED;
		baseline.cores_checked=0;
		if (!BlackholeDramAllCoresHeld(io,baseline)) break;
		r.state=TT_MATMUL_CONFIG_FAILED;
		if (!BlackholeDramConfigure(io,1,0xffa00000,1,2,false)) break;
		if (!BlackholeDramSourceStable(io,baseline) || !BlackholeResidentMathNocStable(io,n,config)
			|| !BlackholeResidentIdle(io) || (io.ResidentNocRead(0x178)&0xffff)!=0) break;
		bool pristine=true;
		for (unsigned w=0;w<10;w++) pristine &= io.ResidentNocRead(w*4)==0;
		for (unsigned w=0;w<3;w++) r.controls[w]=io.MathControlRead(w);
		if (!pristine || (r.controls[0]&1)!=0 || r.controls[1]!=0 || r.controls[2]!=0) break;
		result.stage=2;
		if (!BlackholeDramConfigure(io,0,0x04000000,baseline.translated[0][0],baseline.translated[0][1],false)) break;
		for (unsigned w=0;w<program.GddrWords();w++) gddrSaved[w]=io.ResidentGddrRead(w);
		if (io.ResetRead()!=0x47800 || !BlackholeDramSourceStable(io,baseline)) break;
		gddrModified=true; result.gddr_restored=0;
		for (unsigned w=0;w<program.GddrWords();w++) io.ResidentGddrWrite(w,program.GddrWord(w));
		bool uploaded=true;
		for (unsigned w=0;w<program.GddrWords();w++) uploaded &= io.ResidentGddrRead(w)==program.GddrWord(w);
		if (!uploaded) break;
		result.stage=3;
		if (!BlackholeDramConfigure(io,0,0,1,2,false)) break;
		for (unsigned w=0;w<program.ImageWords();w++) saved[w]=io.MathImageRead(w);
		r.reset_before=io.ResetRead();
		if (r.reset_before!=0x47800 || !BlackholeResidentMathNocStable(io,n,config)) break;
		l1Modified=true; r.l1_restored=0;
		for (unsigned w=1;w<program.ImageWords();w++) io.MathImageWrite(w,program.ImageWord(w));
		io.MathImageWrite(0,program.ImageWord(0));
		r.state=TT_MATMUL_IMAGE_FAILED;
		bool image=true;
		for (unsigned w=0;w<program.ImageWords();w++) image &= io.MathImageRead(w)==program.ImageWord(w);
		if (!image) break;
		result.stage=4; r.state=TT_MATMUL_CONFIG_FAILED;
		if (io.ResetRead()!=0x47800 || !BlackholeResidentIdle(io)) break;
		niuModified=true; result.niu_restored=0;
		io.ResidentNocWrite(0x100,config|0x10000);
		for (unsigned w=0;w<10;w++) io.ResidentNocWrite(w*4,program.CommandWord(w));
		bool ready=BlackholeResidentMathNocStable(io,n,config|0x10000);
		for (unsigned w=0;w<10;w++) ready &= io.ResidentNocRead(w*4)==program.CommandWord(w);
		if (!ready || !BlackholeDramSourceStable(io,baseline) || io.ResetRead()!=0x47800) break;
		result.stage=5; r.context_reset=1; r.state=TT_MATMUL_CORE_CHANGED;
		io.MathResetWrite(0x87ffff); io.Pause();
		r.reset_initialized=io.ResetRead();
		if (r.reset_initialized!=0x87ffff || io.ResetRead()!=0x87ffff) break;
		ready=BlackholeResidentMathNocStable(io,n,config|0x10000) && BlackholeResidentIdle(io);
		for (unsigned w=0;w<10;w++) ready &= io.ResidentNocRead(w*4)==program.CommandWord(w);
		if (!ready || !BlackholeDramSourceStable(io,baseline)) break;
		for (unsigned w=0;w<4;w++) result.before[w]=io.ResidentNocRead(0x208+w*4);
		result.stage=6; r.started=1; r.state=TT_MATMUL_COMPLETION_FAILED;
		io.MathResetWrite(0x46800);
		for (unsigned attempt=0;attempt<2000;attempt++) {
			r.polls++; r.reset_running=io.ResetRead();
			if (r.reset_running!=0x46800) { r.state=TT_MATMUL_CORE_CHANGED; break; }
			r.phase=io.MathImageRead(program.StatusIndex());
			r.fixtures=io.MathImageRead(program.StatusIndex()+1);
			r.marker=io.MathImageRead(program.StatusIndex()+2);
			if (r.marker==0x4e4f4346) break; // device's bounded read timeout
			if (r.phase==program.FinalPhase() && r.fixtures==program.Fixtures() && r.marker==0x4d415431) {
				io.Pause(); r.pc[0]=io.MathPcRead(); io.Pause(); r.pc[1]=io.MathPcRead();
				// Doorbell readback before counters even though the RISC issued it.
				bool quiet=BlackholeResidentIdle(io);
				for (unsigned w=0;w<4;w++) result.after[w]=io.ResidentNocRead(0x208+w*4);
				if (quiet && r.pc[0]==program.ParkPc() && r.pc[1]==program.ParkPc()
					&& uint32_t(result.after[0]-result.before[0])==program.Requests()
					&& uint32_t(result.after[1]-result.before[1])==program.Flits()
					&& uint32_t(result.after[2]-result.before[2])==program.Requests()
					&& uint32_t(result.after[3]-result.before[3])==program.Requests()) { r.completed=1; break; }
			}
			io.Pause();
		}
		// Unlike the old no-NoC tile test, NEVER assert reset on timeout:
		// an on-card request could still be writing L1 after the core parks.
		if (!r.completed) break;
		result.stage=7;
		if (!BlackholeDramSourceStable(io,baseline) || !BlackholeResidentMathNocStable(io,n,config|0x10000)
			|| io.ResetRead()!=0x46800 || !BlackholeResidentIdle(io)) { r.state=TT_MATMUL_SOURCE_CHANGED; break; }
		for (unsigned w=0;w<program.OutputWords();w++) {
			uint32_t value=io.MathImageRead(1+program.CodeWords()+w);
			if (w==0 || w==128) r.representative[w/128]=value;
			r.mismatches += value!=program.ExpectedWord(w);
		}
		io.MathResetWrite(0x87ffff); io.Pause();
		if (io.ResetRead()!=0x87ffff || io.ResetRead()!=0x87ffff) { r.state=TT_MATMUL_STOP_FAILED; break; }
		io.MathResetWrite(0x47800); io.Pause(); r.reset_after=io.ResetRead();
		r.stopped=r.reset_after==0x47800 && io.ResetRead()==0x47800;
		r.state=!r.stopped?TT_MATMUL_STOP_FAILED:r.mismatches?TT_MATMUL_RESULT_MISMATCH:TT_MATMUL_PASSED;
	} while (false);

	if (gddrModified || l1Modified || niuModified) {
	bool canRestore=!r.started || (r.completed && r.stopped);
	// A pre-start backend reset may be returned to the original all-held
	// mask, but never retry a start/stop after uncertain device execution.
	if (canRestore && !r.started && r.context_reset && io.ResetRead()==0x87ffff && io.ResetRead()==0x87ffff) {
		io.MathResetWrite(0x47800); io.Pause();
	}
	canRestore=canRestore && io.ResetRead()==0x47800 && BlackholeDramSourceStable(io,baseline)
		&& BlackholeResidentMathNocStable(io,n,niuModified?config|0x10000:config) && BlackholeResidentIdle(io);
	if (canRestore && niuModified) {
		for (unsigned attempt=0;attempt<2 && !result.niu_restored;attempt++) {
			for (unsigned w=0;w<10;w++) io.ResidentNocWrite(w*4,0);
			io.ResidentNocWrite(0x100,config);
			result.niu_restored=BlackholeResidentMathNocStable(io,n,config);
			for (unsigned w=0;w<10;w++) result.niu_restored &= io.ResidentNocRead(w*4)==0;
		}
		canRestore=result.niu_restored;
	}
	if (canRestore && l1Modified) {
		canRestore=BlackholeDramConfigure(io,0,0,1,2,false);
		if (canRestore) for (unsigned attempt=0;attempt<2 && !r.l1_restored;attempt++) {
			for (unsigned w=1;w<program.ImageWords();w++) io.MathImageWrite(w,saved[w]);
			io.MathImageWrite(0,saved[0]);
			r.l1_restored=io.ResetRead()==0x47800;
			for (unsigned w=0;w<program.ImageWords();w++) r.l1_restored &= io.MathImageRead(w)==saved[w];
		}
		canRestore=canRestore && r.l1_restored;
	}
	if (canRestore && gddrModified) {
		canRestore=io.ResetRead()==0x47800 && BlackholeDramSourceStable(io,baseline)
			&& BlackholeResidentMathNocStable(io,n,config)
			&& BlackholeDramConfigure(io,0,0x04000000,baseline.translated[0][0],baseline.translated[0][1],false);
		if (canRestore) for (unsigned attempt=0;attempt<2 && !result.gddr_restored;attempt++) {
			for (unsigned w=0;w<program.GddrWords();w++) io.ResidentGddrWrite(w,gddrSaved[w]);
			result.gddr_restored=io.ResetRead()==0x47800;
			for (unsigned w=0;w<program.GddrWords();w++) result.gddr_restored &= io.ResidentGddrRead(w)==gddrSaved[w];
		}
	}
	}
	for (unsigned attempt=0;attempt<2 && !r.tlb_restored;attempt++) {
		for (unsigned s=0;s<2;s++) for (unsigned w=0;w<3;w++) io.DramTlbWrite(s,w,r.tlb_before[s][w]);
		r.tlb_restored=1;
		for (unsigned s=0;s<2;s++) for (unsigned w=0;w<3;w++) {
			r.tlb_after[s][w]=io.DramTlbRead(s,w); r.tlb_restored &= r.tlb_after[s][w]==r.tlb_before[s][w];
		}
	}
	if (!result.gddr_restored || !result.niu_restored || !r.l1_restored || !r.tlb_restored
		|| (r.started && (!r.completed || !r.stopped))) result.recovery_required=r.recovery_required=1;
	if (r.state==TT_MATMUL_PASSED && result.recovery_required) r.state=TT_MATMUL_RESTORE_FAILED;
	return r.state==TT_MATMUL_PASSED && r.started && r.completed && r.stopped && !result.recovery_required;
}
#endif
