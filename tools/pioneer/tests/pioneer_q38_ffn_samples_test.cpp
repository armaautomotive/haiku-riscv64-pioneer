/* SPDX-License-Identifier: MIT */
#include <cassert>
#include <cstdio>
#include <limits>
#include <initializer_list>
#include "../PioneerQ38FfnSamples.h"
int main() {
	for (unsigned order=0;order<3;order++) {
		PioneerQ38FfnSamples s; PioneerQ38FfnSamples::Summary result;
		assert(!s.Finish(result));
		for (unsigned i=0;i<32;i++) {
			unsigned value=order==0?i:order==1?31-i:(i*13)%32;
			assert(s.Add(100,100+value) && s.Count()==i+1);
			if(i<31) assert(!s.Finish(result));
		}
		assert(s.Finish(result) && result.mean==15.5 && result.median==15.5 && !result.minimum && result.maximum==31);
		assert(!s.Add(0,1) && !s.Finish(result));
	}
	for (double bad:{-1.0,std::numeric_limits<double>::infinity(),-std::numeric_limits<double>::infinity(),std::numeric_limits<double>::quiet_NaN()}) {
		PioneerQ38FfnSamples s; PioneerQ38FfnSamples::Summary r;
		assert(!s.Add(bad,1)); for(unsigned i=1;i<32;i++) assert(s.Add(0,1)); assert(!s.Finish(r));
	}
	PioneerQ38FfnSamples back; assert(!back.Add(2,1));
	PioneerQ38FfnSamples overflow; PioneerQ38FfnSamples::Summary r;
	for(unsigned i=0;i<32;i++) assert(overflow.Add(0,std::numeric_limits<double>::max()));
	assert(!overflow.Finish(r));
	puts("PASS fixed32 FFN timing DATA:permutations/median/mean/extrema,partial/overflow/backward/nonfinite refusal;NO device/control authority");
}
