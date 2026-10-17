/* SPDX-License-Identifier: MIT */
#define PIONEER_ROPE_GROUP_MACHINE_ONLY
#include "pioneer_rope_attention_image_test.cpp"
int main(int argc,char** argv) {
	assert(argc==3); setvbuf(stdout,NULL,_IONBF,0);
	for (const char* key:{"PIONEER_COORDINATED_PULL_INPUT","PIONEER_ATTENTION_PARALLEL_ROPE","PIONEER_GLU_VECTOR"}) setenv(key,"1",1);
	bool finite=getenv("PIONEER_COMPARE_FINITE_HALF")!=nullptr;
	bool optimized=finite || getenv("PIONEER_COMPARE_OPTIMIZED_ATTENTION")!=nullptr;
	for (const char* key:{"PIONEER_ATTENTION_FAST_SOFTMAX","PIONEER_ATTENTION_TOKEN_LANES","PIONEER_ATTENTION_INLINE_SFPU","PIONEER_ATTENTION_OPTIMIZED","PIONEER_ATTENTION_FAST_HALF"}) unsetenv(key);
	Image prior(argv[1],false,true,true); setenv("PIONEER_ATTENTION_INLINE_SFPU","1",1);
	if (optimized) setenv("PIONEER_ATTENTION_OPTIMIZED","1",1);
	if (finite) setenv("PIONEER_ATTENTION_FAST_HALF","1",1);
	Image next(argv[2],false,true,true); assert(next.inlineSfpu && !prior.inlineSfpu && next.optimizedAttention==optimized && next.finiteHalf==finite);
	for (unsigned tokens:{1U,2U,8U,17U,31U,32U,33U,64U,65U,127U,128U,129U,255U,256U}) for (unsigned group:{0U,7U}) {
		RopeGroupMachine a(prior,tokens,27,group,tokens-1),b(next,tokens,27,group,tokens-1);
		a.Run(); b.Run(); a.VerifyRope(); b.VerifyRope();
		for (unsigned word=0;word<256;word++) assert(a.mem[0xb2000/4+word]==b.mem[0xb2000/4+word]);
		assert(a.dram==b.dram && a.reads==b.reads && a.writes==b.writes && a.pullReads==b.pullReads);
		assert(b.directInstructions && b.mmioWrites<a.mmioWrites && b.steps<a.steps);
		printf("OFFLINE inline-attention n%u group%u ALL256 output bits/whole KV/counters EXACT;steps%u/%u commandMMIO%u/%u direct%u\n",tokens,group,b.steps,a.steps,b.mmioWrites,a.mmioWrites,b.directInstructions);
	}
	for (unsigned layer:{0U,27U}) for (unsigned group=0;group<6;group++) {
		GluMachine a(prior,layer,group),b(next,layer,group); a.Run(); b.Run(); a.VerifyGlu(); b.VerifyGlu();
		for (unsigned word=0;word<272;word++) assert(a.mem[0x91000/4+word]==b.mem[0x91000/4+word]);
		printf("OFFLINE inline-GLU layer%u group%u ALL272 operand/scale bits EXACT;steps%u/%u commandMMIO%u/%u direct%u\n",layer,group,b.steps,a.steps,b.mmioWrites,a.mmioWrites,b.directInstructions);
		assert(b.directInstructions && b.mmioWrites<a.mmioWrites);
	}
	puts("PASS OFFLINE exact inline attention/GLU bit-identical to qualified36,all32-lane boundaries,strict SAME SFPU opcodes/guards/final drain;NOT native performance");
}
