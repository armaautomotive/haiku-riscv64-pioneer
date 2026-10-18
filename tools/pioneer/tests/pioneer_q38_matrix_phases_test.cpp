/* SPDX-License-Identifier: MIT */
#include <cstdint>
#include "../PioneerQ38MatrixPhases.h"
#include <cassert>
#include <cstdio>
#include <vector>
struct PhaseEvent { bool drain; uint32_t word; };
struct PhaseProgram {
	std::vector<PhaseEvent> events;
	void Op(uint32_t word) { events.push_back({false,word}); }
	void Drain() { events.push_back({true,0}); }
};
int main()
{
	PhaseProgram setup; PioneerQ38MatrixPhasesSetup(setup);
	assert(setup.events.size()==3 && setup.events[0].drain && setup.events[2].drain);
	assert(!setup.events[1].drain && setup.events[1].word==0xb21d2000);
	// Independent cfg81989dcdb definitions:Dst modifier1 is thread29;
	// FidelityIncr bits13..14,all other fields0. AB13/Bias48 stay0 from
	// existing all68-thread-register reset. No Src/Dst row/bias changes.
	unsigned config=setup.events[1].word&65535;
	assert(((setup.events[1].word>>16)&255)==29 && config==0x2000);
	assert(((config>>13)&3)==1 && !(config&0x9fff));
	PhaseProgram p; PioneerQ38MatrixPhases(p); assert(p.events.size()==8);
	for (unsigned phase=0;phase<4;phase++) {
		assert(!p.events[phase*2].drain && p.events[phase*2+1].drain);
		assert(p.events[phase*2].word==(phase==3?0x26c04020U:0x26004020U));
	}
	unsigned products=0;
	for (int a=-128;a<128;a++) for (int b=-128;b<128;b++) {
		int sum=0; unsigned phase=0;
		for (unsigned instruction=0;instruction<4;instruction++) {
			uint32_t word=p.events[instruction*2].word;
			assert(phase==instruction && ((word>>14)&3)==1);
			int aa=a<0?-a:a,bb=b<0?-b:b;
			// Independent pinned MVMUL integer masks after Src overlay:
			// A high3/low5 magnitude bits,B high6/low4 (NOT float phases).
			aa&=(phase&1)?31:224; bb&=(phase&2)?15:1008;
			sum+=(a<0?-aa:aa)*(b<0?-bb:bb);
			assert(bool(word&0xc00000)==(instruction==3)); // ONLY final flips both banks
			phase=(phase+((config>>13)&3))&3;
		}
		assert(!phase && sum==a*b); products++;
	}
	printf("PASS OFFLINE matrix phase schedule:%u exhaustive signed8 products,EXACT phases0/1/2/3 and wrap0,ONLY final bank flip,all4 matrix drains retained;NO native timing/qualification\n",products);
}
