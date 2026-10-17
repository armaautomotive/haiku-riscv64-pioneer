/* SPDX-License-Identifier: MIT */
#define PIONEER_GRAPH_MATH_MACHINE_ONLY
#include "pioneer_graph_math_image_test.cpp"
#define PIONEER_EMBEDDING_IO_ONLY
#include "pioneer_decoder_embedding_test.cpp"
struct EmbeddingMachine:ScalarMachine {
	EmbeddingIO source;
	unsigned writes=0,inputReads=0;
	EmbeddingMachine(const char* root,unsigned token):ScalarMachine(root,1024,0),source(token) {
		memory[0x74000]=0x31424d45; memory[0x74004]=token; memory[0x74008]=1024; memory[0x7400c]=0;
		for (unsigned i=0;i<source.packed.size();i++) memory[0x90000+i*4]=source.packed[i];
		for (unsigned i=0;i<1024;i++) memory[0x84000+i*4]=0xdeadbeef;
		memory[0x83ffc]=memory[0x85000]=0xabcdef01;
	}
	uint32_t Load(uint32_t a) override {
		if (Range(a,0x90000,18432)) { inputReads++; return memory.at(a); }
		assert(!Range(a,0xff000000,0x1000000)); // NO NoC,PCI,CSR or control register reads
		return ScalarMachine::Load(a);
	}
	void Store(uint32_t a,uint32_t value) override {
		if (Range(a,0x84000,4096)) { writes++; memory[a]=value; return; }
		assert(Range(a,0x6e000,8192) || Range(a,0x17ffc0,12)); // NO input/code/NoC/control writes
		ScalarMachine::Store(a,value);
	}
	void VerifyEmbedding() {
		assert(writes==1024 && inputReads==9216 && memory.at(0x83ffc)==0xabcdef01 && memory.at(0x85000)==0xabcdef01);
		for (unsigned i=0;i<1024;i++) assert(memory.at(0x84000+i*4)==PioneerAttentionBits(source.reference[i]));
	}
};
int main(int argc,char** argv)
{
	assert(argc==2);
	for (unsigned tile:{0U,1U,947U,9495U}) for (unsigned row=0;row<16;row++) {
		EmbeddingMachine m(argv[1],tile*16+row); assert(m.RunScalar()); m.VerifyEmbedding();
		printf("OFFLINE actual resident embedding token%u IEEE bits/64 complete reads/guards PASS steps%u stack%08x\n",m.source.token,m.steps,m.minStack);
	}
	for (unsigned field=0;field<4;field++) {
		EmbeddingMachine m(argv[1],15); m.memory[0x74000+field*4]=field==0?0:field==1?151936:field==2?1023:1;
		assert(!m.RunScalar() && !m.writes && !m.inputReads);
	}
	for (unsigned bad:{0x3f800001U,0x7f800000U,1U}) {
		EmbeddingMachine m(argv[1],15); m.memory[0x90000+15*4]=bad; assert(!m.RunScalar() && !m.writes);
	}
	puts("PASS OFFLINE actual RV32IMF tied-embedding instruction prototype,all16 lanes/first-last tiles/IEEE dequant/unchanged guards/bounded1152B source copies/format refusal;NO NoC/MMIO/driver or native authority");
}
