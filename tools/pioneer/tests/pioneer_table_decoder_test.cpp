/* SPDX-License-Identifier: MIT */
#define PIONEER_GREEDY_DECODER_MACHINE_ONLY
#include "pioneer_greedy_decoder_image_test.cpp"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerDecoderDescriptorTable.h"
struct TableDecoder:GreedyDecoder {
	PioneerDecoderJob frozen={}; unsigned lookups=0;
	TableDecoder(const char* root,const DecoderSfpuImage& image,bool nonzero,bool small):GreedyDecoder(root,image,nonzero,small) {
		for (unsigned i=0;i<sizeof(frozen)/4;i++) reinterpret_cast<uint32_t*>(&frozen)[i]=memory.at(TT_DECODER_JOB_BASE+i*4);
	}
	void PeerWrite(unsigned worker,uint32_t address,uint32_t value) override {
		assert(!(address>=0x70500 && address<0x70520)); // NO root descriptor staging
		if (address==0x71300) { PioneerDecoderDescriptor entry;
			assert(PioneerDecoderDescriptorEntry(frozen,worker,sequence[worker-1]%PioneerDecoderDescriptorCount(worker),entry));
			for (unsigned i=0;i<8;i++) descriptor[worker-1][i]=entry.words[i];
			lookups++;
		}
		GreedyDecoder::PeerWrite(worker,address,value);
	}
	void Submit() override {
		bool corrupt=counterFault && command[7]==0x212 && command[3]==0x71300 && !issued;
		GreedyDecoder::Submit(); if (corrupt) { const unsigned counters[]={4,5,2,7}; counts[counters[counterFault-1]]++; counterInjected=true; }
	}
};
int main(int argc,char** argv)
{
	assert(argc==3); compactImage=true;
	setenv("PIONEER_COORDINATED_PULL_INPUT","1",1); setenv("PIONEER_GREEDY_PAIRED_FFN","1",1); setenv("PIONEER_GREEDY_DESCRIPTOR_TABLE","1",1);
	DecoderSfpuImage image(argv[2],false,true,true);
	if (!getenv("PIONEER_TABLE_FAULTS_ONLY")) for (bool small:{false,true}) for (bool nonzero:{false,true}) {
		TableDecoder m(argv[1],image,nonzero,small); assert(m.RunDecoder()); m.Verify(); m.VerifyProfile(); assert(m.lookups==9219);
		if (nonzero) { m.Next(); assert(m.RunDecoder()); m.Verify(); m.VerifyProfile(); assert(m.lookups==18438); }
	}
	for (unsigned word=1;word<=10;word++) { TableDecoder m(argv[1],image,false,true); m.dropCommand=word;
		assert(!m.RunDecoder() && m.commandPoisoned && !m.issued && !m.hostWrites); }
	for (unsigned counter:{1U,2U,3U,4U}) { TableDecoder m(argv[1],image,false,true); m.counterFault=counter;
		assert(!m.RunDecoder() && m.counterInjected && m.issued==1 && !m.hostWrites); }
	puts("PASS OFFLINE actual table-master/8 attention/6 GLU,full+128B zero/sparse/two positions,ALL logits/KV/119 sequences/counters,NO root descriptor writes;peer lookups separately instruction-tested;NOT native qualification");
}
