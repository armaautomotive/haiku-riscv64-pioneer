/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_PUBLIC_TIMING_H
#define PIONEER_Q38_PUBLIC_TIMING_H
#include <math.h>
#include <string.h>
// Fixed PUBLIC smoke/timing choices only;never an arbitrary prompt/length.
static inline unsigned PioneerQ38PublicTokenLimit(const char* argument)
{
	if (!argument) return 0;
	return !strcmp(argument,"--public-one-token")?1:!strcmp(argument,"--public-four-tokens")?4:
		!strcmp(argument,"--public-32-tokens")?32:0;
}
// Diagnostics ONLY,NOT device completion/timeout/cleanup authority. Observe
// all32 positions in order. Exclude cold0 plus warm-up1..4 from the rate.
// One short PUBLIC sequence is NOT quality or repeated benchmark evidence.
class PioneerQ38PublicTiming {
	double sample[27]={},total=0;
	unsigned next=0,measured=0;
	bool valid=true;
public:
	bool Observe(unsigned position,double seconds) {
		if (!valid || position!=next || position>=32 || !isfinite(seconds) || seconds<=0) { valid=false; return false; }
		if (position>=5) {
			if (!isfinite(total+seconds)) { valid=false; return false; }
			sample[measured++]=seconds; total+=seconds;
		}
		next++; return true;
	}
	bool Complete() const { return valid && next==32 && measured==27 && total>0 && isfinite(total); }
	unsigned Measured() const { return measured; }
	double Seconds() const { return Complete()?total:0; }
	double Rate() const { return Complete()?27/total:0; }
	double Percentile(unsigned rank) const {
		if (!Complete() || rank>=27) return 0;
		double sorted[27]; for (unsigned i=0;i<27;i++) sorted[i]=sample[i];
		for (unsigned i=1;i<27;i++) for (unsigned j=i;j && sorted[j]<sorted[j-1];j--) {
			double saved=sorted[j]; sorted[j]=sorted[j-1]; sorted[j-1]=saved;
		}
		return sorted[rank];
	}
};
#endif
