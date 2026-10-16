/* SPDX-License-Identifier: MIT */
#include <cassert>
#include <initializer_list>
#include <stdio.h>
#include "../../../src/add-ons/kernel/drivers/misc/PioneerRuntimeActivationCache.h"
static tt_runtime_matvec_result Proven(unsigned k,unsigned n)
{
	tt_runtime_matvec_result r={}; r.version=1; r.state=TT_RUNTIME_PASSED;
	r.prepared=r.context_reset=r.started=r.completed=r.stopped=1;
	r.phase=TT_RUNTIME_Q8_FINAL_PHASE; r.tiles=(k/16)*(n/16); r.marker=0x4d415431;
	r.pc[0]=r.pc[1]=TT_RUNTIME_Q8_PARK_PC;
	for (unsigned w=0;w<4;w++) { r.before[w]=0xfffffff0; r.after[w]=r.before[w]+r.tiles/32*(w==1?TT_Q8_BULK_FLITS:1); }
	return r; // SYNTHETIC metadata, never hardware/performance evidence
}
int main()
{
	uint32_t input[TT_RUNTIME_MATVEC_INPUTS_MAX/2],scales[TT_RUNTIME_MATVEC_INPUTS_MAX/32];
	for (unsigned w=0;w<TT_RUNTIME_MATVEC_INPUTS_MAX/2;w++) input[w]=PioneerInt8Operand(int(w%256)-128)|uint32_t(PioneerInt8Operand(int((w+7)%256)-128))<<16;
	for (unsigned w=0;w<TT_RUNTIME_MATVEC_INPUTS_MAX/32;w++) scales[w]=0x3c800000+w;
	for (unsigned k:{1024U,2048U,3072U,5120U,6144U}) {
		PioneerRuntimeActivationCache cache; auto proof=Proven(k,32);
		assert(!cache.Matches(k,input,scales));
		assert(cache.Remember(k,input,scales) && !cache.Sealed() && !cache.Matches(k,input,scales));
		assert(cache.Seal(proof,32) && cache.Matches(k,input,scales));
		assert(!cache.Matches(k==1024?2048:1024,input,scales) && !cache.Matches(k,NULL,scales) && !cache.Matches(k,input,NULL));
		for (unsigned w=0;w<k/2;w++) { input[w]^=1; assert(!cache.Matches(k,input,scales)); input[w]^=1; }
		for (unsigned w=0;w<k/32;w++) { scales[w]^=1; assert(!cache.Matches(k,input,scales)); scales[w]^=1; }
		cache.Invalidate(); assert(!cache.Matches(k,input,scales));
		for (unsigned field:{0U,1U,2U,3U,4U,5U,6U,7U,10U,11U,12U,13U,14U,15U,16U,17U,18U,19U,20U,21U,22U,23U}) {
			auto bad=proof; ((uint32_t*)&bad)[field]^=1;
			assert(cache.Remember(k,input,scales) && !cache.Seal(bad,32) && !cache.Sealed() && !cache.Matches(k,input,scales));
		}
		assert(cache.Remember(k,input,scales) && !cache.Seal(proof,64));
		assert(!cache.Remember(0,input,scales) && !cache.Remember(k,NULL,scales) && !cache.Remember(k,input,NULL));
	}
	PioneerRuntimeActivationCache cache; auto proof=Proven(1024,32);
	scales[0]=0; assert(cache.Remember(1024,input,scales) && cache.Seal(proof,32));
	scales[0]=0x80000000; assert(!cache.Matches(1024,input,scales)); // exact signed-zero key
	for (uint32_t bad:{0x7f800000U,0xff800000U,0x7fc00000U}) {
		scales[0]=bad; assert(!cache.Remember(1024,input,scales) && !cache.Sealed());
	}
	scales[0]=0; input[0]=0xffffffff; assert(!cache.Remember(1024,input,scales));
	puts("PASS: OFFLINE exact activation/scales shadow, all K, every-key-word changes, signed zero, finite domain and every critical synthetic proof corruption; NOT enabled on hardware");
}
