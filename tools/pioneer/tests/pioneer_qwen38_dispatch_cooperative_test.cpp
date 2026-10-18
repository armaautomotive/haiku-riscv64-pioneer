/* SPDX-License-Identifier: MIT */
#ifdef PIONEER_Q38_VECTOR_COOPERATIVE
#define PIONEER_Q38_VECTOR_PERSISTENT_IMAGE
#endif
#define PIONEER_Q38_PERSISTENT_ROWS_MACHINE_ONLY
#define PIONEER_Q38_REF_DIAGNOSTIC
#define PIONEER_Q38_REFERENCE_ROUNDING_BOUND
#include "pioneer_qwen38_persistent_rows_image_test.cpp"
#ifdef PIONEER_Q38_VECTOR_COOPERATIVE
#define PIONEER_Q38_VECTOR_HAS_ROWS_MACHINE
#define PIONEER_Q38_VECTOR_ROWS_MODEL_ONLY
#include "pioneer_q38_vector_rows_image_test.cpp"
#ifdef PIONEER_Q38_TEST_ACCUMULATOR
using CooperativePeer=VectorRows<PersistentRows,true,true,true>;
#else
#ifdef PIONEER_Q38_TEST_BFLOAT
using CooperativePeer=VectorRows<PersistentRows,true,true>;
#else
using CooperativePeer=VectorRows<PersistentRows,true>;
#endif
#endif
#else
using CooperativePeer=PersistentRows;
#endif
#define PIONEER_Q38_DISPATCH_BASE_ALREADY_INCLUDED
#define PIONEER_Q38_DISPATCH_MACHINE_ONLY
#include "pioneer_qwen38_dispatch_image_test.cpp"
#include <memory>
// Both ROOT and PEERS execute actual emitted RV32/FPU/NoC instructions.
// Only the wire/GDDR and read-only debug reset register are simulated.
struct CooperativeDispatch:DispatchMachine {
	std::vector<std::unique_ptr<CooperativePeer>> actual;
	CooperativeDispatch(const char* root,const char* peer,unsigned workers,unsigned type,unsigned inputs)
		:DispatchMachine(root,workers,type,inputs) {
		for (unsigned w=0;w<workers;w++) {
			actual.emplace_back(new CooperativePeer(peer,type,inputs,1+w%3,w+1));
			assert(actual.back()->Boot());
		}
		Bind();
	}
	void Bind() {
		for (unsigned w=0;w<active;w++) {
			auto& p=*actual[w]; p.base=memory.at(0x74080+w*32);
			for (unsigned i=p.inputReads;i<p.packets.size();i++) {
				p.packets[i].address=p.base+(i-p.inputReads)*unsigned(p.tile.size());
				p.packets[i].route=memory.at(0x74080+w*32+16);
			}
			for (auto& v:p.root) v.second=memory.at(v.first);
			for (unsigned i=0;i<k;i++) p.vector[i]=uint16_t(memory.at(0xa0000+(i/2)*4)>>((i%2)*16));
			for (unsigned b=0;b<p.scales.size();b++) p.scales[b]=memory.at(0xab000+b*4);
		}
	}
	uint32_t ExpectedOutput(unsigned w,unsigned row) const override { assert(row<256); return actual.at(w)->buffers[4096+row]; }
	void StepPeer(unsigned w) {
		auto& p=*actual[w];
		for (unsigned step=0;step<10000;step++) {
			if (p.pc==TT_Q38_PEER_FAULT_PARK || (p.memory.at(0x74144)==peers[w].requested && (p.pc==0x7204c || p.pc==0x72060))) break;
			p.StepBlock();
		}
		peers[w].requested=p.memory.at(0x74140); peers[w].finished=p.memory.at(0x74144);
	}
	void Packet() override {
		assert(pending && !unknown); bool write=command[7]==0x212;
		unsigned worker=Worker(write?command[5]:command[2]),w=worker-1;
		assert(w<actual.size() && !command[1] && !command[4] && !command[6] && !command[9]);
		assert((write?command[2]:command[5])==129 && (write || !command[7]));
		unsigned local=write?command[0]:command[3],remote=write?command[3]:command[0],bytes=command[8];
		assert(local>=0x75000 && bytes && local+bytes<=0x75800 && !((local^remote)&31));
		auto& p=*actual[w]; auto& state=peers[w];
		if (write && remote==0x74000) {
			assert(bytes==64 && !doorbells && p.pc==0x7204c); descriptors++;
			for (unsigned i=0;i<16;i++) state.descriptor[i]=p.memory[remote+i*4]=memory.at(local+i*4);
			assert(state.descriptor[11]==worker && state.descriptor[12]==state.finished+1 && state.descriptor[13]==1);
		} else if (write) {
			assert(remote==0x74100 && bytes==4 && descriptors==active && readbacks==active && !state.launched);
			assert(memory.at(local)==state.descriptor[12]); p.memory[remote]=memory.at(local);
			state.requested=memory.at(local); state.launched=true; state.operation=1; doorbells++;
		} else if (remote==0x74000) {
			assert(bytes==64 && !doorbells); readbacks++;
			for (unsigned i=0;i<16;i++) memory[local+i*4]=p.memory.at(remote+i*4);
		} else if (remote==0x74140) {
			assert(bytes==64); if (state.launched) StepPeer(w);
			for (unsigned i=0;i<16;i++) memory[local+i*4]=p.memory.at(remote+i*4);
		} else if (remote==0xffb13140) { assert(bytes==4); memory[local]=p.pc; }
		else if (remote==0xffb121b0) { assert(bytes==4); memory[local]=0x46800; }
		else {
			assert(remote==0x83f00 && bytes==256+p.groups*64 && AllFinished()); gathers++;
			for (const auto& peer:actual) assert((peer->pc==0x7204c || peer->pc==0x72050) && !peer->pending);
			for (unsigned i=0;i<64;i++) memory[local+i*4]=p.memory.at(remote+i*4);
			for (unsigned row=0;row<p.groups*16;row++) memory[local+256+row*4]=p.buffers[4096+row];
		}
		counts[write?4:0]++; counts[write?5:1]+=(bytes+63)/64; counts[write?7:3]++; counts[2]++;
		responses++; pending=false;
	}
};
int main(int argc,char** argv)
{
	assert(argc==3 || argc==4);
	for (unsigned workers:{1U,2U}) for (unsigned type:{8U,12U,14U}) {
		CooperativeDispatch m(argv[1],argv[2],workers,type,type==12?17408:5120);
		assert(m.RunRoot()); for (auto& p:m.actual) assert(p->CorrectRows());
		for (auto& p:m.actual) p->NextPackets();
		m.Configure(2); m.Bind(); assert(m.RunRoot());
		for (auto& p:m.actual) {
			assert(p->CorrectRows());
			// The gamma_n FP64 observer NEVER relaxes exact FP32 parity.
			uint32_t original=p->buffers[4096]; p->buffers[4096]^=1;
			fputs("EXPECTED observer rejection of injected single-bit error:\n",stderr);
			assert(!p->CorrectRows()); p->buffers[4096]=original;
			assert(p->CorrectRows() && p->Job(3,2));
		}
		printf("OFFLINE BOTH actual root+%u persistent peers type%u:two full math/NoC jobs,bit-exact assembled results vs independent FP32+double,terminal peer STOP PASS\n",workers,type);
	}
	{
		CooperativeDispatch m(argv[1],argv[2],2,12,5120); m.actual[1]->faultAt=5;
		assert(!m.RunRoot() && m.unknown && m.actual[1]->pending && !m.gathers);
#ifdef TT_PIONEER_EXPERIMENTAL_Q38_DISPATCH_FAULT_RETURN
		unsigned site=m.actual[1]->memory.at(0x7414c);
		assert(site==m.actual[1]->regs[1] && site>=0x72000 && site<0x74000 && !(site&3));
		assert(m.l1[m.Index(0x17ffc0)]==0x54000282 && m.l1[m.Index(0x17ffc4)]==site);
		puts("PASS57 injected peer2 fault return-PC preserved by EXISTING status transfer;zero output gathers/extra packets after UNKNOWN");
#endif
	}
	if (argc==4) {
		FILE* f=fopen(argv[3],"rb"); assert(f); PioneerQwen38Gguf model; assert(model.Open(f));
		for (const char* name:{"blk.0.attn_qkv.weight","blk.0.ffn_down.weight","output.weight"}) {
			const auto* t=model.Find(name); assert(t);
			for (unsigned end=0;end<2;end++) {
				CooperativeDispatch m(argv[1],argv[2],2,t->type,unsigned(t->inputs));
				for (auto& p:m.actual) p->BindRows(model,unsigned(t-model.Tensors().data()),end?unsigned(t->outputs)-p->groups*16:0);
				assert(m.RunRoot()); for (auto& p:m.actual) assert(p->CorrectRows());
				for (auto& p:m.actual) p->NextPackets();
				m.Configure(2); m.Bind(); assert(m.RunRoot()); for (auto& p:m.actual) assert(p->CorrectRows() && p->Job(3,2));
				printf("OFFLINE REAL cooperative ROOT+2 peers %s K%u end%u twice:exact assembled math PASS\n",name,unsigned(t->inputs),end);
			}
		}
		assert(model.Stable()); fclose(f);
	}
	puts("PASS OFFLINE cooperative actual ROOT+PEER instructions,ALL completions before outputs,retained peers/no reset,exact math,one-peer UNKNOWN refuses ALL output;NO native/kernel/whole-model/inference/speed qualification");
}
