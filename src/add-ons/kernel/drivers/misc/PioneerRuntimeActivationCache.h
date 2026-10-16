/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_RUNTIME_ACTIVATION_CACHE_H
#define PIONEER_RUNTIME_ACTIVATION_CACHE_H
#include "PioneerRuntimeMatvec.h"
#include <string.h>

// CPU shadow for ONE exclusively owned core/code lease, not a device read
// or a hash-only cache. This does not authorize skipping a transfer by
// itself. A future caller must also prove held reset, source/PCI/TLB/host
// mapping stability and idle NoC, AND the immutable bulk image's read-only
// 80000..83fff input footprint. Never share across sessions/cores/formats.
// Only the explicit reuse operation may enable it; the uncached ABI is unchanged.
class PioneerRuntimeActivationCache {
	bool pending=false,valid=false;
	unsigned inputs=0;
	uint32_t operand[TT_RUNTIME_MATVEC_INPUTS_MAX/2]={};
	uint32_t scale[TT_RUNTIME_MATVEC_INPUTS_MAX/32]={};
public:
	void Invalidate() { pending=false; valid=false; inputs=0; }
	bool Sealed() const { return valid; }
	bool Remember(unsigned k,const uint32_t* x,const uint32_t* s) {
		Invalidate();
		if (!x || !s || !PioneerMatvecShape(k,32)) return false;
		for (unsigned w=0;w<k/2;w++)
			if (!PioneerInt8OperandValid(uint16_t(x[w])) || !PioneerInt8OperandValid(uint16_t(x[w]>>16))) return false;
		for (unsigned w=0;w<k/32;w++) if ((s[w]&0x7f800000)==0x7f800000) return false;
		memcpy(operand,x,k*2); memcpy(scale,s,k/32*4);
		inputs=k; pending=true; return true; // CPU copy alone is NEVER a hit
	}
	bool Seal(const tt_runtime_matvec_result& r,unsigned rows) {
		unsigned tiles=(inputs/16)*(rows/16),reads=tiles/32;
		if (!pending || !PioneerQ8CompactShape(inputs,rows) || r.version!=1 || r.state!=TT_RUNTIME_PASSED
			|| !r.prepared || !r.context_reset || !r.started || !r.completed || !r.stopped || r.recovery_required || r.nonfinite
			|| r.phase!=TT_RUNTIME_Q8_FINAL_PHASE || r.tiles!=tiles || r.marker!=0x4d415431
			|| r.pc[0]!=TT_RUNTIME_Q8_PARK_PC || r.pc[1]!=r.pc[0]
			|| uint32_t(r.after[0]-r.before[0])!=reads || uint32_t(r.after[1]-r.before[1])!=reads*TT_Q8_BULK_FLITS
			|| uint32_t(r.after[2]-r.before[2])!=reads || uint32_t(r.after[3]-r.before[3])!=reads) { Invalidate(); return false; }
		pending=false; valid=true; return true;
	}
	bool Matches(unsigned k,const uint32_t* x,const uint32_t* s) const {
		return valid && x && s && inputs==k && !memcmp(operand,x,k*2) && !memcmp(scale,s,k/32*4);
	}
};
#endif
