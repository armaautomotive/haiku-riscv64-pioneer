/* SPDX-License-Identifier: MIT */
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <vector>
#include "../PioneerQ38Q4Phases.h"
struct Event { bool drain; uint32_t word; };
struct Emitter {
	std::vector<Event> events;
	void Op(uint32_t word) { events.push_back({false,word}); }
	void Drain() { events.push_back({true,0}); }
};
int main()
{
	unsigned products=0;
	for (bool queued:{false,true}) for (unsigned type:{8U,12U,14U}) {
		Emitter setup,body; PioneerQ38Q4PhasesSetup(setup,type);
		if (queued) PioneerQ38QueuedPhases(body,type); else PioneerQ38Q4Phases(body,type);
		assert(setup.events.size()==4 && setup.events[0].drain && setup.events[3].drain);
		assert(!setup.events[1].drain && !setup.events[2].drain);
		assert(setup.events[1].word==(type==12?0xb20b0001U:0xb20b0000U));
		assert(setup.events[2].word==(type==12?0xb21d4000U:0xb21d2000U));
		unsigned base=setup.events[1].word&65535,modifier=setup.events[2].word&65535;
		assert(!(modifier&0x9fff)); // ONLY fidelity increment, no AB/Dst/Bias movement
		unsigned steps=type==12?2:4; assert(body.events.size()==(queued?steps+1:2*steps));
		std::vector<uint32_t> operations; unsigned drains=0;
		for (auto event:body.events) { if (event.drain) drains++; else operations.push_back(event.word); }
		assert(operations.size()==steps && drains==(queued?1U:steps) && body.events.back().drain);
		int first=type==12?0:type==14?-32:-128,last=type==12?15:type==14?31:127;
		for (int a=first;a<=last;a++) for (int b=-1023;b<=1023;b++) {
			int sum=0; unsigned rwc=0,visits[4]={};
			for (unsigned i=0;i<steps;i++) {
				assert(operations[i]==(i+1==steps?0x26c04020U:0x26004020U));
				unsigned phase=(base+rwc)&3; visits[phase]++;
				int aa=(a<0?-a:a)&(phase&1?31:224),bb=(b<0?-b:b)&(phase&2?15:1008);
				sum+=(a<0?-aa:aa)*(b<0?-bb:bb);
				rwc=(rwc+((modifier>>13)&3))&3;
			}
			assert(!rwc && sum==a*b);
			for (unsigned p=0;p<4;p++) assert(visits[p]==(type==12?unsigned(p&1):1U));
			products++;
		}
	}
	// A Q6 value of -32 is NOT legal in the two-phase Q4 contract. The
	// negative control proves blindly extending this optimization loses data.
	int missing=(-32&31)*(127&1008)+(-32&31)*(127&15); assert(missing!=-32*127);
	printf("PASS Q4 INTEGER schedules: %u exhaustive exact products, Q4 phases1/3 only, Q8/Q6 all4; checked per-phase drains OR opt-in queued batch with ONE final drain; SAME order/final flip/no format change\n",products);
}
