/* SPDX-License-Identifier: MIT */
#include "../PioneerQ38PublicTiming.h"
#include <cassert>
#include <cfloat>
#include <cstdio>
#include <initializer_list>
int main()
{
	assert(PioneerQ38PublicTokenLimit("--public-one-token")==1);
	assert(PioneerQ38PublicTokenLimit("--public-four-tokens")==4);
	assert(PioneerQ38PublicTokenLimit("--public-32-tokens")==32);
	assert(PioneerQ38PublicTokenLimit("--public-three-32-token-runs")==32 && PioneerQ38PublicSequenceCount("--public-three-32-token-runs")==3);
	assert(PioneerQ38PublicSequenceCount("--public-four-tokens")==1 && !PioneerQ38PublicSequenceCount(nullptr));
	for (const char* invalid:{static_cast<const char*>(nullptr),"","--public-0-tokens","--public-33-tokens","--public-32-tokens=1000","--public-32-tokens ","--private-prompt","--public-ffn-graph-export"})
		assert(!PioneerQ38PublicTokenLimit(invalid));
	PioneerQ38PublicTiming t;
	for (unsigned p=0;p<32;p++) {
		assert(!t.Complete() && !t.Rate() && !t.Seconds() && !t.Percentile(0));
		assert(t.Measured()==(p>5?p-5:0));
		assert(t.Observe(p,!p?1000:p<5?100:double(p-4)));
	}
	assert(t.Complete() && t.Measured()==27 && t.Seconds()==378 && t.Rate()==1.0/14);
	assert(t.Percentile(0)==1 && t.Percentile(13)==14 && t.Percentile(25)==26 && t.Percentile(26)==27 && !t.Percentile(27));
	assert(!t.Observe(32,1) && !t.Complete() && !t.Rate());
	for (double bad:{0.0,-1.0,double(INFINITY),-double(INFINITY),double(NAN)}) {
		PioneerQ38PublicTiming broken; assert(!broken.Observe(0,bad) && !broken.Complete() && !broken.Rate());
	}
	for (unsigned missing:{0U,1U,5U,31U}) {
		PioneerQ38PublicTiming broken;
		for (unsigned p=0;p<missing;p++) assert(broken.Observe(p,1));
		assert(!broken.Observe(missing+1,1) && !broken.Complete());
	}
	PioneerQ38PublicTiming duplicate; assert(duplicate.Observe(0,1) && !duplicate.Observe(0,1) && !duplicate.Rate());
	PioneerQ38PublicTiming overflow; for (unsigned p=0;p<6;p++) assert(overflow.Observe(p,DBL_MAX));
	assert(!overflow.Observe(6,DBL_MAX) && !overflow.Complete() && !overflow.Rate());
	PioneerQ38PublicRepeatedTiming repeated;
	for (unsigned sequence=0;sequence<3;sequence++) for (unsigned position=0;position<32;position++) {
		assert(!repeated.Complete() && !repeated.Rate());
		assert(repeated.Observe(sequence,position,position<5?1000:double(sequence+1)));
	}
	assert(repeated.Complete() && repeated.Seconds()==162 && repeated.Rate()==.5);
	for (unsigned sequence=0;sequence<3;sequence++) assert(repeated.Sequence(sequence)->Rate()==1.0/(sequence+1));
	assert(!repeated.Sequence(3) && !repeated.Observe(3,0,1) && !repeated.Complete() && !repeated.Rate());
	for (unsigned sequence=0;sequence<3;sequence++) {
		PioneerQ38PublicRepeatedTiming bad;
		for (unsigned previous=0;previous<sequence;previous++) for (unsigned position=0;position<32;position++) assert(bad.Observe(previous,position,1));
		assert(!bad.Observe(sequence,1,1) && !bad.Complete() && !bad.Rate());
	}
	PioneerQ38PublicRepeatedTiming badSequence; assert(!badSequence.Observe(1,0,1) && !badSequence.Rate());
	PioneerQ38PublicRepeatedTiming repeatedOverflow;
	for (unsigned sequence=0;sequence<2;sequence++) for (unsigned position=0;position<32;position++)
		assert(repeatedOverflow.Observe(sequence,position,position<5?1:DBL_MAX/32));
	for (unsigned position=0;position<32;position++) assert(repeatedOverflow.Observe(2,position,position<5?1:DBL_MAX/32));
	assert(!repeatedOverflow.Complete() && !repeatedOverflow.Rate()); // individually finite,aggregate overflow
	puts("PASS PUBLIC timing:ONLY fixed1/4/32-token options,ordered complete32 positions,cold0+warmup1..4 excluded,27-position throughput/median/nearest-rank95th,invalid/missing/duplicate/nonfinite/overflow timing NEVER yields a rate;NO device IO or completion authority");
	puts("PASS repeated PUBLIC timing:ONLY fixed3x32 choice,ALL96 positions ordered,first5 per sequence excluded,81-position aggregate and all3 rates,missing/interleaved/partial/overflow NEVER publishes a rate;NO state reset/device/completion authority");
}
