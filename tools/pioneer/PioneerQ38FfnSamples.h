/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_FFN_SAMPLES_H
#define PIONEER_Q38_FFN_SAMPLES_H
#include <cmath>
// CPU-clock diagnostic DATA ONLY. Invalid/partial timings never authorize
// device work,timeout,reset,cleanup or a numerical result.
class PioneerQ38FfnSamples {
	double samples[32]={}; unsigned count=0; bool valid=true;
public:
	struct Summary { double mean=0,median=0,minimum=0,maximum=0; };
	unsigned Count() const { return count; }
	bool Add(double start,double end) {
		if (count>=32) { valid=false; return false; }
		bool okay=std::isfinite(start) && std::isfinite(end) && start>=0 && end>=start;
		double delta=okay?end-start:0; okay &= std::isfinite(delta);
		samples[count++]=okay?delta:0; valid &= okay; return okay;
	}
	bool Finish(Summary& result) const {
		result={}; if (!valid || count!=32) return false;
		double sorted[32],sum=0;
		for (unsigned i=0;i<32;i++) { sorted[i]=samples[i]; sum+=samples[i]; }
		if (!std::isfinite(sum)) return false;
		for (unsigned i=1;i<32;i++) for (unsigned j=i;j && sorted[j]<sorted[j-1];j--) {
			double value=sorted[j]; sorted[j]=sorted[j-1]; sorted[j-1]=value;
		}
		result.mean=sum/32; result.median=sorted[15]/2+sorted[16]/2;
		result.minimum=sorted[0]; result.maximum=sorted[31]; return true;
	}
};
#endif
