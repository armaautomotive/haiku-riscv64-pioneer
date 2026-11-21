/* SPDX-License-Identifier: MIT */
#include <assert.h>
#include <cmath>
#include <cstdio>

#define PIONEER_Q35_OPERATOR_TEST 1
#include "../PioneerQwen35Operators.h"
#include "../PioneerQwen35DataflowBlackholeDeltaWorker.h"

using namespace PioneerQwen35;

static bool Near(float actual,float expected,float tolerance=2.0e-6f)
{
	return std::fabs(actual-expected)<=tolerance*(1.0f+std::fabs(expected));
}

int main()
{
	float recurrent[DeltaWidth],norm[DeltaWidth],gate[DeltaWidth];
	float expected[DeltaWidth];
	for (unsigned i=0;i<DeltaWidth;i++) {
		recurrent[i]=float(int(i%17)-8)*0.03125f;
		norm[i]=0.75f+float(i%7)*0.03125f;
		gate[i]=float(int(i%11)-5)*0.125f;
	}
	assert(Rms<DeltaWidth>(recurrent,norm,expected));
	for (unsigned i=0;i<DeltaWidth;i++)
		expected[i]*=gate[i]*Sigmoid(gate[i]);
	float actual[DeltaWidth];
	for (unsigned i=0;i<DeltaWidth;i++) actual[i]=recurrent[i];
	assert(DataflowDeltaNormalizeAndGate(actual,norm,gate));
	for (unsigned i=0;i<DeltaWidth;i++)
		assert(PioneerAttentionBits(actual[i])==PioneerAttentionBits(expected[i]));

	float query[DeltaWidth]={},key[DeltaWidth]={};
	query[0]=1.0f;
	key[0]=1.0f;
	assert(DataflowDeltaNormalizeQk(query,key));
	assert(Near(query[0],0.0883883461f));
	assert(Near(key[0],0.999999523f));

	float decay=0,beta=0;
	assert(DataflowDeltaHeadGates(-1.0f,0.0f,0.0f,0.0f,decay,beta));
	assert(Near(decay,0.5f));
	assert(Near(beta,0.5f));
	assert(!DataflowDeltaHeadGates(1.0f,0.0f,0.0f,0.0f,decay,beta));

	float value[16],output[16];
	float stateRead[16*DeltaWidth]={},stateWrite[16*DeltaWidth]={};
	for (unsigned i=0;i<16;i++) value[i]=1.0f;
	assert(DataflowDeltaUpdatePanel(query,key,value,stateRead,stateWrite,
		output,decay,beta));
	for (unsigned row=0;row<16;row++) {
		assert(Near(stateWrite[row*DeltaWidth],0.5f));
		assert(Near(output[row],0.04419417f));
		for (unsigned column=1;column<DeltaWidth;column++)
			assert(stateWrite[row*DeltaWidth+column]==0.0f);
	}

	for (unsigned i=0;i<16*DeltaWidth;i++) stateRead[i]=stateWrite[i];
	for (unsigned i=0;i<16*DeltaWidth;i++) stateWrite[i]=0.0f;
	assert(DataflowDeltaUpdatePanel(query,key,value,stateRead,stateWrite,
		output,decay,beta));
	for (unsigned row=0;row<16;row++) {
		assert(Near(stateWrite[row*DeltaWidth],0.625f));
		assert(Near(output[row],0.0552427f));
	}

	query[0]=NAN;
	assert(!DataflowDeltaNormalizeQk(query,key));
	std::puts("PASS Qwen35 delta recurrence: Q/K normalization, state update, RMS and SiLU gate");
	return 0;
}
