/* SPDX-License-Identifier: MIT */
#include "../PioneerQ38VectorFixedFormat.h"
#include <cassert>
#include <cstdio>
#include <vector>
struct Event { unsigned kind,index; uint32_t value; };
struct FixedProgram {
	std::vector<Event> ops;
	void Config(unsigned i,uint32_t v) { ops.push_back({1,i,v}); }
	void Op(uint32_t v) { ops.push_back({2,0,v}); }
	void Drain() { ops.push_back({3,0,0}); }
};
int main()
{
	uint32_t fields[]={0x4231,0xa03c8000,1,1};
	assert(PioneerQ38VectorFixedMatrixEquivalent(fields[0],fields[1],fields[2],fields[3]));
	unsigned rejected=0;
	for (unsigned f=0;f<4;f++) for (unsigned bit=0;bit<32;bit++) {
		fields[f]^=uint32_t(1)<<bit;
		assert(!PioneerQ38VectorFixedMatrixEquivalent(fields[0],fields[1],fields[2],fields[3])); rejected++;
		fields[f]^=uint32_t(1)<<bit;
	}
	// Independent pinned register definitions81989dcdb + MVMUL/MOVD2A
	// functional semantics8a34e25e. Both forced formats remain1;all other
	// Config1 bits identical. This is not merely relaxing a model constant.
	assert((fields[0]&0x10) && (fields[0]&15)==1 && (fields[0]&0x200) && ((fields[0]>>5)&15)==1);
	assert((fields[1]^0xa0220000)==0x1e8000 && (fields[1]&0xa0000000)==0xa0000000);
	FixedProgram setup; PioneerQ38VectorFixedSetup(setup);
	const unsigned indices[]={1,49,64,72,73};
	const uint32_t values[]={0xa03c8000,0,0x10001e,14,0x13};
	assert(setup.ops.size()==7 && setup.ops.front().kind==3 && setup.ops.back().kind==3);
	for (unsigned i=0;i<5;i++) assert(setup.ops[i+1].kind==1 && setup.ops[i+1].index==indices[i] && setup.ops[i+1].value==values[i]);
	unsigned panels=0;
	for (unsigned type:{8U,12U,14U}) for (unsigned panel=0;panel<(type==8?2U:16U);panel++) {
		FixedProgram p; assert(PioneerQ38VectorFixedUnpack(p,type,panel));
		bool unpackMode=false,published=false; unsigned threadWrites=0,raw=0;
		for (const auto& e:p.ops) {
			if (e.kind==1) assert(e.index==76 || (type==14 && e.index==84));
			if (e.kind!=2) continue;
			if (e.value==0xb2050000) { assert(!published); unpackMode=true; threadWrites++; }
			if ((e.value>>24)==0x42) { assert(unpackMode); raw++; if (e.value&64) published=true; }
			if (e.value==0xb2050004) { assert(published); unpackMode=false; threadWrites++; }
			if ((e.value>>24)==8) assert(published && !unpackMode);
		}
		assert(threadWrites==2 && raw==(type==14?2U:1U) && !unpackMode); panels++;
	}
	for (unsigned type:{0U,8U,12U,14U,99U}) { FixedProgram p; assert(!PioneerQ38VectorFixedUnpack(p,type,type==8?2:16) && p.ops.empty()); }
	printf("PASS OFFLINE fixed-format equivalence:%u single-field mutations refused,%u panel sequences preserve exact raw publication/Thread5 0->4/MOV ownership,ONLY checked source offsets vary;forced A/B format and all remaining matrix bits identical;NO native qualification\n",rejected,panels);
}
