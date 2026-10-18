/* SPDX-License-Identifier: MIT */
#define PIONEER_Q38_VECTOR_PERSISTENT_IMAGE
#define PIONEER_Q38_PERSISTENT_ROWS_MACHINE_ONLY
#include "pioneer_qwen38_persistent_rows_image_test.cpp"
#define PIONEER_Q38_VECTOR_HAS_ROWS_MACHINE
#define PIONEER_Q38_VECTOR_ROWS_MODEL_ONLY
#include "pioneer_q38_vector_rows_image_test.cpp"
using VectorPersistent=VectorRows<PersistentRows,true>;
int main(int argc,char** argv)
{
	assert(argc==2 || argc==3);
	for (unsigned type:{8U,12U,14U}) for (unsigned k:{5120U,6144U,17408U}) {
		VectorPersistent m(argv[1],type,k,k==17408?3:1); m.delay=3; assert(m.Boot());
		assert(m.Job(1)); m.NextPackets(); assert(m.Job(2));
		unsigned requests=m.requests,matrices=m.matrixInstructions;
		for (unsigned i=0;i<1000;i++) m.StepBlock();
		assert(m.requests==requests && m.matrixInstructions==matrices && !m.rawPending);
		assert(m.Job(3,2)); m.pc=TT_Q38_PEER_STOP_FETCH;
		for (unsigned i=0;i<100;i++) m.StepBlock();
		assert(m.pc==TT_Q38_PEER_STOP && m.requests==requests && m.matrixInstructions==matrices);
		printf("OFFLINE VECTOR persistent type%u K%u N%u repeated exact math/duplicate inert/STOP PASS\n",type,k,m.groups*16);
	}
	for (unsigned fault:{1U,5U,40U,208U}) {
		VectorPersistent m(argv[1],12,17408,3); m.Boot(); m.faultAt=fault;
		assert(!m.Job(1) && m.pending && m.requests==fault && m.responses==fault-1);
		unsigned requests=m.requests; for (unsigned i=0;i<1000;i++) m.StepBlock(); assert(m.requests==requests && m.pending);
	}
	for (unsigned source=0;source<16;source++) {
		VectorPersistent m(argv[1],8,5120,1); assert(m.Boot()); m.changeSource=source;
		assert(!m.Job(1) && !m.memory.at(0x74144));
	}
	for (unsigned sequence:{2U,0x7fffffffU}) {
		VectorPersistent m(argv[1],8,5120,1); m.Boot(); assert(!m.Job(sequence) && !m.requests && !m.matrixInstructions);
	}
	{
		VectorPersistent m(argv[1],12,5120,1); m.Boot(); m.changeDescriptor=true;
		assert(!m.Job(1) && !m.memory.at(0x74144));
	}
	if (argc==3) {
		FILE* f=fopen(argv[2],"rb"); assert(f); PioneerQwen38Gguf model; assert(model.Open(f));
		for (const char* name:{"blk.0.attn_qkv.weight","blk.0.ffn_down.weight","output.weight"}) {
			const auto* t=model.Find(name); assert(t);
			for (unsigned end=0;end<2;end++) {
				VectorPersistent m(argv[1],t->type,unsigned(t->inputs),3);
				m.BindRows(model,unsigned(t-model.Tensors().data()),end?unsigned(t->outputs)-48:0);
				assert(m.Boot() && m.Job(1)); m.NextPackets(); assert(m.Job(2) && m.Job(3,2));
				printf("OFFLINE VECTOR REAL persistent %s K%u firstRow%u repeated exact PASS\n",name,m.inputs,end?unsigned(t->outputs)-48:0);
			}
		}
		assert(model.Stable()); fclose(f);
	}
	puts("PASS OFFLINE ACTUAL vector persistent peer:repeated exact FP32+double,zero scalar operand writes,source-bank/Dst ownership,immutable root/descriptor/private nonce/proof,duplicate inert,UNKNOWN parks/STOP safe;NOT kernel/coordinator/native/cycle/model speed qualification");
}
