/* SPDX-License-Identifier: MIT */
#define PIONEER_COORDINATED_GLU_MACHINE_ONLY
#include "pioneer_coordinated_glu_image_test.cpp"
int main(int argc,char** argv) {
	assert(argc==3); unsetenv("PIONEER_ATTENTION_CACHED_QUERY"); Image prior(argv[1],false,true,true);
	setenv("PIONEER_ATTENTION_CACHED_QUERY","1",1); Image cached(argv[2],false,true,true);
	uint64_t oldSteps=0,newSteps=0;
	for (unsigned n:{1U,2U,17U,64U,65U,256U}) for (unsigned g:{0U,7U}) {
		GroupMachine a(prior,n,27,g),b(cached,n,27,g); std::vector<float> q(256); std::vector<uint16_t> k(n*128),v(k.size());
		const uint32_t edges[]={0,0x80000000,1,0x80000001,0x33800000,0xb3800000,0x38800000,0x477fe000};
		for (unsigned i=0;i<256;i++) q[i]=i<8?F(edges[i]):float(sin(i*.173+g*.137)*1.7);
		for (unsigned i=0;i<k.size();i++) { _Float16 kh=_Float16(sin(i*.371)*1.3),vh=_Float16(cos(i*.217)*2); memcpy(&k[i],&kh,2); memcpy(&v[i],&vh,2); }
		a.Inputs(q,k,v); b.Inputs(q,k,v); a.Run(); b.Run(); a.Verify(); b.Verify();
		for (unsigned i=0;i<256;i++) {
			assert(a.mem[0xb2000/4+i]==b.mem[0xb2000/4+i]);
			assert(a.mem[0x90000/4+i]==B(q[i]) && b.mem[0x90000/4+i]==B(q[i]));
			assert(b.mem[0x90800/4+i]==B(PioneerAttentionHalf(PioneerAttentionToHalf(q[i]))));
		}
		assert(a.dram==b.dram && !a.queryCacheWrites && b.queryCacheWrites==256);
		assert(b.mem[0x907fc/4]==0xdeadbeef && b.mem[0x90c00/4]==0xdeadbeef && a.drains==b.drains && a.mads==b.mads);
		oldSteps+=a.steps; newSteps+=b.steps;
		printf("PASS cached query n%u group%u all256 exact output/input/cache bits,KV/proof/guards;steps%u vs%u\n",n,g,b.steps,a.steps);
	}
	assert(newSteps<oldSteps);
	puts("PASS OFFLINE exact cached query all lengths/edge rounding,unchanged operations/NoC/output bits;NOT native performance");
}
