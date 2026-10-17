/* SPDX-License-Identifier: MIT */
// Separate RN multiply/add oracle,not an IEEE fused-FMA claim.
#define PIONEER_ACCUM_MACHINE_ONLY
#include "pioneer_accum_image_test.cpp"
#include "../../../src/add-ons/kernel/drivers/misc/BlackholeRuntimeQ8RoundedImage.h"
struct RoundedProductMachine:AccumMachine {
	RoundedProductMachine(const std::vector<uint8_t>& code,unsigned k,unsigned n,unsigned w):AccumMachine(code,k,n,w) {}
	bool CorrectExpected() const override {
		if (conditional) return false;
		for (unsigned row=0;row<outputs;row++) {
			float expected=0;
			for (unsigned block=0;block<inputs/32;block++) {
				int dot=0; for (unsigned i=0;i<32;i++) dot+=NativeWeight(row,block*32+i)*Input(block*32+i,trial);
				float scale=Half(WeightScale(row,block))*InputScale(block,trial);
				float product=float(dot)*scale;
				expected=expected+product; // independent two IEEE operations,ffp-contract=off
			}
			if (Bits(expected)!=buffers[4096+row]) return false;
		}
		return true;
	}
	void Check() {
		assert(!fidelityPhase && !ownerA && !ownerB && CorrectExpected() && Guards() && loads && stores);
		for (unsigned n:phaseVisits) assert(n*4==matrixInstructions);
	}
};
int main(int argc,char** argv) {
	assert(argc==2); auto code=Read(argv[1]); assert(code.size()==4096);
	for (unsigned word=0;word<1024;word++) assert(Word(code,word*4)==BlackholeRuntimeQ8RoundedCodeWord(word));
	// Only the private accumulation dispatch changes;all public code,
	// source/NoC operations,return/status/STOP PCs and constants are fixed.
	for (unsigned byte=0;byte<4096;byte+=4) if (byte<0xd80 || byte>=0xea8)
		assert(Word(code,byte)==BlackholeRuntimeQ8AccumCodeWord(byte/4));
	for (unsigned worker=0;worker<120;worker++) {
		RoundedProductMachine m(code,1024,32,worker); m.Boot();
		for (unsigned trial:{0U,1U,9U}) { m.Job(trial); m.Check(); } m.Stop();
	}
	for (unsigned k:{1024U,2048U,3072U,5120U,6144U}) {
		RoundedProductMachine m(code,k,64,119); m.Boot();
		for (unsigned trial:{2U,3U}) { m.Job(trial); m.Check(); } m.Stop();
	}
	RoundedProductMachine full(code,1024,4096,1); full.Boot(); full.Job(0); full.Check(); full.Job(1); full.Check(); full.Stop();
	RoundedProductMachine next(code,1024,32,1); next.Boot(); next.Job(0); next.Check();
	PersistentMachine prior(1024,32,1); prior.Accum(); prior.Boot(); prior.Job(0);
	assert(next.steps<prior.steps && next.matrixInstructions==prior.matrixInstructions && next.drains==prior.drains);
	printf("PASS OFFLINE actual separate-RN multiply/add math,all120 peers/K1024..6144/N4096/repeated inputs/STOP,independent output oracle/unchanged public PCs/source proofs/Dst hazards;steps%u vs%u;NOT full-model quality/native performance\n",next.steps,prior.steps);
}
