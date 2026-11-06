/* SPDX-License-Identifier: MIT */
// Host-only operation inventory of the SAME templates used by tag106.
// Counts requested operations, not instructions retired, cycles, or speed.
#include <cstdint>
#include <cstdio>
#include "PioneerQ38Q4FpuCompactStreamCompute.h"

struct Counts {
	uint64_t ops=0,configs=0,drains=0,matrix=0;
	void Op(unsigned value) {
		ops++;
		// The complete four-fidelity MVMUL group has opcode 0x26.
		if ((value>>24)==0x26) matrix++;
	}
	void Config(unsigned,unsigned) {configs++;}
	void Drain() {drains++;}
};

static void Print(const char* scope,unsigned buffer,unsigned group,const Counts& c) {
	std::printf("%s buffer%u group%u ops%llu configs%llu drains%llu matrix%llu\n",
		scope,buffer,group,(unsigned long long)c.ops,(unsigned long long)c.configs,
		(unsigned long long)c.drains,(unsigned long long)c.matrix);
}

int main() {
	using C=PioneerQ38Q4FpuCompactStreamCompute;
	Counts begin;C::Begin(begin);Print("BEGIN",0,0,begin);
	for(unsigned buffer=0;buffer<2;buffer++) {
		Counts metadata;
		if(!C::LoadMetadata(metadata,buffer))return 1;
		Print("METADATA",buffer,0,metadata);
		for(unsigned group=0;group<8;group++) {
			Counts compute;
			if(!C::Group(compute,buffer,group))return 1;
			Print("GROUP",buffer,group,compute);
		}
	}
	// These invalid selectors must have no emitter side effects.
	Counts refused;
	if(C::LoadMetadata(refused,2) || C::Group(refused,2,0)
		|| C::Group(refused,0,8) || refused.ops || refused.configs
		|| refused.drains || refused.matrix)return 1;
	std::puts("HOST_TEMPLATE_OPERATION_COUNTS_ONLY;NO_DEVICE_NO_CYCLE_OR_TPS_MEASUREMENT");
	return 0;
}
