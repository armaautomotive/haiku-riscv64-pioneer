/* SPDX-License-Identifier: MIT */
#if defined(PIONEER_Q38_TEST_FRONTEND_BARRIER)
#define PIONEER_Q38_FRONTEND_BARRIER_IMAGE_MODEL_ONLY
#include "pioneer_q38_frontend_barrier_image_test.cpp"
using CooperativePeer=FrontendBarrierImage;
#elif defined(PIONEER_Q38_TEST_BATCH_RAW_INPUT)
#define PIONEER_Q38_BATCH_RAW_IMAGE_MODEL_ONLY
#include "pioneer_q38_batch_raw_image_test.cpp"
using CooperativePeer=BatchRawImage;
#elif defined(PIONEER_Q38_TEST_BATCH4_FETCH)
#define PIONEER_Q38_BATCH4_IMAGE_MODEL_ONLY
#include "pioneer_q38_batch4_image_test.cpp"
using CooperativePeer=Batch4Image;
#elif defined(PIONEER_Q38_TEST_DIRECT_SRCB)
#define PIONEER_Q38_DIRECT_SRCB_IMAGE_MODEL_ONLY
#include "pioneer_q38_direct_srcb_image_test.cpp"
using CooperativePeer=DirectSrcBImage;
#elif defined(PIONEER_Q38_TEST_AFFINE_RAWDOT)
#define PIONEER_Q38_AFFINE_REPLAY_IMAGE_MODEL_ONLY
#include "pioneer_q38_affine_replay_image_test.cpp"
using CooperativePeer=ReplayImage;
#elif defined(PIONEER_Q38_TEST_NATIVE_Q8)
#define PIONEER_Q38_NATIVE_Q8_IMAGE_MODEL_ONLY
#include "pioneer_q38_native_q8_image_test.cpp"
using CooperativePeer=NativeQ8Image;
#elif defined(PIONEER_Q38_TEST_GROUP32)
#define PIONEER_Q38_GROUP32_IMAGE_MODEL_ONLY
#include "pioneer_q38_group32_image_test.cpp"
using CooperativePeer=Group32Image;
#elif defined(PIONEER_Q38_TEST_DEFER_AFFINE)
#define PIONEER_Q38_DEFER_AFFINE_IMAGE_TEST_MODEL_ONLY
#include "pioneer_q38_defer_affine_image_test.cpp"
using CooperativePeer=DeferredAffineImage;
#else
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
#elif defined(PIONEER_Q38_TEST_INT_ACCUMULATOR)
using CooperativePeer=VectorRows<PersistentRows,true,false,true>;
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
#endif
#define PIONEER_Q38_DISPATCH_BASE_ALREADY_INCLUDED
#define PIONEER_Q38_DISPATCH_MACHINE_ONLY
#include "pioneer_qwen38_dispatch_image_test.cpp"
#include <memory>
#ifdef TT_PIONEER_EXPERIMENTAL_Q38_DISPATCH_PEER_PROGRESS
#include "../PioneerQ38DecodedProbe.h"
#endif
// Both ROOT and PEERS execute actual emitted RV32/FPU/NoC instructions.
// Only the wire/GDDR and read-only debug reset register are simulated.
struct CooperativeDispatch:DispatchMachine {
	std::vector<std::unique_ptr<CooperativePeer>> actual;
	unsigned freezeWorker=0,freezeStage=0; bool frozen=false;
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
#ifdef PIONEER_Q38_TEST_BATCH4_FETCH
			p.packets=p.originalPackets;
#endif
			for (unsigned i=p.inputReads;i<p.packets.size();i++) {
				p.packets[i].address=p.base+(i-p.inputReads)*unsigned(p.tile.size());
				p.packets[i].route=memory.at(0x74080+w*32+16);
			}
#ifdef PIONEER_Q38_TEST_BATCH4_FETCH
			p.originalPackets=p.packets;
#endif
			for (auto& v:p.root) v.second=memory.at(v.first);
#ifdef PIONEER_Q38_TEST_DIRECT_SRCB
			p.PoisonTail();
#endif
			for (unsigned i=0;i<k;i++) p.vector[i]=uint16_t(
#ifdef PIONEER_Q38_COMPACT_ACTIVATIONS
				originalInputs[i/2]
#else
				memory.at(0xa0000+(i/2)*4)
#endif
				>>((i%2)*16));
			for (unsigned b=0;b<p.scales.size();b++) p.scales[b]=memory.at(0xab000+b*4);
		}
	}
	uint32_t ExpectedOutput(unsigned w,unsigned row) const override { assert(row<256); return actual.at(w)->buffers[4096+row]; }
	void StepPeer(unsigned w) {
		auto& p=*actual[w];
		for (unsigned step=0;step<10000;step++) {
#ifdef TT_PIONEER_EXPERIMENTAL_Q38_DISPATCH_PEER_PROGRESS
			if (w+1==freezeWorker && p.memory.at(0x74154)==freezeStage) { frozen=true; break; }
#endif
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
#ifdef PIONEER_Q38_TEST_BATCH4_FETCH
			p.Prepare(state.descriptor[14]!=0);
#endif
		} else if (write) {
			assert(remote==0x74100 && bytes==4 && descriptors==active && readbacks==DescriptorEchoes() && !state.launched);
			assert(memory.at(local)==state.descriptor[12]); p.memory[remote]=memory.at(local);
			state.requested=memory.at(local); state.launched=true; state.operation=1; doorbells++;
#ifdef PIONEER_Q38_COMPACT_ACTIVATIONS
			// Model the peer's actual NoC reads of the NOW-expanded root
			// inputs,not the pre-launch compressed snapshot taken by Bind.
			for (auto& value:p.root) value.second=memory.at(value.first);
#endif
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
#ifndef PIONEER_Q38_COOPERATIVE_MOCK_ONLY
int main(int argc,char** argv)
{
	assert(argc==3 || argc==4);
	for (unsigned workers:{1U,2U}) for (unsigned type:{8U,12U,14U}) {
		CooperativeDispatch m(argv[1],argv[2],workers,type,type==12?17408:5120);
		assert(m.RunRoot()); for (auto& p:m.actual) assert(p->CorrectRows());
		for (auto& p:m.actual) p->NextPackets();
		m.Configure(2); m.Bind(); assert(m.RunRoot());
#ifdef PIONEER_Q38_REUSE_ACTIVATIONS
		for (auto& p:m.actual) p->NextPackets();
		m.Configure(3); m.Bind(); m.memory[0x7401c]=2;
		for (unsigned w=0;w<m.k/2;w++) m.memory[0xa0000+w*4]=m.originalInputs[w];
		assert(m.RunRoot());
#endif
		for (auto& p:m.actual) {
			assert(p->CorrectRows());
			// The gamma_n FP64 observer NEVER relaxes exact FP32 parity.
			uint32_t original=p->buffers[4096]; p->buffers[4096]^=1;
			fputs("EXPECTED observer rejection of injected single-bit error:\n",stderr);
			assert(!p->CorrectRows()); p->buffers[4096]=original;
			assert(p->CorrectRows() && p->Job(
#ifdef PIONEER_Q38_REUSE_ACTIVATIONS
				4,
#else
				3,
#endif
				2));
		}
		printf("OFFLINE BOTH actual root+%u persistent peers type%u:two full math/NoC jobs,bit-exact assembled results vs independent FP32+double,terminal peer STOP PASS\n",workers,type);
	}
	{
		CooperativeDispatch m(argv[1],argv[2],2,12,5120); m.actual[1]->faultAt=5;
		assert(!m.RunRoot() && m.unknown && m.actual[1]->pending && !m.gathers);
#ifdef TT_PIONEER_EXPERIMENTAL_Q38_DISPATCH_FAULT_RETURN
		unsigned site=m.actual[1]->memory.at(0x7414c);
		assert(site==m.actual[1]->regs[1] && site>=0x72000 && site<0x74000 && !(site&3));
#ifdef TT_PIONEER_EXPERIMENTAL_Q38_DISPATCH_PEER_PROGRESS
		assert(m.l1[m.Index(0x17ffc0)]==(0x5a000200|(m.actual[1]->memory.at(0x74154)<<16)) && m.l1[m.Index(0x17ffc4)]==site);
#else
		assert(m.l1[m.Index(0x17ffc0)]==0x54000282 && m.l1[m.Index(0x17ffc4)]==site);
#endif
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
#if defined(PIONEER_Q38_VECTOR_COOPERATIVE) && !defined(TT_PIONEER_EXPERIMENTAL_Q38_DISPATCH_PEER_PROGRESS)
	for (unsigned type:{8U,12U,14U}) for (bool drop:{false,true}) {
		CooperativeDispatch m(argv[1],argv[2],2,type,5120);
		for (auto& peer:m.actual) {
			for (unsigned row=0;row<64;row++) for (unsigned col=0;col<16;col++) {
				// Poison BOTH numeric views beyond the dot guard. The former
				// float123456 remained legal for BF16 and need not fault when
				// a clear is dropped,even though its numeric output is wrong.
				peer->valid[row]=true; peer->dstInt[row][col]=0x3c6f3720; peer->dst[row][col]=1.0e8f;
			}
			for (unsigned row=0;row<1024;row++) for (unsigned col=0;col<16;col++) {
				peer->defined[row]=true; peer->physical[row][col]=uint16_t(row*1237+col*29+0x635a); peer->wrote[row][col]=true;
			}
			peer->dropClear=drop;
		}
		if (!drop) { assert(m.RunRoot()); for (auto& peer:m.actual) assert(peer->CorrectRows()); }
		else assert(!m.RunRoot() && m.unknown && !m.gathers);
	}
	puts("PASS vector/BF16 profile:ALL quant types with poisoned power-on Dst produce exact results;omitted clears refuse ALL output;NO native/speed claim");
#endif
#ifdef TT_PIONEER_EXPERIMENTAL_Q38_DISPATCH_PEER_PROGRESS
	{
		CooperativeDispatch m(argv[1],argv[2],2,8,5120);
		auto& peer=*m.actual[0];
		for (auto& valid:peer.defined) valid=true;
		for (auto& valid:peer.valid) valid=true;
		peer.Instruction(0x10080004); peer.Instruction(0x10080005);
		for (unsigned row=0;row<1024;row++) assert(peer.defined[row]==!(row>=64 && row<96));
		for (unsigned row=0;row<64;row++) assert(peer.valid[row]==!(row>=32 && row<48));
		puts("PASS59 explicit physical ZEROACC pair clears EXACT64..95/both FP32 halves;raw0..31/decoded128..159/source-probe192..223 remain defined");
	}
	{
		// Independent ACTUAL native client's synthetic bytes/activation
		// formula. Model an ineffective per-panel clear,not normal math.
		int cumulative[16]={}; unsigned firstBad=0; int badValue=0;
		for (unsigned panel=0;panel<320 && !firstBad;panel++) for (unsigned row=0;row<16;row++) {
			int dot=0; unsigned block=panel/2,within=panel%2;
			for (unsigned lane=0;lane<16;lane++) {
				unsigned byte=((2+within*16+lane)*37+row*59+block*13+23)&255;
				if (!within && !lane) byte=128;
				int weight=int(byte)-int((byte&128)*2);
				int input=int(((block*32+within*16+lane)*29+17)%256)-128;
				dot+=weight*input;
			}
			assert(dot>=-262144 && dot<=262144); cumulative[row]+=dot;
			if (cumulative[row]<-262144 || cumulative[row]>262144) {
				assert(block==45 && within==1 && !row); firstBad=panel+1; badValue=cumulative[row]; break;
			}
		}
		assert(firstBad==92 && badValue==-272445);
		puts("PASS59 native-signature regression:ineffective per-panel clear FIRST exceeds range at worker2 block45/panel1/row0 EXACT -272445;all individual dots legal");
	}
	for (unsigned code=0;code<256;code++) {
		int value=int(code)-int((code&128)*2);
		unsigned magnitude=unsigned(value<0?-value:value);
		uint16_t operand=PioneerInt8Operand(value);
		uint32_t raw=(uint32_t(operand&0x8000)<<16)|(uint32_t(operand&1023)<<21)|0x00100000;
		uint32_t canonical=((raw&0x7f000000)>>8)|((raw&0x00ff0000)<<7);
		assert(PioneerQ38DecodedQ8Risc(code)==(value<0?0U-canonical:canonical));
		uint32_t source=0x08000000+magnitude*8192;
		assert(PioneerQ38DecodedQ8Risc(code,true)==(value<0?0U-source:source));
	}
	{
		CooperativeDispatch m(argv[1],argv[2],2,8,5120);
		m.actual[1]->corruptDecodedProbe=true;
		assert(!m.RunRoot() && m.unknown && !m.gathers);
		assert(m.actual[1]->decodedProbeReads==38 && !m.actual[1]->matrixInstructions);
		assert(m.l1[m.Index(0x17ffc0)]==0x5a0d0225 && m.l1[m.Index(0x17ffc4)]==0x13572468);
		puts("PASS59 independent converted-Dst Q8 operand probe:exact first bad index37/value before ANY peer2 matrix;existing status packet,zero gathers");
	}
	{
		CooperativeDispatch m(argv[1],argv[2],2,8,5120);
		m.actual[1]->corruptMove=true;
		assert(!m.RunRoot() && m.unknown && !m.gathers);
		assert(m.actual[1]->sourceProbeReads==1 && !m.actual[1]->matrixInstructions);
		assert(m.l1[m.Index(0x17ffc0)]==0x5a0e0200 && m.l1[m.Index(0x17ffc4)]==0U-(0x08000000U|(127U<<13)));
		puts("PASS59 actual corrupted MOVD2A source captured by independent MOVA2D TF32 round-trip BEFORE matrix;index0/value exact,zero gathers");
	}
	for (bool drop:{false,true}) {
		CooperativeDispatch m(argv[1],argv[2],2,8,5120);
		for (auto& p:m.actual) {
			for (unsigned row=0;row<64;row++) for (unsigned col=0;col<16;col++) {
				p->valid[row]=true; p->dstInt[row][col]=0x3c6f3720;
				p->dst[row][col]=123456.0f;
			}
			for (unsigned row=0;row<1024;row++) for (unsigned col=0;col<16;col++) {
				p->defined[row]=true; p->physical[row][col]=uint16_t(row*1237+col*29+0x635a); p->wrote[row][col]=true;
			}
			p->dropClear=drop;
		}
		if (!drop) { assert(m.RunRoot()); for (auto& p:m.actual) assert(p->CorrectRows()); }
		else assert(!m.RunRoot() && m.unknown && !m.gathers);
		puts("PASS59 poisoned power-on Dst/valid bits:exact math with clears;omitting clears refuses ALL output;OFFLINE initialization experiment only");
	}
	for (unsigned reason:{10U,11U}) {
		CooperativeDispatch m(argv[1],argv[2],2,8,5120);
		if (reason==11) m.actual[1]->corruptDotRange=true;
		else { m.actual[1]->tiles.front()[0]=0; m.actual[1]->tiles.front()[16]=0x7c; }
		assert(!m.RunRoot() && m.unknown && !m.gathers);
		assert(m.actual[1]->memory.at(0x74154)==reason);
		assert(m.l1[m.Index(0x17ffc0)]==(0x5a000200|(reason<<16)));
		assert(m.l1[m.Index(0x17ffc4)]==(reason==11?262145U:8U<<16));
		assert(m.l1[m.Index(0x17ffc4)]==m.actual[1]->memory.at(0x74158));
		assert(m.actual[1]->memory.at(0x7414c)==m.actual[1]->regs[1]);
		puts("PASS59 actual nonfinite metadata/out-of-range dot:exact reason/value and retained fault RA in EXISTING status packet;zero gathers");
	}
	for (unsigned stage:{3U,5U,6U,8U}) {
		CooperativeDispatch m(argv[1],argv[2],2,8,5120); m.freezeWorker=2; m.freezeStage=stage;
		assert(!m.RunRoot() && m.unknown && m.frozen && !m.gathers);
		assert(m.l1[m.Index(0x17ffc0)]==0x59020205 && m.l1[m.Index(0x17ffc4)]==stage);
		puts("PASS59 frozen actual peer2 stage captured by ORIGINAL status packet;zero outputs/extra packets after UNKNOWN");
	}
#endif
	puts("PASS OFFLINE cooperative actual ROOT+PEER instructions,ALL completions before outputs,retained peers/no reset,exact math,one-peer UNKNOWN refuses ALL output;NO native/kernel/whole-model/inference/speed qualification");
}
#endif
