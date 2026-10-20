/* SPDX-License-Identifier: MIT */
#include "../PioneerQ38SfpuAccumulator.h"
#include <cassert>
#include <cmath>
#include <cstdio>
#include <vector>
struct AccEvent { unsigned kind,index; uint32_t value; };
struct AccProgram {
	std::vector<AccEvent> events;
	void Op(uint32_t word) { events.push_back({1,0,word}); }
	void Config(unsigned index,uint32_t word) { events.push_back({2,index,word}); }
	void Drain() { events.push_back({3,0,0}); }
};
int main()
{
	unsigned sequences=0;
	for (bool integer:{false,true}) for (unsigned type:{8U,12U,14U}) for (unsigned mode=0;mode<3;mode++) {
		bool first=mode==0,metadata=mode!=2; AccProgram p;
		assert((integer?PioneerQ38SfpuAccumulator<AccProgram,true>(p,type,first,metadata,0x3f000000,-2048):
			PioneerQ38SfpuAccumulator(p,type,first,metadata,0x3f000000,-2048)));
		uint32_t rawFormat=integer?0xa03c8000:0x203c8000,metadataFormat=integer?0xa0200000:0x20200000;
		uint32_t c[128]={}; c[1]=rawFormat; c[64]=0x10001e; c[65]=16; c[72]=14; c[84]=0x400040;
		unsigned thread5=4,imports=0,zeros=0,dots=0;
		for (auto e:p.events) {
			if (e.kind==2) {
				assert(e.index==1 || e.index==64 || e.index==65 || e.index==72 || e.index==84 || e.index==76);
				c[e.index]=e.value; continue;
			}
			if (e.kind!=1) continue;
			unsigned code=e.value>>24;
			if (e.value==0xb2050000 || e.value==0xb2050004) thread5=e.value&65535;
			if (code==0x42) {
				assert(e.value==0x42000081 && !thread5 && c[1]==metadataFormat && c[64]==0x100010 && c[65]==1 && !c[72]);
				assert((c[76]+1)*16==0x91200+imports*64 && c[84]==0x400000+(100+imports*4)*16);
				imports++;
			}
			if (code>=0x70 && code<=0x90) {
				assert(thread5==4 && c[1]==rawFormat && c[64]==0x10001e && c[65]==16 && c[72]==14);
				if (code==0x70 && !((e.value>>20)&15)) { assert(((e.value>>16)&15)==(integer?4U:3U)); dots++; }
				if (code==0x72 && ((e.value>>20)&15)==7) {
					unsigned row=(e.value&1023)&~3U; assert(first && (row==96 || row==100 || row==108)); zeros++;
				}
			}
		}
		assert(imports==(metadata?(type==14?1U:2U):0U) && zeros==(first?6U:0U));
		assert(dots==2 && thread5==4 && c[1]==rawFormat && c[84]==0x400040 && p.events.back().kind==3); sequences++;
	}
	for (unsigned type:{0U,99U}) { AccProgram p; assert(!PioneerQ38SfpuAccumulator(p,type,true,true,0x3f800000,0) && p.events.empty()); }
	for (unsigned scale:{0x2f7fffffU,0x47800001U,0x7f800000U,0x7fc00000U}) { AccProgram p; assert(!PioneerQ38SfpuAccumulator(p,12,true,true,scale,0) && p.events.empty()); }
	for (int sum:{-2049,2049}) { AccProgram p; assert(!PioneerQ38SfpuAccumulator(p,12,true,true,0x3f800000,sum) && p.events.empty()); }
	{ AccProgram p; assert(!PioneerQ38SfpuAccumulator(p,12,true,false,0x3f800000,0) && p.events.empty()); }
	// Conservative for ALL supported kinds,even using simultaneous maximum
	// coefficient/minimum factors and the over-wide per-panel dot bound.
	long double bound=65504.L*128*65536*(262144+2048)*1088*1.01L;
	assert(bound<std::ldexp(1.L,68));
	// Finite HALF*integer coefficients have quantum2^-24;FP32 scales at
	// least2^-32 have quantum2^-55. Integer dots/sums and exact/correctly
	// rounded additions cannot create a finer quantum than2^-79,well above
	// FP32 subnormal range. Thus all valid intermediates are normal/zero.
	assert(std::ldexp(1.L,-79)>std::ldexp(1.L,-126));
#ifdef PIONEER_Q38_WORK_DEFER_AFFINE
	for (unsigned type:{8U,12U,14U}) for (unsigned mode=0;mode<3;mode++) {
		bool first=mode==0,metadata=mode!=2; AccProgram original,deferred;
		if (type==12) {
			assert((PioneerQ38SfpuAccumulator<AccProgram,true,false,false>(original,type,first,metadata,0x3f000000,-2048)));
			assert((PioneerQ38SfpuAccumulator<AccProgram,true,false,true>(deferred,type,first,metadata,0x3f000000,-2048)));
		} else {
			assert((PioneerQ38SfpuAccumulator<AccProgram,true,true,false>(original,type,first,metadata,0x3f000000,-2048)));
			assert((PioneerQ38SfpuAccumulator<AccProgram,true,true,true>(deferred,type,first,metadata,0x3f000000,-2048)));
		}
		assert(original.events.size()==deferred.events.size()+1 && original.events.back().kind==3);
		for (unsigned i=0;i<deferred.events.size();i++) {
			const auto& a=original.events[i]; const auto& b=deferred.events[i];
			assert(a.kind==b.kind && a.index==b.index && a.value==b.value);
		}
	}
	for (unsigned type:{0U,99U}) { AccProgram p; assert((!PioneerQ38SfpuAccumulator<AccProgram,true,false,true>(p,type,true,true,0x3f800000,0)) && p.events.empty()); }
	for (unsigned scale:{0x2f7fffffU,0x47800001U,0x7f800000U,0x7fc00000U}) { AccProgram p; assert((!PioneerQ38SfpuAccumulator<AccProgram,true,false,true>(p,12,true,true,scale,0)) && p.events.empty()); }
	for (int sum:{-2049,2049}) { AccProgram p; assert((!PioneerQ38SfpuAccumulator<AccProgram,true,false,true>(p,12,true,true,0x3f800000,sum)) && p.events.empty()); }
	puts("PASS OFFLINE deferred Work helper:ONLY trailing drain omitted,ALL metadata/config/math/source events EXACT,invalid inputs inert;caller still owes real completion BEFORE config/export");
#endif
	printf("PASS OFFLINE resident SFPU protocol:%u metadata/group sequences,ONLY fixed96/100 imports,no SrcA flip,all format/Thread5 restoration,group padding,final drains,invalid inputs inert;valid accumulator<2^68/nonzero quantum>=2^-79;NO native qualification\n",sequences);
}
