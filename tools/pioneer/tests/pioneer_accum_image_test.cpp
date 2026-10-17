/* SPDX-License-Identifier: MIT */
#define PIONEER_PERSISTENT_MACHINE_ONLY
#include "pioneer_persistent_image_test.cpp"
struct AccumMachine:PersistentMachine {
	unsigned cycle=10,lastWrite[64]={},loads=0,stores=0;
	AccumMachine(const std::vector<uint8_t>& image,unsigned k,unsigned n,unsigned w):PersistentMachine(k,n,w) {
		Phase(); assert(image.size()==4096);
		for (unsigned i=0;i<1024;i++) l1[1+i]=Word(image,i*4);
		l1[Index(TT_RUNTIME_Q8_PARK_PC)]=TT_PERSISTENT_MATH_RETURN;
		l1[Index(0x70500)]=base; l1[Index(0x70504)]=inputs/16; l1[Index(0x70508)]=outputs/16;
		l1[Index(0x7050c)]=w!=0; l1[Index(0x70510)]=PioneerWorkerX(w)|(PioneerWorkerY(w)<<6);
	}
	bool CodeAddress(uint32_t a) const override { return (a>=0x70d80 && a<0x70ea8) || PersistentMachine::CodeAddress(a); }
	void Instruction(uint32_t op) override {
		cycle++;
		if (op==0xa2101816) { cycle+=5; for (auto& written:lastWrite) written=0; }
		unsigned code=op>>24;
		if ((code==0x70 || code==0x72) && ((op>>16)&15)==3) {
			unsigned block=((op&1023)&~3U)/8; assert(block<64);
			if (code==0x70) { assert(!lastWrite[block] || cycle-lastWrite[block]>4); loads++; }
			else { lastWrite[block]=cycle; stores++; }
		}
		PersistentMachine::Instruction(op);
	}
	void Verify() {
		assert(!fidelityPhase && !ownerA && !ownerB && CorrectIeee() && Guards() && loads && stores);
		for (unsigned n:phaseVisits) assert(n*4==matrixInstructions);
	}
};
int main(int argc,char** argv) {
	assert(argc==2); auto code=Read(argv[1]); assert(code.size()==4096);
	for (unsigned word=0;word<1024;word++) assert(Word(code,word*4)==BlackholeRuntimeQ8AccumCodeWord(word));
	// The candidate changes ONLY the reserved exact-accumulation control
	// block;ALL public math/status/STOP PCs and arithmetic constants stay.
	for (unsigned byte=0;byte<4096;byte+=4) if ((byte<0x600 || byte>=0x680) && (byte<0xd80 || byte>=0xea8)) assert(Word(code,byte)==BlackholeRuntimeQ8PhaseCodeWord(byte/4));
	for (unsigned worker=0;worker<120;worker++) {
		AccumMachine m(code,1024,32,worker); m.Boot();
		for (unsigned trial:{0U,1U,9U}) { m.Job(trial); m.Verify(); } m.Stop();
	}
	for (unsigned k:{1024U,2048U,3072U,5120U,6144U}) { AccumMachine m(code,k,64,119); m.Boot(); m.Job(2); m.Verify(); m.Job(3); m.Verify(); m.Stop(); }
	AccumMachine full(code,1024,4096,1); full.Boot(); full.Job(0); full.Verify(); full.Job(1); full.Verify(); full.Stop();
	AccumMachine proposed(code,1024,32,1); proposed.Boot(); proposed.Job(0); proposed.Verify();
	PersistentMachine previous(1024,32,1); previous.Phase(); previous.Boot(); previous.Job(0);
	assert(proposed.steps<previous.steps && proposed.drains<previous.drains && proposed.matrixInstructions==previous.matrixInstructions);
	printf("PASS OFFLINE exact accumulation pipeline,all120 peers/all K/N4096/IEEE output bits/repeated jobs/STOP,ALL SFPU Dst block hazards spaced>4 cycles,unchanged public PCs;steps%u vs%u,drains%u vs%u;NOT native performance\n",proposed.steps,previous.steps,proposed.drains,previous.drains);
}
