/* SPDX-License-Identifier: MIT */
#define PIONEER_Q38_PERSISTENT_ROWS_MACHINE_ONLY
#define PIONEER_Q38_VECTOR_PERSISTENT_IMAGE
#include "pioneer_qwen38_persistent_rows_image_test.cpp"

struct ArgmaxPersistentRows : PersistentRows {
	ArgmaxPersistentRows(const char* path,unsigned type,unsigned inputs,unsigned groups,unsigned worker)
		:PersistentRows(path,type,inputs,groups,worker)
	{
		assert(inputReads>=2);
		auto& scales=packets[inputReads-1];
		assert(scales.address==0xab000 && scales.destination==0x94000);
		for (unsigned at=0;at<scales.bytes;at+=4) {
			root[0x9dc00+at]=root.at(0xab000+at);
			root.erase(0xab000+at);
		}
		scales.address=0x9dc00;
	}

	void WorkProjection(unsigned firstRow,unsigned groupCount,bool argmax)
	{
		memory[0x74000]=0x31503551;
		memory[0x74004]=14;
		memory[0x74008]=3392;
		memory[0x7400c]=firstRow;
		memory[0x74010]=0;
		memory[0x74014]=inputs;
		memory[0x74018]=base;
		memory[0x7401c]=groupCount;
		memory[0x74020]=0x47;
		memory[0x74024]=1;
		memory[0x7403c]=argmax?1U:0U;
	}

	bool WorkJob(unsigned sequence,unsigned firstRow,bool argmax)
	{
		WorkProjection(firstRow,groups,argmax);
		memory[0x74028]=0x32503851; memory[0x7402c]=logicalWorker;
		memory[0x74030]=sequence; memory[0x74034]=1; memory[0x74100]=sequence;
		auto frozenRoot=root;
		for (;;) {
			StepBlock();
		if (pc==TT_Q38_PEER_FAULT_PARK) {
			assert(root==frozenRoot && memory.at(0x74148)==0x46503851);
			return false;
		}
			if (pc==0x72060 || (pc==0x7204c && memory.at(0x74144)==sequence)) break;
		}
		assert(root==frozenRoot && !pending && memory.at(0x74140)==sequence
			&& memory.at(0x74144)==sequence && !memory.at(0x74148)
			&& memory.at(0x7414c)==0x32503851 && minStack>=0x6e000);
		assert(pc==0x7204c && !memory.at(0x74150));
		assert(memory.at(0x83f40)==groups*inputs/16 && memory.at(0x83f44)==sequence
			&& memory.at(0x83f48)==logicalWorker && memory.at(0x83f4c)==0x32503851);
		for (unsigned i=32;i<48;i++) assert(memory.at(0x83f00+i*4)==(i==32?0x16005:0x12340000+i-32));
		for (unsigned i=48;i<64;i++) assert(!memory.at(0x83f00+i*4));
		return requests==packets.size();
	}

	bool ArgmaxJob(unsigned sequence,unsigned firstRow)
	{
		return WorkJob(sequence,firstRow,true);
	}
};

int main(int argc,char** argv)
{
	assert(argc==2);
	constexpr unsigned type=14,inputs=2560,groups=3,firstRow=320;
	ArgmaxPersistentRows reference(argv[1],type,inputs,groups,69);
	if (!reference.Boot()) {
		std::fprintf(stderr,"BOOT_FAILED pc=%08x status=%08x/%08x/%08x/%08x\n",reference.pc,
			reference.memory.at(0x74140),reference.memory.at(0x74144),
			reference.memory.at(0x74148),reference.memory.at(0x7414c));
		return 1;
	}
	reference.WorkProjection(0,groups,false);
	assert(reference.WorkJob(1,0,false));
	float best=-3.402823466e+38F;
	unsigned chosen=UINT32_MAX;
	for (unsigned row=0;row<groups*16;row++) {
		float value=PioneerAttentionFloat(reference.buffers[4096+row]);
		unsigned token=firstRow+row;
		if (chosen==UINT32_MAX || value>best || (value==best && token<chosen)) {
			best=value; chosen=token;
		}
	}

	ArgmaxPersistentRows candidate(argv[1],type,inputs,groups,69);
	assert(candidate.Boot() && candidate.ArgmaxJob(1,firstRow));
	assert(candidate.buffers[4096]==PioneerAttentionBits(best));
	assert(candidate.buffers[4097]==chosen);
	for (unsigned row=2;row<16;row++) assert(!candidate.buffers[4096+row]);
	printf("PASS actual persistent Tensix peer argmax: %u projection rows reduced to one exact value/index pair; peer proof, source/counter completion, no host payload; no card/throughput authority\n",groups*16);
}
