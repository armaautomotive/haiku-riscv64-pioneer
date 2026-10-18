/* SPDX-License-Identifier: MIT */
#define PIONEER_Q38_PERSISTENT_IMAGE
#define PIONEER_Q38_SHARED_ROOT_MACHINE_ONLY
#include "pioneer_qwen38_shared_rows_image_test.cpp"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38DispatchImages.h"
// Exact RV32/FPU/NoC execution,NOT a native route/launch permission. The
// test harness stands in for a future private root's descriptor handoff.
struct PersistentRows:SharedRowsMachine {
	unsigned flitBase=0;
	unsigned changeSource=UINT32_MAX;
	unsigned logicalWorker;
	bool changeDescriptor=false;
	PersistentRows(const char* path,unsigned type,unsigned k,unsigned groups,unsigned worker=69):SharedRowsMachine(path,type,k,groups),logicalWorker(worker) {
		for (unsigned word=0;word<2048;word++) assert(PioneerQ38DispatchRoleWord(worker,word)==(word<code.size()/4?Word(code,word*4):0));
		assert(inputReads>=2 && packets[inputReads-1].address==0x94000);
		packets[inputReads-1].address=0xab000;
		for (unsigned at=0;at<packets[inputReads-1].bytes;at+=4) {
			root[0xab000+at]=root.at(0x94000+at); root.erase(0x94000+at);
		}
		for (unsigned w=0;w<16;w++) if (!memory.count(0x74000+w*4)) memory[0x74000+w*4]=0;
		const unsigned columns[]={1,2,3,4,5,6,7,10,11,12,13,14};
		selfRoute=columns[worker/10]|((worker%10+2)<<6); command[5]=selfRoute;
		const uint32_t boot[]={0x32503851,worker,selfRoute,0x16005,0,0,0,0};
		for (unsigned w=0;w<8;w++) memory[0x74040+w*4]=boot[w];
		for (unsigned w=0;w<16;w++) { memory[0x74100+w*4]=0; memory[0x74140+w*4]=0xdeadbeef; }
		for (unsigned w=0;w<64;w++) memory[0x83f00+w*4]=0xdeadbeef;
		assert(Word(code,0x4c)==0x0002a503 && Word(code,0x60)==0x0000006f);
	}
	uint32_t Load(uint32_t a) override {
		if (Range(a,0x74000,0xc0) || Range(a,0x74100,0x80) || Range(a,0x83f00,256)) return memory.at(a);
		if (a==0xffb20100) { if (changeDescriptor && matrixInstructions) memory[0x7403c]=1; return 0x16005^(changeSource==0 && matrixInstructions?1:0); }
		for (unsigned w=1;w<16;w++) if (a==0xffb20000+BlackholeDramSourceOffset(w))
			return (0x12340000+w)^(changeSource==w && matrixInstructions?1:0);
		if (a==0xffb2020c) {
			unsigned flits=flitBase; for (unsigned i=0;i<responses;i++) flits+=packets.at(i).bytes/64; return flits;
		}
		if (a==0xffb20210 || a==0xffb20214) return counterBase+responses;
		if (a==0xffb20204 || a==0xffb20220 || a==0xffb20228) return 0;
		for (unsigned c=1;c<4;c++) if (a==0xffb20040+c*0x800) return 0;
		if (a>0xffb20240 && a<0xffb202c0) return 0;
		return SharedRowsMachine::Load(a);
	}
	void Store(uint32_t a,uint32_t value) override {
		if (Range(a,0x74140,64)) {
			if (a!=0x74148 && value!=0x46503851) assert(!pending && drained);
			memory[a]=value; return;
		}
		if (Range(a,0x83f00,256)) { assert(!pending && drained); memory[a]=value; return; }
		assert(!Range(a,0x74000,0xc0) && !Range(a,0x74100,64)); // descriptors/boot/doorbell are root-owned
		SharedRowsMachine::Store(a,value);
	}
	bool Boot() {
		while (pc!=0x7204c && pc!=0x72080) StepBlock();
		if (pc==0x72080) return false;
		assert(pc==0x7204c && !requests && !matrixInstructions && memory.at(0x7414c)==0x32503851
			&& !memory.at(0x74140) && !memory.at(0x74144) && !memory.at(0x74148));
		return true;
	}
	void NextPackets() {
		assert(!pending && requests==responses);
		counterBase+=responses; for (unsigned i=0;i<responses;i++) flitBase+=packets.at(i).bytes/64;
		requests=responses=0; matrixInstructions=0; doorbellRead=false;
	}
	bool Job(unsigned sequence,unsigned operation=1) {
		memory[0x74028]=0x32503851; memory[0x7402c]=logicalWorker;
		memory[0x74030]=sequence; memory[0x74034]=operation; memory[0x74100]=sequence;
		auto frozenRoot=root;
		for (;;) {
			StepBlock();
			if (pc==0x72080) { assert(root==frozenRoot && memory.at(0x74148)==0x46503851); return false; }
			if (pc==0x72060 || (pc==0x7204c && memory.at(0x74144)==sequence)) break;
		}
		assert(root==frozenRoot && !pending && memory.at(0x74140)==sequence && memory.at(0x74144)==sequence
			&& !memory.at(0x74148) && memory.at(0x7414c)==0x32503851 && minStack>=0x6e000);
		if (operation==2) return pc==0x72060 && memory.at(0x74150)==2;
		assert(pc==0x7204c && !memory.at(0x74150));
		for (const auto& v:root) assert(memory.at(v.first>=0xab000?0x94000+v.first-0xab000:v.first)==v.second);
		assert(memory.at(0x83f40)==groups*inputs/16 && memory.at(0x83f44)==sequence
			&& memory.at(0x83f48)==logicalWorker && memory.at(0x83f4c)==0x32503851);
		for (unsigned i=0;i<10;i++) assert(memory.at(0x83f50+i*4)==command[i]);
		assert(memory.at(0x83f78)==0x16005 && memory.at(0x83f7c)==selfRoute);
		for (unsigned i=32;i<48;i++) assert(memory.at(0x83f00+i*4)==(i==32?0x16005:0x12340000+i-32));
		for (unsigned i=48;i<64;i++) assert(!memory.at(0x83f00+i*4));
		return requests==packets.size() && CorrectRows();
	}
};
#ifndef PIONEER_Q38_PERSISTENT_ROWS_MACHINE_ONLY
int main(int argc,char** argv)
{
	assert(argc==2 || argc==3);
	for (unsigned type:{8U,12U,14U}) for (unsigned k:{5120U,6144U,17408U}) {
		PersistentRows m(argv[1],type,k,k==17408?3:1); m.delay=3; assert(m.Boot());
		assert(m.Job(1)); m.NextPackets(); assert(m.Job(2));
		unsigned requests=m.requests,matrices=m.matrixInstructions;
		for (unsigned i=0;i<1000;i++) m.StepBlock(); // duplicate doorbell is inert,not a retry
		assert(m.requests==requests && m.matrixInstructions==matrices);
		assert(m.Job(3,2) && m.requests==requests && m.matrixInstructions==matrices);
		printf("OFFLINE persistent peer type%u K%u N%u:two exact jobs,duplicate inert,clean terminal STOP PASS\n",type,k,m.groups*16);
	}
	for (unsigned fault:{1U,5U,40U,208U}) {
		PersistentRows m(argv[1],12,17408,3); m.Boot(); m.faultAt=fault;
		assert(!m.Job(1) && m.pending && m.requests==fault && m.responses==fault-1);
		unsigned requests=m.requests; for (unsigned i=0;i<1000;i++) m.StepBlock(); assert(m.requests==requests && m.pending);
	}
	for (unsigned sequence:{2U,0x7fffffffU}) {
		PersistentRows m(argv[1],8,5120,1); m.Boot(); assert(!m.Job(sequence) && !m.requests && !m.matrixInstructions);
	}
	{
		PersistentRows m(argv[1],12,5120,1); m.Boot(); m.changeDescriptor=true;
		assert(!m.Job(1) && m.requests==m.responses && !m.memory.at(0x74144));
	}
	{
		PersistentRows m(argv[1],14,5120,1); m.counterBase=UINT32_MAX-2; m.flitBase=UINT32_MAX-2;
		m.Boot(); assert(m.Job(1)); m.NextPackets(); assert(m.Job(2));
	}
	for (unsigned field=0;field<8;field++) {
		PersistentRows m(argv[1],8,5120,1); m.memory[0x74040+field*4]^=1;
		assert(!m.Boot() && !m.requests && !m.matrixInstructions);
	}
	for (unsigned source=0;source<16;source++) {
		PersistentRows m(argv[1],8,5120,1); assert(m.Boot()); m.changeSource=source;
		assert(!m.Job(1) && !m.memory.at(0x74144));
	}
	if (argc==3) {
		FILE* f=fopen(argv[2],"rb"); assert(f); PioneerQwen38Gguf model; assert(model.Open(f));
		for (const char* name:{"blk.0.attn_qkv.weight","blk.0.ffn_down.weight","output.weight"}) {
			const auto* t=model.Find(name); assert(t);
			for (unsigned end=0;end<2;end++) {
				PersistentRows m(argv[1],t->type,unsigned(t->inputs),3);
				m.BindRows(model,unsigned(t-model.Tensors().data()),end?unsigned(t->outputs)-48:0);
				assert(m.Boot() && m.Job(1)); m.NextPackets(); assert(m.Job(2) && m.Job(3,2));
				printf("OFFLINE REAL persistent %s K%u N48 end%u repeated twice PASS\n",name,m.inputs,end);
			}
		}
		assert(model.Stable()); fclose(f);
	}
	puts("PASS OFFLINE actual persistent mixed-quant peer instructions:repeated jobs/no reset,exact FP32+independent double,immutable root/private handshake/proof,duplicate inert,nonce wrap/skips/descriptor mutation refuse,UNKNOWN parks without retry,terminal STOP;NO driver/native/coordinator/graph/inference/speed qualification");
}
#endif
