/* SPDX-License-Identifier: MIT */
#define PIONEER_GLU_DECODER_MACHINE_ONLY
#include "pioneer_glu_decoder_image_test.cpp"
struct GreedyDecoder:GluDecoder {
	unsigned edge=0;
	unsigned dropCommand=0,counterFault=0; bool commandPoisoned=false,counterInjected=false;
	GreedyDecoder(const char* path,const DecoderSfpuImage& image,bool nz,bool small=true):GluDecoder(path,image,nz,true) {
		memory[TT_DECODER_JOB_BASE+4]=small?9:8;
	}
	uint32_t PeerRead(unsigned w,uint32_t a) override {
		if (edge && Range(a,0x84000,16384) && Phase(descriptor[w-1][0])==7) {
			unsigned row=(descriptor[w-1][0]-WeightBase(7))/1152+(a-0x84000)/4;
			if (edge==1) return row?0:0x80000000; // first -0 wins every +/-0 tie
			if (edge==2) return row==1200 || row==151935?0xbf000000:0xc0000000;
			if (edge==3 && row==151935) return 0x7fc00000;
			if (edge==4 && row==151935) return 0x7f800000;
			return 0;
		}
		return GluDecoder::PeerRead(w,a);
	}
	void Store(uint32_t a,uint32_t value) override {
		if (dropCommand && Range(a,0xffb20000,40) && memory.at(0x140000)==0x31445243 && memory.at(0x140004)==1 && !issued) {
			if (!commandPoisoned) { for (unsigned i=0;i<10;i++) command[i]=0xa5000000+i; commandPoisoned=true; }
			if (a==0xffb20000+(dropCommand-1)*4) return;
		}
		GluDecoder::Store(a,value);
	}
	void Submit() override {
		bool corrupt=counterFault && command[0]==0x70500 && !command[7] && !issued;
		GluDecoder::Submit(); if (corrupt) { counts[counterFault-1]++; counterInjected=true; }
	}
};
int main(int argc,char** argv)
{
	assert(argc==3); compactImage=true; DecoderSfpuImage image(argv[2],false,true,true);
	bool faultsOnly=getenv("PIONEER_GREEDY_FAULTS_ONLY")!=nullptr;
	if (!faultsOnly) {
	{ GreedyDecoder m(argv[1],image,false); assert(m.RunDecoder()); m.Verify(); m.VerifyProfile(); }
	{ GreedyDecoder m(argv[1],image,true); assert(m.RunDecoder()); m.Verify(); m.VerifyProfile(); m.Next(); assert(m.RunDecoder()); m.Verify(); m.VerifyProfile(); }
	{ GreedyDecoder m(argv[1],image,true,false); assert(m.RunDecoder()); m.Verify(); m.VerifyProfile(); }
	for (unsigned edge=1;edge<=4;edge++) { GreedyDecoder m(argv[1],image,false); m.edge=edge;
		bool okay=m.RunDecoder(); assert(okay==(edge<3));
		if (okay) { assert(PioneerDecoderGreedyValid(m.wide.data(),1,1) && m.wide[3]==(edge==1?0U:1200U) && m.wide[4]==(edge==1?0x80000000:0xbf000000)); }
		else assert(!m.hostWrites && !m.hostReads); }
	for (unsigned fault=0;fault<3;fault++) { GreedyDecoder m(argv[1],image,false);
		m.dropPosted=fault==0; m.partialPosted=fault==1; m.pairedCorrupt=fault==2;
		assert(!m.RunDecoder() && m.hostWrites==1 && m.hostReads==1); }
	}
	for (unsigned word=1;word<=10;word++) { GreedyDecoder m(argv[1],image,false); m.dropCommand=word;
		assert(!m.RunDecoder() && m.commandPoisoned && !m.issued && !m.hostWrites); }
	for (unsigned counter:{1U,2U,3U,4U}) { GreedyDecoder m(argv[1],image,false); m.counterFault=counter;
		assert(!m.RunDecoder() && m.counterInjected && !m.issued && !m.hostWrites); }
	if (faultsOnly) { puts("PASS OFFLINE actual master Transfer:ALL ten individually dropped command stores and ALL four independently corrupted physical counter deltas rejected before any peer launch;full functional image tests separate"); return 0; }
	puts("PASS OFFLINE actual full decoder/8 attention/6 GLU/on-card first-argmax,128B sole ordered/all-bit-compared result,exact independent tokens/logit bits/KV/119 sequences/profile/NoC budgets,TWO positions,retained full-vocabulary mode,zero/negative/tie/nonfinite/posted faults;NOT native qualification");
}
