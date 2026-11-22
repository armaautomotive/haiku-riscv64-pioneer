/* SPDX-License-Identifier: MIT */
#define PIONEER_QWEN35_DEVICE_DISPATCH 1
#include "../PioneerQwen35DataflowGraphSession.h"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ35AlignedAllocation.h"
#include <cassert>
#include <cstdio>

struct alignas(64) Probe {
	unsigned& destroyed;
	unsigned value;
	Probe(unsigned& count,unsigned v):destroyed(count),value(v) {}
	~Probe() { destroyed++; }
};

int main() {
	unsigned destroyed=0;
	for (unsigned i=0;i<32;i++) {
		auto* p=PioneerQ35NewAligned<Probe>(destroyed,i);
		assert(p && !(reinterpret_cast<uintptr_t>(p)&63) && p->value==i);
		PioneerQ35DeleteAligned(p);
	}
	assert(destroyed==32);
	auto* run=PioneerQ35NewAligned<PioneerQwen35::DataflowGraphSessionRun>();
	assert(run && !(reinterpret_cast<uintptr_t>(run)&63)
		&& !(reinterpret_cast<uintptr_t>(&run->scratch)&63) && !run->attempted);
	PioneerQ35DeleteAligned(run);
	PioneerQ35DeleteAligned<Probe>(nullptr);
	puts("PASS graph allocation: 64-byte placement alignment and matching destruction");
	return 0;
}
