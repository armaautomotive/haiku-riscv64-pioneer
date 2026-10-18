/* SPDX-License-Identifier: MIT */
#define PIONEER_Q38_DISPATCH_MACHINE_ONLY
#include "pioneer_qwen38_dispatch_image_test.cpp"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38DispatchProof.h"
#include <memory>
struct ProofFixture {
	DispatchMachine machine;
	std::unique_ptr<tt_q38_dispatch_probe> job{new tt_q38_dispatch_probe{}};
	std::unique_ptr<PioneerQ38DispatchAuthority> authority{new PioneerQ38DispatchAuthority};
	ProofFixture(const char* image,unsigned peers,unsigned type,unsigned k):machine(image,peers,type,k) {
		auto& j=*job; auto& a=*authority;
		j.version=1; j.type=type; j.inputs=k; j.peers=peers; a.nextRoot=1;
		for (unsigned c=0;c<8;c++) a.before[c]=machine.counts[c==6?2:c];
		assert(machine.RunRoot());
		for (unsigned c=0;c<8;c++) a.after[c]=machine.counts[c==6?2:c];
		for (unsigned c=0;c<16;c++) a.source[0][c]=c?0x12340000+c:0x16005;
		for (unsigned w=0;w<k/2;w++) j.activation[w]=machine.memory.at(0xa0000+w*4);
		for (unsigned b=0;b<k/PioneerQ38CohortBlockInputs(type);b++) j.scales[b]=machine.memory.at(0xab000+b*4);
		auto& r=j.result; r.version=1; r.state=TT_RUNTIME_PASSED; r.peers=r.started=r.completed=r.waiting=peers;
		r.root_sources_verified=r.nonce=r.root_started=r.root_completed=r.root_held=1;
		r.root_phase=0x7200c; r.root_count=peers; r.root_marker=0x314d3851; r.root_pc[0]=r.root_pc[1]=0x72038;
		for (unsigned c=0;c<8;c++) { r.before[c]=a.before[c]; r.after[c]=a.after[c]; }
		for (unsigned i=0;i<64;i++) r.stats[i]=machine.memory.at(0x7f800+i*4);
		for (unsigned w=0;w<peers;w++) {
			const unsigned base=0x74080+w*32;
			j.part[w]={machine.memory.at(base+4),machine.memory.at(base+8),uint64_t(machine.memory.at(base))-0x04000000};
			a.nextPeer[w]=1; auto& e=a.extent[w]; e.address=machine.memory.at(base); e.route=machine.memory.at(base+16);
			e.blocks=j.part[w].groups*(k/PioneerQ38CohortBlockInputs(type)); e.bytes=e.blocks*PioneerQ38PanelProbeBytes(type);
			e.reads=e.blocks+(k*2+16383)/16384+1; e.flits=e.bytes/64+(k*2+((k/PioneerQ38CohortBlockInputs(type)*4+63)&~63U))/64;
			for (unsigned c=0;c<16;c++) a.source[w+1][c]=machine.memory.at(0x7d800+w*64+c*4);
			for (unsigned i=0;i<64;i++) r.peer[w][i]=machine.memory.at(0x76000+w*256+i*4);
			for (unsigned row=0;row<256;row++) j.output[w][row]=machine.memory.at(0x80000+w*1024+row*4);
		}
		assert(PioneerQ38DispatchProof(j,a));
	}
};
int main(int argc,char** argv)
{
	assert(argc==2);
	for (unsigned peers:{1U,2U,7U,119U}) for (unsigned type:{8U,12U,14U}) {
		ProofFixture f(argv[1],peers,type,type==12?17408:5120);
		auto& j=*f.job; auto& a=*f.authority;
		j.result.root_held=0; assert(PioneerQ38DispatchProof(j,a,false) && !PioneerQ38DispatchProof(j,a));
		j.result.root_held=1; assert(!PioneerQ38DispatchProof(j,a,false) && PioneerQ38DispatchProof(j,a));
		for (unsigned i=0;i<64;i++) { j.result.stats[i]^=1; assert(!PioneerQ38DispatchProof(j,a)); j.result.stats[i]^=1; }
		for (unsigned w=0;w<peers;w++) {
			for (unsigned i=0;i<64;i++) { j.result.peer[w][i]^=1; assert(!PioneerQ38DispatchProof(j,a)); j.result.peer[w][i]^=1; }
			for (unsigned c=0;c<16;c++) { a.source[w+1][c]^=1; assert(!PioneerQ38DispatchProof(j,a)); a.source[w+1][c]^=1; }
			uint32_t before=j.output[w][j.part[w].groups*16-1]; j.output[w][j.part[w].groups*16-1]=0x7f800000;
			assert(!PioneerQ38DispatchProof(j,a)); j.output[w][j.part[w].groups*16-1]=before;
			if (j.part[w].groups<16) { j.output[w][255]=1; assert(!PioneerQ38DispatchProof(j,a)); j.output[w][255]=0; }
		}
		for (unsigned c=0;c<8;c++) { a.after[c]++; assert(!PioneerQ38DispatchProof(j,a)); a.after[c]--; }
		j.result.root_sources_verified=2; assert(!PioneerQ38DispatchProof(j,a)); j.result.root_sources_verified=1;
		j.result.recovery_required=1; assert(!PioneerQ38DispatchProof(j,a)); j.result.recovery_required=0;
		a.nextRoot++; assert(!PioneerQ38DispatchProof(j,a)); a.nextRoot--;
		assert(PioneerQ38DispatchProof(j,a));
	}
	puts("PASS OFFLINE NEW persistent-WAIT proof against actual dispatcher:1/2/7/119 peers xQ8/Q4/Q6,root pre-HOLD/final-HOLD distinct,ALL64 stats/ALL64 perpeer fields/ALL16 private source changes,physical counter wraps/mismatches/nonces/bools/nonfinite/unused output refuse;NO stopped fabrication/FD/launch/native authority");
}
