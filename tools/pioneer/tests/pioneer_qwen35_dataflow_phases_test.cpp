/* SPDX-License-Identifier: MIT */
#include "../PioneerQwen35DataflowTensixPhases.h"
#include <cassert>
#include <cstdint>
#include <vector>
using namespace PioneerQwen35;
struct Event { bool drain; uint32_t word; };
struct Emitter {
	std::vector<Event> events;
	void Op(uint32_t word) { events.push_back({false,word}); }
	void Drain() { events.push_back({true,0}); }
};
int main()
{
	for (unsigned type:{8U,12U,13U,14U}) {
		Emitter setup,phases;
		DataflowTensixPhaseSetup(setup,type);
		DataflowTensixPhases(phases,type);
		const bool small=type==12 || type==13;
		assert(setup.events.size()==4 && setup.events[0].drain && setup.events[3].drain);
		assert(setup.events[1].word==(small?0xb20b0001U:0xb20b0000U));
		assert(setup.events[2].word==(small?0xb21d4000U:0xb21d2000U));
		const unsigned count=small?2:4;
		assert(phases.events.size()==2*count);
		for (unsigned phase=0;phase<count;phase++) {
			assert(!phases.events[2*phase].drain && phases.events[2*phase+1].drain);
			assert(phases.events[2*phase].word==(phase+1==count?0x26c04020U:0x26004020U));
		}
		if (small) for (int weight=0;weight<=(type==12?15:31);weight++) {
			for (int activation=-127;activation<=127;activation++) {
				const int magnitude=activation<0?-activation:activation;
				const int low=weight*(magnitude&1008),high=weight*(magnitude&15);
				assert((activation<0?-low-high:low+high)==weight*activation);
				assert((weight&224)==0); // omitted phases 0 and 2
			}
		}
	}
}
