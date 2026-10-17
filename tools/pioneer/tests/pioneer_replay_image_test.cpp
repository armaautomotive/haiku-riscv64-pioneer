/* SPDX-License-Identifier: MIT */
#define PIONEER_ACCUM_MACHINE_ONLY
#include "pioneer_accum_image_test.cpp"
struct ReplayMachine:AccumMachine {
	unsigned buffer[32]={},remaining=0,recordAt=0,records=0,replays=0,expanded=0; bool ready=false;
	ReplayMachine(const std::vector<uint8_t>& code,unsigned k,unsigned n,unsigned w):AccumMachine(code,k,n,w) {}
	void Instruction(uint32_t op) override {
		if ((op>>24)==4) {
			assert(!remaining && ((op>>14)&1023)==0 && ((op>>4)&1023)==32);
			if (op&1) { assert(op==0x04000203); records++; remaining=32; recordAt=0; ready=false; return; }
			assert(op==0x04000200 && ready); replays++;
			for (unsigned word:buffer) { AccumMachine::Instruction(word); expanded++; }
			return;
		}
		if (remaining) {
			assert((op>>24)>=0x70 && (op>>24)<=0x90 && recordAt<32); buffer[recordAt++]=op;
			if (!--remaining) ready=true;
		}
		AccumMachine::Instruction(op);
	}
	void VerifyReplay() {
		Verify(); assert(!remaining && ready && records==jobs && replays && expanded==replays*32);
		for (unsigned i=0;i<32;i++) { uint32_t encoded=l1[Index(0x70ec4+i*4)]; assert(buffer[i]==((encoded>>2)|(encoded<<30))); }
	}
};
int main(int argc,char** argv) {
	assert(argc==2); auto code=Read(argv[1]); assert(code.size()==4096);
	for (unsigned byte=0;byte<4096;byte+=4) if (byte<0xd80 || byte>=0xea8) assert(Word(code,byte)==BlackholeRuntimeQ8AccumCodeWord(byte/4));
	for (unsigned worker=0;worker<120;worker++) { ReplayMachine m(code,1024,32,worker); m.Boot();
		for (unsigned trial:{0U,1U,9U}) { m.Job(trial); m.VerifyReplay(); } m.Stop(); }
	for (unsigned k:{1024U,2048U,3072U,5120U,6144U}) { ReplayMachine m(code,k,64,119); m.Boot(); m.Job(2); m.VerifyReplay(); m.Job(3); m.VerifyReplay(); m.Stop(); }
	ReplayMachine full(code,1024,4096,1); full.Boot(); full.Job(0); full.VerifyReplay(); full.Job(1); full.VerifyReplay(); full.Stop();
	ReplayMachine next(code,1024,32,1); next.Boot(); next.Job(0); next.VerifyReplay();
	std::vector<uint8_t> original(4096); for (unsigned i=0;i<1024;i++) { uint32_t word=BlackholeRuntimeQ8AccumCodeWord(i); for (unsigned b=0;b<4;b++) original[i*4+b]=uint8_t(word>>(b*8)); }
	AccumMachine previous(original,1024,32,1); previous.Boot(); previous.Job(0); previous.Verify();
	assert(next.matrixInstructions==previous.matrixInstructions && next.drains==previous.drains && next.steps<previous.steps);
	printf("PASS OFFLINE fixed32-slot replay,all120 peers/all K/N4096/repeated IEEE outputs/STOP,one fresh recording per matrix job,immutable arithmetic prefix/current operands/Dst hazards/unchanged public PCs;steps%u vs%u,replays%u;NOT native performance\n",next.steps,previous.steps,next.replays);
}
