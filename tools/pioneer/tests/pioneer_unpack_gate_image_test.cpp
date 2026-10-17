/* SPDX-License-Identifier: MIT */
// OFFLINE producer/consumer scheduling model, not hardware timing evidence.
#define PIONEER_ACCUM_MACHINE_ONLY
#include "pioneer_accum_image_test.cpp"
struct UnpackGateMachine:AccumMachine {
	bool producer[2]={},waitGate=true;
	unsigned delayA=0,delayB=0,dropProducer=0;
	unsigned deadline[2]={},unpackBank[2]={},publishedBank[2]={},matrixBank=0;
	unsigned publications[2]={},sourceWaits=0;
	float stagedA[16][16]={},stagedB[8][16]={};
	UnpackGateMachine(const std::vector<uint8_t>& code,unsigned k,unsigned n,unsigned w):AccumMachine(code,k,n,w) {}
	void Advance() {
		for (unsigned i=0;i<2;i++) if (producer[i] && cycle>=deadline[i]) {
			if (!i) { memcpy(a,stagedA,sizeof(a)); ownerA=true; }
			else { memcpy(b,stagedB,sizeof(b)); ownerB=true; }
			producer[i]=false; publications[i]++;
		}
	}
	void FinishProducers() {
		for (unsigned i=0;i<2;i++) if (producer[i]) cycle=std::max(cycle,deadline[i]);
		Advance();
	}
	void Instruction(uint32_t op) override {
		Advance();
		if (op==0x420000c1 || op==0x428000c1) {
			unsigned i=op==0x428000c1;
			assert(!producer[i] && !(i?ownerB:ownerA));
			if (dropProducer==i+1) return;
			float oldA[16][16],oldB[8][16];
			if (!i) memcpy(oldA,a,sizeof(a)); else memcpy(oldB,b,sizeof(b));
			AccumMachine::Instruction(op);
			if (!i) { memcpy(stagedA,a,sizeof(a)); memcpy(a,oldA,sizeof(a)); ownerA=false; }
			else { memcpy(stagedB,b,sizeof(b)); memcpy(b,oldB,sizeof(b)); ownerB=false; }
			producer[i]=true; deadline[i]=cycle+(i?delayB:delayA);
			publishedBank[i]=unpackBank[i]; unpackBank[i]^=1;
			return;
		}
		bool matrix=(op&~0x4000U)==0x26000020 || (op&~0x4000U)==0x26c00020;
		if (matrix) {
			// Architectural MVMUL Wait Gate, not a guessed cycle delay.
			if (producer[0] || producer[1]) {
				if (!waitGate) throw uint32_t(0x53524346);
				sourceWaits++; FinishProducers();
			}
			if (!ownerA || !ownerB) throw uint32_t(0x53524346);
			assert(publishedBank[0]==matrixBank && publishedBank[1]==matrixBank);
		}
		if (op==0xa2101816) FinishProducers(); // retained full drain
		AccumMachine::Instruction(op);
		if (matrix && (op&~0x4000U)==0x26c00020) matrixBank^=1;
	}
	void Store(uint32_t address,uint32_t value) override {
		// A pending unpack owns its L1 input and source-configuration spans.
		bool input=(address>=0x88000 && address<0x8c000)
			|| (address>=0x70910 && address<0x70940)
			|| (address>=0xffef0000 && address<0xffef0400);
		if (input) assert(!producer[0] && !producer[1]);
		AccumMachine::Store(address,value);
	}
	void Check() {
		Verify();
		assert(!producer[0] && !producer[1] && !matrixBank && !unpackBank[0] && !unpackBank[1]);
		assert(publications[0]*4==matrixInstructions && publications[1]==publications[0]);
	}
};
int main(int argc,char** argv) {
	assert(argc==2); auto code=Read(argv[1]); assert(code.size()==4096);
	unsigned differences=0;
	for (unsigned byte=0;byte<4096;byte+=4) if (Word(code,byte)!=BlackholeRuntimeQ8AccumCodeWord(byte/4)) {
		assert(byte==0x370 && Word(code,byte)==0x00000013);
		assert((BlackholeRuntimeQ8AccumCodeWord(byte/4)&0xfff)==0xef); differences++;
	}
	assert(differences==1); // ALL other PCs, math/constants/drains/NoC bytes identical
	for (unsigned worker=0;worker<120;worker++) for (unsigned schedule=0;schedule<4;schedule++) {
		UnpackGateMachine m(code,1024,32,worker);
		m.delayA=schedule==1?1:schedule==2?127:schedule==3?1000003:0;
		m.delayB=schedule==1?127:schedule==2?1:schedule==3?17:0;
		m.Boot(); for (unsigned trial:{0U,1U,9U}) { m.Job(trial); m.Check(); } m.Stop();
		if (schedule) assert(m.sourceWaits);
	}
	for (unsigned k:{1024U,2048U,3072U,5120U,6144U}) {
		UnpackGateMachine m(code,k,64,119); m.delayA=997; m.delayB=4099;
		m.Boot(); for (unsigned trial:{2U,3U}) { m.Job(trial); m.Check(); } m.Stop();
	}
	UnpackGateMachine full(code,1024,4096,1); full.delayA=65539; full.delayB=7;
	full.Boot(); full.Job(0); full.Check(); full.Job(1); full.Check(); full.Stop();
	for (unsigned fault=0;fault<3;fault++) {
		UnpackGateMachine m(code,1024,32,1); m.delayA=65539; m.delayB=32771;
		m.waitGate=fault!=0; m.dropProducer=fault; m.Boot(); bool refused=false;
		try { m.Job(1); } catch(uint32_t marker) { refused=marker==0x53524346; }
		// Tile0 retains the AddrMod-setup full drain. With the architectural
		// gate removed, tile1 must fail; a missing producer fails at tile0.
		assert(refused && !m.outputReads && m.matrixInstructions==(fault?0U:4U));
		for (unsigned row=0;row<4096;row++) assert(m.buffers[4096+row]==0x7fc00000);
	}
	UnpackGateMachine next(code,1024,32,1); next.Boot(); next.Job(0); next.Check();
	PersistentMachine prior(1024,32,1); prior.Accum(); prior.Boot(); prior.Job(0);
	assert(next.drains+next.matrixInstructions/4==prior.drains && next.steps<prior.steps);
	printf("PASS OFFLINE actual ONE-word unpack gate candidate,all120 peers/four asynchronous A/B publication schedules/all K/N4096/IEEE bits/repeated jobs/STOP/Dst hazards;missing gate/source refusals before output,ALL other bytes unchanged;drains%u vs%u steps%u vs%u;NOT native timing or hardware qualification\n",next.drains,prior.drains,next.steps,prior.steps);
}
