/* SPDX-License-Identifier: MIT */
#define PIONEER_RESIDENT_MOCK_ONLY
#include "pioneer_resident_math_probe_test.cpp"
#include <vector>
struct GemvMock:Mock {
	BlackholeResidentGemvProgram program;
	std::vector<uint32_t> extendedSaved,extendedGddr;
	explicit GemvMock(unsigned inputs):program(inputs),
		extendedSaved(TT_RESIDENT_MATH_IMAGE_WORDS+(program.Valid()?program.GddrWords():0)),
		extendedGddr(program.Valid()?program.GddrWords():0) {
		for (unsigned w=0;w<extendedGddr.size();w++) extendedGddr[w]=0x54320000+w;
	}
	uint32_t* ResidentMathSavedWords() { return noHeap?nullptr:extendedSaved.data(); }
	uint32_t ResidentGddrRead(unsigned w) { assert(w<extendedGddr.size()); Window(0,0x04000000,18,12); return extendedGddr[w]; }
	void ResidentGddrWrite(unsigned w,uint32_t value) {
		assert(w<extendedGddr.size()); Window(0,0x04000000,18,12);
		if (started) { assert(stopped); payloadAfterStart++; }
		if (++writes!=drop && !(gddrRestoreFails && value==0x54320000+w)) extendedGddr[w]=value;
	}
	void MathResetWrite(uint32_t value) {
		Window(1,0xffa00000,1,2);
		assert(value==0x87ffff || value==0x46800 || value==0x47800);
		if (started) { assert(!timeout && !badFlits && !badPc && !changed); stopWrites++; }
		if (++writes==drop) return;
		reset=value;
		if (value==0x46800) {
			assert(!started && config==0x16005); started=true; starts++;
			for (unsigned w=0;w<10;w++) assert(command[w]==program.CommandWord(w));
			for (unsigned w=0;w<program.GddrWords();w++) assert(extendedGddr[w]==program.GddrWord(w));
			for (unsigned w=0;w<TT_RESIDENT_MATH_IMAGE_WORDS;w++) assert(image[w]==program.ImageWord(w));
			if (!timeout) {
				counters[0]+=program.Requests(); counters[1]+=program.Flits()-(badFlits?1:0);
				counters[2]+=program.Requests(); counters[3]+=program.Requests();
				image[TT_RESIDENT_MATH_STATUS_INDEX]=program.FinalPhase();
				image[TT_RESIDENT_MATH_STATUS_INDEX+1]=program.Fixtures();
				image[TT_RESIDENT_MATH_STATUS_INDEX+2]=0x4d415431;
				for (unsigned w=0;w<program.OutputWords();w++) image[1+TT_RESIDENT_MATH_CODE_WORDS+w]=program.ExpectedWord(w)^(badData?1:0);
			}
		}
		if (started && value==0x47800) stopped=true;
	}
	uint32_t MathPcRead() { assert(started); pcReads++; return badPc?0:program.ParkPc(); }
	void RestoredGemv() {
		Mock::Restored();
		for (unsigned w=0;w<extendedGddr.size();w++) assert(extendedGddr[w]==0x54320000+w);
	}
};
int main()
{
	for (unsigned inputs:{1024U,5120U,6144U}) {
		GemvMock io(inputs); auto b=io.Baseline(); auto n=io.Compute(); tt_resident_math_result r;
		assert(BlackholeResidentMathProbe(io,b,n,r,io.program));
		assert(r.math.version==28 && r.math.completed && r.math.stopped && !r.recovery_required); io.RestoredGemv();
	}
	GemvMock successful(1024); auto b=successful.Baseline(); auto n=successful.Compute(); tt_resident_math_result r;
	assert(BlackholeResidentMathProbe(successful,b,n,r,successful.program));
	unsigned totalWrites=successful.writes;
	// Exhaustive tiny-operation write faults are covered by the shared-policy
	// matrix test. Here sample the larger transfer's boundaries and interior,
	// including the full setup/tail, without quadratic all-word re-execution.
	for (unsigned drop=1;drop<=totalWrites;drop++) {
		if (drop>600 && drop<totalWrites-3000 && drop%97!=0) continue;
		GemvMock io(1024); b=io.Baseline(); n=io.Compute(); io.drop=drop;
		BlackholeResidentMathProbe(io,b,n,r,io.program); assert(io.starts<=1);
		if (!r.recovery_required) io.RestoredGemv();
		if (r.math.started && !r.math.completed) assert(r.recovery_required && io.stopWrites==0 && io.payloadAfterStart==0);
	}
	for (unsigned fault=0;fault<6;fault++) {
		GemvMock io(1024); b=io.Baseline(); n=io.Compute();
		io.timeout=fault==0; io.badFlits=fault==1; io.badPc=fault==2;
		io.changed=fault==3; io.gddrRestoreFails=fault==4; io.l1RestoreFails=fault==5;
		assert(!BlackholeResidentMathProbe(io,b,n,r,io.program) && r.recovery_required);
		if (fault<4) assert(io.stopWrites==0 && io.payloadAfterStart==0);
	}
	GemvMock invalid(16); b=invalid.Baseline(); n=invalid.Compute();
	assert(!BlackholeResidentMathProbe(invalid,b,n,r,invalid.program) && invalid.writes==0);
	printf("PASS: fixed GEMV shape bounds, larger resident upload/rollback, sampled write loss and NoC/stop/source failure gates; NO hardware operation\n");
}
