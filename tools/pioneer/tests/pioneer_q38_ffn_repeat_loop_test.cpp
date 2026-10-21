/* SPDX-License-Identifier: MIT */
#include <cassert>
#include <cstdio>
#include <cstring>
#include <vector>
#include "../PioneerQ38FfnSamples.h"
struct MockTensor { void* data; };
static unsigned calls,failAt,corruptAt,stopAt,badClock;
static bool stopRequested;
static double clockValue;
static MockTensor* active;
static std::vector<float> expected;
static constexpr int GGML_STATUS_SUCCESS=0;
#ifndef PIONEER_TEST_GRAPH_OUTPUTS
#define PIONEER_TEST_GRAPH_OUTPUTS 5120
#endif
static constexpr unsigned graphOutputs=PIONEER_TEST_GRAPH_OUTPUTS;
static double Seconds() { clockValue+=.001; return badClock && calls==badClock?-1:clockValue; }
static int ggml_backend_graph_compute(void* backend,void* graph) {
	assert(backend==reinterpret_cast<void*>(1) && graph==reinterpret_cast<void*>(2));
	for(unsigned i=0;i<graphOutputs;i++) assert(!static_cast<float*>(active->data)[i]);
	calls++; if(calls==failAt) return 1;
	memcpy(active->data,expected.data(),graphOutputs*4);
	if(calls==corruptAt) static_cast<float*>(active->data)[graphOutputs-1]+=1;
	if(calls==stopAt) stopRequested=true;
	return GGML_STATUS_SUCCESS;
}
static void Test(unsigned fail,unsigned corrupt,unsigned stop,unsigned badTime) {
	calls=0;failAt=fail;corruptAt=corrupt;stopAt=stop;badClock=badTime;stopRequested=false;clockValue=1;
	std::vector<float> output(graphOutputs,0);expected.assign(graphOutputs,0);
	for(unsigned i=0;i<graphOutputs;i++) expected[i]=float(i)-124;
	const auto first=expected;
	MockTensor tensor={output.data()}; auto* y=&tensor; active=y;
	void* backend=reinterpret_cast<void*>(1); void* graph=reinterpret_cast<void*>(2);
	bool okay=true; PioneerQ38FfnSamples samples;
#include "ffn_repeat_loop.inc"
	unsigned wanted=fail?fail:corrupt?corrupt:stop?stop:32;
	assert(calls==wanted && samples.Count()==wanted && okay==!(fail||corrupt) && stopRequested==bool(stop));
	PioneerQ38FfnSamples::Summary summary;
	// Timing DATA validity does not certify graph/numeric success. Caller
	// reports the separate okay result and MUST not conflate those facts.
	assert(samples.Finish(summary)==(wanted==32 && !badTime));
}
int main() {
	Test(0,0,0,0); Test(0,0,0,5);
	for(unsigned at:{1U,16U,32U}) {Test(at,0,0,0);Test(0,at,0,0);Test(0,0,at,0);}
	printf("PASS ACTUAL extracted FFN/Q8 repeat32 loop:outputs%u ALL outputs cleared each call,32 repeats,first/middle/last failed graph/final-word mismatch/stop prevent later calls;invalid clock records diagnostic failure WITHOUT changing compute/cleanup authority;NO native/device access\n",graphOutputs);
}
