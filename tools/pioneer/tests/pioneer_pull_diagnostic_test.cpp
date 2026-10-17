/* SPDX-License-Identifier: MIT */
#define PIONEER_GREEDY_DECODER_MACHINE_ONLY
#include "pioneer_greedy_decoder_image_test.cpp"
int main(int argc,char** argv) {
	assert(argc==3); compactImage=true;
	setenv("PIONEER_COORDINATED_PULL_INPUT","1",1); setenv("PIONEER_PULL_DIAGNOSTIC","1",1); setenv("PIONEER_GREEDY_PAIRED_FFN","1",1);
	DecoderSfpuImage image(argv[2],false,true,true);
	for (bool nonzero:{false,true}) { GreedyDecoder m(argv[1],image,nonzero,false); assert(m.RunDecoder()); m.Verify(); m.VerifyProfile();
		for (unsigned i=0;i<16;i++) assert(m.wide[i]==m.memory.at(0x147900+i*4)); }
	for (unsigned word=1;word<=10;word++) { GreedyDecoder m(argv[1],image,false,false); m.dropCommand=word;
		assert(!m.RunDecoder() && m.commandPoisoned && !m.issued && !m.hostWrites); }
	for (unsigned counter:{1U,2U,3U,4U}) { GreedyDecoder m(argv[1],image,false,false); m.counterFault=counter;
		assert(!m.RunDecoder() && m.counterInjected && !m.issued && !m.hostWrites); }
	{ GreedyDecoder m(argv[1],image,false,true); assert(!m.RunDecoder() && !m.issued && !m.hostWrites); }
	puts("PASS OFFLINE diagnostic ONLY:fixed root/peer Q/K/gate/up readbacks in16 full-result slots,remaining logits/KV/119 sequences/112 extra NoC reads exact;sampling refused before work,command/counter faults;NOT inference or performance qualification");
}
