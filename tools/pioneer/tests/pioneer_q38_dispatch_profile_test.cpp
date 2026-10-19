/* SPDX-License-Identifier: MIT */
#include <cassert>
#include <cstdint>
#include <cstdio>
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38DispatchProfile.h"
struct Clock {
	uint64_t now=100;
	unsigned reads=0,reports=0;
	bool valid=false;
	uint64_t result[8]={};
	uint64_t Q38CohortNow() { reads++; return now; }
	void Q38DispatchProfile(unsigned type,unsigned inputs,unsigned peers,bool okay,const uint64_t* values) {
		assert(type==12 && inputs==17408 && peers==119); reports++; valid=okay;
		for (unsigned i=0;i<8;i++) result[i]=values[i];
	}
};
int main() {
	for (unsigned fault=0;fault<4;fault++) {
		Clock c;
		PioneerQ38DispatchProfile<Clock> p(c);
		for (unsigned i=0;i<(fault==2?7U:8U);i++) {
			c.now+=i+1;
			if (fault==1 && i==3) c.now=1;
			p.Mark();
		}
		if (fault==3) p.Mark();
		p.Publish(12,17408,119);
#ifdef TT_PIONEER_Q38_DISPATCH_PROFILE
		assert(c.reports==1 && c.valid==(fault==0));
		if (!fault) for (unsigned i=0;i<8;i++) assert(c.result[i]==i+1);
#else
		assert(!c.reads && !c.reports);
#endif
	}
	puts("PASS CPU-only dispatch profile:8 ordered stages,backward clock/incomplete/extra marks invalidate diagnostics ONLY;disabled build NO clock/report calls");
}
