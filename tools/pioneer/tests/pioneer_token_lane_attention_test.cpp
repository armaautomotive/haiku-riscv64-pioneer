/* SPDX-License-Identifier: MIT */
#define PIONEER_ROPE_GROUP_MACHINE_ONLY
#include "pioneer_rope_attention_image_test.cpp"
int main(int argc,char** argv) {
	assert(argc==3); setvbuf(stdout,NULL,_IONBF,0);
	for (const char* key:{"PIONEER_COORDINATED_PULL_INPUT","PIONEER_ATTENTION_PARALLEL_ROPE","PIONEER_GLU_VECTOR"}) setenv(key,"1",1);
	unsetenv("PIONEER_ATTENTION_FAST_SOFTMAX"); unsetenv("PIONEER_ATTENTION_TOKEN_LANES");
	Image prior(argv[1],false,true,true); setenv("PIONEER_ATTENTION_TOKEN_LANES","1",1);
	Image next(argv[2],false,true,true); assert(next.tokenLanes && !prior.tokenLanes);
	for (unsigned tokens:{1U,2U,8U,17U,31U,32U,33U,64U,65U,127U,128U,129U,255U,256U}) for (unsigned group:{0U,7U}) {
		RopeGroupMachine a(prior,tokens,27,group,tokens-1),b(next,tokens,27,group,tokens-1);
		a.Run(); b.Run(); a.VerifyRope(); b.VerifyRope();
		for (unsigned word=0;word<256;word++) assert(a.mem[0xb2000/4+word]==b.mem[0xb2000/4+word]);
		assert(a.dram==b.dram && a.reads==b.reads && a.writes==b.writes && a.pullReads==b.pullReads);
		printf("OFFLINE token-lane n%u group%u ALL256 output bits/whole KV/NoC counters EXACT;steps%u/%u\n",tokens,group,b.steps,a.steps);
	}
	puts("PASS OFFLINE token-parallel attention bit-identical to qualified36 across all32-lane boundaries,unchanged strict ISA/MMIO/proof;NOT native performance");
}
