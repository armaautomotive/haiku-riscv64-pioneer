/* SPDX-License-Identifier: MIT */
#define PIONEER_Q35_OPERATOR_TEST
#include "../PioneerQwen35Operators.h"
#include "../PioneerQwen35DataflowDenseAttentionMath.h"

#include <assert.h>
#include <cstring>

using namespace PioneerQwen35;

static float Sample(unsigned index,unsigned salt)
{
	const int centered=int((index*17+salt*11)%97)-48;
	return float(centered)*0.0007f;
}

int main()
{
	float input[DenseHeadWidth],weight[DenseHeadWidth];
	float expected[DenseHeadWidth],actual[DenseHeadWidth];
	for (unsigned lane=0;lane<DenseHeadWidth;lane++) {
		input[lane]=Sample(lane,1);
		weight[lane]=0.7f+float(lane%29)*0.003f;
	}
	assert(Rms<DenseHeadWidth>(input,weight,expected));
	assert(DataflowDenseNormalize(input,weight,actual));
	assert(!std::memcmp(expected,actual,sizeof(actual)));

	float expectedRope[Rotary],actualRope[Rotary];
	for (unsigned lane=0;lane<DenseHeadWidth;lane++)
		expected[lane]=actual[lane]=Sample(lane,2);
	for (unsigned lane=0;lane<Rotary;lane++)
		expectedRope[lane]=float(int(lane%13)-6)*0.025f;
	std::memcpy(actualRope,expectedRope,sizeof(actualRope));
	assert(Rotate(expected,expectedRope));
	assert(DataflowDenseRotate(actual,actualRope));
	assert(!std::memcmp(expected,actual,sizeof(actual)));

	uint32_t expectedPacked[DenseHeadWidth/2],actualPacked[DenseHeadWidth/2];
	assert(PackHead(expected,expectedPacked));
	assert(DataflowDensePackHead(actual,actualPacked));
	assert(!std::memcmp(expectedPacked,actualPacked,sizeof(actualPacked)));
	for (unsigned lane=0;lane<DenseHeadWidth;lane++)
		assert(DataflowDenseUnpackLane(actualPacked,lane)==UnpackLane(expectedPacked,lane));

	float expectedSums[DenseHeadWidth]{},actualSums[DenseHeadWidth]{};
	AttentionRow reference{expectedSums};
	DataflowDenseAttentionRow candidate{actualSums};
	float queries[3][DenseHeadWidth],values[3][DenseHeadWidth];
	uint32_t keys[3][DenseHeadWidth/2],vals[3][DenseHeadWidth/2];
	for (unsigned at=0;at<3;at++) {
		for (unsigned lane=0;lane<DenseHeadWidth;lane++) {
			queries[at][lane]=Sample(lane,3);
			values[at][lane]=Sample(lane,at+4);
		}
		assert(PackHead(queries[at],keys[at]));
		assert(PackHead(values[at],vals[at]));
		assert(reference.Add(queries[at],keys[at],vals[at]));
		assert(candidate.Add(queries[at],keys[at],vals[at]));
	}
	float gate[DenseHeadWidth];
	for (unsigned lane=0;lane<DenseHeadWidth;lane++) gate[lane]=Sample(lane,8);
	assert(reference.Finish(gate));
	assert(candidate.Finish(gate));
	assert(!std::memcmp(expectedSums,actualSums,sizeof(actualSums)));
	return 0;
}
